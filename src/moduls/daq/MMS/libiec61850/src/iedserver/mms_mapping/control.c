/*
 *  control.c
 *
 *  Copyright 2013 Michael Zillgith
 *
 *  This file is part of libIEC61850.
 *
 *  libIEC61850 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  libIEC61850 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libIEC61850.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  See COPYING file for the complete license text.
 */


#include "control.h"
#include "mms_mapping.h"
#include "mms_mapping_internal.h"

#include "mms_client_connection.h"

#include <malloc.h>

#define CONTROL_ERROR_NO_ERROR 0
#define CONTROL_ERROR_UNKOWN 1
#define CONTROL_ERROR_TIMEOUT_TEST 2
#define CONTROL_ERROR_OPERATOR_TEST 3

#define CONTROL_ADD_CAUSE_UNKNOWN 0
#define CONTROL_ADD_CAUSE_SELECT_FAILED 3
#define CONTROL_ADD_CAUSE_BLOCKED_BY_INTERLOCKING 10
#define CONTROL_ADD_CAUSE_BLOCKED_BY_SYNCHROCHECK 11
#define CONTROL_ADD_CAUSE_COMMAND_ALREADY_IN_EXECUTION 12
#define CONTROL_ADD_CAUSE_OBJECT_NOT_SELECTED 18
#define CONTROL_ADD_CAUSE_OBJECT_ALREADY_SELECTED 19
#define CONTROL_ADD_CAUSE_INCONSISTENT_PARAMETERS 26
#define CONTROL_ADD_CAUSE_LOCKED_BY_OTHER_CLIENT 27

#define STATE_UNSELECTED 0
#define STATE_READY 1
#define STATE_WAIT_FOR_ACTICATION_TIME 2
#define STATE_PERFORM_TEST 3
#define STATE_WAIT_FOR_EXECUTION 4
#define STATE_OPERATE 5

struct sControlObject {
    MmsDomain* mmsDomain;
    MmsServer mmsServer;
    char* lnName;
    char* name;

    int state;
    Semaphore stateLock;

    MmsValue* mmsValue;
    MmsVariableSpecification* typeSpec;

    MmsValue* oper;
    MmsValue* sbo;
    MmsValue* sbow;
    MmsValue* cancel;

    MmsValue* ctlVal;
    MmsValue* ctlNum;
    MmsValue* origin;
    MmsValue* timestamp;

    char* ctlObjectName;

    /* for LastAppIError */
    MmsValue* error;
    MmsValue* addCause;

    bool selected;
    uint64_t selectTime;
    uint32_t selectTimeout;
    MmsValue* sboClass;

    bool timeActivatedOperate;
    uint64_t operateTime;

    bool operateOnce;
    MmsServerConnection* clientConnection;

    MmsValue* emptyString;

    bool initialized;
    uint32_t ctlModel;

    bool testMode;
    bool interlockCheck;
    bool synchroCheck;

    uint32_t operateInvokeId;

    ControlHandler listener;
    void* listenerParameter;

    ControlPerformCheckHandler checkHandler;
    void* checkHandlerParameter;

    ControlWaitForExecutionHandler waitForExecutionHandler;
    void* waitForExecutionHandlerParameter;
};

void
ControlObject_sendLastApplError(ControlObject* self, MmsServerConnection* connection, char* ctlVariable, int error, int addCause,
		MmsValue* ctlNum, MmsValue* origin);
void
ControlObject_sendCommandTerminationReq(ControlObject* self, MmsServerConnection* connection);


static void
setState(ControlObject* self, int newState)
{
	Semaphore_wait(self->stateLock);
	self->state = newState;
	Semaphore_post(self->stateLock);
}

static int
getState(ControlObject* self)
{
	int state;
	Semaphore_wait(self->stateLock);
	state = self->state;
	Semaphore_post(self->stateLock);

	return state;
}

