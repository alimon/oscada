/*
 *  mms_named_variable_list.c
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

#include "mms_named_variable_list.h"

MmsNamedVariableListEntry
MmsNamedVariableListEntry_create(MmsAccessSpecifier accessSpecifier)
{
	MmsNamedVariableListEntry listEntry = (MmsNamedVariableListEntry) malloc(sizeof(MmsAccessSpecifier));

	listEntry->domain = accessSpecifier.domain;
	listEntry->variableName = copyString(accessSpecifier.variableName);
	listEntry->arrayIndex = accessSpecifier.arrayIndex;

	if (accessSpecifier.componentName != NULL)
		listEntry->componentName = copyString(accessSpecifier.componentName);
	else
		listEntry->componentName = NULL;

	return listEntry;
}

void
MmsNamedVariableListEntry_destroy(MmsNamedVariableListEntry self)
{
	free(self->variableName);
	free(self);
}


MmsDomain*
MmsNamedVariableListEntry_getDomain(MmsNamedVariableListEntry self)
{
	return self->domain;
}

char*
MmsNamedVariableListEntry_getVariableName(MmsNamedVariableListEntry self) {
	return self->variableName;
}

MmsNamedVariableList
MmsNamedVariableList_create(char* name, bool deletable)
{
	MmsNamedVariableList self = (MmsNamedVariableList) malloc(sizeof(struct sMmsNamedVariableList));

	self->deletable = deletable;
	self->name = copyString(name);
	self->listOfVariables = LinkedList_create();

	return self;
}

char*
MmsNamedVariableList_getName(MmsNamedVariableList self)
{
	return self->name;
}

bool
MmsNamedVariableList_isDeletable(MmsNamedVariableList self)
{
	return self->deletable;
}

void
MmsNamedVariableList_addVariable(MmsNamedVariableList self, MmsNamedVariableListEntry variable)
{
	LinkedList_add(self->listOfVariables, variable);
}

LinkedList
MmsNamedVariableList_getVariableList(MmsNamedVariableList self)
{
	return self->listOfVariables;
}

static void
deleteVariableListEntry(void* listEntry)
{
	MmsNamedVariableListEntry entry = (MmsNamedVariableListEntry) listEntry;
	MmsNamedVariableListEntry_destroy(entry);
}

void
MmsNamedVariableList_destroy(MmsNamedVariableList self)
{
	LinkedList_destroyDeep(self->listOfVariables, deleteVariableListEntry);
	free(self->name);
	free(self);
}


