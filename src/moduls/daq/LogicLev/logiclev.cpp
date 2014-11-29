
//OpenSCADA system module DAQ.LogicLev file: logiclev.cpp
/***************************************************************************
 *   Copyright (C) 2006-2014 by Roman Savochenko, <rom_as@oscada.org>      *
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

#include <sys/times.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <terror.h>
#include <tsys.h>
#include <tmess.h>
#include <ttiparam.h>
#include <tdaqs.h>

#include "logiclev.h"

//*************************************************
//* Modul info!                                   *
#define MOD_ID		"LogicLev"
#define MOD_NAME	_("Logic level")
#define MOD_TYPE	SDAQ_ID
#define VER_TYPE	SDAQ_VER
#define MOD_VER		"1.3.0"
#define AUTHORS		_("Roman Savochenko")
#define DESCRIPTION	_("Allow logic level parameters.")
#define LICENSE		"GPL2"
//*************************************************

LogicLev::TTpContr *LogicLev::mod;  //Pointer for direct access to the module

extern "C"
{
#ifdef MOD_INCL
    TModule::SAt daq_LogicLev_module( int n_mod )
#else
    TModule::SAt module( int n_mod )
#endif
    {
	if(n_mod == 0)	return TModule::SAt(MOD_ID,MOD_TYPE,VER_TYPE);
	return TModule::SAt("");
    }

#ifdef MOD_INCL
    TModule *daq_LogicLev_attach( const TModule::SAt &AtMod, const string &source )
#else
    TModule *attach( const TModule::SAt &AtMod, const string &source )
#endif
    {
	if(AtMod == TModule::SAt(MOD_ID,MOD_TYPE,VER_TYPE)) return new LogicLev::TTpContr(source);
	return NULL;
    }
}

using namespace LogicLev;

//*************************************************
//* TTpContr                                      *
//*************************************************
TTpContr::TTpContr( string name ) : TTipDAQ(MOD_ID)
{
    mod		= this;

    mName	= MOD_NAME;
    mType	= MOD_TYPE;
    mVers	= MOD_VER;
    mAuthor	= AUTHORS;
    mDescr	= DESCRIPTION;
    mLicense	= LICENSE;
    mSource	= name;
}

TTpContr::~TTpContr( )
{

}

void TTpContr::load_( )
{
    //Load parameters from command line

}

void TTpContr::postEnable( int flag )
{
    TTipDAQ::postEnable( flag );

    //Controler's bd structure
    fldAdd(new TFld("PRM_BD",_("Parameters table by template"),TFld::String,TFld::NoFlag,"40",""));
    fldAdd(new TFld("PRM_BD_REFL",_("Parameters table for reflection"),TFld::String,TFld::NoFlag,"50",""));
    fldAdd(new TFld("PERIOD",_("Request data period (ms)"),TFld::Integer,TFld::NoFlag,"5","0","0;10000"));	//!!!! Remove at further
    fldAdd(new TFld("SCHEDULE",_("Calculate schedule"),TFld::String,TFld::NoFlag,"100", "1"));
    fldAdd(new TFld("PRIOR",_("Request task priority"),TFld::Integer,TFld::NoFlag,"2","0","-1;99"));

    //Parameter type bd structure
    // Standard parameter type by template
    int t_prm = tpParmAdd("std", "PRM_BD", _("Logical"), true);
    tpPrmAt(t_prm).fldAdd(new TFld("PRM",_("Parameter template"),TFld::String,TCfg::NoVal,"100",""));
    //  Logical level parameter IO BD structure
    elPrmIO.fldAdd(new TFld("PRM_ID",_("Parameter ID"),TFld::String,TCfg::Key,i2s(atoi(OBJ_ID_SZ)*6).c_str()));
    elPrmIO.fldAdd(new TFld("ID",_("ID"),TFld::String,TCfg::Key,OBJ_ID_SZ));
    elPrmIO.fldAdd(new TFld("VALUE",_("Value"),TFld::String,TCfg::TransltText,"200"));

    // A parameter direct reflection
    t_prm = tpParmAdd("pRefl", "PRM_BD_REFL", _("Parameter reflection"), true);
    tpPrmAt(t_prm).fldAdd(new TFld("PSRC",_("Source parameter"),TFld::String,TCfg::NoVal,"100",""));
}

TController *TTpContr::ContrAttach( const string &name, const string &daq_db )	{ return new TMdContr(name,daq_db,this); }

//*************************************************
//* TMdContr                                      *
//*************************************************
TMdContr::TMdContr( string name_c, const string &daq_db, ::TElem *cfgelem) : ::TController(name_c,daq_db,cfgelem),
    mPerOld(cfg("PERIOD").getId()), mPrior(cfg("PRIOR").getId()),
    prcSt(false), callSt(false), endrunReq(false), mPer(1e9)
{
    pthread_mutexattr_t attrM;
    pthread_mutexattr_init(&attrM);
    pthread_mutexattr_settype(&attrM, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&enRes, &attrM);
    pthread_mutexattr_destroy(&attrM);

    cfg("PRM_BD").setS("LogLevPrm_"+name_c);
    cfg("PRM_BD_REFL").setS("LogLevPrmRefl_"+name_c);
}

TMdContr::~TMdContr( )
{
    if(startStat()) stop();

    pthread_mutex_destroy(&enRes);
}

void TMdContr::postDisable(int flag)
{
    TController::postDisable(flag);
    try {
	if(flag) {
	    //Delete parameter's io table
	    string tbl = DB()+"."+cfg("PRM_BD").getS()+"_io";
	    SYS->db().at().open(tbl);
	    SYS->db().at().close(tbl, true);
	}
    }
    catch(TError err)	{ mess_err(err.cat.c_str(),"%s",err.mess.c_str()); }
}

string TMdContr::getStatus( )
{
    string rez = TController::getStatus( );
    if(startStat() && !redntUse()) {
	if(callSt)	rez += TSYS::strMess(_("Call now. "));
	if(period())	rez += TSYS::strMess(_("Call by period: %s. "),tm2s(1e-3*period()).c_str());
	else rez += TSYS::strMess(_("Call next by cron '%s'. "),tm2s(TSYS::cron(cron()),"%d-%m-%Y %R").c_str());
	rez += TSYS::strMess(_("Spent time: %s. "),tm2s(SYS->taskUtilizTm(nodePath('.',true))).c_str());
    }
    return rez;
}

TParamContr *TMdContr::ParamAttach( const string &name, int type )	{ return new TMdPrm(name,&owner().tpPrmAt(type)); }

void TMdContr::load_( )
{
    if(!SYS->chkSelDB(DB())) throw TError();

    TController::load_();

    //Check for get old period method value
    if(mPerOld) { cfg("SCHEDULE").setS(r2s(mPerOld/1e3)); mPerOld = 0; }
}

void TMdContr::start_( )
{
    //Schedule process
    mPer = TSYS::strSepParse(cron(),1,' ').empty() ? vmax(0,1e9*s2r(cron())) : 0;

    //Start the request data task
    if(!prcSt) SYS->taskCreate(nodePath('.',true), mPrior, TMdContr::Task, this);
}

void TMdContr::stop_( )
{
    //Stop the request and calc data task
    if(prcSt) SYS->taskDestroy(nodePath('.',true), &endrunReq);
}

void TMdContr::prmEn( TMdPrm *p, bool val )
{
    MtxAlloc res(enRes, true);

    unsigned i_prm;
    for(i_prm = 0; i_prm < pHd.size(); i_prm++)
	if(&pHd[i_prm].at() == p) break;

    if(val && i_prm >= pHd.size())	pHd.push_back(p);
    if(!val && i_prm < pHd.size())	pHd.erase(pHd.begin()+i_prm);
}

void *TMdContr::Task( void *icntr )
{
    TMdContr &cntr = *(TMdContr*)icntr;

    cntr.endrunReq = false;
    cntr.prcSt = true;

    bool is_start = true;
    bool is_stop  = false;
    int64_t t_cnt = 0, t_prev = TSYS::curTime();

    while(true) {
	//Update controller's data
	if(!cntr.redntUse()) {
	    if(!cntr.period())	t_cnt = TSYS::curTime();
	    pthread_mutex_lock(&cntr.enRes);
	    for(unsigned i_p = 0; i_p < cntr.pHd.size(); i_p++)
		try { cntr.pHd[i_p].at().calc(is_start, is_stop, cntr.period()?(1e9/cntr.period()):(-1e-6*(t_cnt-t_prev))); }
		catch(TError err) { mess_err(err.cat.c_str(),"%s",err.mess.c_str()); }
	    pthread_mutex_unlock(&cntr.enRes);
	    t_prev = t_cnt;
	}

	if(is_stop) break;
	TSYS::taskSleep((int64_t)cntr.period(), (cntr.period()?0:TSYS::cron(cntr.cron())));
	if(cntr.endrunReq)	is_stop = true;
	if(!cntr.redntUse())	is_start = false;
    }

    cntr.prcSt = false;

    return NULL;
}

void TMdContr::redntDataUpdate( )
{
    TController::redntDataUpdate();

    vector<string> pls; list(pls);

    //Request for template's attributes values
    XMLNode req("CntrReqs"); req.setAttr("path",nodePath(0,true));
    for(unsigned i_p = 0; i_p < pls.size(); i_p++) {
	if(!at(pls[i_p]).at().enableStat()) continue;
	req.childAdd("get")->setAttr("path","/prm_"+pls[i_p]+"/%2fserv%2ftmplAttr");
    }

    //Send request to first active station for this controller
    if(owner().owner().rdStRequest(workId(),req).empty()) return;

    //Redirect respond to local parameters
    req.setAttr("path","/");
    for(unsigned i_prm = 0; i_prm < req.childSize(); ) {
	if(s2i(req.childGet(i_prm)->attr("err"))) {
	    req.childDel(i_prm);
	    continue;
	}
	req.childGet(i_prm)->setName("set");
	i_prm++;
    }
    cntrCmd(&req);
}

void TMdContr::cntrCmdProc( XMLNode *opt )
{
    //Get page info
    if(opt->name() == "info") {
	TController::cntrCmdProc(opt);
	ctrRemoveNode(opt,"/cntr/cfg/PERIOD");
	ctrMkNode("fld",opt,-1,"/cntr/cfg/SCHEDULE",cfg("SCHEDULE").fld().descr(),startStat()?R_R_R_:RWRWR_,"root",SDAQ_ID,4,
	    "tp","str","dest","sel_ed","sel_list",TMess::labSecCRONsel(),"help",TMess::labSecCRON());
	ctrMkNode("fld",opt,-1,"/cntr/cfg/PRIOR",cfg("PRIOR").fld().descr(),startStat()?R_R_R_:RWRWR_,"root",SDAQ_ID,1,"help",TMess::labTaskPrior());
	return;
    }
    TController::cntrCmdProc(opt);
}

//*************************************************
//* TMdPrm                                        *
//*************************************************
TMdPrm::TMdPrm( string name, TTipParam *tp_prm ) :
    TParamContr(name,tp_prm), prmRefl(NULL), pEl("w_attr"), chkLnkNeed(false),
    idFreq(-1), idStart(-1), idStop(-1), idSh(-1), idNm(-1), idDscr(-1)
{
    setType(type().name);
}

TMdPrm::~TMdPrm( )
{
    nodeDelAll();
}

TCntrNode &TMdPrm::operator=( TCntrNode &node )
{
    TParamContr::operator=( node );

    TMdPrm *src_n = dynamic_cast<TMdPrm*>(&node);
    if(!src_n || !src_n->enableStat() || !enableStat() || !isStd() || !tmpl->val.func()) return *this;

    //IO values copy
    for(int i_io = 0; i_io < src_n->tmpl->val.func()->ioSize(); i_io++)
	if(src_n->tmpl->val.func()->io(i_io)->flg()&TPrmTempl::CfgLink)
	    lnk(lnkId(i_io)).prmAttr = src_n->lnk(src_n->lnkId(i_io)).prmAttr;
	else tmpl->val.setS(i_io,src_n->tmpl->val.getS(i_io));

    // Init links
    initTmplLnks();

    return *this;
}

bool TMdPrm::isStd( )	{ return (type().name == "std"); }

bool TMdPrm::isPRefl( )	{ return (type().name == "pRefl"); }

void TMdPrm::postEnable( int flag )
{
    TParamContr::postEnable(flag);
    if(!vlElemPresent(&pEl)) vlElemAtt(&pEl);
}

void TMdPrm::postDisable( int flag )
{
    TParamContr::postDisable(flag);

    if(flag) {
	string io_bd = owner().DB()+"."+type().DB(&owner())+"_io";
	TConfig cfg(&mod->prmIOE());
	cfg.cfg("PRM_ID").setS(ownerPath(true), true);
	SYS->db().at().dataDel(io_bd, owner().owner().nodePath()+type().DB(&owner())+"_io", cfg);
    }
}

void TMdPrm::setType( const string &tpId )
{
    //Free old mode
    if(isPRefl() && prmRefl)	{ delete prmRefl; prmRefl = NULL; }
    else if(isStd() && tmpl)	{ delete tmpl; tmpl = NULL; }

    TParamContr::setType(tpId);

    //Init/update new mode
    if(isPRefl() && !prmRefl)	prmRefl = new AutoHD<TValue>;
    else if(isStd() && !tmpl)	tmpl = new STmpl;
}

TMdContr &TMdPrm::owner( )	{ return (TMdContr&)TParamContr::owner(); }

void TMdPrm::enable( )
{
    bool isProc = false, isFullEn = !enableStat();
    if(isFullEn) TParamContr::enable();

    vector<string> als;

    try {
	if(isPRefl()) {
	    vector<string> list;
	    if(prmRefl->freeStat()) *prmRefl = SYS->daq().at().prmAt(cfg("PSRC").getS(), '.', true);
	    if(!prmRefl->freeStat()) {
		prmRefl->at().vlList(list);
		for(unsigned i_l = 0; i_l < list.size(); i_l++) {
		    if(!vlPresent(list[i_l]))
			pEl.fldAdd(new TFld(list[i_l].c_str(),prmRefl->at().vlAt(list[i_l]).at().fld().descr().c_str(),
			    prmRefl->at().vlAt(list[i_l]).at().fld().type(),
			    TVal::DirWrite|TVal::DirRead|(prmRefl->at().vlAt(list[i_l]).at().fld().flg()&TFld::NoWrite)));
		    als.push_back(list[i_l]);
		}
	    }
	    isProc = true;
	}
	else if(isStd() && !tmpl->val.func()) {
	    bool to_make = false;
	    unsigned fId = 0;
	    string prm = cfg("PRM").getS();
	    if(!tmpl->val.func() && prm.size()) {
		tmpl->val.setFunc(&SYS->daq().at().tmplLibAt(TSYS::strSepParse(prm,0,'.')).at().
						   at(TSYS::strSepParse(prm,1,'.')).at().func().at());
		tmpl->val.setVfName(id()+"_tmplprm");
		to_make = true;
	    }
	    // Init attrubutes
	    if(tmpl->val.func()) {
		for(int i_io = 0; i_io < tmpl->val.func()->ioSize(); i_io++) {
		    if((tmpl->val.func()->io(i_io)->flg()&TPrmTempl::CfgLink) && lnkId(i_io) < 0)
			tmpl->lnk.push_back(SLnk(i_io));
		    if((tmpl->val.func()->io(i_io)->flg()&(TPrmTempl::AttrRead|TPrmTempl::AttrFull))) {
			unsigned flg = TVal::DirWrite|TVal::DirRead;
			if(tmpl->val.func()->io(i_io)->flg()&IO::FullText)		flg |= TFld::FullText;
			if(tmpl->val.func()->io(i_io)->flg()&TPrmTempl::AttrRead)	flg |= TFld::NoWrite;

			TFld::Type tp = TFld::type(tmpl->val.ioType(i_io));
			if((fId=pEl.fldId(tmpl->val.func()->io(i_io)->id(),true)) < pEl.fldSize()) {
			    if(pEl.fldAt(fId).type() != tp)
				try{ pEl.fldDel(fId); }
				catch(TError err){ mess_warning(err.cat.c_str(),err.mess.c_str()); }
			    else {
				pEl.fldAt(fId).setFlg(flg);
				pEl.fldAt(fId).setDescr(tmpl->val.func()->io(i_io)->name().c_str());
			    }
			}

			if(!vlPresent(tmpl->val.func()->io(i_io)->id()))
			    pEl.fldAdd(new TFld(tmpl->val.func()->io(i_io)->id().c_str(),tmpl->val.func()->io(i_io)->name().c_str(),tp,flg));

			als.push_back(tmpl->val.func()->io(i_io)->id());
		    }
		    if(to_make && (tmpl->val.func()->io(i_io)->flg()&TPrmTempl::CfgLink)) tmpl->val.setS(i_io,EVAL_STR);
		}
		// Init links
		initTmplLnks();

		//Load IO
		loadIO(true);

		//Init system attributes identifiers
		idFreq	= tmpl->val.ioId("f_frq");
		idStart	= tmpl->val.ioId("f_start");
		idStop	= tmpl->val.ioId("f_stop");
		idErr	= tmpl->val.ioId("f_err");
		idSh	= tmpl->val.ioId("SHIFR");
		idNm	= tmpl->val.ioId("NAME");
		idDscr	= tmpl->val.ioId("DESCR");
		int id_this = tmpl->val.ioId("this");
		if(id_this >= 0) tmpl->val.setO(id_this,new TCntrNodeObj(AutoHD<TCntrNode>(this),"root"));
	    }
	    isProc = true;
	}
    }
    catch(...){ disable(); throw; }

    //Check for delete DAQ parameter's attributes
    for(int i_p = 0; isProc && i_p < (int)pEl.fldSize(); i_p++) {
	unsigned i_l;
	for(i_l = 0; i_l < als.size(); i_l++)
	    if(pEl.fldAt(i_p).name() == als[i_l])
		break;
	if(i_l >= als.size())
	    try{ pEl.fldDel(i_p); i_p--; }
	    catch(TError err){ mess_warning(err.cat.c_str(),err.mess.c_str()); }
    }

    if(isFullEn && owner().startStat()) calc(true, false, 0);
    if(isFullEn) owner().prmEn(this, true);
}

void TMdPrm::disable( )
{
    if(!enableStat())  return;

    owner().prmEn(this, false);

    if(owner().startStat()) calc(false, true, 0);

    if(isPRefl() && prmRefl) prmRefl->free();
    else if(isStd() && tmpl) {
	tmpl->lnk.clear();
	tmpl->val.setFunc(NULL);
    }

    idFreq = idStart = idStop = idErr = -1;

    TParamContr::disable();
}

void TMdPrm::load_( )
{
    TParamContr::load_();

    if(enableStat()) loadIO();
}

void TMdPrm::loadIO( bool force )
{
    //Load IO and init links
    if(isStd() && tmpl->val.func()) {
	if(owner().startStat() && !force) { modif(true); return; }	//Load/reload IO context only allow for stoped controlers for prevent throws

	TConfig cfg(&mod->prmIOE());
	cfg.cfg("PRM_ID").setS(ownerPath(true));
	string io_bd = owner().DB()+"."+type().DB(&owner())+"_io";

	for(int i_io = 0; i_io < tmpl->val.ioSize(); i_io++) {
	    cfg.cfg("ID").setS(tmpl->val.func()->io(i_io)->id());
	    cfg.cfg("VALUE").setNoTransl(!(tmpl->val.func()->io(i_io)->type()==IO::String && !(tmpl->val.func()->io(i_io)->flg()&TPrmTempl::CfgLink)));
	    if(!SYS->db().at().dataGet(io_bd,owner().owner().nodePath()+type().DB(&owner())+"_io",cfg,false,true))
		continue;
	    if(tmpl->val.func()->io(i_io)->flg()&TPrmTempl::CfgLink)
		lnk(lnkId(i_io)).prmAttr = cfg.cfg("VALUE").getS();
	    else tmpl->val.setS(i_io,cfg.cfg("VALUE").getS());
	}
	initTmplLnks();
    }
}

void TMdPrm::save_( )
{
    TParamContr::save_();

    if(enableStat()) saveIO();
}

void TMdPrm::saveIO( )
{
    //Save IO and init links
    if(isStd() && tmpl->val.func()) {
	TConfig cfg(&mod->prmIOE());
	cfg.cfg("PRM_ID").setS(ownerPath(true));
	string io_bd = owner().DB()+"."+type().DB(&owner())+"_io";

	for(int i_io = 0; i_io < tmpl->val.func()->ioSize(); i_io++) {
	    cfg.cfg("ID").setS(tmpl->val.func()->io(i_io)->id());
	    cfg.cfg("VALUE").setNoTransl(!(tmpl->val.func()->io(i_io)->type()==IO::String && !(tmpl->val.func()->io(i_io)->flg()&TPrmTempl::CfgLink)));
	    if(tmpl->val.func()->io(i_io)->flg()&TPrmTempl::CfgLink)
		cfg.cfg("VALUE").setS(lnk(lnkId(i_io)).prmAttr);
	    else cfg.cfg("VALUE").setS(tmpl->val.getS(i_io));
	    SYS->db().at().dataSet(io_bd,owner().owner().nodePath()+type().DB(&owner())+"_io",cfg);
	}
    }
}

void TMdPrm::initTmplLnks( bool checkNoLink )
{
    if(!isStd() || !tmpl->val.func()) return;
    //Init links
    chkLnkNeed = false;
    string nmod, ncntr, nprm, nattr;

    for(int i_l = 0; i_l < lnkSize(); i_l++) {
	if(checkNoLink && !lnk(i_l).aprm.freeStat()) continue;
	try {
	    lnk(i_l).aprm.free();
	    lnk(i_l).detOff = 0;
	    lnk(i_l).aprm = SYS->daq().at().attrAt(TSYS::strParse(lnk(i_l).prmAttr,0,"#",&lnk(i_l).detOff), '.', true);
	    if(!lnk(i_l).aprm.freeStat()) {
		if(lnk(i_l).aprm.at().fld().type() == TFld::Object && lnk(i_l).detOff < (int)lnk(i_l).prmAttr.size())
		    tmpl->val.setS(lnk(i_l).ioId, lnk(i_l).aprm.at().getO().at().propGet(lnk(i_l).prmAttr.substr(lnk(i_l).detOff),'.'));
		else tmpl->val.setS(lnk(i_l).ioId, lnk(i_l).aprm.at().getS());
	    }
	    else chkLnkNeed = true;
	} catch(TError err){ chkLnkNeed = true; }
    }
}

void TMdPrm::vlGet( TVal &val )
{
    if(!enableStat() || !owner().startStat()) {
	if(val.name() == "err") {
	    if(!enableStat()) val.setS(_("1:Parameter is disabled."), 0, true);
	    else if(!owner().startStat()) val.setS(_("2:Controller is stopped."), 0, true);
	}
	else val.setS(EVAL_STR, 0, true);
	return;
    }

    if(owner().redntUse()) return;

    if(val.name() != "err") {
	try {
	    if(isPRefl() && !prmRefl->freeStat()) val.set(prmRefl->at().vlAt(val.name()).at().get(), 0, true);
	    else if(isStd() && tmpl->val.func()) {
		int id_lnk = lnkId(val.name());
		if(id_lnk >= 0 && lnk(id_lnk).aprm.freeStat()) id_lnk = -1;
		if(id_lnk < 0) val.set(tmpl->val.get(tmpl->val.ioId(val.name())), 0, true);
		else if(lnk(id_lnk).aprm.at().fld().type() == TFld::Object && lnk(id_lnk).detOff < (int)lnk(id_lnk).prmAttr.size())
		    val.set(lnk(id_lnk).aprm.at().getO().at().propGet(lnk(id_lnk).prmAttr.substr(lnk(id_lnk).detOff),'.'));
		else val.set(lnk(id_lnk).aprm.at().get(), 0, true);
	    }
	}
	catch(TError err) { }
    }
    else {
	if(isStd() && tmpl->val.func() && idErr >= 0) val.setS(tmpl->val.getS(idErr), 0, true);
	else val.setS("0", 0, true);
    }
}

void TMdPrm::vlSet( TVal &vo, const TVariant &vl, const TVariant &pvl )
{
    if(!enableStat() || !owner().startStat())	{ vo.setS(EVAL_STR, 0, true); return; }

    //Send to active reserve station
    if(owner().redntUse()) {
	if(vl == pvl) return;
	XMLNode req("set");
	req.setAttr("path",nodePath(0,true)+"/%2fserv%2fattr")->childAdd("el")->setAttr("id",vo.name())->setText(vl.getS());
	SYS->daq().at().rdStRequest(owner().workId(),req);
	return;
    }

    //Direct write
    try {
	if(isPRefl() && !prmRefl->freeStat()) prmRefl->at().vlAt(vo.name()).at().set(vl);
	else if(isStd() && tmpl->val.func()) {
	    int id_lnk = lnkId(vo.name());
	    if(id_lnk >= 0 && lnk(id_lnk).aprm.freeStat()) id_lnk = -1;
	    ResAlloc cres(calcRes, true);
	    if(id_lnk < 0) tmpl->val.set(tmpl->val.ioId(vo.name()), vl);
	    else if(lnk(id_lnk).aprm.at().fld().type() == TFld::Object && lnk(id_lnk).detOff < (int)lnk(id_lnk).prmAttr.size())
	    {
		lnk(id_lnk).aprm.at().getO().at().propSet(lnk(id_lnk).prmAttr.substr(lnk(id_lnk).detOff), '.', vl);
		lnk(id_lnk).aprm.at().setO(lnk(id_lnk).aprm.at().getO());	//For modify object sign
	    }
	    else lnk(id_lnk).aprm.at().set(vl);
	}
    }catch(TError err) { }
}

void TMdPrm::vlArchMake( TVal &val )
{
    TParamContr::vlArchMake(val);

    if(val.arch().freeStat()) return;
    val.arch().at().setSrcMode(TVArchive::ActiveAttr);
    val.arch().at().setPeriod(SYS->archive().at().valPeriod()*1000);
    val.arch().at().setHardGrid(true);
    val.arch().at().setHighResTm(true);
}

int TMdPrm::lnkSize( )
{
    if(!isStd() || !tmpl->val.func()) throw TError(nodePath().c_str(),_("Parameter is disabled or is not based on the template."));
    return tmpl->lnk.size();
}

int TMdPrm::lnkId( int id )
{
    if(!isStd() || !tmpl->val.func()) throw TError(nodePath().c_str(),_("Parameter is disabled or is not based on the template."));
    for(int i_l = 0; i_l < lnkSize(); i_l++)
	if(lnk(i_l).ioId == id)
	    return i_l;
    return -1;
}

int TMdPrm::lnkId( const string &id )
{
    if(!isStd() || !tmpl->val.func()) throw TError(nodePath().c_str(),_("Parameter is disabled or is not based on the template."));
    for(int i_l = 0; i_l < lnkSize(); i_l++)
	if(tmpl->val.func()->io(lnk(i_l).ioId)->id() == id)
	    return i_l;
    return -1;
}

TMdPrm::SLnk &TMdPrm::lnk( int num )
{
    if(!isStd() || !tmpl->val.func()) throw TError(nodePath().c_str(),_("Parameter is disabled or is not based on the template."));
    if(num < 0 || num >= (int)tmpl->lnk.size()) throw TError(nodePath().c_str(),_("Parameter id error."));
    return tmpl->lnk[num];
}

void TMdPrm::calc( bool first, bool last, double frq )
{
    if(isPRefl() && (!first || prmRefl->freeStat())) enable();

    if(!isStd() || !tmpl->val.func()) return;
    try {
	ResAlloc cres(calcRes, true);
	if(chkLnkNeed) initTmplLnks(true);

	tmpl->val.setMdfChk(true);

	//Set fixed system attributes
	if(idFreq >= 0)	tmpl->val.setR(idFreq, frq);
	if(idStart >= 0)tmpl->val.setB(idStart, first);
	if(idStop >= 0)	tmpl->val.setB(idStop, last);
	if(idSh >= 0)	tmpl->val.setS(idSh, id());
	if(idNm >= 0)	tmpl->val.setS(idNm, name());
	if(idDscr >= 0)	tmpl->val.setS(idDscr, descr());

	//Get input links
	for(int i_l = 0; i_l < lnkSize(); i_l++)
	    if(lnk(i_l).aprm.freeStat())tmpl->val.setS(lnk(i_l).ioId, EVAL_STR);
	    else if(lnk(i_l).aprm.at().fld().type() == TFld::Object && lnk(i_l).detOff < (int)lnk(i_l).prmAttr.size())
		tmpl->val.set(lnk(i_l).ioId, lnk(i_l).aprm.at().getO().at().propGet(lnk(i_l).prmAttr.substr(lnk(i_l).detOff),'.'));
	    else tmpl->val.set(lnk(i_l).ioId, lnk(i_l).aprm.at().get());

	//Calc template
	tmpl->val.calc();
	modif();

	//Put output links
	for(int i_l = 0; i_l < lnkSize(); i_l++)
	    if(!lnk(i_l).aprm.freeStat() && tmpl->val.ioMdf(lnk(i_l).ioId) &&
		    tmpl->val.ioFlg(lnk(i_l).ioId)&(IO::Output|IO::Return) &&
		    !(lnk(i_l).aprm.at().fld().flg()&TFld::NoWrite))
	    {
		TVariant vl = tmpl->val.get(lnk(i_l).ioId);
		if(!vl.isEVal()) {
		    if(lnk(i_l).aprm.at().fld().type() == TFld::Object && lnk(i_l).detOff < (int)lnk(i_l).prmAttr.size())
		    {
			lnk(i_l).aprm.at().getO().at().propSet(lnk(i_l).prmAttr.substr(lnk(i_l).detOff), '.', vl);
			lnk(i_l).aprm.at().setO(lnk(i_l).aprm.at().getO());	//For modify object sign
		    }
		    else lnk(i_l).aprm.at().set(vl);
		}
	    }

	//Put fixed system attributes
	if(idNm >= 0)	setName(tmpl->val.getS(idNm));
	if(idDscr >= 0)	setDescr(tmpl->val.getS(idDscr));
    }
    catch(TError err) {
	mess_warning(err.cat.c_str(),"%s",err.mess.c_str());
	mess_warning(nodePath().c_str(),_("Error calculate template."));
    }
}

void TMdPrm::cntrCmdProc( XMLNode *opt )
{
    //Service commands process
    string a_path = opt->attr("path");
    if(a_path.substr(0,6) == "/serv/") {
	if(a_path == "/serv/tmplAttr") {
	    if(!isStd() || !tmpl->val.func()) throw TError(nodePath().c_str(),_("No template parameter or error."));
	    if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD))
		for(int i_a = 0; i_a < tmpl->val.ioSize(); i_a++)
		    opt->childAdd("ta")->setAttr("id",tmpl->val.func()->io(i_a)->id())->setText(tmpl->val.getS(i_a));
	    if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR))
		for(unsigned i_a = 0; i_a < opt->childSize(); i_a++) {
		    int ioId = -1;
		    if(opt->childGet(i_a)->name() != "ta" || (ioId=tmpl->val.ioId(opt->childGet(i_a)->attr("id"))) < 0) continue;
		    tmpl->val.setS(ioId,opt->childGet(i_a)->text());
		}
	}
	else TParamContr::cntrCmdProc(opt);
	return;
    }

    vector<string> list;
    //Get page info
    if(opt->name() == "info") {
	TParamContr::cntrCmdProc(opt);
	if(isPRefl()) ctrMkNode("fld",opt,-1,"/prm/cfg/PSRC",cfg("PSRC").fld().descr(),RWRW__,"root",SDAQ_ID,3,"tp","str","dest","sel_ed","select","/prm/cfg/prmp_lst");
	else if(isStd()) ctrMkNode("fld",opt,-1,"/prm/cfg/PRM",cfg("PRM").fld().descr(),RWRW__,"root",SDAQ_ID,3,"tp","str","dest","select","select","/prm/tmplList");
	if(isStd() && tmpl->val.func() && ctrMkNode("area",opt,-1,"/cfg",_("Template configuration")))
	{
	    ctrMkNode("fld",opt,-1,"/cfg/attr_only",_("Only attributes are to be shown"),RWRWR_,"root",SDAQ_ID,1,"tp","bool");
	    if(ctrMkNode("area",opt,-1,"/cfg/prm",_("Parameters")))
		for(int i_io = 0; i_io < tmpl->val.ioSize(); i_io++) {
		    if(!(tmpl->val.func()->io(i_io)->flg()&(TPrmTempl::CfgLink|TPrmTempl::CfgPublConst)))
			continue;
		    // Check select param
		    bool is_lnk = tmpl->val.func()->io(i_io)->flg()&TPrmTempl::CfgLink;
		    if(is_lnk && tmpl->val.func()->io(i_io)->def().size() &&
			!s2i(TBDS::genDBGet(mod->nodePath()+"onlAttr","0",opt->attr("user"))))
		    {
			string nprm = TSYS::strSepParse(tmpl->val.func()->io(i_io)->def(),0,'|');
			// Check already to present parameters
			bool f_ok = false;
			for(unsigned i_l = 0; i_l < list.size() && !f_ok; i_l++)
			    if(list[i_l] == nprm) f_ok = true;
			if(!f_ok) {
			    ctrMkNode("fld",opt,-1,(string("/cfg/prm/pr_")+i2s(i_io)).c_str(),nprm,RWRWR_,"root",SDAQ_ID,
				    3,"tp","str","dest","sel_ed","select",(string("/cfg/prm/pl_")+i2s(i_io)).c_str());
			    list.push_back(nprm);
			}
		    }
		    else {
			const char *tip = "str";
			bool fullTxt = false;
			if(!is_lnk)
			    switch(tmpl->val.ioType(i_io)) {
				case IO::Integer:	tip = "dec";	break;
				case IO::Real:		tip = "real";	break;
				case IO::Boolean:	tip = "bool";	break;
				case IO::String:
				    if(tmpl->val.func()->io(i_io)->flg()&IO::FullText) fullTxt = true;
				    break;
				case IO::Object:	fullTxt = true;	break;
			    }
			XMLNode *wn = ctrMkNode("fld",opt,-1,(string("/cfg/prm/el_")+i2s(i_io)).c_str(),
				tmpl->val.func()->io(i_io)->name(),RWRWR_,"root",SDAQ_ID,1,"tp",tip);
			if(wn && is_lnk) wn->setAttr("dest","sel_ed")->setAttr("select","/cfg/prm/ls_"+i2s(i_io));
			if(wn && fullTxt)wn->setAttr("cols","100")->setAttr("rows","4");
		    }
		}
	}
	return;
    }

    //Process command to page
    if(isPRefl() && a_path == "/prm/cfg/PSRC") {
	if(ctrChkNode(opt,"get",RWRW__,"root",SDAQ_ID,SEC_RD)) {
	    string prmVal = cfg("PSRC").getS();
	    if(!SYS->daq().at().prmAt(TSYS::strParse(prmVal,0,"#"),'.',true).freeStat()) prmVal += " (+)";
	    opt->setText(prmVal);
	}
	if(ctrChkNode(opt,"set",RWRW__,"root",SDAQ_ID,SEC_WR))
	    try {
		if(enableStat()) disable();
		string prmValm = TSYS::strParse(opt->text(), 0, " ");
		//Link to self exclude
		if(prmValm == owner().owner().modId()+"."+owner().id()+"."+id())
		    prmValm = owner().owner().modId()+"."+owner().id();
		cfg("PSRC").setS(prmValm);
	    } catch(...){ disable(); throw; }
    }
    else if(isStd() && a_path == "/prm/cfg/PRM") {
	if(ctrChkNode(opt,"get",RWRW__,"root",SDAQ_ID,SEC_RD))	opt->setText(cfg("PRM").getS());
	if(ctrChkNode(opt,"set",RWRW__,"root",SDAQ_ID,SEC_WR))
	    try {
		if(enableStat()) disable();
		cfg("PRM").setS(opt->text());
	    } catch(...){ disable(); throw; }
    }
    else if(a_path == "/prm/cfg/prmp_lst" && ctrChkNode(opt)) SYS->daq().at().ctrListPrmAttr(opt, cfg("PSRC").getS(), true, '.');
    else if(isStd() && a_path == "/cfg/attr_only") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD))	opt->setText(TBDS::genDBGet(mod->nodePath()+"onlAttr","0",opt->attr("user")));
	if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR))	TBDS::genDBSet(mod->nodePath()+"onlAttr",opt->text(),opt->attr("user"));
    }
    else if(isStd() && tmpl->val.func() && a_path.substr(0,12) == "/cfg/prm/pr_") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD)) {
	    string lnk_val = lnk(lnkId(s2i(a_path.substr(12)))).prmAttr;
	    if(!SYS->daq().at().attrAt(TSYS::strParse(lnk_val,0,"#"),'.',true).freeStat()) {
		opt->setText(lnk_val.substr(0,lnk_val.rfind(".")));
		opt->setText(opt->text()+" (+)");
	    }
	    else opt->setText(lnk_val);
	}
	if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR)) {
	    string no_set;
	    string p_nm = TSYS::strSepParse(tmpl->val.func()->io(lnk(lnkId(s2i(a_path.substr(12)))).ioId)->def(),0,'|');
	    string p_vl = TSYS::strParse(opt->text(), 0, " ");
	    if(p_vl == DAQPath()) throw TError(nodePath().c_str(),_("Self to self linking error."));
	    AutoHD<TValue> prm = SYS->daq().at().prmAt(p_vl, '.', true);

	    for(int i_l = 0; i_l < lnkSize(); i_l++)
		if(p_nm == TSYS::strSepParse(tmpl->val.func()->io(lnk(i_l).ioId)->def(),0,'|')) {
		    lnk(i_l).prmAttr = p_vl;
		    string p_attr = TSYS::strSepParse(tmpl->val.func()->io(lnk(i_l).ioId)->def(),1,'|');
		    if(!prm.freeStat()) {
			if(prm.at().vlPresent(p_attr)) {
			    lnk(i_l).prmAttr= p_vl+"."+p_attr;
			    modif();
			}
			else no_set += p_attr+",";
		    }
		}
	    initTmplLnks();
	}
    }
    else if(isStd() && tmpl->val.func() && (a_path.compare(0,12,"/cfg/prm/pl_") == 0 || a_path.compare(0,12,"/cfg/prm/ls_") == 0) && ctrChkNode(opt))
    {
	bool is_pl = (a_path.compare(0,12,"/cfg/prm/pl_") == 0);
	string m_prm = lnk(lnkId(s2i(a_path.substr(12)))).prmAttr;
	if(is_pl && !SYS->daq().at().attrAt(m_prm,'.',true).freeStat()) m_prm = m_prm.substr(0,m_prm.rfind("."));
	SYS->daq().at().ctrListPrmAttr(opt, m_prm, is_pl, '.');
    }
    else if(isStd() && tmpl->val.func() && a_path.substr(0,12) == "/cfg/prm/el_") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD)) {
	    int i_io = s2i(a_path.substr(12));
	    if(tmpl->val.func()->io(i_io)->flg()&TPrmTempl::CfgLink) {
		opt->setText(lnk(lnkId(i_io)).prmAttr);
		if(!SYS->daq().at().attrAt(TSYS::strParse(opt->text(),0,"#"),'.',true).freeStat()) opt->setText(opt->text()+" (+)");
	    }
	    else if(tmpl->val.func()->io(i_io)->flg()&TPrmTempl::CfgPublConst)
		opt->setText(tmpl->val.getS(i_io));
	}
	if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR)) {
	    int i_io = s2i(a_path.substr(12));
	    if(tmpl->val.func()->io(i_io)->flg()&TPrmTempl::CfgLink) {
		string a_vl = TSYS::strParse(opt->text(), 0, " ");
		if(TSYS::strSepParse(a_vl,0,'.') == owner().owner().modId() &&
			TSYS::strSepParse(a_vl,1,'.') == owner().id() &&
			TSYS::strSepParse(a_vl,2,'.') == id())
		    throw TError(nodePath().c_str(),_("Self to self linking error."));
		lnk(lnkId(i_io)).prmAttr = a_vl;
		initTmplLnks();
	    }
	    else if(tmpl->val.func()->io(i_io)->flg()&TPrmTempl::CfgPublConst) tmpl->val.setS(i_io,opt->text());
	    modif();
	}
    }
    else TParamContr::cntrCmdProc(opt);
}
