
//OpenSCADA system file: tprmtmpl.cpp
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

#include "tsys.h"
#include "tprmtmpl.h"

using namespace OSCADA;

//*************************************************
//* TPrmTempl                                     *
//*************************************************
TPrmTempl::TPrmTempl( const char *iid, const char *iname ) :
    TFunction(string("tmpl_")+iid), TConfig(&SYS->daq().at().tplE()), m_id(cfg("ID").getSd()), m_prog(cfg("PROGRAM").getSd())
{
    m_id = iid;
    setName(iname);
}

TPrmTempl::~TPrmTempl(  )
{

}

TCntrNode &TPrmTempl::operator=( TCntrNode &node )
{
    TPrmTempl *src_n = dynamic_cast<TPrmTempl*>(&node);
    if( !src_n ) return *this;

    string tid = id();
    *(TConfig *)this = *(TConfig*)src_n;
    *(TFunction *)this = *(TFunction*)src_n;
    m_id = tid;

    if( src_n->startStat( ) && !startStat( ) )  setStart( true );

    return *this;
}

void TPrmTempl::postEnable( int flag )
{
    //> Create default IOs
    if( flag&TCntrNode::NodeConnect )
    {
	ioIns( new IO("f_frq",_("Function calculate frequency (Hz)"),IO::Real,TPrmTempl::LockAttr,"1000",false),0);
	ioIns( new IO("f_start",_("Function start flag"),IO::Boolean,TPrmTempl::LockAttr,"0",false),1);
	ioIns( new IO("f_stop",_("Function stop flag"),IO::Boolean,TPrmTempl::LockAttr,"0",false),2);
	ioIns( new IO("f_err",_("Function error"),IO::String,TPrmTempl::LockAttr,"0",false),3);
    }
}

void TPrmTempl::postDisable(int flag)
{
    try
    {
	if( flag )
	{
	    SYS->db().at().dataDel(owner().fullDB(),owner().owner().nodePath()+owner().tbl(),*this,true);
	    //> Delete template's IO
	    TConfig cfg(&owner().owner().tplIOE());
	    cfg.cfg("TMPL_ID").setS(id(),true);
	    SYS->db().at().dataDel(owner().fullDB()+"_io",owner().owner().nodePath()+owner().tbl()+"_io/",cfg);
	}
    }catch(TError err)
    { mess_warning(err.cat.c_str(),"%s",err.mess.c_str()); }
}

TPrmTmplLib &TPrmTempl::owner( )	{ return *(TPrmTmplLib*)nodePrev(); }

string TPrmTempl::name( )		{ string nm = cfg("NAME").getS(); return nm.size() ? nm : id(); }

void TPrmTempl::setName( const string &inm )	{ cfg("NAME").setS(inm); modif(); }

string TPrmTempl::descr( )		{ return cfg("DESCR").getS(); }

void TPrmTempl::setDescr( const string &idsc )	{ cfg("DESCR").setS(idsc); modif(); }

int TPrmTempl::maxCalcTm( )		{ return cfg("MAXCALCTM").getI(); }

void TPrmTempl::setMaxCalcTm( int vl )	{ cfg("MAXCALCTM").setI(vl); modif(); }

string TPrmTempl::progLang()
{
    string tPrg = m_prog.getVal();
    return tPrg.substr(0,tPrg.find("\n"));
}

string TPrmTempl::prog( )
{
    string tPrg = m_prog.getVal();
    size_t lngEnd = tPrg.find("\n");
    return tPrg.substr((lngEnd==string::npos)?0:lngEnd+1);
}

void TPrmTempl::setProgLang( const string &ilng )
{
    if(startStat()) setStart(false);

    m_prog = ilng+"\n"+prog();
    modif();
}

void TPrmTempl::setProg( const string &iprg )
{
    if(startStat()) setStart(false);

    m_prog = progLang()+"\n"+iprg;
    modif();
}

void TPrmTempl::setStart( bool vl )
{
    if(startStat() == vl) return;
    if(vl)
    {
	//> Check for doubled attributes clear
	std::map<string,bool> ioIds;
	for(int id = 0; id < ioSize(); )
	    if(ioIds.find(io(id)->id()) != ioIds.end()) { ioDel(id); modif(); }
	    else { ioIds[io(id)->id()] = true; id++; }

	//> Compile new function
	if(prog().size())
	    work_prog = SYS->daq().at().at(TSYS::strSepParse(progLang(),0,'.')).at().
					compileFunc(TSYS::strSepParse(progLang(),1,'.'),*this,prog(),"",maxCalcTm());
    }
    TFunction::setStart(vl);
}

