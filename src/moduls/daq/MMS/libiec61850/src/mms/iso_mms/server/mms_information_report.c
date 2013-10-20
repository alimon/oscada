/*
 *  mms_information_report.c
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

#include "libiec61850_platform_includes.h"

#include "mms_server_internal.h"
#include "mms_common_internal.h"
#include "mms_access_result.h"

#include "ber_encoder.h"

void
MmsServerConnection_sendInformationReportSingleVariableVMDSpecific(MmsServerConnection* self,
		char* itemId, MmsValue* value)
{
	uint32_t itemIdSize = strlen(itemId);
	uint32_t varSpecSize = 1 + BerEncoder_determineLengthSize(itemIdSize) + itemIdSize;
	uint32_t sequenceSize = 1 + BerEncoder_determineLengthSize(varSpecSize) + varSpecSize;
	uint32_t listOfVariableSize = 1 + BerEncoder_determineLengthSize(sequenceSize) + sequenceSize;

	uint32_t accessResultSize = mmsServer_encodeAccessResult(value, NULL, 0, false);

	uint32_t listOfAccessResultSize = 1 + BerEncoder_determineLengthSize(accessResultSize) + accessResultSize;

	uint32_t variableSpecSize = 1 + BerEncoder_determineLengthSize(listOfVariableSize) + listOfVariableSize;

	uint32_t informationReportContentSize = variableSpecSize + listOfAccessResultSize;

	uint32_t informationReportSize = 1 + BerEncoder_determineLengthSize(informationReportContentSize) +
			informationReportContentSize;

	if (DEBUG) printf("sendInfReportSingle variable: %s\n", itemId);

	ByteBuffer* reportBuffer = ByteBuffer_create(NULL, self->maxPduSize);

	uint8_t* buffer = reportBuffer->buffer;
	int bufPos = 0;

	/* encode information report header */
	bufPos = BerEncoder_encodeTL(0xa3, informationReportSize, buffer, bufPos);
	bufPos = BerEncoder_encodeTL(0xa0, informationReportContentSize, buffer, bufPos);

	/* encode list of variable access specifications */
	bufPos = BerEncoder_encodeTL(0xa0, listOfVariableSize, buffer, bufPos);
	bufPos = BerEncoder_encodeTL(0x30, sequenceSize, buffer, bufPos);
	bufPos = BerEncoder_encodeTL(0xa0, varSpecSize, buffer, bufPos);
	bufPos = BerEncoder_encodeStringWithTag(0x80, itemId, buffer, bufPos);

	/* encode access result (variable value) */
	bufPos = BerEncoder_encodeTL(0xa0, accessResultSize, buffer, bufPos);
	bufPos = mmsServer_encodeAccessResult(value, buffer, bufPos, true);

    reportBuffer->size = bufPos;

    IsoConnection_sendMessage(self->isoConnection, reportBuffer);

    ByteBuffer_destroy(reportBuffer);
}

