/*
 *  mms_goose.c
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

#include "stack_config.h"

#if CONFIG_INCLUDE_GOOSE_SUPPORT == 1

#include "libiec61850_platform_includes.h"
#include "mms_mapping.h"
#include "linked_list.h"
#include "array_list.h"

#include "thread.h"

#include "reporting.h"
#include "mms_mapping_internal.h"

#include "mms_goose.h"
#include "goose_publisher.h"

struct sMmsGooseControlBlock {
    char* name;
    bool goEna;

    char* dstAddress;

    MmsDomain* domain;
    LogicalNode* logicalNode;
    MmsVariableSpecification* mmsType;
    MmsValue* mmsValue;
    GoosePublisher publisher;

    DataSet* dataSet;
    bool isDynamicDataSet;

    LinkedList dataSetValues;
    uint64_t nextPublishTime;
    int retransmissionsLeft; /* number of retransmissions left for the last event */
    Semaphore publisherMutex;

    MmsMapping* mmsMapping;

    char* goCBRef;
    char* goId;
    char* dataSetRef;
};

MmsGooseControlBlock
MmsGooseControlBlock_create()
{
	MmsGooseControlBlock self = (MmsGooseControlBlock) calloc(1, sizeof(struct sMmsGooseControlBlock));

    self->publisherMutex = Semaphore_create(1);

    return self;
}

void
MmsGooseControlBlock_destroy(MmsGooseControlBlock self)
{
    Semaphore_destroy(self->publisherMutex);

    if (self->publisher != NULL)
        GoosePublisher_destroy(self->publisher);

    if (self->dataSetValues != NULL)
        LinkedList_destroyStatic(self->dataSetValues);

    if (self->goCBRef != NULL)
        free(self->goCBRef);

    if (self->goId != NULL)
        free(self->goId);

    if (self->dataSetRef != NULL)
        free(self->dataSetRef);

    if (self->dataSet != NULL) {
        if (self->isDynamicDataSet)
            MmsMapping_freeDynamicallyCreatedDataSet(self->dataSet);
    }

    MmsValue_delete(self->mmsValue);

    free(self);
}

MmsDomain*
MmsGooseControlBlock_getDomain(MmsGooseControlBlock self)
{
    return self->domain;
}

DataSet*
MmsGooseControlBlock_getDataSet(MmsGooseControlBlock self)
{
    return self->dataSet;
}

char*
MmsGooseControlBlock_getLogicalNodeName(MmsGooseControlBlock self)
{
    return self->logicalNode->name;
}

char*
MmsGooseControlBlock_getName(MmsGooseControlBlock self)
{
    return self->name;
}

MmsValue*
MmsGooseControlBlock_getMmsValues(MmsGooseControlBlock self)
{
    return self->mmsValue;
}

MmsVariableSpecification*
MmsGooseControlBlock_getVariableSpecification(MmsGooseControlBlock self)
{
    return self->mmsType;
}

bool
MmsGooseControlBlock_isEnabled(MmsGooseControlBlock self)
{
    return self->goEna;
}

