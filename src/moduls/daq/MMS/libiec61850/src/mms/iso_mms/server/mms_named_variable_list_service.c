/*
 *  mms_named_variable_list_service.c
 *
 *  Copyright 2013 Michael Zillgith
 *
 *	This file is part of libIEC61850.
 *
 *	libIEC61850 is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	libIEC61850 is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with libIEC61850.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	See COPYING file for the complete license text.
 */

#include "mms_server_internal.h"
#include "mms_named_variable_list.h"

#include "string_utilities.h"
#include "ber_encoder.h"

#if MMS_DATA_SET_SERVICE == 1

static void
createDeleteNamedVariableListResponse(uint32_t invokeId, ByteBuffer* response,
        uint32_t numberMatched, uint32_t numberDeleted)
{
    uint32_t invokeIdSize = BerEncoder_UInt32determineEncodedSize(invokeId) + 2;

    uint32_t numberMatchedSize =
            2 + BerEncoder_UInt32determineEncodedSize(numberMatched);

    uint32_t numberDeletedSize =
            2 + BerEncoder_UInt32determineEncodedSize(numberDeleted);

    uint32_t deleteNVLSize = 2 + numberMatchedSize + numberDeletedSize;

    uint32_t confirmedResponsePDUSize = invokeIdSize + deleteNVLSize;

    int bufPos = 0;
    uint8_t* buffer = response->buffer;

    bufPos = BerEncoder_encodeTL(0xa1, confirmedResponsePDUSize, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0x02, invokeIdSize - 2, buffer, bufPos);
    bufPos = BerEncoder_encodeUInt32(invokeId, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0xad, numberMatchedSize + numberDeletedSize, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0x80, numberMatchedSize - 2, buffer, bufPos);
    bufPos = BerEncoder_encodeUInt32(numberMatched, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0x81, numberDeletedSize - 2, buffer, bufPos);
    bufPos = BerEncoder_encodeUInt32(numberDeleted, buffer, bufPos);

    response->size = bufPos;
}

void
mmsServer_handleDeleteNamedVariableListRequest(MmsServerConnection* connection,
		uint8_t* buffer, int bufPos, int maxBufPos,
		int invokeId,
		ByteBuffer* response)
{
	DeleteNamedVariableListRequest_t* request = 0;

    MmsPdu_t* mmsPdu = 0;

    asn_dec_rval_t rval; /* Decoder return value  */

    rval = ber_decode(NULL, &asn_DEF_MmsPdu, (void**) &mmsPdu, buffer, maxBufPos);

    request = &(mmsPdu->choice.confirmedRequestPdu.confirmedServiceRequest.choice.deleteNamedVariableList);

	long scopeOfDelete;

	asn_INTEGER2long(request->scopeOfDelete, &scopeOfDelete);

	MmsDevice* device = MmsServer_getDevice(connection->server);

	if (scopeOfDelete == DeleteNamedVariableListRequest__scopeOfDelete_specific) {
		int numberMatched = 0;
		int numberDeleted = 0;

		int numberItems = request->listOfVariableListName->list.count;

		int i;

		for (i = 0; i < numberItems; i++) {
			if (request->listOfVariableListName->list.array[i]->present == ObjectName_PR_domainspecific) {
				char* domainId = mmsMsg_createStringFromAsnIdentifier(
						request->listOfVariableListName->list.array[i]->choice.domainspecific.domainId);

				MmsDomain* domain = MmsDevice_getDomain(device, domainId);

				char* itemId = mmsMsg_createStringFromAsnIdentifier(
						request->listOfVariableListName->list.array[i]->choice.domainspecific.itemId);

				MmsNamedVariableList variableList = MmsDomain_getNamedVariableList(domain, itemId);

				if (variableList != NULL) {
					numberMatched++;

					if (MmsNamedVariableList_isDeletable(variableList)) {
						MmsDomain_deleteNamedVariableList(domain, itemId);
						numberDeleted++;
					}
				}

				free(domainId);
				free(itemId);
			}
			else if (request->listOfVariableListName->list.array[i]->present == ObjectName_PR_aaspecific) {
				char* itemId = mmsMsg_createStringFromAsnIdentifier(
						request->listOfVariableListName->list.array[i]->choice.aaspecific);

				MmsNamedVariableList variableList = MmsServerConnection_getNamedVariableList(connection, itemId);

				if (variableList != NULL) {
					numberMatched++;
					numberDeleted++;

					MmsServerConnection_deleteNamedVariableList(connection, itemId);
				}

				free(itemId);
			}
			//TODO else send error???
		}

		createDeleteNamedVariableListResponse(invokeId, response, numberMatched, numberDeleted);
	}
	else {
		mmsServer_createConfirmedErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_ACCESS_UNSUPPORTED);
	}

    asn_DEF_MmsPdu.free_struct(&asn_DEF_MmsPdu, mmsPdu, 0);
}