static void
initialize(ControlObject* self)
{
    if (!(self->initialized)) {

    	self->emptyString = MmsValue_newVisibleString(NULL);

        char* ctlModelName = createString(4, self->lnName, "$CF$", self->name, "$ctlModel");

        if (DEBUG) printf("initialize control for %s\n", ctlModelName);

        MmsValue* ctlModel = MmsServer_getValueFromCache(self->mmsServer,
                self->mmsDomain, ctlModelName);

        if (ctlModel == NULL) {
        	printf("No control model found for variable %s\n", ctlModelName);
	    }

        free(ctlModelName);

        char* sboClassName = createString(4, self->lnName, "$CF$", self->name, "$sboClass");

        self->sboClass = MmsServer_getValueFromCache(self->mmsServer, self->mmsDomain, sboClassName);

        free(sboClassName);

        self->ctlObjectName = (char*) malloc(130);

		createStringInBuffer(self->ctlObjectName, 5, MmsDomain_getName(self->mmsDomain), "/",
				self->lnName, "$CO$", self->name);

		self->error = MmsValue_newIntegerFromInt32(0);
		self->addCause = MmsValue_newIntegerFromInt32(0);

        if (ctlModel != NULL) {
            uint32_t ctlModelVal = MmsValue_toInt32(ctlModel);

            self->ctlModel = ctlModelVal;

            if (DEBUG) printf("  ctlModel: %i\n", ctlModelVal);

            if ((ctlModelVal == 2) || (ctlModelVal == 4)) { /* SBO */
                char* sboTimeoutName = createString(4, self->lnName, "$CF$", self->name, "$sboTimeout");

                char* controlObjectReference = createString(6, self->mmsDomain->domainName, "/", self->lnName, "$", self->name, "$SBO");

                self->sbo = MmsValue_newVisibleString(controlObjectReference);

                MmsValue* sboTimeout = MmsServer_getValueFromCache(self->mmsServer,
                                self->mmsDomain, sboTimeoutName);

                if (sboTimeout != NULL) {
                    uint32_t sboTimeoutVal = MmsValue_toInt32(sboTimeout);

                    if (DEBUG) printf("set timeout for %s to %u\n", sboTimeoutName, sboTimeoutVal);

                    self->selectTimeout = sboTimeoutVal;
                }
                else
                	self->selectTimeout = CONFIG_CONTROL_DEFAULT_SBO_TIMEOUT;

                setState(self, STATE_UNSELECTED);

                if (DEBUG) printf("timeout for %s is %i\n", sboTimeoutName, self->selectTimeout);

                free(controlObjectReference);
				free(sboTimeoutName);
            }
            else {
                self->sbo = MmsValue_newVisibleString(NULL);

                setState(self, STATE_READY);
            }
        }

        self->initialized = true;
    }
}

static bool
isSboClassOperateOnce(ControlObject* self)
{
	if (self->sboClass != NULL) {
		if (MmsValue_toInt32(self->sboClass) == 1)
			return false;
		else
			return true;
	}
	else
		return true; /* default is operate-once ! */
}

static void
abortControlOperation(ControlObject* self)
{
    if ((self->ctlModel == 2) || (self->ctlModel == 4)) {

    	if (isSboClassOperateOnce(self))
    		setState(self, STATE_UNSELECTED);
    	else
    		setState(self, STATE_READY);
    }
    else
    	setState(self, STATE_READY);
}

static MmsValue*
getOperParameterOperTime(MmsValue* operParameters)
{
	if (operParameters->type == MMS_STRUCTURE) {
		if (operParameters->value.structure.size == 7)
			return MmsValue_getElement(operParameters, 1);
	}

	return NULL;
}

static void
controlOperateThread(ControlObject* self)
{
    bool checkOk = true;

    bool isTimeActivatedControl = false;

    if (getState(self) == STATE_WAIT_FOR_ACTICATION_TIME)
    	isTimeActivatedControl = true;

    setState(self, STATE_WAIT_FOR_EXECUTION);

    if (self->waitForExecutionHandler != NULL) {
        checkOk = self->waitForExecutionHandler(self->waitForExecutionHandlerParameter, self->ctlVal,
                    self->testMode, self->synchroCheck);
    }

    if (!checkOk) {

    	if (isTimeActivatedControl) {
    		ControlObject_sendLastApplError(self, self->clientConnection, "Oper",
    		                    CONTROL_ERROR_NO_ERROR, CONTROL_ADD_CAUSE_BLOCKED_BY_SYNCHROCHECK,
    		                    self->ctlNum, self->origin);
    	}
    	else
    		MmsServerConnection_sendWriteResponse(self->clientConnection, self->operateInvokeId, DATA_ACCESS_ERROR_OBJECT_ACCESS_DENIED);

        abortControlOperation(self);


    }
    else {
    	if (isTimeActivatedControl) {
    		ControlObject_sendCommandTerminationReq(self, self->clientConnection);

    		MmsValue* operTm = getOperParameterOperTime(self->oper);

    		MmsValue_setUtcTime(operTm, 0);

    	}
    	else
    		MmsServerConnection_sendWriteResponse(self->clientConnection, self->operateInvokeId, DATA_ACCESS_ERROR_SUCCESS);

        uint64_t currentTime = Hal_getTimeInMs();

        setState(self, STATE_OPERATE);

        if (ControlObject_operate(self, self->ctlVal, currentTime, self->testMode)) {

            if ((self->ctlModel == 4) || (self->ctlModel == 3)) {
                ControlObject_sendCommandTerminationReq(self, self->clientConnection);
            }
        }
        else {

            if ((self->ctlModel == 4) || (self->ctlModel == 3)) {
				if (DEBUG) printf("Oper: operate failed!\n");
				ControlObject_sendLastApplError(self, self->clientConnection, "Oper",
						CONTROL_ERROR_NO_ERROR, CONTROL_ADD_CAUSE_SELECT_FAILED,
						self->ctlNum, self->origin);
			}
        }

        abortControlOperation(self);

    }
}

