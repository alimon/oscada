
//OpenSCADA system file: tmess.cpp
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

#include <sys/types.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <langinfo.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <errno.h>

#include <algorithm>

#include "tsys.h"
#include "resalloc.h"
#include "tmess.h"

#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif
#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#endif

using namespace OSCADA;

//*************************************************
//* TMess                                         *
//*************************************************
TMess::TMess( ) : IOCharSet("UTF-8"), mMessLevel(Info), mLogDir(DIR_STDOUT|DIR_ARCHIVE),
    mConvCode(true), mIsUTF8(true), mTranslEn(false), mTranslSet(false)
{
    pthread_mutexattr_t attrM;
    pthread_mutexattr_init(&attrM);
    pthread_mutexattr_settype(&attrM, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mRes, &attrM);
    pthread_mutexattr_destroy(&attrM);

    setenv("LC_NUMERIC","C",1);
    openlog(PACKAGE,0,LOG_USER);
    setlocale(LC_ALL,"");
    IOCharSet = nl_langinfo(CODESET);

#ifdef HAVE_LIBINTL_H
    bindtextdomain(PACKAGE, localedir_full);
    textdomain(PACKAGE);
#endif

    mLang2Code = lang();
    if(mLang2Code.size() < 2 || mLang2Code == "POSIX" || mLang2Code == "C") mLang2Code = "en";
    else mLang2Code = mLang2Code.substr(0,2);
    mIsUTF8 = (IOCharSet == "UTF-8" || IOCharSet == "UTF8" || IOCharSet == "utf8");

    if(mLang2Code == "en" && (IOCharSet.compare(0,10,"ISO-8859-1")==0 || IOCharSet.compare(0,14,"ANSI_X3.4-1968")==0))
	mConvCode = false;
}

TMess::~TMess( )
{
    closelog();

    pthread_mutex_destroy(&mRes);
}

void TMess::setMessLevel( int level )
{
    mMessLevel = vmax(Debug, vmin(Crit,level));
    SYS->modif();
}

void TMess::setLogDirect( int dir )
{
    mLogDir = dir;
    SYS->modif();
}

void TMess::put( const char *categ, int8_t level, const char *fmt,  ... )
{
    if(abs(vmin(Emerg,vmax(-Emerg,level))) < messLevel()) return;

    //messLevel() = TMess::Debug process for selected category and categories list combining
    if(messLevel() == TMess::Debug && level == TMess::Debug) {
	MtxAlloc res(mRes, true);
	// Check for present into debugCats and put new
	if(debugCats.find(categ) == debugCats.end()) {
	    string curCatLev, tCat;
	    for(int off = 0; (tCat=TSYS::pathLev(categ,0,true,&off)).size(); ) {
		curCatLev += "/"+tCat;
		if(debugCats.find(curCatLev) == debugCats.end()) debugCats[curCatLev] = false;
	    }
	}

	// Check for match to selectDebugCats
	bool matchOK = false;
	for(unsigned i_dc = 0; !matchOK && i_dc < selectDebugCats.size(); i_dc++)
	    matchOK = (strncmp(categ,selectDebugCats[i_dc].c_str(),selectDebugCats[i_dc].size()) == 0);
	if(!matchOK) return;
    }

    //Put the message
    va_list argptr;

    va_start(argptr, fmt);
    putArg(categ, level, fmt, argptr);
    va_end(argptr);
}

void TMess::put_( const char *categ, int8_t level, const char *fmt,  ... )
{
    va_list argptr;

    va_start(argptr, fmt);
    putArg(categ, level, fmt, argptr);
    va_end(argptr);
}

void TMess::putArg( const char *categ, int8_t level, const char *fmt, va_list ap )
{
    char mess[STR_BUF_LEN];
    vsnprintf(mess, sizeof(mess), fmt, ap);

    level = vmin(Emerg, vmax(-Emerg,level));
    int64_t ctm = TSYS::curTime();
    string s_mess = i2s(level) + "|" + categ + " | " + mess;

    if(mLogDir & DIR_SYSLOG) {
	int level_sys;
	switch(abs(level))
	{
	    case Debug:		level_sys = LOG_DEBUG;	break;
	    case Info:		level_sys = LOG_INFO;	break;
	    case Notice:	level_sys = LOG_NOTICE;	break;
	    case Warning:	level_sys = LOG_WARNING;break;
	    case Error:		level_sys = LOG_ERR;	break;
	    case Crit:		level_sys = LOG_CRIT;	break;
	    case Alert:		level_sys = LOG_ALERT;	break;
	    case Emerg:		level_sys = LOG_EMERG;	break;
	    default: 		level_sys = LOG_DEBUG;
	}
	syslog(level_sys, "%s", s_mess.c_str());
    }
    if(mLogDir & DIR_STDOUT)	fprintf(stdout, "%s \n", s_mess.c_str());
    if(mLogDir & DIR_STDERR)	fprintf(stderr, "%s \n", s_mess.c_str());
    if((mLogDir&DIR_ARCHIVE) && SYS->present("Archive"))
	SYS->archive().at().messPut(ctm/1000000, ctm%1000000, categ, level, mess);
}

