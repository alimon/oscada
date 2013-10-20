/*
 *  asn1_ber_primitive_value.c
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

#include "asn1_ber_primitive_value.h"

Asn1PrimitiveValue*
Asn1PrimitiveValue_create(int size)
{
	Asn1PrimitiveValue* self = (Asn1PrimitiveValue*) malloc(sizeof(Asn1PrimitiveValue));

	self->size = size;
	self->maxSize = size;
	self->octets = (uint8_t*) calloc(1, size);

	return self;
}

//Asn1PrimitiveValue*
//Asn1PrimitiveValue_createFromBuffer(uint8_t buffer, int bufferSize)
//{
//    Asn1PrimitiveValue* self = malloc(sizeof(Asn1PrimitiveValue));
//    self->size = bufferSize;
//    self->maxSize = bufferSize;
//    self->octets = malloc(1, bufferSize);
//
//}

Asn1PrimitiveValue*
Asn1PrimitiveValue_clone(Asn1PrimitiveValue* self)
{
	Asn1PrimitiveValue* clone = (Asn1PrimitiveValue*) malloc(sizeof(Asn1PrimitiveValue));

	clone->size = self->size;
	clone->maxSize = self->maxSize;

	clone->octets = (uint8_t*) malloc(self->maxSize);

	memcpy(clone->octets, self->octets, clone->maxSize);

	return clone;
}

bool
Asn1PrimitivaValue_compare(Asn1PrimitiveValue* self, Asn1PrimitiveValue* otherValue)
{
    if (self->size == otherValue->size) {
        if (memcmp(self->octets, otherValue->octets, self->size) == 0)
            return true;
        else
            return false;
    }
    else
        return false;
}

int
Asn1PrimitiveValue_getSize(Asn1PrimitiveValue* self)
{
	return self->size;
}

int
Asn1PrimitiveValue_getMaxSize(Asn1PrimitiveValue* self)
{
	return self->maxSize;
}

void
Asn1PrimitiveValue_destroy(Asn1PrimitiveValue* self)
{
	free(self->octets);
	free(self);
}