static void
createDefineNamedVariableListResponse(uint32_t invokeId, ByteBuffer* response)
{
    uint32_t invokeIdSize = BerEncoder_UInt32determineEncodedSize((uint32_t) invokeId) + 2;

    uint32_t confirmedResponsePDUSize = 2 + invokeIdSize;

    int bufPos = 0;
    uint8_t* buffer = response->buffer;

    bufPos = BerEncoder_encodeTL(0xa1, confirmedResponsePDUSize, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0x02, invokeIdSize - 2, buffer, bufPos);
    bufPos = BerEncoder_encodeUInt32((uint32_t) invokeId, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0x8b, 0, buffer, bufPos);

    response->size = bufPos;
}

static MmsNamedVariableList
createNamedVariableList(MmsDevice* device,
		DefineNamedVariableListRequest_t* request,
		char* variableListName)
{
	MmsNamedVariableList namedVariableList = MmsNamedVariableList_create(variableListName, true);

	int variableCount = request->listOfVariable.list.count;

	int i;
	for (i = 0; i < variableCount; i++) {
		VariableSpecification_t* varSpec =
				&request->listOfVariable.list.array[i]->variableSpecification;

		long arrayIndex = -1;
		char* componentName = NULL;

		/* Handle alternate access specification - for array element definitiion */
		if (request->listOfVariable.list.array[i]->alternateAccess != NULL) {


			if (request->listOfVariable.list.array[i]->alternateAccess->list.count != 1) {
				MmsNamedVariableList_destroy(namedVariableList);
				namedVariableList = NULL;
				break;
			}
			else {

				struct AlternateAccess__Member* alternateAccess =
						request->listOfVariable.list.array[i]->alternateAccess->list.array[0];

				if (alternateAccess->present ==	AlternateAccess__Member_PR_unnamed)
				{
					//TODO add checks!

					asn_INTEGER2long(&(alternateAccess->choice.unnamed->choice.selectAlternateAccess.accessSelection.choice.index),
							&arrayIndex);

					printf("named variable element with array index %li\n", arrayIndex);

					componentName =
						mmsMsg_createStringFromAsnIdentifier(alternateAccess->choice.unnamed->
								choice.selectAlternateAccess.alternateAccess->list.array[0]->
								choice.unnamed->choice.selectAccess.choice.component);

				}
				else {
					MmsNamedVariableList_destroy(namedVariableList);
					namedVariableList = NULL;
					break;
				}

			}

		}

		if (varSpec->present == VariableSpecification_PR_name) {
			char* variableName = createStringFromBuffer(
					varSpec->choice.name.choice.domainspecific.itemId.buf,
					varSpec->choice.name.choice.domainspecific.itemId.size);

			char* domainId = createStringFromBuffer(
					varSpec->choice.name.choice.domainspecific.domainId.buf,
					varSpec->choice.name.choice.domainspecific.domainId.size);

			MmsDomain* domain = MmsDevice_getDomain(device, domainId);

			MmsAccessSpecifier accessSpecifier;

			accessSpecifier.domain = domain;
			accessSpecifier.variableName = variableName;
			accessSpecifier.arrayIndex = arrayIndex;
			accessSpecifier.componentName = componentName;

			MmsNamedVariableListEntry variable =
					MmsNamedVariableListEntry_create(accessSpecifier);

			MmsNamedVariableList_addVariable(namedVariableList, variable);

			free(domainId);
			free(variableName);
		}
		else {
			MmsNamedVariableList_destroy(namedVariableList);
			namedVariableList = NULL;
			break;
		}
	}

	return namedVariableList;
}

