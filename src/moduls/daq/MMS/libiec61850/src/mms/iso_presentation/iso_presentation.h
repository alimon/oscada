/*
 *  iso_presentation.h
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

#ifndef ISO_PRESENTATION_H_
#define ISO_PRESENTATION_H_

#include "byte_buffer.h"
#include "buffer_chain.h"

typedef struct {
	uint32_t callingPresentationSelector;
	uint32_t calledPresentationSelector;
	uint8_t nextContextId;
	uint8_t acseContextId;
	uint8_t mmsContextId;
	ByteBuffer nextPayload;
} IsoPresentation;

void
IsoPresentation_init(IsoPresentation* session);

int
IsoPresentation_parseUserData(IsoPresentation* session, ByteBuffer* message);

int
IsoPresentation_parseConnect(IsoPresentation* session, ByteBuffer* message);

void
IsoPresentation_createConnectPdu(
		IsoPresentation* self, ByteBuffer* writeBuffer, ByteBuffer* payload);

void
IsoPresentation_createCpaMessage(
		IsoPresentation* presentation,
		ByteBuffer* writeBuffer,
		ByteBuffer* payload
);

void
IsoPresentation_createCpaMessageBC(IsoPresentation* self, BufferChain writeBuffer, BufferChain payload);

void
IsoPresentation_createUserData(IsoPresentation* self, ByteBuffer* writeBuffer, ByteBuffer* payload);

void
IsoPresentation_createUserDataBC(IsoPresentation* self, BufferChain writeBuffer, BufferChain payload);

int
IsoPresentation_parseAcceptMessage(IsoPresentation* self, ByteBuffer* byteBuffer);

#endif /* ISO_PRESENTATION_H_ */
