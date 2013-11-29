
//OpenSCADA system module BD.MySQL file: my_sql.cpp
/***************************************************************************
 *   Copyright (C) 2003-2013 by Roman Savochenko                           *
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

#include <string>
#include <mysql.h>
#include <errmsg.h>
#include <mysqld_error.h>

#include <tsys.h>
#include <tmess.h>
#include "my_sql.h"


//************************************************
//* Modul info!                                  *
#define MOD_ID		"MySQL"
#define MOD_NAME	_("DB MySQL")
#define MOD_TYPE	SDB_ID
#define VER_TYPE	SDB_VER
#define MOD_VER		"1.7.1"
#define AUTHORS		_("Roman Savochenko")
#define DESCRIPTION	_("BD module. Provides support of the BD MySQL.")
#define MOD_LICENSE	"GPL2"
//************************************************

BDMySQL::BDMod *BDMySQL::mod;

extern "C"
{
#ifdef MOD_INCL
    TModule::SAt bd_MySQL_module( int n_mod )
#else
    TModule::SAt module( int n_mod )
#endif
    {
	if(n_mod == 0)	return TModule::SAt(MOD_ID,MOD_TYPE,VER_TYPE);
	return TModule::SAt("");
    }

#ifdef MOD_INCL
    TModule *bd_MySQL_attach( const TModule::SAt &AtMod, const string &source )
#else
    TModule *attach( const TModule::SAt &AtMod, const string &source )
#endif
    {
	if(AtMod == TModule::SAt(MOD_ID,MOD_TYPE,VER_TYPE))
	    return new BDMySQL::BDMod( source );
	return NULL;
    }
}

using namespace BDMySQL;

//************************************************
//* BDMySQL::BDMod				 *
//************************************************
BDMod::BDMod( string name ) : TTipBD(MOD_ID)
{
    mod		= this;

    mName	= MOD_NAME;
    mType	= MOD_TYPE;
    mVers	= MOD_VER;
    mAuthor	= AUTHORS;
    mDescr	= DESCRIPTION;
    mLicense	= MOD_LICENSE;
    mSource	= name;
}

BDMod::~BDMod( )
{

}

TBD *BDMod::openBD( const string &name )
{
    return new MBD(name,&owner().openDB_E());
}

void BDMod::load_( )
{
    //> Load parameters from command line

}

//************************************************
//* BDMySQL::MBD				 *
//************************************************
MBD::MBD( string iid, TElem *cf_el ) : TBD(iid,cf_el)
{
    setAddr("localhost;root;123456;test;;;utf8");
}

MBD::~MBD( )
{

}

void MBD::postDisable(int flag)
{
    TBD::postDisable(flag);

    if(flag && owner().fullDeleteDB())
    {
	MYSQL connect;

	if(!mysql_init(&connect)) throw TError(TSYS::DBInit,nodePath().c_str(),_("Error initializing client."));
	connect.reconnect = 1;
	if(!mysql_real_connect(&connect,host.c_str(),user.c_str(),pass.c_str(),"",port,(u_sock.size()?u_sock.c_str():NULL),CLIENT_MULTI_STATEMENTS))
	    throw TError(TSYS::DBConn,nodePath().c_str(),_("Connect to DB error: %s"),mysql_error(&connect));

	string req = "DROP DATABASE `"+bd+"`";
	if(mysql_real_query(&connect,req.c_str(),req.size()))
	    throw TError(TSYS::DBRequest,nodePath().c_str(),_("Query to DB error: %s"),mysql_error(&connect));

	mysql_close(&connect);
    }
}

void MBD::enable( )
{
    if(enableStat())	return;

    host = TSYS::strSepParse(addr(),0,';');
    user = TSYS::strSepParse(addr(),1,';');
    pass = TSYS::strSepParse(addr(),2,';');
    bd   = TSYS::strSepParse(addr(),3,';');
    port = atoi(TSYS::strSepParse(addr(),4,';').c_str());
    u_sock = TSYS::strSepParse(addr(),5,';');
    names = TSYS::strSepParse(addr(),6,';');
    cd_pg  = codePage().size()?codePage():Mess->charset();
    string tms = TSYS::strSepParse(addr(),7,';');

    if(!mysql_init(&connect))
	throw TError(TSYS::DBInit,nodePath().c_str(),_("Error initializing client."));
    if(!tms.empty())
    {
	unsigned int tTm = atoi(TSYS::strParse(tms,0,",").c_str());
	if(tTm) mysql_options(&connect, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&tTm);
	tTm = atoi(TSYS::strParse(tms,1,",").c_str());
	if(tTm) mysql_options(&connect, MYSQL_OPT_READ_TIMEOUT, (const char*)&tTm);
	tTm = atoi(TSYS::strParse(tms,2,",").c_str());
	if(tTm) mysql_options(&connect, MYSQL_OPT_WRITE_TIMEOUT, (const char*)&tTm);
    }
    connect.reconnect = 1;
    if(!mysql_real_connect(&connect,host.c_str(),user.c_str(),pass.c_str(),"",port,(u_sock.size()?u_sock.c_str():NULL),CLIENT_MULTI_STATEMENTS))
	throw TError(TSYS::DBConn,nodePath().c_str(),_("Connect to DB error: %s"),mysql_error(&connect));

    TBD::enable( );

    sqlReq("CREATE DATABASE IF NOT EXISTS `"+TSYS::strEncode(bd,TSYS::SQL)+"`");
    if(!names.empty()) sqlReq("SET NAMES '"+names+"'");
}

void MBD::disable( )
{
    if( !enableStat() )  return;

    TBD::disable( );

    ResAlloc resource(conn_res,true);
    mysql_close(&connect);
}

void MBD::allowList( vector<string> &list )
{
    if( !enableStat() )  return;
    list.clear();
    vector< vector<string> > tbl;
    sqlReq("SHOW TABLES FROM `"+TSYS::strEncode(bd,TSYS::SQL)+"`",&tbl);
    for( unsigned i_t = 1; i_t < tbl.size(); i_t++ )
	list.push_back(tbl[i_t][0]);
}

TTable *MBD::openTable( const string &inm, bool create )
{
    if( !enableStat() )
	throw TError(TSYS::DBOpen,nodePath().c_str(),_("Error open table '%s'. DB is disabled."),inm.c_str());

    return new MTable(inm,this,create);
}

void MBD::sqlReq( const string &ireq, vector< vector<string> > *tbl, char intoTrans )
{
    MYSQL_RES *res = NULL;

    if(tbl) tbl->clear();
    if(!enableStat()) return;

    string req = Mess->codeConvOut(cd_pg.c_str(), ireq);

    ResAlloc resource(conn_res, true);

    int irez;
    rep:
    if((irez = mysql_real_query(&connect,req.c_str(),req.size())))
    {
	if(irez == CR_SERVER_GONE_ERROR || irez == CR_SERVER_LOST)
	{
	    resource.release();
	    disable();
	    enable();
	    resource.request(true);
	    irez = mysql_real_query(&connect, req.c_str(), req.size());
	}
	if(irez)
	{
	    if(mysql_errno(&connect) == ER_NO_DB_ERROR)
	    {
		resource.release();
		sqlReq("USE `"+TSYS::strEncode(bd,TSYS::SQL)+"`");
		resource.request(true);
		goto rep;
	    }
	    if(mess_lev() == TMess::Debug) mess_debug(nodePath().c_str(),_("Query '%s' is error."),ireq.c_str());
	    throw TError(TSYS::DBRequest, nodePath().c_str(), _("Query to DB error %d: %s"), irez, mysql_error(&connect));
	}
    }

    do
    {
	if(!(res=mysql_store_result(&connect)) && mysql_field_count(&connect))
	    throw TError(TSYS::DBRequest, nodePath().c_str(), _("Store result error: %s"), mysql_error(&connect));

	if(res && tbl && tbl->empty())	//Process only first statement's result
	{
	    int num_fields = mysql_num_fields(res);
	    MYSQL_ROW row;

	    vector<string> fld;
	    //> Add head
	    for(int i=0; i < num_fields; i++)
		fld.push_back(mysql_fetch_field_direct(res,i)->name);
	    tbl->push_back(fld);
	    //> Add data
	    while((row = mysql_fetch_row(res)))
	    {
		fld.clear();
		for(int i = 0; i < num_fields; i++)
		    fld.push_back(row[i]?Mess->codeConvIn(cd_pg.c_str(),row[i]):"");
		tbl->push_back(fld);
	    }
	}

	if(res) mysql_free_result(res);

	if((irez=mysql_next_result(&connect)) > 0)
	    throw TError(TSYS::DBRequest, nodePath().c_str(), _("Could not execute statement: %s"), mysql_error(&connect));
    }while(irez == 0);
}

void MBD::cntrCmdProc( XMLNode *opt )
{
    //Get page info
    if(opt->name() == "info")
    {
	TBD::cntrCmdProc(opt);
	ctrMkNode("fld",opt,-1,"/prm/cfg/addr",cfg("ADDR").fld().descr(),enableStat()?R_R___:RWRW__,"root",SDB_ID,2,"tp","str","help",
	    _("MySQL DB address must be written as: [<host>;<user>;<pass>;<db>;<port>;<u_sock>;<names>;<tms>].\n"
	      "Where:\n"
	      "  host - MySQL server hostname;\n"
	      "  user - DB user name;\n"
	      "  pass - user's password for DB access;\n"
	      "  db - DB name;\n"
	      "  port - DB server port (default 3306);\n"
	      "  u_sock - UNIX-socket name, for local access to DB (/var/lib/mysql/mysql.sock);\n"
	      "  names - MySQL SET NAMES charset;\n"
	      "  tms - MySQL timeouts in form [<connect>,<read>,<write>] and in seconds.\n"
	      "For local DB: [;roman;123456;OpenSCADA;;/var/lib/mysql/mysql.sock;utf8;5,2,2].\n"
	      "For remote DB: [server.nm.org;roman;123456;OpenSCADA;3306]."));
	return;
    }
    TBD::cntrCmdProc(opt);
}

//************************************************
//* MBDMySQL::Table                              *
//************************************************
MTable::MTable(string name, MBD *iown, bool create ) : TTable(name)
{
    string req;

    setNodePrev(iown);

    if(create)
    {
	req = "CREATE TABLE IF NOT EXISTS `"+TSYS::strEncode(owner().bd,TSYS::SQL)+"`.`"+
	    TSYS::strEncode(name,TSYS::SQL)+"` (`name` char(20) NOT NULL DEFAULT '' PRIMARY KEY)";
	owner().sqlReq(req);
    }
    //> Get table structure description
    req ="DESCRIBE `"+TSYS::strEncode(owner().bd,TSYS::SQL)+"`.`"+TSYS::strEncode(name,TSYS::SQL)+"`";
    owner().sqlReq(req,&tblStrct);
    //req = "SELECT * FROM `"+TSYS::strEncode(name,TSYS::SQL)+"` LIMIT 0,1";
    //owner().sqlReq( req );
}

MTable::~MTable( )
{

}

void MTable::postDisable(int flag)
{
    if( flag )
    {
	try
	{
	    owner().sqlReq("DROP TABLE `"+TSYS::strEncode(owner().bd,TSYS::SQL)+"`.`"+
		TSYS::strEncode(name(),TSYS::SQL)+"`");
	}
	catch(TError err){ mess_warning(err.cat.c_str(),"%s",err.mess.c_str()); }
    }
}

MBD &MTable::owner( )	{ return (MBD&)TTable::owner(); }

void MTable::fieldStruct( TConfig &cfg )
{
    if(tblStrct.empty()) throw TError(TSYS::DBTableEmpty,nodePath().c_str(),_("Table is empty!"));
    mLstUse = SYS->sysTm();

    for(unsigned i_fld = 1; i_fld < tblStrct.size(); i_fld++)
    {
	int pr1, pr2;
	string sid = tblStrct[i_fld][0];
	if(cfg.cfgPresent(sid)) continue;

	int flg = (tblStrct[i_fld][3]=="PRI") ? (int)TCfg::Key : (int)TFld::NoFlag;
	/*if(tblStrct[i_fld][1] == "tinyint(1)")
	    cfg.elem().fldAdd( new TFld(sid.c_str(),sid.c_str(),TFld::Boolean,flg,"1") );
	else */
	if(sscanf(tblStrct[i_fld][1].c_str(),"char(%d)",&pr1) || sscanf(tblStrct[i_fld][1].c_str(),"varchar(%d)",&pr1))
	    cfg.elem().fldAdd(new TFld(sid.c_str(),sid.c_str(),TFld::String,flg,TSYS::int2str(pr1).c_str()));
	else if(tblStrct[i_fld][1] == "text")
	    cfg.elem().fldAdd(new TFld(sid.c_str(),sid.c_str(),TFld::String,flg,"65535"));
	else if(tblStrct[i_fld][1] == "mediumtext")
	    cfg.elem().fldAdd(new TFld(sid.c_str(),sid.c_str(),TFld::String,flg,"16777215"));
	else if(tblStrct[i_fld][1] == "int")
	    cfg.elem().fldAdd(new TFld(sid.c_str(),sid.c_str(),TFld::Integer,flg));
	else if(sscanf(tblStrct[i_fld][1].c_str(),"int(%d)",&pr1) || sscanf(tblStrct[i_fld][1].c_str(),"tinyint(%d)",&pr1) ||
		sscanf(tblStrct[i_fld][1].c_str(),"bigint(%d)",&pr1))
	    cfg.elem().fldAdd(new TFld(sid.c_str(),sid.c_str(),TFld::Integer,flg,TSYS::int2str(pr1).c_str()));
	else if(tblStrct[i_fld][1] == "double")
	    cfg.elem().fldAdd(new TFld(sid.c_str(),sid.c_str(),TFld::Real,flg));
	else if(sscanf(tblStrct[i_fld][1].c_str(),"double(%d,%d)",&pr1,&pr2))
	    cfg.elem().fldAdd( new TFld(sid.c_str(),sid.c_str(),TFld::Real,flg,(TSYS::int2str(pr1)+"."+TSYS::int2str(pr2)).c_str()) );
	else if(tblStrct[i_fld][1] == "datetime")
	    cfg.elem().fldAdd( new TFld(sid.c_str(),sid.c_str(),TFld::Integer,flg|TFld::DateTimeDec,"10") );
    }
}

