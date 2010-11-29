
//OpenSCADA system module BD.DBF file: dbf_mod.cpp
/***************************************************************************
 *   Copyright (C) 2003-2010 by Roman Savochenko                           *
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

#include <getopt.h>
#include <unistd.h>
#include <string>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <tsys.h>
#include <terror.h>
#include <string.h>
#include <tmess.h>
#include "dbf.h"
#include "dbf_mod.h"

//************************************************
//* Modul info!                                  *
#define MOD_ID		"DBF"
#define MOD_NAME	_("DB DBF")
#define MOD_TYPE	SDB_ID
#define VER_TYPE	SDB_VER
#define VERSION		"2.0.2"
#define AUTORS		_("Roman Savochenko")
#define DESCRIPTION	_("BD module. Provides support of the *.dbf files, version 3.0.")
#define LICENSE		"GPL2"
//************************************************

BDDBF::BDMod *BDDBF::mod;

extern "C"
{
    TModule::SAt module( int n_mod )
    {
	if( n_mod==0 ) return TModule::SAt(MOD_ID,MOD_TYPE,VER_TYPE);
	return TModule::SAt("");
    }

    TModule *attach( const TModule::SAt &AtMod, const string &source )
    {
	if( AtMod == TModule::SAt(MOD_ID,MOD_TYPE,VER_TYPE) )
	    return new BDDBF::BDMod( source );
	return NULL;
    }
}

using namespace BDDBF;

//************************************************
//* BDDBF::BDMod                                 *
//************************************************
BDMod::BDMod( string name ) : TTipBD(MOD_ID)
{
    mod		= this;

    mName	= MOD_NAME;
    mType	= MOD_TYPE;
    mVers	= VERSION;
    mAutor	= AUTORS;
    mDescr	= DESCRIPTION;
    mLicense	= LICENSE;
    mSource	= name;
}

BDMod::~BDMod(  )
{

}

TBD *BDMod::openBD( const string &iid )
{
    return new MBD(iid,&owner().openDB_E());
}

void BDMod::load_( )
{
    //> Load parameters from command line

    //> Load parameters from config file
}

//************************************************
//* BDDBF::MBD                                   *
//************************************************
MBD::MBD( string iid, TElem *cf_el ) : TBD(iid,cf_el)
{

}

MBD::~MBD(  )
{

}

void MBD::postDisable(int flag)
{
    TBD::postDisable(flag);

    if( flag && owner().fullDeleteDB() )
	if(rmdir(addr().c_str()) != 0)
	    mess_warning(nodePath().c_str(),_("Delete DB error!"));
}

void MBD::enable( )
{
    char   buf[STR_BUF_LEN];

    char *rez = getcwd(buf,sizeof(buf));
    if( chdir(addr().c_str()) != 0 && mkdir(addr().c_str(),S_IRWXU|S_IRGRP|S_IROTH) != 0 )
	throw TError(TSYS::DBInit,nodePath().c_str(),_("Error create DB directory <%s>!"),addr().c_str());
    if( rez && chdir(buf) ) throw TError(TSYS::DBInit,nodePath().c_str(),_("Restore previous directory as current error."));

    TBD::enable( );
}

void MBD::allowList( vector<string> &list )
{
    string nfile;
    list.clear();

    struct stat file_stat;
    dirent *scan_dirent;
    DIR *IdDir = opendir(addr().c_str());
    if(IdDir == NULL) return;
    while((scan_dirent = readdir(IdDir)) != NULL)
    {
	nfile = scan_dirent->d_name;
	if( nfile == ".." || nfile == "." ||
	    nfile.rfind(".") == string::npos || nfile.substr(nfile.rfind(".")) != ".dbf" ) continue;
	stat((addr()+"/"+nfile).c_str(),&file_stat);
	if( (file_stat.st_mode&S_IFMT) != S_IFREG ) continue;
	list.push_back(nfile.substr(0,nfile.rfind(".")));
    }
    closedir(IdDir);
}

TTable *MBD::openTable( const string &nm, bool create )
{
    if( !enableStat() )
	throw TError(TSYS::DBOpenTable,nodePath().c_str(),_("Error open table <%s>. DB is disabled."),nm.c_str());
    return new MTable(nm,this,create);
}

void MBD::cntrCmdProc( XMLNode *opt )
{
    //Get page info
    if(opt->name() == "info")
    {
	TBD::cntrCmdProc(opt);
	ctrRemoveNode(opt,"/sql");
	ctrMkNode("fld",opt,-1,"/prm/cfg/addr",cfg("ADDR").fld().descr(),RWRW__,"root",SDB_ID,2,"tp","str","help",
	    _("For DBF address DB is the directory which contains files of tables (*.dbf).\n"
	      "For example: /opt/dbf ."));
	return;
    }
    TBD::cntrCmdProc(opt);
}

//************************************************
//* BDDBF::MTable                                *
//************************************************
MTable::MTable(const string &inm, MBD *iown, bool create) : 
    TTable(inm), m_modify(false)
{
    string tbl_nm = name();
    setNodePrev(iown);

    //> Set file extend
    if( !(tbl_nm.size() > 4 && tbl_nm.substr(tbl_nm.size()-4,4) == ".dbf") )
	tbl_nm=tbl_nm+".dbf";

    codepage = owner().codePage().size()?owner().codePage():Mess->charset();
    n_table = owner().addr()+'/'+tbl_nm;

    basa = new TBasaDBF(  );
    if( basa->LoadFile( (char *)n_table.c_str() ) == -1 && !create )
    {
	delete basa;
	throw TError(TSYS::DBOpenTable,nodePath().c_str(),_("Open table error!"));
    }
}

MTable::~MTable(  )
{
    delete basa;
}

void MTable::postDisable(int flag)
{
    if(m_modify) save();

    if( flag )
    {
	string n_tbl = name();
	//> Set file extend
	if( !(n_tbl.size() > 4 && n_tbl.substr(n_tbl.size()-4,4) == ".dbf") )
	    n_tbl=n_tbl+".dbf";
	
	if(remove((owner().addr()+"/"+n_tbl).c_str()) < 0 )
	    mess_err(nodePath().c_str(),"%s",strerror(errno));
    }
}

MBD &MTable::owner( )	{ return (MBD&)TTable::owner(); }

bool MTable::fieldSeek( int i_ln, TConfig &cfg )
{
    int i_clm;

    ResAlloc res(m_res,false);

    i_ln = findKeyLine(cfg,i_ln,true);
    if( i_ln < 0 ) return false;

    //> Get config fields list
    vector<string> cf_el;
    cfg.cfgList(cf_el);

    //> Write data to cfg
    for( int i_cf = 0; i_cf < cf_el.size(); i_cf++ )
    {
	TCfg &e_cfg = cfg.cfg(cf_el[i_cf]);

	//>> Find collumn
	db_str_rec *fld_rec;
	for(i_clm = 0;(fld_rec = basa->getField(i_clm)) != NULL;i_clm++)
	    if( cf_el[i_cf].substr(0,10) == fld_rec->name ) break;
	if(fld_rec == NULL) continue;

	//>> Get table volume
	string val;
	if( basa->GetFieldIt( i_ln, i_clm, val ) < 0)
	    throw TError(TSYS::DBInernal,nodePath().c_str(),_("Cell error!"));

	//>> Write value
	setVal(e_cfg,val);
    }

    return true;
}

void MTable::fieldGet( TConfig &cfg )
{
    int i_ln, i_clm;

    //> Alloc resource
    ResAlloc res(m_res,false);

    //> Get key line
    i_ln = findKeyLine( cfg );
    if( i_ln < 0 ) throw TError(TSYS::DBRowNoPresent,nodePath().c_str(),_("Field is not present!"));

    //> Get config fields list
    vector<string> cf_el;
    cfg.cfgList(cf_el);

    //> Write data to cfg
    for( int i_cf = 0; i_cf < cf_el.size(); i_cf++ )
    {
	TCfg &e_cfg = cfg.cfg(cf_el[i_cf]);

	//>> Find collumn
	db_str_rec *fld_rec;
	for(i_clm = 0;(fld_rec = basa->getField(i_clm)) != NULL;i_clm++)
	    if( cf_el[i_cf].substr(0,10) == fld_rec->name ) break;
	if(fld_rec == NULL) continue;

	//>> Get table volume
	string val;
	if( basa->GetFieldIt( i_ln, i_clm, val ) < 0)
	    throw TError(TSYS::DBInernal,nodePath().c_str(),_("Cell error!"));
	//>> Write value
	setVal(e_cfg,val);
    }
}

void MTable::fieldSet( TConfig &cfg )
{
    int i_ln, i_clm;

    //> Alloc resource
    ResAlloc res(m_res,true);

    //> Get config fields list
    vector<string> cf_el;
    cfg.cfgList(cf_el);

    //> Check and fix structure of table
    for( int i_cf = 0; i_cf < cf_el.size(); i_cf++ )
    {
	TCfg &e_cfg = cfg.cfg(cf_el[i_cf]);

	//>> Find collumn
	db_str_rec *fld_rec;
	for(i_clm = 0;(fld_rec = basa->getField(i_clm)) != NULL;i_clm++)
	    if( cf_el[i_cf].substr(0,10) == fld_rec->name ) break;
	if(fld_rec == NULL)
	{
	    //>> Create new collumn
	    db_str_rec n_rec;

	    fieldPrmSet( e_cfg, n_rec );
	    if( basa->addField(i_cf,&n_rec) < 0 )
		throw TError(TSYS::DBInernal,nodePath().c_str(),_("Column error!"));
	}
	else
	{
	    //>> Check collumn parameters
	    switch(e_cfg.fld().type())
	    {
		case TFld::String:
		    if( fld_rec->tip_fild == 'C' && e_cfg.fld().len() == fld_rec->len_fild )	continue;
		    break;
		case TFld::Integer:
		    if( fld_rec->tip_fild == 'N' && e_cfg.fld().len() == fld_rec->len_fild )	continue;
		    break;
		case TFld::Real:
		    if( fld_rec->tip_fild == 'N' && e_cfg.fld().len() == fld_rec->len_fild &&
			    e_cfg.fld().dec() == fld_rec->dec_field )continue;
		    break;
		case TFld::Boolean:
		    if( fld_rec->tip_fild == 'L' )	continue;
		    break;
	    }

	    db_str_rec n_rec;

	    fieldPrmSet( e_cfg, n_rec );
	    if( basa->setField(i_clm,&n_rec) < 0 )
		throw TError(TSYS::DBInernal,nodePath().c_str(),_("Column error!"));
	}
    }
    //> Del no used collumn
    db_str_rec *fld_rec;
    for(i_clm = 0;(fld_rec = basa->getField(i_clm)) != NULL;i_clm++)
    {
	int i_cf;
	for( i_cf = 0; i_cf < cf_el.size(); i_cf++ )
	    if( cf_el[i_cf].substr(0,10) == fld_rec->name ) break;
	if( i_cf >= cf_el.size() )
	    if( basa->DelField(i_clm) < 0 )
		throw TError(TSYS::DBClose,nodePath().c_str(),_("Delete field error!"));
    }
    //> Get key line
    i_ln = findKeyLine( cfg );
    if( i_ln < 0 ) i_ln = basa->CreateItems(-1);
    //> Write data to bd
    for( int i_cf = 0; i_cf < cf_el.size(); i_cf++ )
    {
	TCfg &e_cfg = cfg.cfg(cf_el[i_cf]);

	//>> Find collumn
	db_str_rec *fld_rec;
	for(i_clm = 0;(fld_rec = basa->getField(i_clm)) != NULL;i_clm++)
	    if( cf_el[i_cf].substr(0,10) == fld_rec->name ) break;
	if(fld_rec == NULL) continue;

	//>> Set table volume
	if( basa->ModifiFieldIt( i_ln, i_clm, getVal(e_cfg,fld_rec).c_str() ) < 0 )
	    throw TError(TSYS::DBInernal,nodePath().c_str(),_("Cell error!"));
    }

    m_modify = true;
}

void MTable::fieldDel( TConfig &cfg )
{
    //> Alloc resource
    ResAlloc res(m_res,true);

    //> Get key line
    bool i_ok = false;
    int i_ln;
    while((i_ln = findKeyLine(cfg,0,true)) >= 0)
    {
	if( basa->DeleteItems(i_ln,1) < 0 )
	    throw TError(TSYS::DBInernal,nodePath().c_str(),_("Line error!"));

	i_ok = true;
	m_modify = true;
    }
    if( !i_ok ) throw TError(TSYS::DBInernal,nodePath().c_str(),_("Field is not present!"));
}

int MTable::findKeyLine( TConfig &cfg, int cnt, bool useKey )
{
    int i_ln, i_clm, i_cnt = 0;

    mLstUse = time(NULL);

    //> Get config fields list
    vector<string> cf_el;
    cfg.cfgList(cf_el);
    //> Left only keys into list
    for( int i_cf = 0; i_cf < cf_el.size(); )
	if( cfg.cfg(cf_el[i_cf]).fld().flg()&TCfg::Key ) i_cf++;
	else cf_el.erase(cf_el.begin()+i_cf);

    //> Find want field
    for( i_ln = 0; i_ln < basa->GetCountItems(  ); i_ln++ )
    {
	int cnt_key = 0;
	for( int i_cf = 0; i_cf < cf_el.size(); i_cf++ )
	{
	    if( useKey && !cfg.cfg(cf_el[i_cf]).keyUse( ) )	{ cnt_key++; continue; }
	    //string key = cfg.cfg(cf_el[i_cf]).name();
	    //> Check key
	    //>> Find collumn
	    db_str_rec *fld_rec;
	    for(i_clm = 0;(fld_rec = basa->getField(i_clm)) != NULL;i_clm++)
		if( cf_el[i_cf].substr(0,10) == fld_rec->name ) break;
	    if(fld_rec == NULL)
		throw TError(TSYS::DBInernal,nodePath().c_str(),_("Key column <%s> is not present!"),cf_el[i_cf].c_str());
	    //>> Get table volume
	    string val;
	    if( basa->GetFieldIt( i_ln, i_clm, val ) < 0)
		throw TError(TSYS::DBInernal,nodePath().c_str(),_("Cell error!"));
	    //>> Remove spaces from end
	    int i;
	    for(i = val.size(); i > 0; i--) if(val[i-1]!=' ') break;
	    if(i != (int)val.size()) val.resize(i);
	    //>> Compare value
	    if( val != getVal(cfg.cfg(cf_el[i_cf])) )
	    {
		cnt_key = 0;
		break;
	    }
	    cnt_key++;
	}
	if( cnt_key && cnt <= i_cnt++ ) break;
    }
    if(i_ln >= basa->GetCountItems(  )) return -1;

    return i_ln;
}

void MTable::fieldPrmSet( TCfg &e_cfg, db_str_rec &n_rec )
{
    memset(&n_rec,0,sizeof(db_str_rec));

    strncpy(n_rec.name,e_cfg.name().c_str(),10);
    switch(e_cfg.fld().type())
    {
	case TFld::String:
	    n_rec.tip_fild  = 'C';
	    n_rec.len_fild  = (e_cfg.fld().len()>255)?255:e_cfg.fld().len();
	    n_rec.dec_field = 0;
	    break;
	case TFld::Integer:
	    n_rec.tip_fild = 'N';
	    n_rec.len_fild = (e_cfg.fld().len() == 0)?5:(e_cfg.fld().len()>255)?255:e_cfg.fld().len();
	    n_rec.dec_field = 0;
	    break;
	case TFld::Real:
	    n_rec.tip_fild = 'N';
	    n_rec.len_fild = (e_cfg.fld().len() == 0)?7:(e_cfg.fld().len()>255)?255:e_cfg.fld().len();
	    n_rec.dec_field = (e_cfg.fld().dec() == 0)?2:(e_cfg.fld().dec()>255)?255:e_cfg.fld().dec();
	    break;
	case TFld::Boolean:
	    n_rec.tip_fild  = 'L';
	    n_rec.len_fild  = 1;
	    n_rec.dec_field = 0;
	    break;
    }
}

void MTable::save( )
{
    ResAlloc res(m_res,true);
    basa->SaveFile((char *)n_table.c_str());
    m_modify = false;
}

string MTable::getVal( TCfg &cfg, db_str_rec *fld_rec )
{
    switch(cfg.fld().type())
    {
	case TFld::String:	return Mess->codeConvOut(codepage,cfg.getS());
	case TFld::Integer:	return TSYS::int2str(cfg.getI());
	case TFld::Real:
	{
	    if( !fld_rec ) return TSYS::real2str(cfg.getR());

	    char str[200];
	    snprintf(str,sizeof(str),"%*.*f",fld_rec->len_fild,fld_rec->dec_field,cfg.getR());
	    return str;
	}
	case TFld::Boolean:	return cfg.getB()?"T":"F";
    }
    return "";
}

void MTable::setVal( TCfg &cfg, const string &val )
{
    switch( cfg.fld().type() )
    {
	case TFld::String:
	{
	    //> Remove spaces from end
	    int i;
	    for( i = val.size(); i > 0; i--) if(val[i-1]!=' ' ) break;
	    cfg.setS(Mess->codeConvIn(codepage.c_str(),val.substr(0,i)));
	    break;
	}
	case TFld::Integer:	cfg.setI(atoi(val.c_str()));	break;
	case TFld::Real:	cfg.setR(atof(val.c_str()));	break;
	case TFld::Boolean:	cfg.setB(val.c_str()[0]=='T');	break;
    }
}