static void
startControlOperateThread(ControlObject* self)
{
    Thread thread = Thread_create((ThreadExecutionFunction) controlOperateThread, (void*) self, true);

    Thread_start(thread);
}



ControlObject*
ControlObject_create(MmsServer mmsServer, MmsDomain* domain, char* lnName, char* name)
{
    ControlObject* self = (ControlObject*) calloc(1, sizeof(ControlObject));

    self->stateLock = Semaphore_create(1);

    self->name = name;
    self->lnName = lnName;
    self->mmsDomain = domain;
    self->mmsServer = mmsServer;

    return self;
}

void
ControlObject_destroy(ControlObject* self)
{
    if (self->mmsValue != NULL)
        MmsValue_delete(self->mmsValue);

    if (self->sbo != NULL)
    	MmsValue_delete(self->sbo);

    if (self->emptyString != NULL)
    	MmsValue_delete(self->emptyString);

    if (self->ctlObjectName != NULL)
    	free(self->ctlObjectName);

    if (self->error != NULL)
    	MmsValue_delete(self->error);

    if (self->addCause != NULL)
    	MmsValue_delete(self->addCause);

    if (self->ctlVal != NULL)
    	MmsValue_delete(self->ctlVal);

    if (self->ctlNum != NULL)
    	MmsValue_delete(self->ctlNum);

    if (self->origin != NULL)
    	MmsValue_delete(self->origin);

    Semaphore_destroy(self->stateLock);

    free(self);
}

void
ControlObject_setOper(ControlObject* self, MmsValue* oper)
{
    self->oper = oper;
}

void
ControlObject_setCancel(ControlObject* self, MmsValue* cancel)
{
    self->cancel = cancel;
}

void
ControlObject_setSBOw(ControlObject* self, MmsValue* sbow)
{
    self->sbow = sbow;
}

void
ControlObject_setSBO(ControlObject* self, MmsValue* sbo)
{
	self->sbo = sbo;
}

void
ControlObject_setCtlVal(ControlObject* self, MmsValue* ctlVal)
{
    self->ctlVal = ctlVal;
}

char*
ControlObject_getName(ControlObject* self)
{
    return self->name;
}

char*
ControlObject_getLNName(ControlObject* self)
{
    return self->lnName;
}

MmsDomain*
ControlObject_getDomain(ControlObject* self)
{
    return self->mmsDomain;
}

MmsValue*
ControlObject_getOper(ControlObject* self)
{
    return self->oper;
}

MmsValue*
ControlObject_getSBOw(ControlObject* self)
{
    return self->sbow;
}

MmsValue*
ControlObject_getSBO(ControlObject* self)
{
    return self->sbo;
}

MmsValue*
ControlObject_getCancel(ControlObject* self)
{
    return self->cancel;
}

void
ControlObject_setMmsValue(ControlObject* self, MmsValue* value)
{
    self->mmsValue = value;
}

void
ControlObject_setTypeSpec(ControlObject* self, MmsVariableSpecification* typeSpec)
{
    self->typeSpec = typeSpec;
}

MmsVariableSpecification*
ControlObject_getTypeSpec(ControlObject* self)
{
    return self->typeSpec;
}

MmsValue*
ControlObject_getMmsValue(ControlObject* self)
{
    return self->mmsValue;
}

static void
selectObject(ControlObject* self, uint64_t selectTime, MmsServerConnection* connection)
{
    self->selected = true;
    self->selectTime = selectTime;
    self->clientConnection = connection;
    setState(self, STATE_READY);
}