void TMess::get( time_t b_tm, time_t e_tm, vector<TMess::SRec> &recs, const string &category, int8_t level )
{
    if(mLogDir & DIR_ARCHIVE)	SYS->archive().at().messGet(b_tm, e_tm, recs, category, level);
}

string TMess::translGet( const string &base, const string &src )
{
    string rez;

    MtxAlloc res(Mess->mRes, true);
    //Request from cache at first
    map<string,CacheEl>::iterator itr = trMessCache.find(base);
    if(itr != trMessCache.end()) { itr->second.tm = SYS->sysTm(); rez = itr->second.val; }
    else {
	//Request to data source direct
	if(src.compare(0,5,"uapi:") == 0) { // Check/Get/Place from user API translations table
	    string srcAddrs = src.substr(5), tStrVl;
	    if(srcAddrs.empty()) srcAddrs = SYS->workDB();

	    TConfig req;
	    req.setIncmplTblStrct(true);
	    int8_t isCfg = -1;

	    vector<string> addrs;
	    for(int off = 0; (tStrVl=TSYS::strParse(srcAddrs,0,";",&off)).size(); ) addrs.push_back(tStrVl);
	    for(vector<string>::reverse_iterator iA = addrs.rbegin(); iA != addrs.rend(); ++iA) {
		tStrVl = translFld(lang2Code(), "base", (isCfg==1));
		if(isCfg != (*iA==DB_CFG)) {
		    isCfg = (*iA==DB_CFG) ? 1 : 0;
		    req.elem().fldClear();
		    req.elem().fldAdd(new TFld("base","Base",TFld::String,TCfg::Key,"1000"));
		    req.elem().fldAdd(new TFld(tStrVl.c_str(),"Tr",TFld::String,0));
		    req.cfg("base").setS(base);
		}
		if(SYS->db().at().dataGet(*iA+"."+translUApiTbl(),"/"mess_TrUApiTbl,req,false,true)) {
		    rez = req.cfg(tStrVl).getS();
		    break;
		}
		// Create new record into the translation table of the data source
		else if((iA+1) == addrs.rend() && lang2CodeBase().size()) {
		    if(lang2CodeBase() == lang2Code())	req.elem().fldDel(req.elem().fldId(tStrVl.c_str()));
		    SYS->db().at().dataSet(*iA+"."mess_TrUApiTbl, "/"mess_TrUApiTbl, req, false, true);
		}
	    }
	    trMessCache[base] = CacheEl(rez, SYS->sysTm());
	}

	//Cache data and limit update
	if(trMessCache.size() > (STD_CACHE_LIM+STD_CACHE_LIM/10)) {
	    vector< pair<time_t,string> > sortQueue;
	    for(map<string,CacheEl>::iterator itr = trMessCache.begin(); itr != trMessCache.end(); ++itr)
		sortQueue.push_back(pair<time_t,string>(itr->second.tm,itr->first));
	    sort(sortQueue.begin(), sortQueue.end());
	    for(unsigned i_del = 0; i_del < (STD_CACHE_LIM/10); ++i_del) trMessCache.erase(sortQueue[i_del].second);
	}
    }

    return rez.empty() ? base : rez;
}

void TMess::setTranslEn( bool vl, bool passive )
{
    if(vl == mTranslEn || mTranslSet) return;
    mTranslSet = true;
    mTranslEn = vl;
    if(!passive) {
	if(vl) {
	    SYS->load(true);		//Built messages load
	    translReg("", "uapi:");	//User API messages load
	}
	else trMessIdx.clear();
    }

    mTranslSet = false;

    SYS->modif();
}