AutoHD<TFunction> TPrmTempl::func()
{
    if(!startStat())	throw TError(nodePath().c_str(),_("Tempate is disabled."));
    if(!prog().size())	return AutoHD<TFunction>(this);
    return SYS->nodeAt(work_prog);
}

void TPrmTempl::load_( )
{
    if( !SYS->chkSelDB(owner().DB()) ) return;

    //> Self load
    SYS->db().at().dataGet(owner().fullDB(),owner().owner().nodePath()+owner().tbl(),*this);

    //> Load IO
    vector<string> u_pos;
    TConfig cfg(&owner().owner().tplIOE());
    cfg.cfg("TMPL_ID").setS(id(),true);
    for( int io_cnt = 0; SYS->db().at().dataSeek(owner().fullDB()+"_io",owner().owner().nodePath()+owner().tbl()+"_io",io_cnt++,cfg); )
    {
	string sid = cfg.cfg("ID").getS();

	//> Position storing
	int pos = cfg.cfg("POS").getI();
	while((int)u_pos.size() <= pos)	u_pos.push_back("");
	u_pos[pos] = sid;

	int iid = ioId(sid);
	if(iid < 0)
	    ioIns( new IO(sid.c_str(),cfg.cfg("NAME").getS().c_str(),(IO::Type)cfg.cfg("TYPE").getI(),cfg.cfg("FLAGS").getI(),
			cfg.cfg("VALUE").getS().c_str(),false), pos );
	else
	{
	    io(iid)->setName(cfg.cfg("NAME").getS());
	    io(iid)->setType((IO::Type)cfg.cfg("TYPE").getI());
	    io(iid)->setFlg(cfg.cfg("FLAGS").getI());
	    io(iid)->setDef(cfg.cfg("VALUE").getS());
	}
    }

    //> Remove holes
    for(unsigned i_p = 0; i_p < u_pos.size(); )
	if(u_pos[i_p].empty()) u_pos.erase(u_pos.begin()+i_p);
	else i_p++;

    //> Position fixing
    for(int i_p = 0; i_p < (int)u_pos.size(); i_p++)
    {
	int iid = ioId(u_pos[i_p]);
	if(iid != i_p) try{ ioMove(iid,i_p); } catch(...){ }
    }
}

void TPrmTempl::save_( )
{
    string w_db = owner().fullDB();
    string w_cfgpath = owner().owner().nodePath()+owner().tbl();
    //> Self save
    SYS->db().at().dataSet(w_db,w_cfgpath,*this);

    //> Save IO
    TConfig cfg(&owner().owner().tplIOE());
    cfg.cfg("TMPL_ID").setS(id(),true);
    for(int i_io = 0; i_io < ioSize(); i_io++)
    {
	if( io(i_io)->flg()&TPrmTempl::LockAttr ) continue;
	cfg.cfg("ID").setS(io(i_io)->id());
	cfg.cfg("NAME").setS(io(i_io)->name());
	cfg.cfg("TYPE").setI(io(i_io)->type());
	cfg.cfg("FLAGS").setI(io(i_io)->flg());
	cfg.cfg("VALUE").setNoTransl( !(io(i_io)->type()==IO::String || io(i_io)->flg()&TPrmTempl::CfgLink) );
	cfg.cfg("VALUE").setS(io(i_io)->def());
	cfg.cfg("POS").setI(i_io);
	SYS->db().at().dataSet(w_db+"_io",w_cfgpath+"_io",cfg);
    }
    //> Clear IO
    cfg.cfgViewAll(false);
    for( int fld_cnt=0; SYS->db().at().dataSeek(w_db+"_io",w_cfgpath+"_io",fld_cnt++,cfg); )
    {
	string sio = cfg.cfg("ID").getS( );
	if( ioId(sio) < 0 || io(ioId(sio))->flg()&TPrmTempl::LockAttr )
	{
	    SYS->db().at().dataDel(w_db+"_io",w_cfgpath+"_io",cfg,true);
	    fld_cnt--;
	}
    }
}