bool MTable::fieldSeek( int row, TConfig &cfg )
{
    vector< vector<string> > tbl;

    if(tblStrct.empty()) throw TError(TSYS::DBTableEmpty,nodePath().c_str(),_("Table is empty!"));
    mLstUse = SYS->sysTm();

        //> Check for no present and no empty keys allow
    if(row == 0)
    {
        vector<string> cf_el;
        cfg.cfgList(cf_el);

        for(unsigned i_c = 0, i_fld = 1; i_c < cf_el.size(); i_c++)
        {
            TCfg &cf = cfg.cfg(cf_el[i_c]);
            if(!(cf.fld().flg()&TCfg::Key) || !cf.getS().size()) continue;
	    for( ; i_fld < tblStrct.size(); i_fld++)
                if(cf.name() == tblStrct[i_fld][0]) break;
            if(i_fld >= tblStrct.size()) return false;
        }
    }

    string sid;
    //> Make SELECT and WHERE
    string req = "SELECT ";
    string req_where = "WHERE ";
    //> Add use keys to list
    bool first_sel = true, next = false, trPresent = false;
    for(unsigned i_fld = 1; i_fld < tblStrct.size(); i_fld++)
    {
	sid = tblStrct[i_fld][0];
	TCfg *u_cfg = cfg.at(sid,true);
	if(!cfg.noTransl() && !u_cfg && sid.size() > 3 && sid.substr(0,3) == (Mess->lang2Code()+"#"))
	{
	    u_cfg = cfg.at(sid.substr(3),true);
	    if( u_cfg && !(u_cfg->fld().flg()&TCfg::TransltText) ) continue;
	    trPresent = true;
	}
	if(!u_cfg) continue;

	if(u_cfg->fld().flg()&TCfg::Key && u_cfg->keyUse())
	{
	    req_where = req_where + (next?"AND `":"`") + TSYS::strEncode(sid,TSYS::SQL) + "`='" + TSYS::strEncode(getVal(*u_cfg),TSYS::SQL) + "' ";
	    next = true;
	}
	else if(u_cfg->fld().flg()&TCfg::Key || u_cfg->view())
	{
	    req = req + (first_sel?"`":",`") + TSYS::strEncode(sid,TSYS::SQL) + "`";
	    first_sel = false;
	}
    }

    //> Request
    if(first_sel) return false;
    req = req + " FROM `" + TSYS::strEncode(owner().bd,TSYS::SQL) + "`.`" + TSYS::strEncode(name(),TSYS::SQL) + "` " +
	((next)?req_where:"") + " LIMIT " + TSYS::int2str(row) + ",1";
    owner().sqlReq(req, &tbl);
    if(tbl.size() < 2) return false;
    for(unsigned i_fld = 0; i_fld < tbl[0].size(); i_fld++)
    {
	sid = tbl[0][i_fld];
	TCfg *u_cfg = cfg.at(sid,true);
	if(u_cfg) setVal(*u_cfg,tbl[1][i_fld]);
	else if(trPresent && sid.size() > 3 && sid.substr(0,3) == (Mess->lang2Code()+"#") && tbl[1][i_fld].size())
	{
	    u_cfg = cfg.at(sid.substr(3),true);
	    if(u_cfg) setVal(*u_cfg,tbl[1][i_fld]);
	}
    }

    return true;
}