void
MmsGooseControlBlock_enable(MmsGooseControlBlock self)
{
    Semaphore_wait(self->publisherMutex);

    if (!MmsGooseControlBlock_isEnabled(self)) {


        if (self->dataSetRef != NULL) {
            free(self->dataSetRef);

            if (self->dataSet != NULL)
                if (self->isDynamicDataSet)
                    MmsMapping_freeDynamicallyCreatedDataSet(self->dataSet);

            if (self->dataSetValues != NULL) {
                LinkedList_destroyStatic(self->dataSetValues);
                self->dataSetValues = NULL;
            }
        }

        self->dataSet = NULL;

        char* dataSetRef = MmsValue_toString(MmsValue_getElement(self->mmsValue, 2));

        if (dataSetRef != NULL) {

            self->dataSetRef = copyString(dataSetRef);

            self->dataSet = IedModel_lookupDataSet(self->mmsMapping->model, self->dataSetRef);

            self->isDynamicDataSet = false;

            if (self->dataSet == NULL) {
                self->dataSet = MmsMapping_getDomainSpecificDataSet(self->mmsMapping, self->dataSetRef);

                self->isDynamicDataSet = true;
            }

        }

        if (self->dataSet != NULL) {

            MmsValue* goEna = MmsValue_getElement(self->mmsValue, 0);

            MmsValue_setBoolean(goEna, true);


            MmsValue* dstAddress = MmsValue_getElement(self->mmsValue, 5);

            CommParameters commParameters;
            commParameters.appId = MmsValue_toInt32(MmsValue_getElement(dstAddress, 3));
            commParameters.vlanId = MmsValue_toInt32(MmsValue_getElement(dstAddress, 2));
            commParameters.vlanPriority = MmsValue_toInt32(MmsValue_getElement(dstAddress, 1));

            MmsValue* macAddress = MmsValue_getElement(dstAddress, 0);

            int i;
            for (i = 0; i < 6; i++) {
               commParameters.dstAddress[i] = macAddress->value.octetString.buf[i];
            }

            self->publisher = GoosePublisher_create(&commParameters, NULL);

            GoosePublisher_setTimeAllowedToLive(self->publisher, CONFIG_GOOSE_STABLE_STATE_TRANSMISSION_INTERVAL * 3);

            GoosePublisher_setDataSetRef(self->publisher, self->dataSetRef);

            GoosePublisher_setGoCbRef(self->publisher, self->goCBRef);

            uint32_t confRev = MmsValue_toUint32(MmsValue_getElement(self->mmsValue, 3));

            GoosePublisher_setConfRev(self->publisher, confRev);

            bool needsCom = MmsValue_getBoolean(MmsValue_getElement(self->mmsValue, 4));

            GoosePublisher_setNeedsCommission(self->publisher, needsCom);

            if (self->goId != NULL)
                GoosePublisher_setGoID(self->publisher, self->goId);

            //prepare data set values
            self->dataSetValues = LinkedList_create();

            DataSet* dataSet = self->dataSet;

            for (i = 0; i < dataSet->elementCount; i++) {
                LinkedList_add(self->dataSetValues, dataSet->fcda[i]->value);
            }

            self->goEna = true;
        }

    }

    Semaphore_post(self->publisherMutex);
}

void
MmsGooseControlBlock_disable(MmsGooseControlBlock self)
{
    if (MmsGooseControlBlock_isEnabled(self)) {
        MmsValue* goEna = MmsValue_getElement(self->mmsValue, 0);

        MmsValue_setBoolean(goEna, false);

        self->goEna = false;

        Semaphore_wait(self->publisherMutex);

        if (self->publisher != NULL) {
            GoosePublisher_destroy(self->publisher);
            self->publisher = NULL;
            LinkedList_destroyStatic(self->dataSetValues);
            self->dataSetValues = NULL;
        }

        Semaphore_post(self->publisherMutex);
    }
}


void
MmsGooseControlBlock_checkAndPublish(MmsGooseControlBlock self, uint64_t currentTime)
{
    if (currentTime >= self->nextPublishTime) {

        Semaphore_wait(self->publisherMutex);

        GoosePublisher_publish(self->publisher, self->dataSetValues);

        if (self->retransmissionsLeft > 0) {
            self->nextPublishTime = currentTime + CONFIG_GOOSE_EVENT_RETRANSMISSION_INTERVAL;


            if (self->retransmissionsLeft > 1)
                GoosePublisher_setTimeAllowedToLive(self->publisher,
                        CONFIG_GOOSE_EVENT_RETRANSMISSION_INTERVAL * 3);
            else
                GoosePublisher_setTimeAllowedToLive(self->publisher,
                        CONFIG_GOOSE_STABLE_STATE_TRANSMISSION_INTERVAL * 3);

            self->retransmissionsLeft--;
        }
        else {
            GoosePublisher_setTimeAllowedToLive(self->publisher,
                                CONFIG_GOOSE_STABLE_STATE_TRANSMISSION_INTERVAL * 3);

            self->nextPublishTime = currentTime +
                CONFIG_GOOSE_STABLE_STATE_TRANSMISSION_INTERVAL;
        }

        Semaphore_post(self->publisherMutex);
    }
}