void
mmsServer_handleDefineNamedVariableListRequest(
		MmsServerConnection* connection,
		uint8_t* buffer, int bufPos, int maxBufPos,
		uint32_t invokeId,
		ByteBuffer* response)
{
	DefineNamedVariableListRequest_t* request = 0;

	MmsPdu_t* mmsPdu = 0;

	asn_dec_rval_t rval; /* Decoder return value  */

	rval = ber_decode(NULL, &asn_DEF_MmsPdu, (void**) &mmsPdu, buffer, maxBufPos);

	request = &(mmsPdu->choice.confirmedRequestPdu.confirmedServiceRequest.choice.defineNamedVariableList);

	MmsDevice* device = MmsServer_getDevice(connection->server);

	if (request->variableListName.present == ObjectName_PR_domainspecific) {

		char* domainName = createStringFromBuffer(
				request->variableListName.choice.domainspecific.domainId.buf,
				request->variableListName.choice.domainspecific.domainId.size);

		MmsDomain* domain = MmsDevice_getDomain(device, domainName);

		free(domainName);

		if (domain == NULL) {
			mmsServer_createConfirmedErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_NON_EXISTENT);
			return;
		}

		char* variableListName = createStringFromBuffer(
				request->variableListName.choice.domainspecific.itemId.buf,
				request->variableListName.choice.domainspecific.itemId.size);

		if (MmsDomain_getNamedVariableList(domain, variableListName) != NULL) {
            mmsServer_createConfirmedErrorPdu(invokeId, response, MMS_ERROR_DEFINITION_OBJECT_EXISTS);
		}
		else {
		    MmsNamedVariableList namedVariableList = createNamedVariableList(device,
		                    request, variableListName);

	        if (namedVariableList != NULL) {
	            MmsDomain_addNamedVariableList(domain, namedVariableList);
	            createDefineNamedVariableListResponse(invokeId, response);
	        }
	        else
	            mmsServer_createConfirmedErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_ACCESS_UNSUPPORTED);
		}

		free(variableListName);
	}
	else if (request->variableListName.present == ObjectName_PR_aaspecific) {

		char* variableListName = createStringFromBuffer(
				request->variableListName.choice.aaspecific.buf,
				request->variableListName.choice.aaspecific.size);


		if (MmsServerConnection_getNamedVariableList(connection, variableListName) != NULL) {
		    mmsServer_createConfirmedErrorPdu(invokeId, response, MMS_ERROR_DEFINITION_OBJECT_EXISTS);
		}
		else {
            MmsNamedVariableList namedVariableList = createNamedVariableList(device,
                    request, variableListName);

            if (namedVariableList != NULL) {
                MmsServerConnection_addNamedVariableList(connection, namedVariableList);
                createDefineNamedVariableListResponse(invokeId, response);
            }
            else
                mmsServer_createConfirmedErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_ACCESS_UNSUPPORTED);
		}

		free(variableListName);
	}
	else
		mmsServer_createConfirmedErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_ACCESS_UNSUPPORTED);

	asn_DEF_MmsPdu.free_struct(&asn_DEF_MmsPdu, mmsPdu, 0);
}

