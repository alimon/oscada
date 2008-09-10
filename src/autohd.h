
//OpenSCADA system file: autohd.h
/***************************************************************************
 *   Copyright (C) 2003-2008 by Roman Savochenko                           *
 *   rom_as@fromru.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
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

#ifndef AUTOHD_H
#define AUTOHD_H

#include <string>
#include <exception>
#include <typeinfo>

#include "terror.h"

using std::string;
using std::bad_cast;

//***************************************************************
//* AutoHD - for auto released HD resources			*
//***************************************************************
template <class ORes> class AutoHD
{
    public:
	AutoHD( ): m_node(NULL)		{  }
	AutoHD( ORes *node, const string &who = "" ) : m_node(node)
	{
	    if(m_node)	m_node->AHDConnect();
	}
	AutoHD( const AutoHD &hd ): m_node(NULL)	{ operator=(hd); }
	template <class ORes1> AutoHD( const AutoHD<ORes1> &hd_s, bool nosafe = false )
	{
	    m_node = NULL;
	    if( hd_s.freeStat() ) return;
	    if(nosafe)	m_node = (ORes *)&hd_s.at();
	    else
	    {
		m_node = dynamic_cast<ORes*>(&hd_s.at());
		if( !m_node )	throw TError("AutoHD","Type casting error!");
	    }
	    m_node->AHDConnect();
	}
	~AutoHD( )	{ free(); }

	ORes &at( ) const
	{
	    if(m_node) return *m_node;
	    throw TError("AutoHD","No init!");
	}

	void operator=( const AutoHD &hd )
	{
	    free( );
	    m_node = hd.m_node;
	    if(m_node)	m_node->AHDConnect();
	}

	void free( )
	{
	    if(m_node)	m_node->AHDDisConnect();
	    m_node = NULL;
	}

	bool freeStat( ) const
	{ return (m_node==NULL)?true:false; }

    private:
	ORes *m_node;
};

#endif //AUTOHD_H