void
MmsServerConnection_sendInformationReportListOfVariables(
        MmsServerConnection* self,
        LinkedList /* MmsVariableAccessSpecification */ variableAccessDeclarations,
        LinkedList /* MmsValue */ values
        )
{
    /* determine message size */
    uint32_t listOfVarSpecSize = 0;

    int listSize = LinkedList_size(variableAccessDeclarations);

    uint32_t* varSpecSize = (uint32_t*) alloca(listSize * sizeof(uint32_t));

    int i = 0;

    LinkedList specElement = LinkedList_getNext(variableAccessDeclarations);

    while (specElement != NULL) {
        MmsVariableAccessSpecification* spec = (MmsVariableAccessSpecification*) specElement->data;

        varSpecSize[i] = BerEncoder_determineEncodedStringSize(spec->itemId);

        if (spec->domainId != NULL) {
            varSpecSize[i] += BerEncoder_determineEncodedStringSize(spec->domainId);
            varSpecSize[i] += BerEncoder_determineLengthSize(varSpecSize[i]) + 1;
        }

        listOfVarSpecSize += varSpecSize[i];

        i++;
        specElement = LinkedList_getNext(specElement);
    }

    uint32_t sequenceSize = 1 + BerEncoder_determineLengthSize(listOfVarSpecSize) + listOfVarSpecSize;
    uint32_t listOfVariableSize = 1 + BerEncoder_determineLengthSize(sequenceSize) + sequenceSize;


    uint32_t accessResultSize = 0;

    LinkedList valueElement = LinkedList_getNext(values);

    while (valueElement != NULL) {
        MmsValue* value = (MmsValue*) valueElement->data;

        accessResultSize += mmsServer_encodeAccessResult(value, NULL, 0, false);

        valueElement = LinkedList_getNext(valueElement);
    }

    uint32_t listOfAccessResultSize = 1 + BerEncoder_determineLengthSize(accessResultSize) + accessResultSize;

    uint32_t variableSpecSize = 1 + BerEncoder_determineLengthSize(listOfVariableSize) + listOfVariableSize;

    uint32_t informationReportContentSize = variableSpecSize + listOfAccessResultSize;

    uint32_t informationReportSize = 1 + BerEncoder_determineLengthSize(informationReportContentSize) +
            informationReportContentSize;

    /* encode message */
    ByteBuffer* reportBuffer = ByteBuffer_create(NULL, self->maxPduSize);

    uint8_t* buffer = reportBuffer->buffer;
    int bufPos = 0;

    /* encode information report header */
    bufPos = BerEncoder_encodeTL(0xa3, informationReportSize, buffer, bufPos);
    bufPos = BerEncoder_encodeTL(0xa0, informationReportContentSize, buffer, bufPos);

    /* encode list of variable access specifications */
    bufPos = BerEncoder_encodeTL(0xa0, listOfVariableSize, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0x30, sequenceSize, buffer, bufPos);

    specElement = LinkedList_getNext(variableAccessDeclarations);
    i = 0;

    while (specElement != NULL) {
        MmsVariableAccessSpecification* spec = (MmsVariableAccessSpecification*) specElement->data;

        if (spec->domainId != NULL) {
            bufPos = BerEncoder_encodeTL(0xa0, varSpecSize[i], buffer, bufPos); /* domain-specific */
            bufPos = BerEncoder_encodeTL(0xa1, varSpecSize[i] - 2, buffer, bufPos);
            bufPos = BerEncoder_encodeStringWithTag(0x1a, spec->domainId, buffer, bufPos);
            bufPos = BerEncoder_encodeStringWithTag(0x1a, spec->itemId, buffer, bufPos);
        }
        else {
            bufPos = BerEncoder_encodeTL(0xa0, varSpecSize[i], buffer, bufPos); /* vmd-specific */
            bufPos = BerEncoder_encodeStringWithTag(0x80, spec->itemId, buffer, bufPos);
        }


        i++;
        specElement = LinkedList_getNext(specElement);
    }

    /* encode list of access results (variable values) */
    bufPos = BerEncoder_encodeTL(0xa0, accessResultSize, buffer, bufPos);

    valueElement = LinkedList_getNext(values);

    while (valueElement != NULL) {
        MmsValue* value = (MmsValue*) valueElement->data;

        bufPos = mmsServer_encodeAccessResult(value, buffer, bufPos, true);

        valueElement = LinkedList_getNext(valueElement);
    }

    reportBuffer->size = bufPos;

    IsoConnection_sendMessage(self->isoConnection, reportBuffer);

    ByteBuffer_destroy(reportBuffer);
}


void /* send information report for a named variable list */
MmsServerConnection_sendInformationReportVMDSpecific(MmsServerConnection* self, char* itemId, LinkedList values)
{

    uint32_t variableAccessSpecSize = 0;
    uint32_t objectNameSize = 0;

    uint32_t accessResultSize = 0;
    uint32_t listOfAccessResultSize;
    uint32_t informationReportSize;

	objectNameSize = BerEncoder_determineEncodedStringSize(itemId);
	variableAccessSpecSize += objectNameSize;
	variableAccessSpecSize += BerEncoder_determineLengthSize(objectNameSize);
	variableAccessSpecSize += 1; /* space for tag (a1) */

    int variableCount = LinkedList_size(values);

    /* iterate values list and add values to the accessResultList */
    LinkedList value = LinkedList_getNext(values);

    int i;

    for (i = 0; i < variableCount; i++) {

       MmsValue* data = (MmsValue*) value->data;

       accessResultSize += mmsServer_encodeAccessResult(data, NULL, 0, false);

        value = LinkedList_getNext(value);
    }

    listOfAccessResultSize = accessResultSize +
            BerEncoder_determineLengthSize(accessResultSize) + 1;

    uint32_t informationReportContentSize = variableAccessSpecSize + listOfAccessResultSize;

    informationReportSize = 1 +  informationReportContentSize +
            BerEncoder_determineLengthSize(informationReportContentSize);

    if (DEBUG) printf("sendInfReport: %i items\n", variableCount);

    ByteBuffer* reportBuffer = ByteBuffer_create(NULL, self->maxPduSize);

    uint8_t* buffer = reportBuffer->buffer;
    int bufPos = 0;


    /* encode */
    bufPos = BerEncoder_encodeTL(0xa3, informationReportSize, buffer, bufPos);
    bufPos = BerEncoder_encodeTL(0xa0, informationReportContentSize, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0xa1, objectNameSize, buffer, bufPos);
    bufPos = BerEncoder_encodeStringWithTag(0x80, itemId, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0xa0, accessResultSize, buffer, bufPos);

    value = LinkedList_getNext(values);

    for (i = 0; i < variableCount; i++) {

        MmsValue* data = (MmsValue*) value->data;

        bufPos = mmsServer_encodeAccessResult(data, buffer, bufPos, true);

        value = LinkedList_getNext(value);
    }

    reportBuffer->size = bufPos;

    IsoConnection_sendMessage(self->isoConnection, reportBuffer);

    ByteBuffer_destroy(reportBuffer);
}



