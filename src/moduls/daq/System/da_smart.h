
//OpenSCADA system module DAQ.System file: da_smart.h
/***************************************************************************
 *   Copyright (C) 2005-2006 by Roman Savochenko                           *
 *   rom_as@fromru.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
 
#ifndef DA_SMART_H
#define DA_SMART_H

#include "da.h"

namespace SystemCntr
{

class HddSmart: public DA
{
    public:
        HddSmart( );
	~HddSmart( );
	
        string id( ) 	{ return "hddsmart"; }
        string name( )	{ return "HDD Smart"; }			
		    
        void init( TMdPrm *prm );
        void getVal( TMdPrm *prm );
	void setEVAL( TMdPrm *prm );
	
	void makeActiveDA( TMdContr *a_cntr );
	
    private:
        void dList( vector<string> &list, bool part = false );
	
    private:
	static char *smartval_cmd;
};

} //End namespace 

#endif //DA_SMART_H