void
MmsGooseControlBlock_observedObjectChanged(MmsGooseControlBlock self)
{
    Semaphore_wait(self->publisherMutex);

    uint64_t currentTime = GoosePublisher_increaseStNum(self->publisher);

    self->retransmissionsLeft = CONFIG_GOOSE_EVENT_RETRANSMISSION_COUNT;

    if (self->retransmissionsLeft > 0) {
        self->nextPublishTime = currentTime +
                CONFIG_GOOSE_EVENT_RETRANSMISSION_INTERVAL;

        GoosePublisher_setTimeAllowedToLive(self->publisher,
                           CONFIG_GOOSE_EVENT_RETRANSMISSION_INTERVAL * 3);
    }
    else {
        self->nextPublishTime = currentTime +
            CONFIG_GOOSE_STABLE_STATE_TRANSMISSION_INTERVAL;

        GoosePublisher_setTimeAllowedToLive(self->publisher,
                           CONFIG_GOOSE_STABLE_STATE_TRANSMISSION_INTERVAL * 3);
    }

    GoosePublisher_publish(self->publisher, self->dataSetValues);

    Semaphore_post(self->publisherMutex);
}

static MmsVariableSpecification*
createMmsGooseControlBlock(char* gcbName)
{
    MmsVariableSpecification* gcb = (MmsVariableSpecification*) calloc(1, sizeof(MmsVariableSpecification));
    gcb->name = copyString(gcbName);
    gcb->type = MMS_STRUCTURE;
    gcb->typeSpec.structure.elementCount = 9;
    gcb->typeSpec.structure.elements = (MmsVariableSpecification**) calloc(9, sizeof(MmsVariableSpecification*));

    MmsVariableSpecification* namedVariable;

    namedVariable = (MmsVariableSpecification*) calloc(1, sizeof(MmsVariableSpecification));
    namedVariable->name = copyString("GoEna");
    namedVariable->type = MMS_BOOLEAN;

    gcb->typeSpec.structure.elements[0] = namedVariable;

    namedVariable = (MmsVariableSpecification*) calloc(1, sizeof(MmsVariableSpecification));
    namedVariable->name = copyString("GoID");
    namedVariable->typeSpec.visibleString = -129;
    namedVariable->type = MMS_VISIBLE_STRING;

    gcb->typeSpec.structure.elements[1] = namedVariable;

    namedVariable = (MmsVariableSpecification*) calloc(1, sizeof(MmsVariableSpecification));
    namedVariable->name = copyString("DatSet");
    namedVariable->typeSpec.visibleString = -129;
    namedVariable->type = MMS_VISIBLE_STRING;

    gcb->typeSpec.structure.elements[2] = namedVariable;

    namedVariable = (MmsVariableSpecification*) calloc(1, sizeof(MmsVariableSpecification));
    namedVariable->name = copyString("ConfRev");
    namedVariable->type = MMS_UNSIGNED;
    namedVariable->typeSpec.unsignedInteger = 32;

    gcb->typeSpec.structure.elements[3] = namedVariable;

    namedVariable = (MmsVariableSpecification*) calloc(1, sizeof(MmsVariableSpecification));
    namedVariable->name = copyString("NdsCom");
    namedVariable->type = MMS_BOOLEAN;

    gcb->typeSpec.structure.elements[4] = namedVariable;

    namedVariable = (MmsVariableSpecification*) calloc(1, sizeof(MmsVariableSpecification));
    namedVariable->name = copyString("DstAddress");
    MmsMapping_createPhyComAddrStructure(namedVariable);

    gcb->typeSpec.structure.elements[5] = namedVariable;

    namedVariable = (MmsVariableSpecification*) calloc(1, sizeof(MmsVariableSpecification));
    namedVariable->name = copyString("MinTime");
    namedVariable->type = MMS_UNSIGNED;
    namedVariable->typeSpec.unsignedInteger = 32;

    gcb->typeSpec.structure.elements[6] = namedVariable;

    namedVariable = (MmsVariableSpecification*) calloc(1, sizeof(MmsVariableSpecification));
    namedVariable->name = copyString("MaxTime");
    namedVariable->type = MMS_UNSIGNED;
    namedVariable->typeSpec.unsignedInteger = 32;

    gcb->typeSpec.structure.elements[7] = namedVariable;

    namedVariable = (MmsVariableSpecification*) calloc(1, sizeof(MmsVariableSpecification));
    namedVariable->name = copyString("FixedOffs");
    namedVariable->type = MMS_BOOLEAN;

    gcb->typeSpec.structure.elements[8] = namedVariable;

    return gcb;
}

static GSEControlBlock*
getGCBForLogicalNodeWithIndex(MmsMapping* self, LogicalNode* logicalNode, int index)
{
    int gseCount = 0;

    GSEControlBlock** gseControlBlocks = self->model->gseCBs;

    int i = 0;

    while (gseControlBlocks[i] != NULL ) {
        if (gseControlBlocks[i]->parent == logicalNode) {
            if (gseCount == index)
                return gseControlBlocks[i];

            gseCount++;
        }

        i++;
    }

    return NULL ;
}