void TMess::translReg( const string &mess, const string &src, const string &prms )
{
    if(!translEn()) return;

    if(src.compare(0,5,"uapi:") == 0) {
	vector<string> ls;
	if(src.size() > 5) ls.push_back(src.substr(5));
	else {
	    SYS->db().at().dbList(ls);
	    ls.push_back("<cfg>");
	}

	TConfig req;
	req.elem().fldAdd(new TFld("base","Base",TFld::String,TCfg::Key,"1000"));

	MtxAlloc res(Mess->mRes, true);
	for(unsigned i_l = 0; i_l < ls.size(); i_l++)
	    if(ls[i_l] == DB_CFG)
		for(int io_cnt = 0; SYS->db().at().dataSeek("","/"mess_TrUApiTbl,io_cnt++,req); )
		    trMessIdx[req.cfg("base").getS()]["cfg:/"mess_TrUApiTbl] = prms;
	    else
		for(int io_cnt = 0; SYS->db().at().dataSeek(ls[i_l]+"."mess_TrUApiTbl,"",io_cnt++,req); )
		    trMessIdx[req.cfg("base").getS()]["db:"+ls[i_l]+"."mess_TrUApiTbl"#base"] = prms;
    }
    else {
	if(TSYS::strNoSpace(mess).empty()) return;
	MtxAlloc res(Mess->mRes, true);
	trMessIdx[mess][src] = prms;
    }
}

string TMess::translFld( const string &lng, const string &fld, bool isCfg )
{
    return isCfg ? fld+"_"+lng : lng+"#"+fld;
}

string TMess::lang( )
{
    char *lng = NULL;
    if(((lng=getenv("LANGUAGE")) && strlen(lng)) || ((lng=getenv("LC_MESSAGES")) && strlen(lng)) || ((lng=getenv("LANG")) && strlen(lng)))
	return lng;
    else return "C";
}

string TMess::selDebCats( )
{
    string rez;

    MtxAlloc res(Mess->mRes, true);
    for(unsigned i_sdc = 0; i_sdc < selectDebugCats.size(); i_sdc++)
	rez += selectDebugCats[i_sdc]+";";

    return rez;
}

void TMess::setSelDebCats( const string &vl )
{
    MtxAlloc res(Mess->mRes, true);
    debugCats.clear();
    selectDebugCats.clear();

    string curCat;
    for(int off = 0; (curCat=TSYS::strParse(vl,0,";",&off)).size(); ) {
	debugCats[curCat] = true;
	selectDebugCats.push_back(curCat);
    }
    SYS->modif();
}

void TMess::setLang( const string &lng, bool init )
{
    char *prvLng = NULL;
    if((prvLng=getenv("LANGUAGE")) && strlen(prvLng)) setenv("LANGUAGE", lng.c_str(), 1);
    else setenv("LC_MESSAGES", lng.c_str(), 1);
    setlocale(LC_ALL, "");

    IOCharSet = nl_langinfo(CODESET);

    mLang2Code = lang();
    if(mLang2Code.size() < 2 || mLang2Code == "POSIX" || mLang2Code == "C") mLang2Code = "en";
    else mLang2Code = mLang2Code.substr(0, 2);
    mIsUTF8 = (IOCharSet == "UTF-8" || IOCharSet == "UTF8" || IOCharSet == "utf8");

    if(init) SYS->sysModifFlgs &= ~TSYS::MDF_LANG;
    else { SYS->sysModifFlgs |= TSYS::MDF_LANG; SYS->modif(); }
}

string TMess::codeConv( const string &fromCH, const string &toCH, const string &mess )
{
    if(!mConvCode || fromCH == toCH) return mess;

#ifdef HAVE_ICONV_H
    //Make convert to blocks 1000 bytes
    string buf;
    buf.reserve(mess.size());
    char   *ibuf, outbuf[1000], *obuf;
    size_t ilen, olen, chwrcnt = 0;
    iconv_t hd;

    hd = iconv_open(toCH.c_str(), fromCH.c_str());
    if(hd == (iconv_t)(-1)) {
	mess_crit("IConv",_("Error 'iconv' open: %s"),strerror(errno));
	return mess;
    }

    ibuf = (char *)mess.data();
    ilen = mess.size();

    while(ilen) {
	obuf = outbuf;
	olen = sizeof(outbuf)-1;
	size_t rez = iconv(hd, &ibuf, &ilen, &obuf, &olen);
	if(rez == (size_t)(-1) && (errno == EINVAL || errno == EBADF)) {
	    mess_crit("IConv", _("Error input sequence convert: %s"), strerror(errno));
	    buf = mess;
	    break;
	}
	if(obuf > outbuf) buf.append(outbuf, obuf-outbuf);
	if(rez == (size_t)(-1) && errno == EILSEQ) { buf += '?'; ilen--; ibuf++; chwrcnt++; }
    }
    iconv_close(hd);

    //> Deadlock possible on the error message print
    //if(chwrcnt)	mess_err("IConv", _("Error converting %d symbols from '%s' to '%s' for message part: '%s'(%d)"),
    //		    chwrcnt, fromCH.c_str(), toCH.c_str(), mess.substr(0,20).c_str(), mess.size());

    return buf;
#else
    return mess;
#endif
}

const char *TMess::I18N( const char *mess, const char *d_name )
{
#ifdef HAVE_LIBINTL_H
    return dgettext(d_name, mess);
#else
    return mess;
#endif
}

