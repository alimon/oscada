/***************************************************************************
                          rlinifile.cpp  -  description
                             -------------------
    begin                : Tue Jan 02 2001
    copyright            : (C) 2001 by Rainer Lehrig
    email                : lehrig@t-online.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU LESSER GENERAL PUBLIC LICENSE as        *
 *   published by the Free Software Foundation                             *
 *                                                                         *
 ***************************************************************************/
#include "rldefine.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifdef RLWIN32
#include <windows.h>
#endif
#include "rlinifile.h"
#include "rlcutil.h"
#include "rlsocket.h"

static const char null_string[] = "";

rlIniFile::rlIniFile()
{
  _firstSection = new rlSection;      // first section holds names with section name = null_string
  _firstSection->nextSection = NULL;
  _firstSection->firstName = NULL;
  _firstSection->name = new char[1];
  _firstSection->name[0] = '\0';
  currentSection = currentName = -1;
}

rlIniFile::~rlIniFile()
{
  rlSection *section, *last_section;

  section = _firstSection;
  while(section != NULL)
  {
    deleteSectionNames(section);
    last_section = section;
    section = section->nextSection;
    delete last_section;
  }
}

void rlIniFile::deleteSectionNames(rlSection *section)
{
  rlSectionName *name, *last_name;

  if(section == NULL) return;
  name = section->firstName;
  while(name != NULL)
  {
    if(name->name  != NULL) delete [] name->name;
    if(name->param != NULL) delete [] name->param;
    last_name = name;
    name = name->nextName;
    delete last_name;
  }
  if(section->name != NULL) delete [] section->name;
}

void rlIniFile::copyIdentifier(char *buf, const char *line)
{
  int i = 1;
  buf[0] = '\0';
  if(line[0] != '[') return;
  while(line[i] != ']' && line[i] != '\0')
  {
    *buf++ = line[i++];
  }
  *buf = '\0';
}

void rlIniFile::copyName(char *buf, const char *line)
{
  int i = 0;
  buf[0] = '\0';
  //while(line[i] > ' ' && line[i] != '=')
  while(line[i] != '\0' && line[i] != '=')
  {
    *buf++ = line[i++];
  }
  *buf = '\0';
  i--; // eventually delete spaces
  while(i>=0 && (buf[i] == ' ' || buf[i] == '\t'))
  {
    buf[i--] = '\0';
  } 
}

void rlIniFile::copyParam(char *buf, const char *line)
{
  const char *cptr;
  buf[0] = '\0';
  cptr = strchr(line,'=');
  if(cptr == NULL) return;
  cptr++;
  while((*cptr == ' ' || *cptr == '\t') && *cptr != '\0') cptr++;
  if(*cptr == '\0') return;
  while(*cptr != '\0' && *cptr != '\n') *buf++ = *cptr++;
  *buf = '\0';
}


int rlIniFile::read(const char * filename)
{
    return read(filename, NULL);
}

int rlIniFile::read(rlSocket *  sock)
{
    return read(NULL, sock);
}


int rlIniFile::read(const char *filename, rlSocket * sock)
{
  FILE *fp;
//  int len;
  char line[rl_PRINTF_LENGTH],
       name_section[rl_PRINTF_LENGTH],
       name_name[rl_PRINTF_LENGTH],
       name_param[rl_PRINTF_LENGTH];
  rlSection *s, *s_old;

  // delete old content
  s = _firstSection;
  while(s != NULL)
  {
    deleteSectionNames(s);
    s_old = s;
    s = s->nextSection;
    delete s_old;
  }

  // read the file
  _firstSection = new rlSection;      // first section holds names with section name = null_string
  _firstSection->nextSection = NULL;
  _firstSection->firstName = NULL;
  _firstSection->name = new char[1];
  _firstSection->name[0] = '\0';
  if(filename != NULL)
  {
      fp = fopen(filename,"r");
      if(fp == NULL) return -1;
  }
  name_section[0] = name_name[0] = name_param[0] = '\0';
  while((filename != 0 ? (fgets(line,sizeof(line)-1,fp) != NULL) : (sock->readStr(line, sizeof(line)-1) > 0 )) != 0)
  {
    int len = strlen(line)-1;
    if(line[len] == '\r')
	line[len] = 0;
	
    if(line[0] == '[') // section identifier
    {
      copyIdentifier(name_section,line);
      setText(name_section, NULL, NULL);
    }
    else if(line[0] > ' ' && line[0] != '\t' && line[0] != '#') // name identifier
    {
      copyName(name_name,line);
      copyParam(name_param,line);
      setText(name_section, name_name, name_param);
    }
    else // it must be a comment line
    {
      len = strlen(line);
      if(len>0) line[len-1] = '\0';
      setText(name_section, line, NULL);
    }
  }
  if(filename != NULL)
      fclose(fp);
  return 0;
}

int rlIniFile::write(const char *filename)
{
  FILE *fp;
  rlSection *s;
  rlSectionName *n;

  fp = fopen(filename,"w");
  if(fp == NULL) return -1;

  s = _firstSection;
  while(s != NULL)
  {
    if     (s->name[0] == '#')  fprintf(fp,"%s\n",s->name);
    else if(s->name[0] == '\0') ;
    else                        fprintf(fp,"[%s]\n",s->name);
    n = s->firstName;
    while(n != NULL)
    {
      if     (n->name[0]  == '#')  fprintf(fp,"%s\n",n->name);
      else if(n->name[0]  == '\0') fprintf(fp,"\n");
      else if(n->param[0] == '\0') fprintf(fp,"\n");
      else                         fprintf(fp,"%s=%s\n",n->name,n->param);
      n = n->nextName;
    }
    s = s->nextSection;
  }

  fclose(fp);
  return 0;
}

