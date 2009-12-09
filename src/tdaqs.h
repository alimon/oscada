
//OpenSCADA system file: tdaqs.h
/***************************************************************************
 *   Copyright (C) 2003-2009 by Roman Savochenko                           *
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

#ifndef TDAQS_H
#define TDAQS_H

#define  VER_CNTR 3    //ControllerS type modules version

#include <string>

#include "tprmtmpl.h"
#include "tsubsys.h"
#include "ttipdaq.h"

using std::string;

//*************************************************
//* TDAQS                                         *
//*************************************************
class TController;
class TTipDAQ;

class TDAQS : public TSubSYS
{
    public:
	//Public methods
	TDAQS( );
	~TDAQS( );

	int subVer( )	{ return VER_CNTR; }
	void subStart(  );
	void subStop( );

	AutoHD<TTipDAQ> at( const string &name )		{ return modAt(name); }

	//> Parameter's templates library
	string tmplLibTable( )					{ return "ParamTemplLibs"; }
	void tmplLibList( vector<string> &list )		{ chldList(mTmplib,list); }
	bool tmplLibPresent( const string &id )			{ return chldPresent(mTmplib,id); }
	void tmplLibReg( TPrmTmplLib *lib )			{ chldAdd(mTmplib,lib); }
	void tmplLibUnreg( const string &id, int flg = 0 )	{ chldDel(mTmplib,id,-1,flg); }
	AutoHD<TPrmTmplLib> tmplLibAt( const string &id )	{ return chldAt(mTmplib,id); }

	//> Redundance
	bool rdActive( );
	int rdStLevel( )		{ return mRdStLevel; }
	void setRdStLevel( int vl );
	int rdTaskPer( )		{ return mRdTaskPer; }
	void setRdTaskPer( int vl );
	int rdRestConnTm( )		{ return mRdRestConnTm; }
	void setRdRestConnTm( int vl );
	float rdRestDtTm( )		{ return mRdRestDtTm; }
	void setRdRestDtTm( float vl );
	void rdStList( vector<string> &ls );
	void rdActCntrList( vector<string> &ls, bool isRun = false );
	string rdStRequest( const string &cntr, XMLNode &req, const string &prevSt = "", bool toRun = true );

	TElem &elLib( )	{ return lb_el; }
	TElem &tplE( )	{ return el_tmpl; }
	TElem &tplIOE( ){ return el_tmpl_io; }

	TElem &errE( )	{ return el_err; }	//Error atributes structure

    protected:
	void load_( );
	void save_( );

    private:
	//Private data
	class SStat
	{
	    public:
		SStat( ) : isLive(false), cnt(0), lev(0) { }

		bool	isLive;
		float	cnt;
		char	lev;
		map<string,bool> actCntr;
	};

	//Private methods
	string optDescr( );

	void cntrCmdProc( XMLNode *opt );	//Control interface command process

	static void *RdTask( void *param );

	//Private attributes
	TElem	el_err, lb_el, el_tmpl, el_tmpl_io;
	int	mTmplib;

	unsigned char	mRdStLevel,		//Current station level
			mRdTaskPer,		//Redundant task period in seconds
			mRdRestConnTm;		//Redundant restore connection to reserve stations timeout in seconds
	float		mRdRestDtTm,		//Redundant history restore length time in hour
			mRdPrcTm;		//Redundant process time
	bool		prcStRd, endrunRd;
	map<string,SStat> mSt;
};

#endif // TDAQS_H
