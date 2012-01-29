
//OpenSCADA system file: tprotocols.h
/***************************************************************************
 *   Copyright (C) 2003-2010 by Roman Savochenko                           *
 *   rom_as@oscada.org, rom_as@fromru.com                                  *
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

#ifndef TPROTOCOLS_H
#define TPROTOCOLS_H

#define SPRT_VER	6		//ProtocolS type modules version
#define SPRT_ID		"Protocol"

#include <string>

#include "tsubsys.h"

using std::string;

namespace OSCADA
{

//************************************************
//* TProtocolIn                                  *
//************************************************
class TProtocol;

class TProtocolIn : public TCntrNode
{
    public:
	//Methods
	TProtocolIn( const string &name );
	virtual ~TProtocolIn( );

	const string &name( )		{ return mName; }
	const string &srcTr( )		{ return mSrcTr; }

	void setSrcTr( const string &vl )	{ mSrcTr = vl; }

	//> Process input messages
	virtual bool mess( const string &request, string &answer, const string &sender )
	{ answer = ""; return false; }

	TProtocol &owner( );

    private:
	//Methods
	const string &nodeName( )	{ return mName; }

	//Attributes
	const string	mName;
	string		mSrcTr;
};

//************************************************
//* TProtocol                                    *
//************************************************ 
class TTransportOut;

class TProtocol: public TModule
{
    public:
	TProtocol( const string &id );
	virtual ~TProtocol( );

	//> Addon items list for input protocol addressing
	virtual void itemListIn( vector<string> &ls, const string &curIt = "" );

	//> Input protocol
	void list( vector<string> &list )		{ chldList(m_pr,list); }
	bool openStat( const string &name )		{ return chldPresent(m_pr,name); }
	void open( const string &name, const string &tr );
	void close( const string &name );
	AutoHD<TProtocolIn> at( const string &name )	{ return chldAt(m_pr,name); }

	//> Output protocol
	virtual void outMess( XMLNode &io, TTransportOut &tro )
	{ throw TError(nodePath().c_str(),"Function '%s' no support!","outMess"); }

    private:
	//Methods
	virtual TProtocolIn *in_open( const string &name )
	{ throw TError(nodePath().c_str(),"Function '%s' no support!","in_open"); }

	//Attributes
	int	m_pr;
};

//************************************************
//* TProtocolS                                   *
//************************************************ 
class TProtocolS : public TSubSYS
{
    public:
	TProtocolS( );
	~TProtocolS( );

	int subVer( )		{ return SPRT_VER; }

	AutoHD<TProtocol> at( const string &iid )	{ return modAt(iid); }

	string optDescr( );

    protected:
	void load_( );

    private:
	void cntrCmdProc( XMLNode *opt );       //Control interface command process
};

}

#endif // TPROTOCOLS_H