void MTable::fieldGet( TConfig &cfg )
{
    vector< vector<string> > tbl;

    if( tblStrct.empty() ) throw TError(TSYS::DBTableEmpty,nodePath().c_str(),_("Table is empty!"));
    mLstUse = SYS->sysTm();

    string sid;
    //> Prepare request
    string req = "SELECT ";
    string req_where, first_key;
    //>> Add fields list to queue
    bool first_sel = true, next_wr = false, trPresent = false;
    for(unsigned i_fld = 1; i_fld < tblStrct.size(); i_fld++)
    {
	sid = tblStrct[i_fld][0];
	TCfg *u_cfg = cfg.at(sid,true);
	if(!cfg.noTransl() && !u_cfg && sid.size() > 3 && sid.substr(0,3) == (Mess->lang2Code()+"#"))
	{
	    u_cfg = cfg.at(sid.substr(3),true);
	    if(u_cfg && !(u_cfg->fld().flg()&TCfg::TransltText)) continue;
	    trPresent = true;
	}
	if(!u_cfg) continue;

	if(u_cfg->fld().flg()&TCfg::Key)
	{
	    req_where = req_where + (next_wr?"AND `":"`") + TSYS::strEncode(sid,TSYS::SQL) + "`='"  +TSYS::strEncode(getVal(*u_cfg),TSYS::SQL) + "' ";
	    if(first_key.empty()) first_key = TSYS::strEncode(sid,TSYS::SQL);
	    next_wr = true;
	}
	else if(u_cfg->view())
	{
	    req = req + (first_sel?"`":",`") + TSYS::strEncode(sid,TSYS::SQL) + "`";
	    first_sel = false;
	}
    }
    if(first_sel) req += "`"+first_key+"`";
    req = req + " FROM `" + TSYS::strEncode(owner().bd,TSYS::SQL) + "`.`" + TSYS::strEncode(name(),TSYS::SQL) + "` WHERE " + req_where;

    //> Query
    owner().sqlReq( req, &tbl );
    if( tbl.size() < 2 ) throw TError(TSYS::DBRowNoPresent,nodePath().c_str(),_("Row is not present!"));

    //> Processing of query
    for( unsigned i_fld = 0; i_fld < tbl[0].size(); i_fld++ )
    {
	sid = tbl[0][i_fld];
	TCfg *u_cfg = cfg.at(sid,true);
	if( u_cfg ) setVal(*u_cfg,tbl[1][i_fld]);
	else if( trPresent && sid.size() > 3 && sid.substr(0,3) == (Mess->lang2Code()+"#") && tbl[1][i_fld].size() )
	{
	    u_cfg = cfg.at(sid.substr(3),true);
	    if( u_cfg ) setVal(*u_cfg,tbl[1][i_fld]);
	}
    }
}