static void
checkSelectTimeout(ControlObject* self, uint64_t currentTime)
{
	if ((self->ctlModel == 2) || (self->ctlModel == 4)) {

		if (getState(self) == STATE_READY) {
			if (self->selectTimeout > 0) {
				if (currentTime > (self->selectTime + self->selectTimeout)) {
					if (DEBUG) printf("isSelected: select-timeout (timeout-val = %i)\n", self->selectTimeout);
					setState(self, STATE_UNSELECTED);
				}
			}
		}
	}
}

bool
ControlObject_unselect(ControlObject* self, MmsServerConnection* connection)
{
	if (self->clientConnection == connection) {
		abortControlOperation(self);
		return true;
	}
	else
		return false;
}

bool
ControlObject_operate(ControlObject* self, MmsValue* value, uint64_t currentTime, bool testCondition)
{
	self->selectTime = currentTime;

	if (self->listener != NULL) {
		self->listener(self->listenerParameter, value, testCondition);
	}

	return true;
}

void
ControlObject_installListener(ControlObject* self, ControlHandler listener, void* parameter)
{
    self->listener = listener;
    self->listenerParameter = parameter;
}

void
ControlObject_installCheckHandler(ControlObject* self, ControlPerformCheckHandler handler, void* parameter)
{
    self->checkHandler = handler;
    self->checkHandlerParameter = parameter;
}

void
ControlObject_installWaitForExecutionHandler(ControlObject* self, ControlWaitForExecutionHandler handler, void* parameter)
{
    self->waitForExecutionHandler = handler;
    self->waitForExecutionHandlerParameter = parameter;
}

void
Control_processControlActions(MmsMapping* self, uint64_t currentTimeInMs)
{
	LinkedList element = LinkedList_getNext(self->controlObjects);

	while (element != NULL) {
		ControlObject* controlObject = (ControlObject*) element->data;

		if (controlObject->timeActivatedOperate) {

			if (controlObject->operateTime <= currentTimeInMs) {

				if (DEBUG) printf("time activated operate: start operation\n");

				controlObject->timeActivatedOperate = false;

			    bool checkOk = true;


				if (controlObject->checkHandler != NULL) { /* perform operative tests */
					checkOk = controlObject->checkHandler(
							controlObject->checkHandlerParameter, controlObject->ctlVal, controlObject->testMode,
								controlObject->interlockCheck);
				}

				if (checkOk)
					startControlOperateThread(controlObject);
				else {
		    		ControlObject_sendLastApplError(controlObject, controlObject->clientConnection, "Oper",
		    		                    CONTROL_ERROR_NO_ERROR, CONTROL_ADD_CAUSE_BLOCKED_BY_INTERLOCKING,
		    		                    controlObject->ctlNum, controlObject->origin);

					abortControlOperation(controlObject);
				}
			}

		}

		element = LinkedList_getNext(element);
	}
}

ControlObject*
Control_lookupControlObject(MmsMapping* self, MmsDomain* domain, char* lnName, char* objectName)
{
    LinkedList element = LinkedList_getNext(self->controlObjects);

    while (element != NULL) {
        ControlObject* controlObject = (ControlObject*) element->data;

        if (ControlObject_getDomain(controlObject) == domain) {
            if (strcmp(ControlObject_getLNName(controlObject), lnName) == 0) {
                if (strcmp(ControlObject_getName(controlObject), objectName) == 0) {
                    return controlObject;
                }
            }
        }

        element = LinkedList_getNext(element);
    }

    return NULL;
}

static MmsValue*
getCtlVal(MmsValue* operParameters)
{
    if (operParameters->type == MMS_STRUCTURE) {
        if (operParameters->value.structure.size > 5) {
            return MmsValue_getElement(operParameters, 0);
        }
    }

    return NULL;
}

static MmsValue*
getOperParameterCtlNum(MmsValue* operParameters)
{
	if (operParameters->type == MMS_STRUCTURE) {
		if (operParameters->value.structure.size == 7)
			return MmsValue_getElement(operParameters, 3);
		else if (operParameters->value.structure.size == 6)
			return MmsValue_getElement(operParameters, 2);
	}

	return NULL;
}

static MmsValue*
getCancelParameterCtlNum(MmsValue* operParameters)
{
	if (operParameters->type == MMS_STRUCTURE) {
		if (operParameters->value.structure.size == 6)
			return MmsValue_getElement(operParameters, 3);
		else if (operParameters->value.structure.size == 5)
			return MmsValue_getElement(operParameters, 2);
	}

	return NULL;
}