void TPrmTempl::preIOCfgChange()
{
    if(startStat()) setStart(false);
}

TVariant TPrmTempl::objFuncCall( const string &iid, vector<TVariant> &prms, const string &user )
{
    //> Config functions call
    TVariant cfRez = objFunc(iid, prms, user);
    if(!cfRez.isNull()) return cfRez;

    return TCntrNode::objFuncCall(iid, prms, user);
}

void TPrmTempl::cntrCmdProc( XMLNode *opt )
{
    //> Get page info
    if(opt->name() == "info")
    {
	TCntrNode::cntrCmdProc(opt);
	ctrMkNode("oscada_cntr",opt,-1,"/",_("Parameter template: ")+name(),RWRWR_,"root",SDAQ_ID);
	if(ctrMkNode("area",opt,-1,"/tmpl",_("Template")))
	{
	    if(ctrMkNode("area",opt,-1,"/tmpl/st",_("State")))
	    {
		ctrMkNode("fld",opt,-1,"/tmpl/st/st",_("Accessing"),RWRWR_,"root",SDAQ_ID,1,"tp","bool");
		ctrMkNode("fld",opt,-1,"/tmpl/st/use",_("Used"),R_R_R_,"root",SDAQ_ID,1,"tp","dec");
	    }
	    if(ctrMkNode("area",opt,-1,"/tmpl/cfg",_("Config")))
	    {
		ctrMkNode("fld",opt,-1,"/tmpl/cfg/id",_("Id"),R_R_R_,"root",SDAQ_ID,1,"tp","str");
		ctrMkNode("fld",opt,-1,"/tmpl/cfg/name",_("Name"),RWRWR_,"root",SDAQ_ID,2,"tp","str","len","50");
		ctrMkNode("fld",opt,-1,"/tmpl/cfg/descr",_("Description"),RWRWR_,"root",SDAQ_ID,3,"tp","str","cols","100","rows","4");
		ctrMkNode("fld",opt,-1,"/tmpl/cfg/m_calc_tm",_("Maximum calc time (sec)"),RWRWR_,"root",SDAQ_ID,3,"tp","dec","min","0","max","3600");
	    }
	}
	if(ctrMkNode("area",opt,-1,"/io",_("IO")))
	{
	    if(ctrMkNode("table",opt,-1,"/io/io",_("IO"),RWRWR_,"root",SDAQ_ID,2,"s_com","add,del,ins,move","rows","15"))
	    {
		ctrMkNode("list",opt,-1,"/io/io/0",_("Id"),RWRWR_,"root",SDAQ_ID,1,"tp","str");
		ctrMkNode("list",opt,-1,"/io/io/1",_("Name"),RWRWR_,"root",SDAQ_ID,1,"tp","str");
		ctrMkNode("list",opt,-1,"/io/io/2",_("Type"),RWRWR_,"root",SDAQ_ID,5,"tp","dec","idm","1","dest","select",
		    "sel_id",(TSYS::int2str(IO::Real)+";"+TSYS::int2str(IO::Integer)+";"+TSYS::int2str(IO::Boolean)+";"+TSYS::int2str(IO::String)+";"+TSYS::int2str(IO::Object)).c_str(),
		    "sel_list",_("Real;Integer;Boolean;String;Object"));
		ctrMkNode("list",opt,-1,"/io/io/3",_("Mode"),RWRWR_,"root",SDAQ_ID,5,"tp","dec","idm","1","dest","select",
		    "sel_id",(TSYS::int2str(IO::Default)+";"+TSYS::int2str(IO::Output)).c_str(),"sel_list",_("Input;Output"));
		ctrMkNode("list",opt,-1,"/io/io/4",_("Attribute"),RWRWR_,"root",SDAQ_ID,5,"tp","dec","idm","1","dest","select",
		    "sel_id",(TSYS::int2str(IO::Default)+";"+TSYS::int2str(TPrmTempl::AttrRead)+";"+TSYS::int2str(TPrmTempl::AttrFull)).c_str(),
		    "sel_list",_("No attribute;Read only;Full access"));
		ctrMkNode("list",opt,-1,"/io/io/5",_("Configure"),RWRWR_,"root",SDAQ_ID,5,"tp","dec","idm","1","dest","select",
		    "sel_id",(TSYS::int2str(IO::Default)+";"+TSYS::int2str(TPrmTempl::CfgPublConst)+";"+TSYS::int2str(TPrmTempl::CfgLink)).c_str(),
		    "sel_list",_("Constant;Public constant;Link"));
		ctrMkNode("list",opt,-1,"/io/io/6",_("Value"),RWRWR_,"root",SDAQ_ID,1,"tp","str");
	    }
	    ctrMkNode("fld",opt,-1,"/io/prog_lang",_("Programm language"),RWRW__,"root",SDAQ_ID,3,"tp","str","dest","sel_ed","select","/io/plang_ls");
	    ctrMkNode("fld",opt,-1,"/io/prog",_("Programm"),RWRW__,"root",SDAQ_ID,3,"tp","str","rows","10","SnthHgl","1");
	}
	return;
    }

    //> Process command to page
    vector<string> list;
    string a_path = opt->attr("path");
    if(a_path == "/tmpl/st/st")
    {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD))	opt->setText(startStat()?"1":"0");
	if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR))	setStart(atoi(opt->text().c_str()));
    }
    else if(a_path == "/tmpl/st/use" && ctrChkNode(opt))	opt->setText(TSYS::int2str(startStat()?func().at().use():0));
    else if(a_path == "/tmpl/cfg/id" && ctrChkNode(opt))	opt->setText(id());
    else if(a_path == "/tmpl/cfg/name")
    {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD))	opt->setText(name());
	if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR))	setName(opt->text());
    }
    else if(a_path == "/tmpl/cfg/descr")
    {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD))	opt->setText(descr());
	if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR))	setDescr(opt->text());
    }
    else if(a_path == "/tmpl/cfg/m_calc_tm")
    {
        if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD))  opt->setText(TSYS::int2str(maxCalcTm()));
        if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR))  setMaxCalcTm(atoi(opt->text().c_str()));
    }
    else if(a_path == "/io/io")
    {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD))
	{
	    XMLNode *n_id   = ctrMkNode("list",opt,-1,"/io/io/0","");
	    XMLNode *n_nm   = ctrMkNode("list",opt,-1,"/io/io/1","");
	    XMLNode *n_type = ctrMkNode("list",opt,-1,"/io/io/2","");
	    XMLNode *n_mode = ctrMkNode("list",opt,-1,"/io/io/3","");
	    XMLNode *n_attr = ctrMkNode("list",opt,-1,"/io/io/4","");
	    XMLNode *n_accs = ctrMkNode("list",opt,-1,"/io/io/5","");
	    XMLNode *n_val  = ctrMkNode("list",opt,-1,"/io/io/6","");

	    for(int id = 0; id < ioSize(); id++)
	    {
		if(n_id)	n_id->childAdd("el")->setText(io(id)->id());
		if(n_nm)	n_nm->childAdd("el")->setText(io(id)->name());
		if(n_type)	n_type->childAdd("el")->setText(TSYS::int2str(io(id)->type()));
		if(n_mode)	n_mode->childAdd("el")->setText(TSYS::int2str(io(id)->flg()&(IO::Output|IO::Return)));
		if(n_attr)	n_attr->childAdd("el")->setText(TSYS::int2str(io(id)->flg()&(TPrmTempl::AttrRead|TPrmTempl::AttrFull)));
		if(n_accs)	n_accs->childAdd("el")->setText(TSYS::int2str(io(id)->flg()&(TPrmTempl::CfgPublConst|TPrmTempl::CfgLink)));
		if(n_val)	n_val->childAdd("el")->setText(io(id)->def());
	    }
	}
	if(ctrChkNode(opt,"add",RWRWR_,"root",SDAQ_ID,SEC_WR))	{ ioAdd(new IO("new",_("New IO"),IO::Real,IO::Default)); modif(); }
	if(ctrChkNode(opt,"ins",RWRWR_,"root",SDAQ_ID,SEC_WR))	{ ioIns(new IO("new",_("New IO"),IO::Real,IO::Default), atoi(opt->attr("row").c_str())); modif(); }
	if(ctrChkNode(opt,"del",RWRWR_,"root",SDAQ_ID,SEC_WR))
	{
	    int row = atoi(opt->attr("row").c_str());
	    if(io(row)->flg()&TPrmTempl::LockAttr)
		throw TError(nodePath().c_str(),_("Deleting lock attribute in not allow."));
	    ioDel(row);
	    modif();
	}
	if(ctrChkNode(opt,"move",RWRWR_,"root",SDAQ_ID,SEC_WR))	{ ioMove( atoi(opt->attr("row").c_str()), atoi(opt->attr("to").c_str()) ); modif(); }
	if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR))
	{
	    int row = atoi(opt->attr("row").c_str());
	    int col = atoi(opt->attr("col").c_str());
	    if(io(row)->flg()&TPrmTempl::LockAttr)	throw TError(nodePath().c_str(),_("Changing locked attribute is not allowed."));
	    if((col == 0 || col == 1) && !opt->text().size())	throw TError(nodePath().c_str(),_("Empty value is not valid."));
	    switch(col)
	    {
		case 0:	io(row)->setId(opt->text());	break;
		case 1:	io(row)->setName(opt->text());	break;
		case 2:	io(row)->setType((IO::Type)atoi(opt->text().c_str()));	break;
		case 3:	io(row)->setFlg(io(row)->flg()^((io(row)->flg()^atoi(opt->text().c_str()))&(IO::Output|IO::Return)));		break;
		case 4:	io(row)->setFlg(io(row)->flg()^((io(row)->flg()^atoi(opt->text().c_str()))&(TPrmTempl::AttrRead|TPrmTempl::AttrFull)));		break;
		case 5:	io(row)->setFlg(io(row)->flg()^((io(row)->flg()^atoi(opt->text().c_str()))&(TPrmTempl::CfgPublConst|TPrmTempl::CfgLink)));	break;
		case 6:	io(row)->setDef(opt->text());	break;
	    }
	    modif();
	}
    }
    else if(a_path == "/io/prog_lang")
    {
	if(ctrChkNode(opt,"get",RWRW__,"root",SDAQ_ID,SEC_RD))	opt->setText(progLang());
	if(ctrChkNode(opt,"set",RWRW__,"root",SDAQ_ID,SEC_WR))	setProgLang(opt->text());
    }
    else if(a_path == "/io/prog")
    {
	if(ctrChkNode(opt,"get",RWRW__,"root",SDAQ_ID,SEC_RD))	opt->setText(prog());
	if(ctrChkNode(opt,"set",RWRW__,"root",SDAQ_ID,SEC_WR))	setProg(opt->text());
	if(ctrChkNode(opt,"SnthHgl",RWRW__,"root",SDAQ_ID,SEC_RD))
	    try
	    {
		SYS->daq().at().at(TSYS::strParse(progLang(),0,".")).at().
				compileFuncSynthHighl(TSYS::strParse(progLang(),1,"."),*opt);
	    } catch(...){ }
    }
    else if(a_path == "/io/plang_ls" && ctrChkNode(opt))
    {
	string tplng = progLang();
	int c_lv = 0;
	string c_path = "", c_el;
	opt->childAdd("el")->setText(c_path);
	for(int c_off = 0; (c_el=TSYS::strSepParse(tplng,0,'.',&c_off)).size(); c_lv++)
	{
	    c_path += c_lv ? "."+c_el : c_el;
	    opt->childAdd("el")->setText(c_path);
	}
	if(c_lv) c_path+=".";
	vector<string>  ls;
	switch(c_lv)
	{
	    case 0:
		SYS->daq().at().modList(ls);
		for(unsigned i_l = 0; i_l < ls.size(); )
		    if(!SYS->daq().at().at(ls[i_l]).at().compileFuncLangs()) ls.erase(ls.begin()+i_l);
		    else i_l++;
		break;
	    case 1:
		if( SYS->daq().at().modPresent(TSYS::strSepParse(tplng,0,'.')) )
		    SYS->daq().at().at(TSYS::strSepParse(tplng,0,'.')).at().compileFuncLangs(&ls);
		break;
	}
	for(unsigned i_l = 0; i_l < ls.size(); i_l++)
	    opt->childAdd("el")->setText(c_path+ls[i_l]);
    }
    else TCntrNode::cntrCmdProc(opt);
}