const char *rlIniFile::text(const char *section, const char *name)
{
  rlSection *s;
  rlSectionName *n;

  s = _firstSection;
  while(s != NULL)
  {
    if(strcmp(section,s->name) == 0)
    {
      n = s->firstName;
      while(n != NULL)
      {
        if(n->name != NULL && strcmp(name,n->name) == 0)
        {
          return n->param;
        }
        n = n->nextName;
      }
      return null_string;
    }
    s = s->nextSection;
  }
  return null_string;
}

void rlIniFile::setText(const char *section, const char *name, const char *text)
{
  rlSection *s, *last_section;
  rlSectionName *n, *last_name;

  if(section == NULL) return;
  last_section = NULL;
  last_name = NULL;
  s = _firstSection;
  while(s != NULL)
  {
    if(strcmp(section,s->name) == 0)
    {
      last_name = NULL;
      n = s->firstName;
      while(n != NULL)
      {
        if(name != NULL && name[0] != '#' && name[0] != '\0' && strcmp(name,n->name) == 0)
        {
          if(n->param != NULL) delete [] n->param;
          if(text == NULL)
          {
            n->param = new char[1];
            n->param[0] = '\0';
          }
          else
          {
            n->param = new char[strlen(text)+1];
            strcpy(n->param,text);
          }
          return;
        }
        last_name = n;
        n = n->nextName;
      }
      if(last_name == NULL)
      {
        s->firstName = new rlSectionName;
        n = s->firstName;
      }
      else
      {
        last_name->nextName = new rlSectionName;
        n = last_name->nextName;
      }
      if(name == NULL)
      {
        n->name = new char[1];
        n->name[0] = '\0';
      }
      else
      {
        n->name = new char[strlen(name)+1];
        strcpy(n->name,name);
      }
      if(text == NULL)
      {
        n->param = new char[1];
        n->param[0] = '\0';
      }
      else
      {
        n->param = new char[strlen(text)+1];
        strcpy(n->param,text);
      }
      n->nextName = NULL;
      return;
    }
    last_section = s;
    s = s->nextSection;
  }
  if(last_section == NULL)
  {
    _firstSection = new rlSection;
    last_section = _firstSection;
  }
  else
  {
    last_section->nextSection = new rlSection;
    last_section = last_section->nextSection;
  }
  last_section->name = new char[strlen(section)+1];
  strcpy(last_section->name,section);
  last_section->nextSection = NULL;
  if(name == NULL)
  {
    last_section->firstName = NULL;
  }
  else
  {
    last_section->firstName = new rlSectionName;
    n = last_section->firstName;
    n->name = new char[strlen(name)+1];
    strcpy(n->name,name);
    if(text == NULL)
    {
      n->param = new char[1];
      n->param[0] = '\0';
    }
    else
    {
      n->param = new char[strlen(text)+1];
      strcpy(n->param,text);
    }
    n->nextName = NULL;
  }
  return;
}

int rlIniFile::printf(const char *section, const char *name, const char *format, ...)
{
  int ret;
  char buf[rl_PRINTF_LENGTH]; // should be big enough
  
  va_list ap;
  va_start(ap,format);
  ret = rlvsnprintf(buf, rl_PRINTF_LENGTH - 1, format, ap);
  va_end(ap);
  if(ret > 0) setText(section, name, buf);
  return ret;
}

void rlIniFile::remove(const char *section)
{
  rlSection *s, *last;

  last = NULL;
  s = _firstSection;
  while(s != NULL)
  {
    if(strcmp(section,s->name) == 0)
    {
      deleteSectionNames(s);
      if(last != NULL) last->nextSection = s->nextSection;
      delete s;
      return;
    }
    last = s;
    s = s->nextSection;
  }
}

void rlIniFile::remove(const char *section, const char *name)
{
  rlSection *s;
  rlSectionName *n, *last;

  s = _firstSection;
  while(s != NULL)
  {
    if(strcmp(section,s->name) == 0)
    {
      last = NULL;
      n = s->firstName;
      while(n != NULL)
      {
        if(strcmp(name,n->name) == 0)
        {
          if(n->name  != NULL) delete [] n->name;
          if(n->param != NULL) delete [] n->param;
          if(last != NULL) last->nextName = n->nextName;
          delete n;
          return;
        }
        last = n;
        n = n->nextName;
      }
      return;
    }
    s = s->nextSection;
  }
}

const char *rlIniFile::firstSection()
{
  currentSection = 0;
  return _firstSection->name;
}

const char *rlIniFile::nextSection()
{  
  rlSection *s;
  int i;
    
  if(currentSection < 0) return NULL;
  currentSection++;
  i = 0;
  s = _firstSection;
  while(s != NULL)
  {
    if(i == currentSection) return s->name;
    s = s->nextSection;
    i++;
  }
  currentSection = -1;
  return NULL;
}

const char *rlIniFile::firstName(const char *section)
{
  rlSection *s;
  rlSectionName *n;

  s = _firstSection;
  while(s != NULL)
  {
    if(strcmp(section,s->name) == 0)
    {
      n = s->firstName;
      currentName = 0;
      return n == NULL? NULL : n->name;
    }
    s = s->nextSection;
  }
  return NULL;
}

const char *rlIniFile::nextName(const char *section)
{
  rlSection *s;
  rlSectionName *n;
  int i;
  
  if(currentName < 0) return NULL;
  currentName++;
  i = 0;
  s = _firstSection;
  while(s != NULL)
  {
    if(strcmp(section,s->name) == 0)
    {
      n = s->firstName;
      while(n != NULL)
      {
        if(i == currentName) return n->name;
	i++;
        n = n->nextName;
      }
      return NULL;
    }
    s = s->nextSection;
  }
  return NULL;
}