static MmsValue*
getCancelParameterOrigin(MmsValue* operParameters)
{
	if (operParameters->type == MMS_STRUCTURE) {
		if (operParameters->value.structure.size == 6)
			return MmsValue_getElement(operParameters, 2);
		else if (operParameters->value.structure.size == 5)
			return MmsValue_getElement(operParameters, 1);
	}

	return NULL;
}

static MmsValue*
getOperParameterTest(MmsValue* operParameters)
{
	if (operParameters->type == MMS_STRUCTURE) {
		if (operParameters->value.structure.size == 7)
			return MmsValue_getElement(operParameters, 5);
		else if (operParameters->value.structure.size == 6)
			return MmsValue_getElement(operParameters, 4);
	}

	return NULL;
}

static MmsValue*
getOperParameterCheck(MmsValue* operParameters)
{
    if (operParameters->type == MMS_STRUCTURE) {
        if (operParameters->value.structure.size == 7)
            return MmsValue_getElement(operParameters, 6);
        else if (operParameters->value.structure.size == 6)
            return MmsValue_getElement(operParameters, 5);
    }

    return NULL;
}

static MmsValue*
getOperParameterOrigin(MmsValue* operParameters)
{
	if (operParameters->type == MMS_STRUCTURE) {
		if (operParameters->value.structure.size == 7)
			return MmsValue_getElement(operParameters, 2);
		else if (operParameters->value.structure.size == 6)
			return MmsValue_getElement(operParameters, 1);
	}

	return NULL;
}

static MmsValue*
getOperParameterTime(MmsValue* operParameters)
{
    MmsValue* timeParameter = NULL;

    if (operParameters->type == MMS_STRUCTURE) {
        if (operParameters->value.structure.size == 7)
            timeParameter = MmsValue_getElement(operParameters, 4);
        else if (operParameters->value.structure.size == 6)
            timeParameter = MmsValue_getElement(operParameters, 3);
    }

    if (MmsValue_getType(timeParameter) == MMS_UTC_TIME)
        return timeParameter;
    else
        return NULL;
}

void
ControlObject_sendCommandTerminationReq(ControlObject* self, MmsServerConnection* connection)
{
    char* itemId = (char*) alloca(130);

    createStringInBuffer(itemId, 4, self->lnName, "$CO$", self->name, "$Oper");

    if (DEBUG) printf("CmdTermination: %s\n", itemId);

    char* domainId = MmsDomain_getName(self->mmsDomain);

    MmsVariableAccessSpecification* varSpec = (MmsVariableAccessSpecification*) 
		calloc(1, sizeof(MmsVariableAccessSpecification));
    varSpec->itemId = itemId;
    varSpec->domainId = domainId;

    LinkedList varSpecList = LinkedList_create();
    LinkedList values = LinkedList_create();

    LinkedList_add(varSpecList, varSpec);
    LinkedList_add(values, self->oper);

    MmsServerConnection_sendInformationReportListOfVariables(connection, varSpecList, values);

    LinkedList_destroyStatic(varSpecList);
    LinkedList_destroyStatic(values);
}

void
ControlObject_sendLastApplError(ControlObject* self, MmsServerConnection* connection, char* ctlVariable, int error, int addCause,
		MmsValue* ctlNum, MmsValue* origin)
{
	MmsValue* lastAppIError = (MmsValue*) alloca(sizeof(struct sMmsValue));
	lastAppIError->type = MMS_STRUCTURE;
	lastAppIError->value.structure.size = 5;
	lastAppIError->value.structure.components = (MmsValue**) alloca(5 * sizeof(MmsValue*));

    char* ctlObj = (char*) alloca(130);

    createStringInBuffer(ctlObj, 3, self->ctlObjectName, "$", ctlVariable);

    if (DEBUG) {
		printf("sendLastApplError:\n");
		printf("  control object: %s\n", ctlObj);
		printf("  ctlNum: %u\n", MmsValue_toUint32(self->ctlNum));
    }

    MmsValue* ctlObjValue = (MmsValue*) alloca(sizeof(struct sMmsValue));
    ctlObjValue->type = MMS_VISIBLE_STRING;
    ctlObjValue->value.visibleString = ctlObj;

    MmsValue_setElement(lastAppIError, 0, ctlObjValue);

    MmsValue_setInt32(self->error, error);
    MmsValue_setInt32(self->addCause, addCause);

    MmsValue_setElement(lastAppIError, 1, self->error);
    MmsValue_setElement(lastAppIError, 2, origin);
    MmsValue_setElement(lastAppIError, 3, ctlNum);
    MmsValue_setElement(lastAppIError, 4, self->addCause);

    MmsServerConnection_sendInformationReportSingleVariableVMDSpecific(connection,
    		"LastApplError", lastAppIError);
}