//*************************************************
//* TPrmTmplLib                                   *
//*************************************************
TPrmTmplLib::TPrmTmplLib( const char *id, const char *name, const string &lib_db ) :
    TConfig(&SYS->daq().at().elLib()), run_st(false), m_id(cfg("ID").getSd()), m_db(cfg("DB").getSd()), work_lib_db(lib_db)
{
    m_id = id;
    setName( name );
    m_db = string("tmplib_")+id;
    m_ptmpl = grpAdd("tmpl_");
}

TPrmTmplLib::~TPrmTmplLib()
{

}

TCntrNode &TPrmTmplLib::operator=( TCntrNode &node )
{
    TPrmTmplLib *src_n = dynamic_cast<TPrmTmplLib*>(&node);
    if( !src_n ) return *this;

    //- Configuration copy -
    string tid = id();
    *(TConfig*)this = *(TConfig*)src_n;
    m_id = tid;
    work_lib_db = src_n->work_lib_db;

    //- Templates copy -
    vector<string> ls;
    src_n->list(ls);
    for(unsigned i_p = 0; i_p < ls.size(); i_p++)
    {
	if(!present(ls[i_p])) add(ls[i_p].c_str());
	(TCntrNode&)at(ls[i_p]).at() = (TCntrNode&)src_n->at(ls[i_p]).at();
    }
    if(src_n->startStat() && !startStat()) start(true);

    return *this;
}