void MTable::fieldSet( TConfig &cfg )
{
    vector< vector<string> > tbl;

    if( tblStrct.empty() ) throw TError(TSYS::DBTableEmpty,nodePath().c_str(),_("Table is empty!"));
    mLstUse = SYS->sysTm();

    string sid, sval;
    bool isVarTextTransl = (!Mess->lang2CodeBase().empty() && !cfg.noTransl() && Mess->lang2Code() != Mess->lang2CodeBase());
    //> Get config fields list
    vector<string> cf_el;
    cfg.cfgList(cf_el);

    //> Check for translation present
    bool trPresent = isVarTextTransl, trDblDef = false;
    for( unsigned i_fld = 1; i_fld < tblStrct.size(); i_fld++ )
    {
	if( (trPresent || cfg.noTransl()) && (!isVarTextTransl || trDblDef) ) break;
	sid = tblStrct[i_fld][0];
	if( sid.size() > 3 )
	{
	    if( !trPresent && sid.substr(0,3) == (Mess->lang2Code()+"#") ) trPresent = true;
	    if( Mess->lang2Code() == Mess->lang2CodeBase() && !trDblDef && sid.compare(0,3,Mess->lang2CodeBase()+"#") == 0 ) trDblDef = true;
	}
    }
    if( trDblDef ) fieldFix(cfg);

    //> Get present fields list
    string req_where = "WHERE ";
    //>> Add key list to query
    bool next = false;
    for( unsigned i_el = 0; i_el < cf_el.size(); i_el++ )
    {
	TCfg &u_cfg = cfg.cfg(cf_el[i_el]);
	if( !(u_cfg.fld().flg()&TCfg::Key) ) continue;
	req_where = req_where + (next?"AND `":"`") + TSYS::strEncode(cf_el[i_el],TSYS::SQL) + "`='" + TSYS::strEncode(getVal(u_cfg),TSYS::SQL) + "' ";
	next = true;
    }

    //> Prepare query
    //>> Try for get already present field
    string req = "SELECT 1 FROM `" + TSYS::strEncode(owner().bd,TSYS::SQL) + "`.`" + TSYS::strEncode(name(),TSYS::SQL) + "` " + req_where;
    try{ owner().sqlReq( req, &tbl ); }
    catch(TError err)	{ fieldFix(cfg); owner().sqlReq( req ); }
    if( tbl.size() < 2 )
    {
	//>> Add new record
	req = "INSERT INTO `" + TSYS::strEncode(owner().bd,TSYS::SQL) + "`.`" + TSYS::strEncode(name(),TSYS::SQL) + "` ";
	string ins_name, ins_value;
	next = false;
	for( unsigned i_el = 0; i_el < cf_el.size(); i_el++ )
	{
	    TCfg &u_cfg = cfg.cfg(cf_el[i_el]);
	    if( !(u_cfg.fld().flg()&TCfg::Key) && !u_cfg.view() ) continue;

	    bool isTransl = (u_cfg.fld().flg()&TCfg::TransltText && trPresent && !u_cfg.noTransl());
	    ins_name = ins_name + (next?",`":"`") + TSYS::strEncode(cf_el[i_el],TSYS::SQL) + "` " +
		( isTransl ? (",`" + TSYS::strEncode(Mess->lang2Code()+"#"+cf_el[i_el],TSYS::SQL) + "` ") : "" );
	    sval = getVal(u_cfg);
	    ins_value = ins_value + (next?",'":"'") + TSYS::strEncode(sval,TSYS::SQL) + "' " +
		( isTransl ? (",'" + TSYS::strEncode(sval,TSYS::SQL) + "' ") : "" );
	    next = true;
	}
	req = req + "("+ins_name+") VALUES ("+ins_value+")";
    }
    else
    {
	//>> Update present record
	req = "UPDATE `" + TSYS::strEncode(owner().bd,TSYS::SQL) + "`.`" + TSYS::strEncode(name(),TSYS::SQL) + "` SET ";
	next = false;
	for( unsigned i_el = 0; i_el < cf_el.size(); i_el++ )
	{
	    TCfg &u_cfg = cfg.cfg(cf_el[i_el]);
	    if( u_cfg.fld().flg()&TCfg::Key || !u_cfg.view() ) continue;

	    bool isTransl = (u_cfg.fld().flg()&TCfg::TransltText && trPresent && !u_cfg.noTransl());
	    sid = isTransl ? (Mess->lang2Code()+"#"+cf_el[i_el]) : cf_el[i_el];
	    sval = getVal(u_cfg);
	    req = req + (next?",`":"`") + TSYS::strEncode(sid,TSYS::SQL) + "`='" + TSYS::strEncode(sval,TSYS::SQL) + "' ";
	    next = true;
	}
	req = req + req_where;
    }

    //> Query
    try{ owner().sqlReq( req ); }
    catch(TError err)	{ fieldFix(cfg); owner().sqlReq( req ); }
}