static void
updateControlParameters(ControlObject* controlObject, MmsValue* ctlVal, MmsValue* ctlNum, MmsValue* origin)
{
    if (controlObject->ctlVal != NULL)
        MmsValue_delete(controlObject->ctlVal);

    if (controlObject->ctlNum != NULL)
        MmsValue_delete(controlObject->ctlNum);

    if (controlObject->origin != NULL)
        MmsValue_delete(controlObject->origin);

    controlObject->ctlVal = MmsValue_clone(ctlVal);
    controlObject->ctlNum = MmsValue_clone(ctlNum);
    controlObject->origin = MmsValue_clone(origin);
}

MmsValue*
Control_readAccessControlObject(MmsMapping* self, MmsDomain* domain, char* variableIdOrig,
        MmsServerConnection* connection)
{
    MmsValue* value = NULL;

    if (DEBUG) printf("readAccessControlObject: %s\n", variableIdOrig);

    char* variableId = copyString(variableIdOrig);

    char* separator = strchr(variableId, '$');

    *separator = 0;

    char* lnName = variableId;

    if (lnName == NULL )
       return NULL;

    char* objectName = MmsMapping_getNextNameElement(separator + 1);

    if (objectName == NULL )
       return NULL;

    char* varName = MmsMapping_getNextNameElement(objectName);

    if (varName != NULL)
       *(varName - 1) = 0;

    ControlObject* controlObject = Control_lookupControlObject(self, domain, lnName, objectName);

    if (controlObject != NULL) {

    	initialize(controlObject);

        if (varName != NULL) {
            if (strcmp(varName, "Oper") == 0)
                value = ControlObject_getOper(controlObject);
            else if (strcmp(varName, "SBOw") == 0)
                value = ControlObject_getSBOw(controlObject);
            else if (strcmp(varName, "SBO") == 0) {
                if (controlObject->ctlModel == 2) {

                    uint64_t currentTime = Hal_getTimeInMs();

                    value = controlObject->emptyString;

                    checkSelectTimeout(controlObject, currentTime);

                    if (getState(controlObject) == STATE_UNSELECTED) {
                        bool checkOk = true;

                        if (controlObject->checkHandler != NULL) { /* perform operative tests */
                            checkOk = controlObject->checkHandler(
                                    controlObject->checkHandlerParameter, NULL, false, false);
                        }

                        if (checkOk) {
                            selectObject(controlObject, currentTime, connection);
                            value = ControlObject_getSBO(controlObject);
                        }
                    }

                }
                else {
                	if (DEBUG) printf("select not applicable for control model %i\n", controlObject->ctlModel);
                    value = NULL;
                }
            }

            else if (strcmp(varName, "Cancel") == 0)
                value = ControlObject_getCancel(controlObject);
            else {
                value = MmsValue_getSubElement(ControlObject_getMmsValue(controlObject),
                        ControlObject_getTypeSpec(controlObject), varName);
            }
        }
        else
            value = ControlObject_getMmsValue(controlObject);
    }

    free(variableId);

    return value;
}