void TPrmTmplLib::preDisable(int flag)
{
    start(false);
}

void TPrmTmplLib::postDisable(int flag)
{
    if( flag )
    {
	//> Delete libraries record
	SYS->db().at().dataDel(work_lib_db.getVal()+"."+owner().tmplLibTable(),owner().nodePath()+"tmplib",*this,true);

	//> Delete temlate librarie's DBs
	SYS->db().at().open(fullDB());
	SYS->db().at().close(fullDB(),true);

	SYS->db().at().open(fullDB()+"_io");
	SYS->db().at().close(fullDB()+"_io",true);
    }
}

TDAQS &TPrmTmplLib::owner( )	{ return *(TDAQS*)nodePrev(); }

string TPrmTmplLib::name( )	{ string nm = cfg("NAME").getS(); return nm.size() ? nm : id(); }

void TPrmTmplLib::setName( const string &vl )	{ cfg("NAME").setS(vl); modif(); }

string TPrmTmplLib::descr( )	{ return cfg("DESCR").getS(); }

void TPrmTmplLib::setDescr( const string &vl )	{ cfg("DESCR").setS(vl); modif(); }

void TPrmTmplLib::setFullDB( const string &vl )
{
    work_lib_db = TSYS::strSepParse(vl,0,'.')+"."+TSYS::strSepParse(vl,1,'.');
    m_db = TSYS::strSepParse(vl,2,'.');
    modifG();
}