static void
createGetNamedVariableListAttributesResponse(int invokeId, ByteBuffer* response,
		MmsNamedVariableList variableList, char* domainName)
{
	MmsPdu_t* mmsPdu = mmsServer_createConfirmedResponse(invokeId);

	BOOLEAN_t moreFollows = 0;

	mmsPdu->choice.confirmedResponsePdu.confirmedServiceResponse.present =
			ConfirmedServiceResponse_PR_getNamedVariableListAttributes;

	GetNamedVariableListAttributesResponse_t* varListResponse =
		&(mmsPdu->choice.confirmedResponsePdu.confirmedServiceResponse.
				choice.getNamedVariableListAttributes);

	varListResponse->mmsDeletable = MmsNamedVariableList_isDeletable(variableList);

	LinkedList variables = MmsNamedVariableList_getVariableList(variableList);

	int variableCount = LinkedList_size(variables);

	varListResponse->listOfVariable.list.count = variableCount;
	varListResponse->listOfVariable.list.size = variableCount;

	varListResponse->listOfVariable.list.array = (struct GetNamedVariableListAttributesResponse__listOfVariable__Member**) 
		 calloc(variableCount, sizeof(void*));

	LinkedList variable = LinkedList_getNext(variables);

	int i;
	for (i = 0; i < variableCount; i++) {
		MmsNamedVariableListEntry variableEntry = (MmsNamedVariableListEntry) variable->data;

		varListResponse->listOfVariable.list.array[i] =  (struct GetNamedVariableListAttributesResponse__listOfVariable__Member*) 
			calloc(1, sizeof(struct GetNamedVariableListAttributesResponse__listOfVariable__Member));

		varListResponse->listOfVariable.list.array[i]->variableSpecification.present =
				VariableSpecification_PR_name;

		varListResponse->listOfVariable.list.array[i]->variableSpecification.choice.name.present =
				ObjectName_PR_domainspecific;

		char* variableDomainName = MmsDomain_getName(variableEntry->domain);

		varListResponse->listOfVariable.list.array[i]->variableSpecification.choice.name.choice.
			domainspecific.domainId.buf = (uint8_t*) copyString(variableDomainName);

		varListResponse->listOfVariable.list.array[i]->variableSpecification.choice.name.choice.
			domainspecific.domainId.size = strlen(variableDomainName);

		varListResponse->listOfVariable.list.array[i]->variableSpecification.choice.name.choice.
			domainspecific.itemId.buf = (uint8_t*) copyString(variableEntry->variableName);

		varListResponse->listOfVariable.list.array[i]->variableSpecification.choice.name.choice.
			domainspecific.itemId.size = strlen(variableEntry->variableName);

		variable = LinkedList_getNext(variable);
	}

	asn_enc_rval_t rval;

	rval = der_encode(&asn_DEF_MmsPdu, mmsPdu,
			mmsServer_write_out, (void*) response);

	if (DEBUG) xer_fprint(stdout, &asn_DEF_MmsPdu, mmsPdu);


	asn_DEF_MmsPdu.free_struct(&asn_DEF_MmsPdu, mmsPdu, 0);
}

void
mmsServer_handleGetNamedVariableListAttributesRequest(
		MmsServerConnection* connection,
		uint8_t* buffer, int bufPos, int maxBufPos,
		uint32_t invokeId,
		ByteBuffer* response)
{
	GetNamedVariableListAttributesRequest_t* request = 0;

	asn_dec_rval_t rval; /* Decoder return value  */

	rval = ber_decode(NULL, &asn_DEF_GetNamedVariableListAttributesRequest,
				(void**) &request, buffer + bufPos, maxBufPos - bufPos);

	if (request->present == ObjectName_PR_domainspecific) {

		char* domainName = createStringFromBuffer(
				request->choice.domainspecific.domainId.buf,
				request->choice.domainspecific.domainId.size);

		char* itemName = createStringFromBuffer(
				request->choice.domainspecific.itemId.buf,
				request->choice.domainspecific.itemId.size);

		MmsDevice* mmsDevice = MmsServer_getDevice(connection->server);

		MmsDomain* domain = MmsDevice_getDomain(mmsDevice, domainName);

		if (domain != NULL) {
			MmsNamedVariableList variableList =
					MmsDomain_getNamedVariableList(domain, itemName);

			if (variableList != NULL)
				createGetNamedVariableListAttributesResponse(invokeId, response, variableList, domainName);
			else
				mmsServer_createConfirmedErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_NON_EXISTENT);
		}
		else
			mmsServer_createConfirmedErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_NON_EXISTENT);


		free(domainName);
		free(itemName);
	}
	else {
		mmsServer_createConfirmedErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_ACCESS_UNSUPPORTED);
	}

	asn_DEF_GetVariableAccessAttributesRequest.free_struct(&asn_DEF_GetNamedVariableListAttributesRequest,
			request, 0);
}

#endif