MmsDataAccessError
Control_writeAccessControlObject(MmsMapping* self, MmsDomain* domain, char* variableIdOrig,
                         MmsValue* value, MmsServerConnection* connection)
{
    MmsDataAccessError indication = DATA_ACCESS_ERROR_OBJECT_ACCESS_DENIED;

    char* variableId = copyString(variableIdOrig);

    char* separator = strchr(variableId, '$');

    *separator = 0;

    char* lnName = variableId;

    if (lnName == NULL )
        goto free_and_return;

    char* objectName = MmsMapping_getNextNameElement(separator + 1);

    if (objectName == NULL )
        goto free_and_return;

    char* varName = MmsMapping_getNextNameElement(objectName);

    if (varName != NULL)
        *(varName - 1) = 0;
    else
        goto free_and_return;

    ControlObject* controlObject = Control_lookupControlObject(self, domain, lnName, objectName);

    if (controlObject == NULL) {
        indication = DATA_ACCESS_ERROR_OBJECT_ACCESS_DENIED;
        goto free_and_return;
    }

    initialize(controlObject);

    if (strcmp(varName, "SBOw") == 0) { /* select with value */

    	if (controlObject->ctlModel == 4) {

    		MmsValue* ctlVal = getCtlVal(value);

    		if (ctlVal != NULL) {

    		    MmsValue* ctlNum = getOperParameterCtlNum(value);
    		    MmsValue* origin = getOperParameterOrigin(value);
    		    MmsValue* check = getOperParameterCheck(value);
    		    MmsValue* test = getOperParameterTest(value);

    		    uint64_t currentTime = Hal_getTimeInMs();

    		    checkSelectTimeout(controlObject, currentTime);

    		    int state = getState(controlObject);

    		    if (state != STATE_UNSELECTED) {
    		        indication = DATA_ACCESS_ERROR_TEMPORARILY_UNAVAILABLE;

    		        if (connection != controlObject->clientConnection)
    		        	ControlObject_sendLastApplError(controlObject, connection, "SBOw",  0,
    		        	                           CONTROL_ADD_CAUSE_LOCKED_BY_OTHER_CLIENT, ctlNum, origin);
    		        else
						ControlObject_sendLastApplError(controlObject, connection, "SBOw",  0,
							   CONTROL_ADD_CAUSE_OBJECT_ALREADY_SELECTED, ctlNum, origin);

                    if (DEBUG) printf("SBOw: select failed!\n");
    		    }
    		    else {

    		        bool checkOk = true;

    		        bool interlockCheck = MmsValue_getBitStringBit(check, 1);

    		        bool testCondition = MmsValue_getBoolean(test);

    	            if (controlObject->checkHandler != NULL) { /* perform operative tests */
    	                checkOk = controlObject->checkHandler(
    	                        controlObject->checkHandlerParameter, ctlVal, testCondition, interlockCheck);
    	            }

    	            if (checkOk) {
                        selectObject(controlObject, currentTime, connection);

                        updateControlParameters(controlObject, ctlVal, ctlNum, origin);

                        indication = DATA_ACCESS_ERROR_SUCCESS;

                        if (DEBUG) printf("SBOw: selected successful\n");
    	            }
    	            else {
    	                indication = DATA_ACCESS_ERROR_OBJECT_ACCESS_DENIED;

    	                ControlObject_sendLastApplError(controlObject, connection, "SBOw",  0,
    	                        CONTROL_ADD_CAUSE_SELECT_FAILED, ctlNum, origin);

    	                if (DEBUG) printf("SBOw: select rejected by application!\n");
    	            }
				}
    		}
			else {
				indication = DATA_ACCESS_ERROR_OBJECT_VALUE_INVALID;
			}
    	}
    	else {
    		indication = DATA_ACCESS_ERROR_OBJECT_ACCESS_DENIED;
    		goto free_and_return;
    	}
    }
    else if (strcmp(varName, "Oper") == 0) {
    	MmsValue* ctlVal = getCtlVal(value);
    	MmsValue* test = getOperParameterTest(value);
    	MmsValue* ctlNum = getOperParameterCtlNum(value);
    	MmsValue* origin = getOperParameterOrigin(value);
    	MmsValue* check = getOperParameterCheck(value);
    	MmsValue* timeParameter = getOperParameterTime(value);

    	if ((ctlVal == NULL) || (test == NULL) || (ctlNum == NULL) || (origin == NULL) || (check == NULL) || (timeParameter == NULL)) {
    	    indication = DATA_ACCESS_ERROR_OBJECT_VALUE_INVALID;
    	    goto free_and_return;
    	}


    	uint64_t currentTime = Hal_getTimeInMs();

    	checkSelectTimeout(controlObject, currentTime);

    	int state = getState(controlObject);

    	if (state == STATE_WAIT_FOR_ACTICATION_TIME) {
    	    indication = DATA_ACCESS_ERROR_TEMPORARILY_UNAVAILABLE;

    	    ControlObject_sendLastApplError(controlObject, connection, "Oper",
                                    CONTROL_ERROR_NO_ERROR, CONTROL_ADD_CAUSE_COMMAND_ALREADY_IN_EXECUTION,
                                    ctlNum, origin);

    	    goto free_and_return;
    	}
    	else if (state == STATE_READY) {


        	bool interlockCheck = MmsValue_getBitStringBit(check, 1);
        	bool synchroCheck = MmsValue_getBitStringBit(check, 0);

        	bool testCondition = MmsValue_getBoolean(test);

        	controlObject->testMode = testCondition;


        	if ((controlObject->ctlModel == 2) || (controlObject->ctlModel == 4)) {
        		if (controlObject->clientConnection != connection) {
        		    indication = DATA_ACCESS_ERROR_TEMPORARILY_UNAVAILABLE;
                    if (DEBUG) printf("Oper: operate from wrong client connection!\n");
                    goto free_and_return;
                }

                if (controlObject->ctlModel == 4) { /* select-before-operate with enhanced security */
                	if ((MmsValue_equals(ctlVal, controlObject->ctlVal) &&
                		 MmsValue_equals(origin, controlObject->origin))  == false)
                	{

                		indication = DATA_ACCESS_ERROR_TYPE_INCONSISTENT;
                		ControlObject_sendLastApplError(controlObject, connection, "Oper",
                								CONTROL_ERROR_NO_ERROR, CONTROL_ADD_CAUSE_INCONSISTENT_PARAMETERS,
                								ctlNum, origin);

                		goto free_and_return;
                	}
                }
        	}

            updateControlParameters(controlObject, ctlVal, ctlNum, origin);

            MmsValue* operTm = getOperParameterOperTime(value);

            if (operTm != NULL) {
                controlObject->operateTime = MmsValue_getUtcTimeInMs(operTm);

                if (controlObject->operateTime != 0) {
                    controlObject->timeActivatedOperate = true;
                    controlObject->synchroCheck = synchroCheck;
                    controlObject->interlockCheck = interlockCheck;
                    controlObject->clientConnection = connection;

                    setState(controlObject, STATE_WAIT_FOR_ACTICATION_TIME);

                    if (DEBUG) printf("Oper: activate time activated control\n");

                    indication = DATA_ACCESS_ERROR_SUCCESS;
                }
            }

            MmsValue_update(controlObject->oper, value);

            if (controlObject->timeActivatedOperate == false) {

                bool checkOk = true;

                setState(controlObject, STATE_PERFORM_TEST);

                if (controlObject->checkHandler != NULL) { /* perform operative tests */
                    checkOk = controlObject->checkHandler(
                            controlObject->checkHandlerParameter, ctlVal, testCondition, interlockCheck);
                }

                if (checkOk) {
                    indication = DATA_ACCESS_ERROR_NO_RESPONSE;

                    controlObject->clientConnection = connection;

                    controlObject->operateInvokeId = MmsServerConnection_getLastInvokeId(connection);
                    startControlOperateThread(controlObject);
                }
                else {
                	abortControlOperation(controlObject);

                    indication = DATA_ACCESS_ERROR_TEMPORARILY_UNAVAILABLE;
                }
            }

    	}
    	else if (state == STATE_UNSELECTED) {
    		if (DEBUG) printf("Oper: not selected!\n");

			indication = DATA_ACCESS_ERROR_OBJECT_ACCESS_DENIED;
			ControlObject_sendLastApplError(controlObject, connection, "Oper",
										CONTROL_ERROR_NO_ERROR,	CONTROL_ADD_CAUSE_OBJECT_NOT_SELECTED,
										ctlNum, origin);

			goto free_and_return;
    	}
    }
    else if (strcmp(varName, "Cancel") == 0) {
    	if (DEBUG) printf("Received cancel!\n");

    	int state = getState(controlObject);

    	MmsValue* ctlNum = getCancelParameterCtlNum(value);
		MmsValue* origin = getCancelParameterOrigin(value);

		if ((ctlNum == NULL) || (origin == NULL)) {
			indication = DATA_ACCESS_ERROR_TYPE_INCONSISTENT;
			if (DEBUG) printf("Invalid cancel message!\n");
			goto free_and_return;
		}

    	if ((controlObject->ctlModel == 2) || (controlObject->ctlModel == 4)) {
    		if (state != STATE_UNSELECTED) {
				if (controlObject->clientConnection == connection) {
					indication = DATA_ACCESS_ERROR_SUCCESS;
					setState(controlObject, STATE_UNSELECTED);
					goto free_and_return;
				}
				else {
					indication = DATA_ACCESS_ERROR_TEMPORARILY_UNAVAILABLE;
					ControlObject_sendLastApplError(controlObject, connection, "Cancel",
										CONTROL_ERROR_NO_ERROR,	CONTROL_ADD_CAUSE_LOCKED_BY_OTHER_CLIENT,
										ctlNum, origin);
				}
    		}
    	}

   		if (controlObject->timeActivatedOperate) {
			controlObject->timeActivatedOperate = false;
			abortControlOperation(controlObject);
			indication = DATA_ACCESS_ERROR_SUCCESS;
			goto free_and_return;
		}
    }

free_and_return:
    free(variableId);

    return indication;
}