void MTable::fieldDel( TConfig &cfg )
{
    if( tblStrct.empty() ) throw TError(TSYS::DBTableEmpty,nodePath().c_str(),_("Table is empty!"));
    mLstUse = SYS->sysTm();

    //> Get config fields list
    vector<string> cf_el;
    cfg.cfgList(cf_el);

    //> Prepare request
    string req = "DELETE FROM `" + TSYS::strEncode(owner().bd,TSYS::SQL) + "`.`" + TSYS::strEncode(name(),TSYS::SQL) + "` WHERE ";
    //>> Add key list to query
    bool next = false;
    for( unsigned i_el = 0; i_el < cf_el.size(); i_el++ )
    {
	TCfg &u_cfg = cfg.cfg(cf_el[i_el]);
	if( u_cfg.fld().flg()&TCfg::Key && u_cfg.keyUse() )
	{
	    req = req + (next?"AND `":"`") + TSYS::strEncode(cf_el[i_el],TSYS::SQL) + "`='" + TSYS::strEncode(getVal(u_cfg),TSYS::SQL) + "' ";
	    next = true;
	}
    }

    owner().sqlReq( req );
}

void MTable::fieldFix( TConfig &cfg )
{
    bool next = false, next_key = false;

    if( tblStrct.empty() ) throw TError(TSYS::DBTableEmpty,nodePath().c_str(),_("Table is empty!"));

    //string lang2Code = cfg.noTransl( ) ? "" : Mess->lang2Code();
    bool isVarTextTransl = (!Mess->lang2CodeBase().empty() && !cfg.noTransl() && Mess->lang2Code() != Mess->lang2CodeBase());
    //> Get config fields list
    vector<string> cf_el;
    cfg.cfgList(cf_el);

    //> Prepare request for fix structure
    string req = "ALTER TABLE `" + TSYS::strEncode(owner().bd,TSYS::SQL) + "`.`" + TSYS::strEncode(name(),TSYS::SQL) + "` DROP PRIMARY KEY, ";

    string pr_keys;
    int keyCnt = 0;
    for( unsigned i_cf = 0, i_fld; i_cf < cf_el.size(); i_cf++ )
    {
	TCfg &u_cfg = cfg.cfg(cf_el[i_cf]);
	//>> Check primary key
	if( u_cfg.fld().flg()&TCfg::Key )
	{
	    pr_keys = pr_keys + (next_key?",`":"`") + TSYS::strEncode(u_cfg.name(),TSYS::SQL) + "`";
	    next_key = true;
	    keyCnt++;
	}

	for( i_fld = 1; i_fld < tblStrct.size(); i_fld++ )
	    if( cf_el[i_cf] == tblStrct[i_fld][0] ) break;
	/*{
	    int fsz = cf_el[i_cf].size();
	    int tsz = fsz - tblStrct[i_fld][0].size();
	    if( (tsz == 0 && cf_el[i_cf] == tblStrct[i_fld][0]) || (tsz == 3 && cf_el[i_cf] == tblStrct[i_fld][0].substr(0,fsz-3) && tblStrct[i_fld][0][fsz] == '#' ) )
		break;
	}*/
	//>> Change field
	string f_tp;
	if( i_fld < tblStrct.size() )
	{
	    switch(u_cfg.fld().type())
	    {
		case TFld::String:
		    if( u_cfg.fld().len() < 256 || u_cfg.fld().flg()&TCfg::Key )
			f_tp = "varchar("+TSYS::int2str(vmax(1,vmin((u_cfg.fld().flg()&TCfg::Key)?(333/(2*keyCnt)):255,u_cfg.fld().len())))+")";
		    else if( u_cfg.fld().len() < 65536 )
			f_tp = "text";
		    else f_tp = "mediumtext";
		    break;
		case TFld::Integer:
		    if( u_cfg.fld().flg()&TFld::DateTimeDec )	f_tp = "datetime";
		    else if( !u_cfg.fld().len() ) f_tp = "int";
		    else f_tp = "int("+TSYS::int2str(vmax(1,u_cfg.fld().len()))+")";
		    break;
		case TFld::Real:
		    if( !u_cfg.fld().len() ) f_tp = "double";
		    else f_tp = "double("+TSYS::int2str(vmax(3,u_cfg.fld().len()))+","+TSYS::int2str(vmax(2,u_cfg.fld().dec()))+")";
		    break;
		case TFld::Boolean:
		    f_tp = "tinyint(1)";
		    break;
		default: break;
	    }
	    if( tblStrct[i_fld][1] != f_tp )
	    {
		req = req + (next?",CHANGE `":"CHANGE `") + TSYS::strEncode(cf_el[i_cf],TSYS::SQL) + "` `" + TSYS::strEncode(cf_el[i_cf],TSYS::SQL) + "` ";
		next = true;
		fieldPrmSet(u_cfg,(i_cf>0)?cf_el[i_cf-1]:"",req,keyCnt);
	    }
	}
	//>> Add field
	else
	{
	    req = req + (next?",ADD `":"ADD `") + TSYS::strEncode(cf_el[i_cf],TSYS::SQL) + "` ";
	    next = true;
	    fieldPrmSet(u_cfg,(i_cf>0)?cf_el[i_cf-1]:"",req,keyCnt);
	}
	//> Check other languages
	if( u_cfg.fld().flg()&TCfg::TransltText )
	{
	    bool col_cur = false;
	    for( unsigned i_c = i_fld; i_c < tblStrct.size(); i_c++ )
		if( tblStrct[i_c][0].size() > 3 && tblStrct[i_c][0].substr(2) == ("#"+cf_el[i_cf]) && tblStrct[i_c][0].substr(0,2) != Mess->lang2CodeBase() )
		{
		    if( tblStrct[i_c][1] != f_tp )
		    {
			req = req + (next?",CHANGE `":"CHANGE `") + TSYS::strEncode(tblStrct[i_c][0],TSYS::SQL) + "` `" + TSYS::strEncode(tblStrct[i_c][0],TSYS::SQL) + "` ";
			next = true;
			fieldPrmSet(u_cfg,(i_cf>0)?cf_el[i_cf-1]:"",req,keyCnt);
		    }
		    if( tblStrct[i_c][0].substr(0,2) == Mess->lang2Code() ) col_cur = true;
		}
	    if( !col_cur && isVarTextTransl )
	    {
		req = req + (next?",ADD `":"ADD `") + TSYS::strEncode(Mess->lang2Code()+"#"+cf_el[i_cf],TSYS::SQL) + "` ";
		next = true;
		fieldPrmSet(u_cfg,(i_cf>0)?cf_el[i_cf-1]:"",req,keyCnt);
	    }
	}
    }
    //> DROP fields
    for( unsigned i_fld = 1, i_cf; i_fld < tblStrct.size(); i_fld++ )
    {
	for( i_cf = 0; i_cf < cf_el.size(); i_cf++ )
	    if( cf_el[i_cf] == tblStrct[i_fld][0] ||
		    (cfg.cfg(cf_el[i_cf]).fld().flg()&TCfg::TransltText && tblStrct[i_fld][0].size() > 3 && 
		    tblStrct[i_fld][0].substr(2) == ("#"+cf_el[i_cf]) && tblStrct[i_fld][0].substr(0,2) != Mess->lang2CodeBase()) )
		break;
	if( i_cf >= cf_el.size() )
	{
	    req = req + (next?",DROP `":"DROP `") + TSYS::strEncode(tblStrct[i_fld][0],TSYS::SQL) + "` ";
	    next = true;
	}
    }
    req += ",ADD PRIMARY KEY (" + pr_keys + ") ";

    if( next )
    {
	owner().sqlReq( req );
	//> Update structure information
	req = "DESCRIBE `" + TSYS::strEncode(owner().bd,TSYS::SQL) + "`.`" + TSYS::strEncode(name(),TSYS::SQL) + "`";
	owner().sqlReq( req, &tblStrct );
    }
}

