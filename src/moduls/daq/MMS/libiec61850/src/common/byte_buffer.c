/*
 *  byte_buffer.c
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

#include "libiec61850_platform_includes.h"
#include "byte_buffer.h"

ByteBuffer*
ByteBuffer_create(ByteBuffer* self, int maxSize)
{
	if (self == NULL) {
		self = (ByteBuffer*) calloc(1, sizeof(ByteBuffer));
	}

	self->buffer = (uint8_t*) calloc(maxSize, sizeof(uint8_t));
	self->maxSize = maxSize;
	self->size = 0;

	return self;
}

void
ByteBuffer_destroy(ByteBuffer* self)
{
	free(self->buffer);
	free(self);
}

void
ByteBuffer_wrap(ByteBuffer* self, uint8_t* buf, int size, int maxSize)
{
	self->buffer = buf;
	self->size = size;
	self->maxSize = maxSize;
}

int
ByteBuffer_append(ByteBuffer* self, uint8_t* data, int dataSize)
{
	if (self->size + dataSize <= self->maxSize) {
		memcpy(self->buffer + self->size, data, dataSize);
		self->size += dataSize;
		return 1;
	}
	else
		return 0;
}

int
ByteBuffer_appendByte(ByteBuffer* self, uint8_t byte)
{
	if (self->size  < self->maxSize) {
		self->buffer[self->size] = byte;
		self->size ++;
		return 1;
	}
	else
		return 0;
}


uint8_t*
ByteBuffer_getBuffer(ByteBuffer* self)
{
	return self->buffer;
}

int
ByteBuffer_getMaxSize(ByteBuffer* self)
{
	return self->maxSize;
}

int
ByteBuffer_getSize(ByteBuffer* self)
{
	return self->size;
}

int
ByteBuffer_setSize(ByteBuffer* self, int size)
{
	if (size <= self->maxSize)
		self->size = size;

	return self->size;
}
