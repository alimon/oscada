
//OpenSCADA system module UI.VISION file: tdbgw.h
/***************************************************************************
 *   Copyright (C) 2005-2006 by Evgen Zaichuk                               
 *   evgen@diyaorg.dp.ua                                                     
 *                                                                         
 *   This program is free software; you can redistribute it and/or modify  
 *   it under the terms of the GNU General Public License as published by  
 *   the Free Software Foundation; either version 2 of the License, or     
 *   (at your option) any later version.                                   
 *                                                                         
 *   This program is distributed in the hope that it will be useful,       
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         
 *   GNU General Public License for more details.                          
 *                                                                         
 *   You should have received a copy of the GNU General Public License     
 *   along with this program; if not, write to the                         
 *   Free Software Foundation, Inc.,                                       
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             
 ***************************************************************************/

#ifndef TDBGW_H
#define TDBGW_H

#include <list>
#include <vector>
#include <string>

#include "titem.h"

namespace VISION
{
//������� (��������) ��������� ����������� ������:
struct TAttrDev {std::string name; //��� ��������
		 std::string descr; //�������� ��������
		 TPropType type; //��� ��������
		 std::string value; //�������� ��������
		 };

//�������� ����������� ������:
struct TParamDev {std::string name; //��� ���������
   	          list <TAttrDev> properties; //�������� (��������) ���������
                  };

typedef list <TParamDev> TListParamDev;//������ ���������� ���������� ������

//����� "������ � ���������� ���������� OpenSCADA"
class TDBGW
{
public:
   TDBGW();
   ~TDBGW();
   //�������� ������ ���������� ����������� ������ � �� ���������; ������ �� ��������� - �������� ���� TPropType:
   bool getParamsDev(TListParamDev &listParamDev, const TPropType); 
   
   //�������� �������� �������� attrName ��������� paramName:
   bool getValue(const QString &paramName, const QString &attrName, QString &value); 
};

extern TDBGW *dbgw;
}
#endif