void TMess::setLang2CodeBase( const string &vl )
{
    mLang2CodeBase = vl;
    if((!mLang2CodeBase.empty() && mLang2CodeBase.size() < 2) || mLang2CodeBase == "POSIX" || mLang2CodeBase == "C")
	mLang2CodeBase = "en";
    else mLang2CodeBase = mLang2CodeBase.substr(0,2);

    SYS->modif();
}

void TMess::load( )
{
    //Load params from command line
    string argCom, argVl;
    for(int argPos = 0; (argCom=SYS->getCmdOpt(argPos,&argVl)).size(); )
	if(argCom == "h" || argCom == "help")	return;
	else if(argCom == "MessLev") {
	    int i = atoi(optarg);
	    if(i >= Debug && i <= Emerg) setMessLevel(i);
	}
	else if(argCom == "log") setLogDirect(s2i(argVl));

    //Load params config-file
    setMessLevel(s2i(TBDS::genDBGet(SYS->nodePath()+"MessLev",i2s(messLevel()),"root",TBDS::OnlyCfg)));
    setSelDebCats(TBDS::genDBGet(SYS->nodePath()+"SelDebCats",selDebCats(),"root",TBDS::OnlyCfg));
    setLogDirect(s2i(TBDS::genDBGet(SYS->nodePath()+"LogTarget",i2s(logDirect()),"root",TBDS::OnlyCfg)));
    setLang(TBDS::genDBGet(SYS->nodePath()+"Lang",lang(),"root",TBDS::OnlyCfg), true);
    mLang2CodeBase = TBDS::genDBGet(SYS->nodePath()+"Lang2CodeBase",mLang2CodeBase,"root",TBDS::OnlyCfg);
    setTranslEn(s2i(TBDS::genDBGet(SYS->nodePath()+"TranslEn",i2s(translEn()),"root",TBDS::OnlyCfg)), true);
}

void TMess::save( )
{
    TBDS::genDBSet(SYS->nodePath()+"MessLev",i2s(messLevel()),"root",TBDS::OnlyCfg);
    TBDS::genDBSet(SYS->nodePath()+"SelDebCats",selDebCats(),"root",TBDS::OnlyCfg);
    TBDS::genDBSet(SYS->nodePath()+"LogTarget",i2s(logDirect()),"root",TBDS::OnlyCfg);
    if(SYS->sysModifFlgs&TSYS::MDF_LANG) TBDS::genDBSet(SYS->nodePath()+"Lang",lang(),"root",TBDS::OnlyCfg);
    TBDS::genDBSet(SYS->nodePath()+"Lang2CodeBase",mLang2CodeBase,"root",TBDS::OnlyCfg);
    TBDS::genDBSet(SYS->nodePath()+"TranslEn",i2s(translEn()),"root",TBDS::OnlyCfg);
}

const char *TMess::labDB( )
{
    return _("DB address in format [<DB module>.<DB name>].\n"
	     "For use main work DB set '*.*'.");
}

const char *TMess::labSecCRON( )
{
    return _("Schedule is wrote in seconds periodic form or in standard CRON form.\n"
	     "Seconds form is one real number (1.5, 1e-3).\n"
	     "Cron it is standard form \"* * * * *\".\n"
	     "  Where items by order:\n"
	     "    - minutes (0-59);\n"
	     "    - hours (0-23);\n"
	     "    - days (1-31);\n"
	     "    - month (1-12);\n"
	     "    - week day (0[Sunday]-6).\n"
	     "  Where an item variants:\n"
	     "    - \"*\" - any value;\n"
	     "    - \"1,2,3\" - allowed values;\n"
	     "    - \"1-5\" - raw range of allowed values;\n"
	     "    - \"*/2\" - range divider for allowed values.\n"
	     "Examples:\n"
	     "  \"1e-3\" - periodic call by one millisecond;\n"
	     "  \"* * * * *\" - each minute;\n"
	     "  \"10 23 * * *\" - only 23 hour and 10 minute for any day and month;\n"
	     "  \"*/2 * * * *\" - for minutes: 0,2,4,...,56,58;\n"
	     "  \"* 2-4 * * *\" - for any minutes in hours from 2 to 4(include).");
}

const char *TMess::labSecCRONsel( )	{ return "1;1e-3;* * * * *;10 * * * *;10-20 2 */2 * *"; }

const char *TMess::labTaskPrior( )
{
    return _("Task priority level (-1...99), where:\n"
	     "  -1     - lowest priority batch policy;\n"
	     "  0      - standard userspace priority;\n"
	     "  1...99 - realtime priority level (round-robin), often allowed only for \"root\".");
}
