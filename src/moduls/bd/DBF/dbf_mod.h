
//OpenSCADA system module BD.DBF file: dbf_mod.h
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

#ifndef TEST_BD_H
#define TEST_BD_H

#include <string>
#include <vector>

#include <tmodule.h>
#include <tbds.h>

#undef _
#define _(mess) mod->I18N(mess)

using std::string;
using std::vector;

class TBasaDBF;

namespace BDDBF
{

//*************************************************
//* SHd                                           *
//*************************************************
struct Shd
{
    int      use;
    string   name_bd;
    TBasaDBF *basa;
    string   codepage;
};

//************************************************
//* BDDBF::MTable                                *
//************************************************
class MBD;
class MTable : public TTable
{
    public:
	//Public methods
	MTable(const string &name, MBD *iown, bool create );
	~MTable(  );

	bool fieldSeek( int row, TConfig &cfg );
	void fieldGet( TConfig &cfg );
	void fieldSet( TConfig &cfg );
	void fieldDel( TConfig &cfg );

	MBD &owner( );

    private:
	//Private methods
	void postDisable(int flag);
	void save( );
	int  findKeyLine( TConfig &cfg, int cnt = 0, bool useKey = false );
	void fieldPrmSet( TCfg &e_cfg, db_str_rec &n_rec );

	//Private atributes
	string n_table;
	string codepage;
	TBasaDBF *basa;

	Res    m_res;
	bool   m_modify;
};
 
//************************************************
//* BDDBF::MBD                                   *
//************************************************
class BDMod;
class MBD : public TBD
{
    public:
	//Public methods
	MBD( string name, TElem *cf_el );
	~MBD(  );

	void enable( );
	void allowList( vector<string> &list );

    protected:
	//Protected methods
	void cntrCmdProc( XMLNode *opt );       //Control interface command process

    private:
	//Private methods
	void postDisable(int flag);
	TTable *openTable( const string &table, bool create );
};

//************************************************
//* BDDBF::BDMod                                 *
//************************************************
class BDMod : public TTipBD
{
    public:
	//Public methods
	BDMod( string name );
	~BDMod(  );

    protected:
	//Protected methods
	void load_( );

    private:
	//Private methods
	TBD *openBD( const string &iid );
	string optDescr( );
};

extern BDMod *mod;

}

#endif // TEST_BD_H
