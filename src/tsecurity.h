
//OpenSCADA system file: tsecurity.h
/***************************************************************************
 *   Copyright (C) 2003-2006 by Roman Savochenko                           *
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

#ifndef TSECURITY_H
#define TSECURITY_H

#include "tbds.h"

#define SEQ_XT 0x01
#define SEQ_WR 0x02
#define SEQ_RD 0x04

class TSYS;
class XMLNode;

class TSecurity;

class TUser : public TCntrNode, public TConfig 
{
    public:
	//Methods
	TUser( const string &name, const string &idb, TElem *el );
	~TUser(  );
	
	const string	&name() 	{ return m_name; }
	const string	&lName()	{ return m_lname; }
	const string	&picture()	{ return m_pict; }	
	bool sysItem()			{ return m_sysIt; }	

	bool auth( const string &ipass );
	
	string DB( )            { return m_db; }
        string tbl( );
        string fullDB( )        { return DB()+'.'+tbl(); }
	
	void name( const string &nm )		{ m_name = nm; }
	void lName( const string &nm )		{ m_lname = nm; }
	void picture( const string &pct )	{ m_pict = pct; }
	void pass( const string &n_pass );
	void sysItem( bool vl )			{ m_sysIt = vl; }

	void load( );
	void save( );
	
	TSecurity &owner()	{ return *(TSecurity*)nodePrev(); }
	
    private:	
	//Methods    
	string nodeName()	{ return m_name; }
	void cntrCmdProc( XMLNode *opt );       //Control interface command process
	void postDisable( int flag );     	//Delete all DB if flag 1

	//Attributes	
	string	&m_name;
	string	&m_lname;
	string	&m_pass;
	string	&m_pict;
	string	m_db;
	bool	m_sysIt;
};

class TGroup : public TCntrNode, public TConfig
{
    public:
	//Methods
	TGroup( const string &name, const string &idb, TElem *el );
	~TGroup(  );

	const string &name()  	{ return(m_name); }
	const string &lName() 	{ return(m_lname); }
	bool sysItem()		{ return m_sysIt; }
	
	string DB( )            { return m_db; }
        string tbl( );
        string fullDB( )        { return DB()+'.'+tbl(); }
	
	void name( const string &nm )	{ m_name = nm; }
	void lName( const string &nm )	{ m_lname = nm; }
	void sysItem( bool vl )		{ m_sysIt = vl; }
		
	bool user( const string &name );
	void userAdd( const string &name );
	void userDel( const string &name );

	void load( );
	void save( );
	
	TSecurity &owner(){ return *(TSecurity*)nodePrev(); }
	
    private:	 
	//Methods   
	string nodeName(){ return m_name; }
	void cntrCmdProc( XMLNode *opt );       //Control interface command process
	void postDisable(int flag);     //Delete all DB if flag 1
	
	//Attributes
	string	&m_name;
	string	&m_lname;
	string	&m_usrs;
	string  m_db;
	bool    m_sysIt;
};

class TSecurity : public TSubSYS
{
    public:
	//Data
	/*enum Permit
	{
	    Execute = 0x01,
	    Write   = 0x02,
	    Read    = 0x04
	};*/
	//Methods
	TSecurity( );    
	~TSecurity( );

	void subLoad( );
	void subSave( );

	char access( const string &user, char mode, const string &owner, const string &group, int access );
	
	//- Users -
	void usrList( vector<string> &list )	{ chldList(m_usr,list); }
	void usrGrpList( const string &name, vector<string> &list );
	bool usrPresent( const string &name ) 	{ return chldPresent(m_usr,name); }
	void usrAdd( const string &name, const string &idb = "*.*" );
	void usrDel( const string &name, bool complete = false );
	AutoHD<TUser> usrAt( const string &name )
	{ return chldAt(m_usr,name); }
	
	//- Groups -
	void grpList( vector<string> &list ) 	{ chldList(m_grp,list); }
	bool grpPresent( const string &name )	{ return chldPresent(m_grp,name); }
	void grpAdd( const string &name, const string &idb = "*.*" );
	void grpDel( const string &name, bool complete = false );
	AutoHD<TGroup> grpAt( const string &name )
	{ return chldAt(m_grp,name); }
	
	string optDescr( );
	
    private:
	//Methods
	void cntrCmdProc( XMLNode *opt );       //Control interface command process
	
	void postEnable();
	
	//Attributes
	int	m_usr, m_grp;
	
	TElem	user_el;
	TElem	grp_el;

	unsigned	hd_res;   
};

#endif // TSECURITY_H

