
//OpenSCADA system file: tconfig.cpp
/***************************************************************************
 *   Copyright (C) 2003-2014 by Roman Savochenko, <rom_as@oscada.org>      *
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

using namespace OSCADA;

//*************************************************
//* TConfig                                       *
//*************************************************
TConfig::TConfig( TElem *Elements ) : mRes(true), mElem(NULL), mIncmplTblStrct(false), mReqKeys(false)
{
    setElem(Elements, true);
}

TConfig::~TConfig( )
{
    //Deinit value
    TCfgMap::iterator p;
    while((p=value.begin()) != value.end()) {
	delete p->second;
	value.erase(p);
    }

    mElem->valDet(this);
    if(single) delete mElem;
}

TConfig &TConfig::operator=(TConfig &config)	{ return exclCopy(config); }

TConfig &TConfig::exclCopy( TConfig &config, const string &passCpLs )
{
    vector<string> list_el;

    cfgList(list_el);
    for(unsigned i_el = 0; i_el < list_el.size(); i_el++) {
	if(!config.cfgPresent(list_el[i_el]) || passCpLs.find(list_el[i_el]+";") != string::npos) continue;
	TCfg &s_cfg = config.cfg(list_el[i_el]);
	TCfg &d_cfg = cfg(list_el[i_el]);
	switch(d_cfg.fld().type()) {
	    case TFld::String:
		d_cfg.setExtVal(s_cfg.extVal());
		if(s_cfg.extVal()) d_cfg.TVariant::operator=(s_cfg);
		d_cfg.setS(s_cfg.getS());
		break;
	    case TFld::Real:	d_cfg.setR(s_cfg.getR());break;
	    case TFld::Integer:	d_cfg.setI(s_cfg.getI());break;
	    case TFld::Boolean:	d_cfg.setB(s_cfg.getB());break;
	    default: break;
	}
    }

    return *this;
}

void TConfig::detElem( TElem *el )
{
    if(el == mElem)	setElem(NULL);
}

void TConfig::addFld( TElem *el, unsigned id )
{
    value.insert(std::pair<string,TCfg*>(mElem->fldAt(id).name(),new TCfg(mElem->fldAt(id),*this)));
}

void TConfig::delFld( TElem *el, unsigned id )
{
    TCfgMap::iterator p = value.find(mElem->fldAt(id).name());
    if(p == value.end()) return;
    delete p->second;
    value.erase(p);
}

void TConfig::reqKeysUpdate( )
{
    vector<string> ls;
    cfgList(ls);
    mReqKeys = false;
    for(unsigned i_c = 0; i_c < ls.size() && !mReqKeys; i_c++) mReqKeys = cfg(ls[i_c]).reqKey();
}

TCfg &TConfig::cfg( const string &n_val )
{
    TCfgMap::iterator p = value.find(n_val);
    if(p == value.end()) throw TError("TConfig", _("Attribute '%s' is not present!"), n_val.c_str());

    return *p->second;
}

TCfg *TConfig::at( const string &n_val, bool noExpt )
{
    TCfgMap::iterator p = value.find(n_val);
    if(p != value.end()) return p->second;
    if(noExpt) return NULL;
    throw TError("TConfig", _("Attribute '%s' is not present!"), n_val.c_str());
}

void TConfig::cfgList( vector<string> &list )
{
    list.clear();
    if(mElem)	mElem->fldList(list);
}

bool TConfig::cfgPresent( const string &n_val )	{ return (value.find(n_val) != value.end()); }

void TConfig::cfgViewAll( bool val )
{
    for(TCfgMap::iterator p = value.begin(); p != value.end(); ++p)
	p->second->setView(val);
}

void TConfig::cfgKeyUseAll( bool val )
{
    for(TCfgMap::iterator p = value.begin(); p != value.end(); ++p)
	if(p->second->fld().flg()&TCfg::Key)
	    p->second->setKeyUse(val);
}

void TConfig::setElem( TElem *Elements, bool first )
{
    if(mElem == Elements && !first) return;

    //Clear previos setting
    if(mElem) {
	TCfgMap::iterator p;
	while((p=value.begin()) != value.end()) {
	    delete p->second;
	    value.erase(p);
	}
	mElem->valDet(this);
	if(single) delete mElem;
    }

    //Set new setting
    if(!Elements) {
	mElem = new TElem("single");
	single = true;
    }
    else {
	mElem = Elements;
	single = false;
    }

    mElem->valAtt(this);
    for(unsigned i = 0; i < mElem->fldSize(); i++)
	value.insert(std::pair<string,TCfg*>(mElem->fldAt(i).name(),new TCfg(mElem->fldAt(i),*this)));
}

void TConfig::cntrCmdMake( XMLNode *opt, const string &path, int pos, const string &user, const string &grp, int perm )
{
    vector<string> list_c;
    cfgList(list_c);
    for(unsigned i_el = 0; i_el < list_c.size(); i_el++)
	if(cfg(list_c[i_el]).view())
	    cfg(list_c[i_el]).fld().cntrCmdMake(opt, path, (pos<0)?pos:pos++, user, grp, perm);
}

void TConfig::cntrCmdProc( XMLNode *opt, const string &elem, const string &user, const string &grp, int perm )
{
    if(elem.compare(0,4,"sel_") == 0 && TCntrNode::ctrChkNode(opt)) {
	TFld &n_e_fld = cfg(elem.substr(4)).fld();
	for(unsigned i_a = 0; i_a < n_e_fld.selNm().size(); i_a++)
	    opt->childAdd("el")->setText(n_e_fld.selNm()[i_a]);
	return;
    }
    TCfg &cel = cfg(elem);
    if(TCntrNode::ctrChkNode(opt,"get",(cel.fld().flg()&TFld::NoWrite)?(perm&~0222):perm,user.c_str(),grp.c_str(),SEC_RD)) {
	if(Mess->translDyn() && cel.fld().type() == TFld::String && !(cel.fld().flg()&TFld::NoStrTransl))
	    opt->setText(trU(cel.getS(),opt->attr("user")));
	else opt->setText(cel.getS());
    }
    if(TCntrNode::ctrChkNode(opt,"set",(cel.fld().flg()&TFld::NoWrite)?(perm&~0222):perm,user.c_str(),grp.c_str(),SEC_WR)) {
	if(Mess->translDyn() && cel.fld().type() == TFld::String && !(cel.fld().flg()&TFld::NoStrTransl))
	    cel.setS(trSetU(cel.getS(),opt->attr("user"),opt->text()));
	else cel.setS(opt->text());
    }
}

TVariant TConfig::objFunc( const string &iid, vector<TVariant> &prms, const string &user )
{
    // ElTp cfg(string nm) - config variable 'nm' get.
    //  nm - config variable name.
    if(iid == "cfg" && prms.size() >= 1) {
	TCfg *cf = at(prms[0].getS(), true);
	if(!cf) return EVAL_REAL;
	return *cf;
    }
    // ElTp cfgSet(string nm, ElTp val) - set config variable 'nm' to 'val'.
    //  nm - config variable name;
    //  val - variable value.
    if(iid == "cfgSet" && prms.size() >= 2) {
	TCfg *cf = at(prms[0].getS(), true);
	if(!cf || (cf->fld().flg()&TFld::NoWrite)) return false;
	*(TVariant*)cf = prms[1];
	return true;
    }

    return TVariant();
}

//*************************************************
//* TCfg                                          *
//*************************************************
TCfg::TCfg( TFld &fld, TConfig &owner ) : mView(true), mKeyUse(false), mNoTransl(false), mReqKey(false), mExtVal(false), mOwner(owner)
{
    //Chek for self field for dinamic elements
    if(fld.flg()&TFld::SelfFld) {
	mFld = new TFld();
	*mFld = fld;
    }
    else mFld = &fld;

    switch(mFld->type()) {
	case TFld::String:	setType(TVariant::String, true);  TVariant::setS(mFld->def());		break;
	case TFld::Integer:	setType(TVariant::Integer, true); TVariant::setI(s2ll(mFld->def()));	break;
	case TFld::Real:	setType(TVariant::Real, true);	  TVariant::setR(s2r(mFld->def()));	break;
	case TFld::Boolean:	setType(TVariant::Boolean, true); TVariant::setB((bool)s2i(mFld->def()));break;
	default: break;
    }
    if(fld.flg()&TCfg::Hide)	mView = false;
}

TCfg::~TCfg( )
{
    if(mFld->flg()&TFld::SelfFld)	delete mFld;
}

const string &TCfg::name( )	{ return mFld->name(); }

bool TCfg::isKey( )	{ return owner().reqKeys() ? reqKey() : fld().flg()&TCfg::Key; }

void TCfg::setReqKey( bool vl )
{
    mReqKey = mKeyUse = vl;
    mOwner.reqKeysUpdate();
}

string TCfg::getSEL( )
{
    if(!(mFld->flg()&TFld::Selected))	throw TError("Cfg", _("Element type is not selected!"));
    switch(type()) {
	case TVariant::String:	return mFld->selVl2Nm(getS());
	case TVariant::Integer:	return mFld->selVl2Nm(getI());
	case TVariant::Real:	return mFld->selVl2Nm(getR());
	case TVariant::Boolean:	return mFld->selVl2Nm(getB());
	default: break;
    }
    return "";
}

string TCfg::getS( )
{
    mOwner.mRes.lock();
    string rez = TVariant::getS();
    mOwner.mRes.unlock();
    if(!extVal()) return rez;
    else {
	if(fld().flg()&TransltText && !noTransl()) {
	    string rezT = TSYS::strSepParse(rez, 1, 0), rezSrc = TSYS::strSepParse(rez, 2, 0);
	    rez = TSYS::strSepParse(rez, 0, 0);
	    if(rez.size() && rezSrc.size()) Mess->translReg(rez, rezSrc);	//!!!! May be too busy
	    return rezT.size() ? rezT : rez;
	}
	else return TSYS::strSepParse(rez, 0, 0);
    }
}

string TCfg::getS( uint8_t RqFlg )
{
    if(extVal() && RqFlg&(ExtValOne|ExtValTwo|ExtValThree)) {
	mOwner.mRes.lock();
	string rez = TVariant::getS();
	mOwner.mRes.unlock();

	return TSYS::strSepParse(rez, ((RqFlg&ExtValTwo)?1:((RqFlg&ExtValThree)?2:0)), 0);
    }
    return getS();
}

const char *TCfg::getSd( )
{
    if(type() != TVariant::String)	throw TError("Cfg", _("Element type is not string!"));
    return (mSize < sizeof(val.sMini)) ? val.sMini : val.sPtr;
}

double &TCfg::getRd( )
{
    if(type() != TVariant::Real)	throw TError("Cfg", _("Element type is not real!"));
    return val.r;
}

int64_t &TCfg::getId( )
{
    if(type() != TVariant::Integer)	throw TError("Cfg", _("Element type is not int!"));
    return val.i;
}

char &TCfg::getBd( )
{
    if(type() != TVariant::Boolean)	throw TError("Cfg", _("Element type is not Boolean!"));
    return val.b;
}

void TCfg::setS( const string &val )
{
    switch(type()) {
	case TVariant::Integer:	setI(s2ll(val));	break;
	case TVariant::Real:	setR(s2r(val));		break;
	case TVariant::Boolean:	setB((bool)s2i(val));	break;
	case TVariant::String: {
	    mOwner.mRes.lock();
	    string tVal = TVariant::getS();
	    if(extVal() && (fld().flg()&TransltText) && !noTransl() && val.find(char(0)) == string::npos) {
		if(Mess->lang2Code()==Mess->lang2CodeBase()) TVariant::setS(val+string(2,0)+getS(ExtValThree));
		else TVariant::setS(getS(ExtValOne)+string(1,0)+val+string(1,0)+getS(ExtValThree));
	    }
	    else TVariant::setS(val);
	    mOwner.mRes.unlock();
	    try {
		if(!mOwner.cfgChange(*this,tVal)) {
		    mOwner.mRes.lock();
		    TVariant::setS(tVal);
		    mOwner.mRes.unlock();
		}
	    } catch(TError &err) {
		mOwner.mRes.lock();
		TVariant::setS(tVal);
		mOwner.mRes.unlock();
		throw;
	    }
	    break;
	}
	default: break;
    }
}

void TCfg::setR( double val )
{
    switch(type()) {
	case TVariant::String:	setS(r2s(val));	break;
	case TVariant::Integer:	setI((int)val);	break;
	case TVariant::Boolean:	setB((bool)val);break;
	case TVariant::Real: {
	    if(!(mFld->flg()&TFld::Selected) && mFld->selValR()[0] < mFld->selValR()[1])
		val = vmin(mFld->selValR()[1],vmax(mFld->selValR()[0],val));
	    double tVal = TVariant::getR();
	    TVariant::setR(val);
	    try{ if(!mOwner.cfgChange(*this,tVal)) TVariant::setR(tVal); }
	    catch(TError &err) { TVariant::setR(tVal); throw; }
	    break;
	}
	default: break;
    }
}

void TCfg::setI( int64_t val )
{
    switch(type()) {
	case TVariant::String:	setS(i2s(val));	break;
	case TVariant::Real:	setR(val);	break;
	case TVariant::Boolean:	setB((bool)val);break;
	case TVariant::Integer: {
	    if(!(mFld->flg()&TFld::Selected) && mFld->selValI()[0] < mFld->selValI()[1])
		val = vmin(mFld->selValI()[1],vmax(mFld->selValI()[0],val));
	    int tVal = TVariant::getI();
	    TVariant::setI(val);
	    try{ if(!mOwner.cfgChange(*this,tVal)) TVariant::setI(tVal); }
	    catch(TError &err) { TVariant::setI(tVal); throw; }
	    break;
	}
	default: break;
    }
}

void TCfg::setB( char val )
{
    switch(type()) {
	case TVariant::String:	setS(i2s(val));	break;
	case TVariant::Integer:	setI(val);	break;
	case TVariant::Real:	setR(val);	break;
	case TVariant::Boolean: {
	    bool tVal = TVariant::getB();
	    TVariant::setB(val);
	    try { if(!mOwner.cfgChange(*this,tVal)) TVariant::setB(tVal); }
	    catch(TError &err) { TVariant::setB(tVal); throw; }
	    break;
	}
	default: break;
    }
}

void TCfg::setSEL( const string &val, uint8_t RqFlg )
{
    if(!(mFld->flg()&TFld::Selected)) throw TError("Cfg", _("Element type is not selected!"));
    switch(type()) {
	case TVariant::String:	setS(mFld->selNm2VlS(val), RqFlg);	break;
	case TVariant::Integer:	setI(mFld->selNm2VlI(val), RqFlg);	break;
	case TVariant::Real:	setR(mFld->selNm2VlR(val), RqFlg);	break;
	case TVariant::Boolean:	setB(mFld->selNm2VlB(val), RqFlg);	break;
	default: break;
    }
}

void TCfg::setS( const string &val, uint8_t RqFlg )
{
    if(!extVal() && (RqFlg&(ExtValTwo|ExtValOne|ExtValThree))) { mExtVal = true; setType(TVariant::String); }
    if(!extVal()) setS(val);
    else setS(((RqFlg&ExtValOne || !(RqFlg&(ExtValTwo|ExtValThree)))?val:getS(ExtValOne))+string(1,0)+
		((RqFlg&ExtValTwo)?val:getS(ExtValTwo))+string(1,0)+
		((RqFlg&ExtValThree)?val:getS(ExtValThree)));
    if(RqFlg&TCfg::ForceUse)	{ setView(true); setKeyUse(true); }
}

void TCfg::setR( double val, uint8_t RqFlg )
{
    setR(val);
    if(RqFlg&TCfg::ForceUse)	{ setView(true); setKeyUse(true); }
}

void TCfg::setI( int64_t val, uint8_t RqFlg )
{
    setI(val);
    if(RqFlg&TCfg::ForceUse)	{ setView(true); setKeyUse(true); }
}

void TCfg::setB( char val, uint8_t RqFlg )
{
    setB(val);
    if(RqFlg&TCfg::ForceUse)	{ setView(true); setKeyUse(true); }
}

bool TCfg::operator==( TCfg &cfg )
{
    if(fld().type() == cfg.fld().type())
	switch(fld().type()) {
	    case TFld::String:	return (getS() == cfg.getS());
	    case TFld::Integer:	return (getI() == cfg.getI());
	    case TFld::Real:	return (getR() == cfg.getR());
	    case TFld::Boolean:	return (getB() == cfg.getB());
	    default: break;
	}
    return false;
}

TCfg &TCfg::operator=(TCfg & cfg)
{
    switch(type()) {
	case TVariant::String:	setS(cfg.getS());	break;
	case TVariant::Integer:	setI(cfg.getI());	break;
	case TVariant::Real:	setR(cfg.getR());	break;
	case TVariant::Boolean:	setB(cfg.getB());	break;
	default: break;
    }
    return *this;
}