void MTable::fieldPrmSet( TCfg &cfg, const string &last, string &req, int keyCnt )
{
    //> Type param
    switch(cfg.fld().type())
    {
	case TFld::String:
	    if( cfg.fld().len() < 256 || cfg.fld().flg()&TCfg::Key )
		req=req+"varchar("+SYS->int2str(vmax(1,vmin((cfg.fld().flg()&TCfg::Key)?(333/(2*keyCnt)):255,cfg.fld().len())))+") "+
			((cfg.fld().flg()&TCfg::Key)?"BINARY":"")+" NOT NULL DEFAULT '"+cfg.fld().def()+"' ";
	    else if( cfg.fld().len() < 65536 )
		req=req+"text NOT NULL ";// DEFAULT '"+cfg.fld().def()+"' ";
	    else req=req+"mediumtext NOT NULL ";// DEFAULT '"+cfg.fld().def()+"' ";
	    break;
	case TFld::Integer:
	    if( cfg.fld().flg()&TFld::DateTimeDec )
		req=req+"datetime NOT NULL DEFAULT '"+UTCtoSQL(atoi(cfg.fld().def().c_str()))+"' ";
	    else if( !cfg.fld().len() ) req=req+"int NOT NULL DEFAULT '"+TSYS::int2str(atoi(cfg.fld().def().c_str()))+"' ";
	    else req=req+"int("+SYS->int2str(vmax(1,cfg.fld().len()))+") NOT NULL DEFAULT '"+TSYS::int2str(atoi(cfg.fld().def().c_str()))+"' ";
	    break;
	case TFld::Real:
	    if( !cfg.fld().len() ) req=req+"double NOT NULL DEFAULT '"+TSYS::real2str(atof(cfg.fld().def().c_str()))+"' ";
	    else req=req+"double("+SYS->int2str(vmax(3,cfg.fld().len()))+","+SYS->int2str(vmax(2,cfg.fld().dec()))+") NOT NULL DEFAULT '"+TSYS::real2str(atof(cfg.fld().def().c_str()))+"' ";
	    break;
	case TFld::Boolean:
	    req=req+"tinyint(1) NOT NULL DEFAULT '"+TSYS::int2str(atoi(cfg.fld().def().c_str()))+"' ";
	    break;
	default: break;
    }
    //> Position param
    //if( last.size() )	req=req+"AFTER `"+last+"` ";
}