static char*
createDataSetReference(char* domainName, char* lnName, char* dataSetName)
{
    char* dataSetReference;

    dataSetReference = createString(5, domainName, "/", lnName, "$", dataSetName);

    return dataSetReference;
}

MmsVariableSpecification*
GOOSE_createGOOSEControlBlocks(MmsMapping* self, MmsDomain* domain,
        LogicalNode* logicalNode, int gseCount)
{
    MmsVariableSpecification* namedVariable = (MmsVariableSpecification*) calloc(1,
            sizeof(MmsVariableSpecification));
    namedVariable->name = copyString("GO");
    namedVariable->type = MMS_STRUCTURE;

    namedVariable->typeSpec.structure.elementCount = gseCount;
    namedVariable->typeSpec.structure.elements = (MmsVariableSpecification**) calloc(gseCount,
            sizeof(MmsVariableSpecification*));

    int currentGCB = 0;

    while (currentGCB < gseCount) {
        GSEControlBlock* gooseControlBlock = getGCBForLogicalNodeWithIndex(
                self, logicalNode, currentGCB);

        MmsVariableSpecification* gseTypeSpec = createMmsGooseControlBlock(gooseControlBlock->name);

        MmsValue* gseValues = MmsValue_newStructure(gseTypeSpec);

        namedVariable->typeSpec.structure.elements[currentGCB] = gseTypeSpec;

        MmsGooseControlBlock mmsGCB = MmsGooseControlBlock_create();


        mmsGCB->goCBRef = createString(5, MmsDomain_getName(domain), "/", logicalNode->name,
                "$GO$", gooseControlBlock->name);

        if (gooseControlBlock->appId != NULL) {
            // gcb->goID = copyString(gooseControlBlock->appId);
            MmsValue* goID = MmsValue_getElement(gseValues, 1);

            MmsValue_setVisibleString(goID, gooseControlBlock->appId);
        }

        if (gooseControlBlock->dataSetName != NULL)
        	mmsGCB->dataSetRef = createDataSetReference(MmsDomain_getName(domain),
        			logicalNode->name, gooseControlBlock->dataSetName);
        else
        	mmsGCB->dataSetRef = NULL;

        MmsValue* dataSetRef = MmsValue_getElement(gseValues, 2);

        MmsValue_setVisibleString(dataSetRef, mmsGCB->dataSetRef);

        /* Set communication parameters */
        uint8_t priority = CONFIG_GOOSE_DEFAULT_PRIORITY;
        uint8_t dstAddr[] = CONFIG_GOOSE_DEFAULT_DST_ADDRESS;
        uint16_t vid = CONFIG_GOOSE_DEFAULT_VLAN_ID;
        uint16_t appId = CONFIG_GOOSE_DEFAULT_APPID;

        if (gooseControlBlock->address != NULL) {
            priority = gooseControlBlock->address->vlanPriority;
            vid = gooseControlBlock->address->vlanId;
            appId = gooseControlBlock->address->appId;

            int i;
            for (i = 0; i < 6; i++) {
                dstAddr[i] = gooseControlBlock->address->dstAddress[i];
            }
        }

        MmsValue* dstAddress = MmsValue_getElement(gseValues, 5);
        MmsValue* addr = MmsValue_getElement(dstAddress, 0);
        MmsValue_setOctetString(addr, dstAddr, 6);

        MmsValue* prio = MmsValue_getElement(dstAddress, 1);
        MmsValue_setUint8(prio, priority);

        MmsValue* vlanId = MmsValue_getElement(dstAddress, 2);
        MmsValue_setUint16(vlanId, vid);

        MmsValue* appIdVal = MmsValue_getElement(dstAddress, 3);
        MmsValue_setUint16(appIdVal, appId);

        mmsGCB->dataSet = NULL;

        mmsGCB->mmsValue = gseValues;
        mmsGCB->mmsType = gseTypeSpec;
        mmsGCB->name = gooseControlBlock->name;

        mmsGCB->domain = domain;
        mmsGCB->logicalNode = logicalNode;

        mmsGCB->mmsMapping = self;

        LinkedList_add(self->gseControls, mmsGCB);

        currentGCB++;
    }

    return namedVariable;
}

#endif