void TPrmTmplLib::load_( )
{
    if( !SYS->chkSelDB(DB()) ) return;

    SYS->db().at().dataGet(DB()+"."+owner().tmplLibTable(),owner().nodePath()+"tmplib",*this);

    //> Load templates
    TConfig c_el(&owner().tplE());
    c_el.cfgViewAll(false);
    for( int fld_cnt = 0; SYS->db().at().dataSeek(fullDB(),owner().nodePath()+tbl(), fld_cnt++,c_el); )
    {
	string f_id = c_el.cfg("ID").getS();
	if( !present(f_id) )	add(f_id.c_str());
    }
}

void TPrmTmplLib::save_( )
{
    SYS->db().at().dataSet(DB()+"."+owner().tmplLibTable(),owner().nodePath()+"tmplib",*this);
}

void TPrmTmplLib::start( bool val )
{
    bool isErr = false;
    vector<string> lst;
    list(lst);
    for(unsigned i_f = 0; i_f < lst.size(); i_f++)
	try{ at(lst[i_f]).at().setStart(val); }
	catch(TError err)
	{
	    mess_err(err.cat.c_str(),"%s",err.mess.c_str());
	    mess_err(nodePath().c_str(),_("Template '%s' start is error."),lst[i_f].c_str());
	    isErr = true;
	}

    run_st = val;

    if( isErr )	throw TError(nodePath().c_str(),_("Some templates start error."));
}