string MTable::getVal( TCfg &cfg )
{
    switch(cfg.fld().type())
    {
	case TFld::String:	return cfg.getS();
	case TFld::Integer:
	    if(cfg.fld().flg()&TFld::DateTimeDec) return UTCtoSQL(cfg.getI());
	    else		return SYS->int2str(cfg.getI());
	case TFld::Real:
	{
	    double vl = cfg.getR();
	    if(vl == EVAL_REAL)	return SYS->real2str(EVAL_REAL_MYSQL);
	    return SYS->real2str(vl);
	}
	case TFld::Boolean:	return SYS->int2str(cfg.getB());
	default: break;
    }
    return "";
}

void MTable::setVal( TCfg &cfg, const string &val )
{
    switch( cfg.fld().type() )
    {
	case TFld::String:	cfg.setS(val);	break;
	case TFld::Integer:
	    if( cfg.fld().flg()&TFld::DateTimeDec )	cfg.setI(SQLtoUTC(val));
	    else cfg.setI(atoi(val.c_str()));
	    break;
	case TFld::Real:
	{
	    double vl = atof(val.c_str());
	    cfg.setR( (vl!=EVAL_REAL_MYSQL) ? vl : EVAL_REAL );
	    break;
	}
	case TFld::Boolean:	cfg.setB(atoi(val.c_str()));	break;
	default: break;
    }
}

string MTable::UTCtoSQL( time_t val )
{
    char buf[255];
    struct tm tm_tm;

    //localtime_r(&val,&tm_tm);
    gmtime_r(&val,&tm_tm);
    int rez = strftime( buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_tm );

    return (rez>0) ? string(buf,rez) : "";
}

time_t MTable::SQLtoUTC( const string &val )
{
    struct tm stm;
    strptime(val.c_str(),"%Y-%m-%d %H:%M:%S",&stm);

    //return mktime(&stm);
    return timegm(&stm);
}
