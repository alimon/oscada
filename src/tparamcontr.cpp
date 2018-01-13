
//OpenSCADA system file: tparamcontr.cpp
/***************************************************************************
 *   Copyright (C) 2003-2018 by Roman Savochenko, <rom_as@oscada.org>      *
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

#include "tbds.h"
#include "tsys.h"
#include "tmess.h"
#include "tdaqs.h"
#include "tcontroller.h"
#include "ttypedaq.h"
#include "ttypeparam.h"
#include "tparamcontr.h"

using namespace OSCADA;

//*************************************************
//* TParamContr                                   *
//*************************************************
TParamContr::TParamContr( const string &name, TTypeParam *tpprm ) : TConfig(tpprm), mRdPrcTm(0), mPrm(-1), mEn(false), tpParm(tpprm)
{
    cfg("SHIFR") = mId = name;	//!! For prevent ID location change on the parameter type change

    setName(name);
    if(type().isPrmCntr) mPrm = grpAdd("prm_");
    type().create(this);

    if(mess_lev() == TMess::Debug) SYS->cntrIter(objName(), 1);
}

TParamContr::~TParamContr( )
{
    type().destroy(this);
    nodeDelAll();

    if(mess_lev() == TMess::Debug) SYS->cntrIter(objName(), -1);
}

string TParamContr::objName( )	{ return TValue::objName() + ":TParamContr"; }

string TParamContr::DAQPath( )
{
    string oPath = ownerPath();
    return owner().DAQPath()+(oPath.size()?".":"") + oPath + "." + id();
}

string TParamContr::ownerPath( bool inclSelf )
{
    string rez;

    if(inclSelf) rez = nodeName();
    for(TCntrNode *own = nodePrev(); dynamic_cast<TParamContr*>(own); own = own->nodePrev())
	rez = rez.empty() ? string(own->nodeName()) : (string(own->nodeName())+"."+rez);

    return rez;
}

TCntrNode &TParamContr::operator=( const TCntrNode &node )
{
    const TParamContr *src_n = dynamic_cast<const TParamContr*>(&node);
    if(!src_n) return *this;

    //Check for parameter type and change it if different and alow
    if(type().name != src_n->type().name && owner().owner().tpPrmToId(src_n->type().name) >= 0) {
	if(enableStat()) disable();
	setType(src_n->type().name);
    }

    //Configuration copy
    exclCopy(*src_n, "SHIFR;");

    //Enable new parameter
    if(src_n->enableStat() && toEnable() && !enableStat()) {
	enable();

	//Archives creation and copy
	vector<string> a_ls;
	vlList(a_ls);
	for(unsigned i_a = 0; i_a < a_ls.size(); i_a++) {
	    if(!src_n->vlPresent(a_ls[i_a]) || src_n->vlAt(a_ls[i_a]).at().arch().freeStat()) continue;

	    vlAt(a_ls[i_a]).at().setArch();
	    (TCntrNode&)vlAt(a_ls[i_a]).at().arch().at() = (TCntrNode&)src_n->vlAt(a_ls[i_a]).at().arch().at();
	}

	//Parameters copy
	if(mPrm >= 0) {
	    vector<string> prm_ls;
	    src_n->list(prm_ls);
	    for(unsigned i_p = 0; i_p < prm_ls.size(); i_p++) {
		if(!owner().owner().tpPrmPresent(src_n->at(prm_ls[i_p]).at().type().name)) continue;
		if(!present(prm_ls[i_p])) add(prm_ls[i_p], owner().owner().tpPrmToId(src_n->at(prm_ls[i_p]).at().type().name));
		(TCntrNode&)at(prm_ls[i_p]).at() = (TCntrNode&)src_n->at(prm_ls[i_p]).at();
		//if(toEnable() && !enableStat()) enable();
	    }
	}
    }

    return *this;
}

TController &TParamContr::owner( ) const
{
    TCntrNode *own = nodePrev();
    while(!dynamic_cast<TController*>(own)) own = own->nodePrev();

    return *(TController*)own;
}

string TParamContr::name( )			{ string nm = cfg("NAME").getS(); return nm.size() ? nm : id(); }

void TParamContr::setName( const string &inm )	{ cfg("NAME").setS(inm); }

string TParamContr::descr( )			{ return cfg("DESCR").getS(); }

bool TParamContr::dataActive( )			{ return owner().startStat(); }

void TParamContr::setDescr( const string &idsc ){ cfg("DESCR").setS(idsc); }

void TParamContr::list( vector<string> &list ) const
{
    if(mPrm < 0) return;
    chldList(mPrm, list);
}

bool TParamContr::present( const string &name ) const
{
    if(mPrm < 0) return false;
    return chldPresent(mPrm, name);
}

void TParamContr::add( const string &name, unsigned type )
{
    if(mPrm < 0) return;
    chldAdd(mPrm, owner().ParamAttach(name,type));
}

void TParamContr::del( const string &name, int full )
{
    if(mPrm < 0) return;
    chldDel(mPrm, name, -1, full);
}

AutoHD<TParamContr> TParamContr::at( const string &name, const string &who ) const
{
    if(mPrm < 0) return AutoHD<TParamContr>();
    return chldAt(mPrm, name);
}

void TParamContr::LoadParmCfg( )
{
    if(mPrm < 0) return;

    map<string, bool>	itReg;
    vector<vector<string> > full;

    //Search and create new parameters
    for(unsigned iTp = 0; iTp < owner().owner().tpPrmSize(); iTp++) {
	if(owner().owner().tpPrmAt(iTp).DB(&owner()).empty()) continue;
	try {
	    TConfig cEl(&owner().owner().tpPrmAt(iTp));
	    //cEl.cfgViewAll(false);
	    cEl.cfg("OWNER").setS(ownerPath(true), TCfg::ForceUse);

	    // Search new into DB and Config-file
	    for(int fld_cnt = 0; SYS->db().at().dataSeek(owner().DB()+"."+owner().owner().tpPrmAt(iTp).DB(&owner()),
		    owner().owner().nodePath()+owner().owner().tpPrmAt(iTp).DB(&owner()),fld_cnt++,cEl,false,&full); )
	    {
		try {
		    string shfr = cEl.cfg("SHIFR").getS();
		    if(!present(shfr))	add(shfr, iTp);
		    at(shfr).at().load(&cEl);
		    itReg[shfr] = true;
		} catch(TError &err) {
		    mess_err(err.cat.c_str(), "%s", err.mess.c_str());
		    mess_sys(TMess::Error, _("Add parameter '%s' error."), cEl.cfg("SHIFR").getS().c_str());
		}
	    }
	} catch(TError &err) {
	    mess_err(err.cat.c_str(), "%s", err.mess.c_str());
	    mess_sys(TMess::Error, _("Search and create new parameters error."));
	}
    }

    //Check for remove items removed from the DB
    if(!SYS->selDB().empty()) {
	vector<string> it_ls;
	list(it_ls);
	for(unsigned i_it = 0; i_it < it_ls.size(); i_it++)
	    if(itReg.find(it_ls[i_it]) == itReg.end())
		del(it_ls[i_it]);
    }

    //Force load present parameters
    vector<string> prm_ls;
    list(prm_ls);
    for(unsigned i_p = 0; i_p < prm_ls.size(); i_p++) {
	at(prm_ls[i_p]).at().modifG();
	at(prm_ls[i_p]).at().load();
    }
}

void TParamContr::postEnable( int flag )
{
    TValue::postEnable(flag);

    if(!vlCfg()) setVlCfg(this);
    if(!vlElemPresent(&SYS->daq().at().elErr()))
	vlElemAtt(&SYS->daq().at().elErr());
}

void TParamContr::preDisable( int flag )
{
    //Delete or stop the archives
    vector<string> a_ls;
    vlList(a_ls);
    for(unsigned i_a = 0; i_a < a_ls.size(); i_a++)
	if(!vlAt(a_ls[i_a]).at().arch().freeStat()) {
	    string arh_id = vlAt(a_ls[i_a]).at().arch().at().id();
	    if((flag>>8) == RM_Full) SYS->archive().at().valDel(arh_id, true);
	    else SYS->archive().at().valAt(arh_id).at().stop();
	}

    if(enableStat())	disable();
}

void TParamContr::postDisable( int flag )
{
    if(flag) {
	//Delete the parameter from DB
	cfg("OWNER") = ownerPath();
	SYS->db().at().dataDel(owner().DB()+"."+type().DB(&owner()), owner().owner().nodePath()+type().DB(&owner()), *this, true);
    }
}

void TParamContr::load_( TConfig *icfg )
{
    if(!SYS->chkSelDB(owner().DB())) throw TError();

    if(icfg) *(TConfig*)this = *icfg;
    else {
	//cfgViewAll(true);
	cfg("OWNER") = ownerPath();
	SYS->db().at().dataGet(owner().DB()+"."+type().DB(&owner()), owner().owner().nodePath()+type().DB(&owner()), *this);
    }

    LoadParmCfg();
}

void TParamContr::save_( )
{
    cfg("OWNER") = ownerPath();
    SYS->db().at().dataSet(owner().DB()+"."+type().DB(&owner()), owner().owner().nodePath()+type().DB(&owner()), *this);

    //Save archives
    vector<string> a_ls;
    vlList(a_ls);
    for(unsigned i_a = 0; i_a < a_ls.size(); i_a++)
	if(!vlAt(a_ls[i_a]).at().arch().freeStat())
	    vlAt(a_ls[i_a]).at().arch().at().save();
}

bool TParamContr::cfgChange( TCfg &co, const TVariant &pc )
{
    modif();
    return type().cfgChange(this, co);
}

void TParamContr::enable( )
{
    type().enable(this);

    bool enErr = false;
    //Enable the parameters
    vector<string> prm_list;
    list(prm_list);
    for(unsigned i_prm = 0; i_prm < prm_list.size(); i_prm++)
	if(at(prm_list[i_prm]).at().toEnable())
	    try{ at(prm_list[i_prm]).at().enable(); }
	    catch(TError &err) {
		mess_warning(err.cat.c_str(), "%s", err.mess.c_str());
		mess_sys(TMess::Warning, _("Enable parameter '%s' error."), prm_list[i_prm].c_str());
		enErr = true;
	    }

    mEn = true;

    if(enErr) throw err_sys(_("Some parameters enable error."));
}

void TParamContr::disable( )
{
    //Disable parameters
    vector<string> prm_list;
    list(prm_list);
    for(unsigned i_prm = 0; i_prm < prm_list.size(); i_prm++)
	if(at(prm_list[i_prm]).at().enableStat())
	    try{ at(prm_list[i_prm]).at().disable(); }
	    catch(TError &err) {
		mess_warning(err.cat.c_str(), "%s", err.mess.c_str());
		mess_sys(TMess::Warning, _("Disable parameter '%s' error."), prm_list[i_prm].c_str());
	    }

    type().disable(this);
    mEn = false;
}

void TParamContr::vlGet( TVal &val )
{
    if(val.name() == "err") {
	if(!enableStat()) val.setS(_("1:Parameter is disabled."), 0, true);
	else if(!owner().startStat()) val.setS(_("2:Controller is stopped."), 0, true);
	else val.setS("0", 0, true);
    }

    type().vlGet(this, val);
}

void TParamContr::vlSet( TVal &vo, const TVariant &vl, const TVariant &pvl )
{
    type().vlSet(this, vo, vl, pvl);
}

void TParamContr::vlArchMake( TVal &val )
{
    if(!val.arch().freeStat())	val.arch().at().setDB(owner().DB());
    type().vlArchMake(this, val);
}

void TParamContr::setType( const string &tpId )
{
    if(enableStat() || tpId == type().name || !owner().owner().tpPrmPresent(tpId))	return;

    //Remove all included parameters
    if(mPrm >= 0 && !owner().owner().tpPrmAt(owner().owner().tpPrmToId(tpId)).isPrmCntr) {
	vector<string> pls;
	list(pls);
	for(unsigned i_p = 0; i_p < pls.size(); i_p++) del(pls[i_p], true);
	grpDel(mPrm); mPrm = -1;
    }

    type().destroy(this);
    setNodeMode(TCntrNode::Disabled);

    try {
	//Wait for disconnect other
	while(nodeUse(true) > 1) TSYS::sysSleep(1e-3);
	//Remove from DB
	postDisable(true);

	//Create temporary structure
	TConfig tCfg(&type());
	tCfg = *(TConfig*)this;

	//Set new config structure
	tpParm = &owner().owner().tpPrmAt(owner().owner().tpPrmToId(tpId));
	setElem(tpParm);

	//Restore configurations
	*(TConfig*)this = tCfg;
    } catch(...) { }

    if(mPrm < 0 && tpParm->isPrmCntr) mPrm = grpAdd("prm_");

    setNodeMode(TCntrNode::Enabled);

    setVlCfg(this);

    type().create(this);

    modif();
}

TVariant TParamContr::objFuncCall( const string &iid, vector<TVariant> &prms, const string &user )
{
    //TCntrNodeObj cntr() - get the controller node
    if(iid == "cntr")	return new TCntrNodeObj(AutoHD<TCntrNode>(&owner()), user);

    //Configuration functions call
    TVariant cfRez = objFunc(iid, prms, user);
    if(!cfRez.isNull()) return cfRez;

    return TValue::objFuncCall(iid, prms, user);
}

void TParamContr::cntrCmdProc( XMLNode *opt )
{
    string a_path = opt->attr("path");

    //Service commands process
    if(a_path.substr(0,6) == "/serv/")	{ TValue::cntrCmdProc(opt); return; }

    //Get page info
    if(opt->name() == "info") {
	TValue::cntrCmdProc(opt);
	ctrMkNode("oscada_cntr",opt,-1,"/",_("Parameter: ")+name(),RWRWR_,"root",SDAQ_ID);
	ctrMkNode("branches",opt,-1,"/br","",R_R_R_);
	if(ctrMkNode("area",opt,0,"/prm",_("Parameter"))) {
	    if(ctrMkNode("area",opt,-1,"/prm/st",_("State"))) {
		if(!enableStat() && owner().owner().tpPrmSize() > 1)
		    ctrMkNode("fld",opt,-1,"/prm/st/type",_("Type"),RWRWR_,"root",SDAQ_ID,4,"tp","str","dest","select","select","/prm/tpLst",
			"help",_("The type changing leads to lose some data of the specific configurations."));
		else ctrMkNode("fld",opt,-1,"/prm/st/type",_("Type"),R_R_R_,"root",SDAQ_ID,1,"tp","str");
		if(owner().enableStat())
		    ctrMkNode("fld",opt,-1,"/prm/st/en",_("Enabled"),RWRWR_,"root",SDAQ_ID,1,"tp","bool");
	    }
	    if(ctrMkNode("area",opt,-1,"/prm/cfg",_("Configuration"))) {
		TConfig::cntrCmdMake(opt,"/prm/cfg",0,"root",SDAQ_ID,RWRWR_);
		ctrRemoveNode(opt,"/prm/cfg/OWNER");
	    }
	}

	type().cntrCmdProc(this, opt);

	if(mPrm >= 0) {
	    ctrMkNode("grp",opt,-1,"/br/prm_",_("Parameter"),RWRWR_,"root",SDAQ_ID,2,"idm",OBJ_NM_SZ,"idSz",OBJ_ID_SZ);
	    if(ctrMkNode("area",opt,-1,"/iPrms",_("Inclusion"))) {
		ctrMkNode("fld",opt,-1,"/iPrms/nmb",_("Number"),R_R_R_,"root",SDAQ_ID,1,"tp","str");
		ctrMkNode("list",opt,-1,"/iPrms/prm",_("Parameters"),RWRWR_,"root",SDAQ_ID,5,"tp","br","idm",OBJ_NM_SZ,"s_com","add,del","br_pref","prm_","idSz",OBJ_ID_SZ);
	    }
	}

	return;
    }
    //Process command to page
    if(a_path == "/prm/st/type") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD))	opt->setText(type().name);
	if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR))	setType(opt->text());
    }
    else if(a_path == "/prm/st/en") {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD))	opt->setText(enableStat()?"1":"0");
	if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR)) {
	    if(!owner().enableStat())	throw err_sys(_("Controller is not started!"));
	    else s2i(opt->text()) ? enable() : disable();
	}
    }
    else if(mPrm >= 0 && a_path == "/iPrms/nmb" && ctrChkNode(opt)) {
	vector<string> c_list;
	list(c_list);
	unsigned e_c = 0;
	for(unsigned i_a = 0; i_a < c_list.size(); i_a++)
	    if(at(c_list[i_a]).at().enableStat()) e_c++;
	opt->setText(TSYS::strMess(_("All: %d; Enabled: %d"),c_list.size(),e_c));
    }
    else if((a_path == "/br/prm_" || a_path == "/iPrms/prm")) {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD)) {
	    vector<string> c_list;
	    list(c_list);
	    for(unsigned i_a = 0; i_a < c_list.size(); i_a++) {
	        XMLNode *cN = opt->childAdd("el")->setAttr("id",c_list[i_a])->setText(at(c_list[i_a]).at().name());
		if(!s2i(opt->attr("recurs"))) continue;
		cN->setName(opt->name())->setAttr("path",TSYS::strEncode(opt->attr("path"),TSYS::PathEl))->setAttr("recurs","1");
		at(c_list[i_a]).at().cntrCmd(cN);
		cN->setName("el")->setAttr("path","")->setAttr("rez","")->setAttr("recurs","")->setText("");
	    }
	}
	if(ctrChkNode(opt,"add",RWRWR_,"root",SDAQ_ID,SEC_WR)) {
	    string vid = TSYS::strEncode(opt->attr("id"),TSYS::oscdID);
	    add(vid);
	    at(vid).at().setName(opt->text());
	}
	if(ctrChkNode(opt,"del",RWRWR_,"root",SDAQ_ID,SEC_WR))	del(opt->attr("id"),true);
    }
    else if(type().cntrCmdProc(this, opt)) /* Process OK */;
    else if(a_path.compare(0,8,"/prm/cfg") == 0) TConfig::cntrCmdProc(opt,TSYS::pathLev(a_path,2),"root",SDAQ_ID,RWRWR_);
    else if(a_path == "/prm/tmplList" && ctrChkNode(opt)) {
	opt->childAdd("el")->setText("");
	vector<string> lls, ls;
	SYS->daq().at().tmplLibList(lls);
	for(unsigned i_l = 0; i_l < lls.size(); i_l++) {
	    SYS->daq().at().tmplLibAt(lls[i_l]).at().list(ls);
	    for(unsigned i_t = 0; i_t < ls.size(); i_t++)
		opt->childAdd("el")->setText(lls[i_l]+"."+ls[i_t]);
	}
    }
    else if(a_path == "/prm/tpLst" && ctrChkNode(opt))
	for(unsigned iTp = 0; iTp < owner().owner().tpPrmSize(); iTp++)
	    opt->childAdd("el")->setAttr("id",owner().owner().tpPrmAt(iTp).name)->setText(owner().owner().tpPrmAt(iTp).descr);
    else TValue::cntrCmdProc(opt);
}