void TPrmTmplLib::add( const char *id, const char *name )
{
    chldAdd(m_ptmpl,new TPrmTempl(id,name));
}

TVariant TPrmTmplLib::objFuncCall( const string &iid, vector<TVariant> &prms, const string &user )
{
    //> Config functions call
    TVariant cfRez = objFunc(iid, prms, user);
    if(!cfRez.isNull()) return cfRez;

    return TCntrNode::objFuncCall(iid, prms, user);
}

void TPrmTmplLib::cntrCmdProc( XMLNode *opt )
{
    //> Get page info
    if(opt->name() == "info")
    {
	TCntrNode::cntrCmdProc(opt);
	ctrMkNode("oscada_cntr",opt,-1,"/",_("Parameter templates library: ")+id(),RWRWR_,"root",SDAQ_ID);
	if(ctrMkNode("branches",opt,-1,"/br","",R_R_R_))
	    ctrMkNode("grp",opt,-1,"/br/tmpl_",_("Template"),RWRWR_,"root",SDAQ_ID,2,"idm","1","idSz","20");
	if(ctrMkNode("area",opt,-1,"/lib",_("Library")))
	{
	    if(ctrMkNode("area",opt,-1,"/lib/st",_("State")))
	    {
		ctrMkNode("fld",opt,-1,"/lib/st/st",_("Accessing"),RWRWR_,"root",SDAQ_ID,1,"tp","bool");
		ctrMkNode("fld",opt,-1,"/lib/st/db",_("Library BD"),RWRWR_,"root",SDAQ_ID,4,"tp","str","dest","sel_ed","select","/db/tblList",
		    "help",_("DB address in format [<DB module>.<DB name>.<Table name>].\nFor use main work DB set '*.*'."));
	    }
	    if(ctrMkNode("area",opt,-1,"/lib/cfg",_("Config")))
	    {
		ctrMkNode("fld",opt,-1,"/lib/cfg/id",_("Id"),R_R_R_,"root",SDAQ_ID,1,"tp","str");
		ctrMkNode("fld",opt,-1,"/lib/cfg/name",_("Name"),RWRWR_,"root",SDAQ_ID,2,"tp","str","len","50");
		ctrMkNode("fld",opt,-1,"/lib/cfg/descr",_("Description"),RWRWR_,"root",SDAQ_ID,3,"tp","str","cols","100","rows","3");
	    }
	}
	if(ctrMkNode("area",opt,-1,"/tmpl",_("Parameter templates")))
	    ctrMkNode("list",opt,-1,"/tmpl/tmpl",_("Templates"),RWRWR_,"root",SDAQ_ID,5,"tp","br","idm","1","s_com","add,del","br_pref","tmpl_","idSz","20");
	return;
    }
    //> Process command to page
    string a_path = opt->attr("path");
    if(a_path == "/lib/st/st")
    {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD))	opt->setText(startStat() ? "1" : "0");
	if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR))	start(atoi(opt->text().c_str()));
    }
    else if(a_path == "/lib/st/db")
    {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD))	opt->setText(fullDB());
	if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR))	setFullDB(opt->text());
    }
    else if(a_path == "/lib/cfg/id" && ctrChkNode(opt))		opt->setText(id());
    else if(a_path == "/lib/cfg/name")
    {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD))	opt->setText(name());
	if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR))	setName(opt->text());
    }
    else if(a_path == "/lib/cfg/descr")
    {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD))	opt->setText(descr());
	if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR))	setDescr(opt->text());
    }
    else if(a_path == "/br/tmpl_" || a_path == "/tmpl/tmpl")
    {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD))
	{
	    vector<string> lst;
	    list(lst);
	    for(unsigned i_f=0; i_f < lst.size(); i_f++)
		opt->childAdd("el")->setAttr("id",lst[i_f])->setText(at(lst[i_f]).at().name());
	}
	if(ctrChkNode(opt,"add",RWRWR_,"root",SDAQ_ID,SEC_WR))	add(TSYS::strEncode(opt->attr("id"),TSYS::oscdID).c_str(),opt->text().c_str());
	if(ctrChkNode(opt,"del",RWRWR_,"root",SDAQ_ID,SEC_WR))	del(opt->attr("id").c_str(),true);
    }
    else TCntrNode::cntrCmdProc(opt);
}
