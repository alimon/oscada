
//OpenSCADA system module DAQ.DCON file: DCON_client.cpp
/***************************************************************************
 *   Copyright (C) 2008-2011 by Roman Savochenko, Almaz Karimov            *
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
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include <tsys.h>
#include <ttiparam.h>

#include "DCON_client.h"

//*************************************************
//* Modul info!                                   *
#define MOD_ID		"DCON"
#define MOD_NAME	_("DCON client")
#define MOD_TYPE	SDAQ_ID
#define VER_TYPE	SDAQ_VER
#define MOD_VER		"0.5.0"
#define AUTORS		_("Roman Savochenko, Almaz Karimov")
#define DESCRIPTION	_("Allow realisation of DCON client service. Supported I-7000 DCON protocol.")
#define LICENSE		"GPL2"
//*************************************************

DCONDAQ::TTpContr *DCONDAQ::mod;  //Pointer for direct access to module

extern "C"
{
    TModule::SAt module(int n_mod)
    {
	if(n_mod == 0)	return TModule::SAt(MOD_ID,MOD_TYPE,VER_TYPE);
	return TModule::SAt("");
    }

    TModule *attach(const TModule::SAt &AtMod, const string &source)
    {
	if(AtMod == TModule::SAt(MOD_ID,MOD_TYPE,VER_TYPE))
	    return new DCONDAQ::TTpContr(source);
	return NULL;
    }
}

using namespace DCONDAQ;

//******************************************************
//* TTpContr                                           *
//******************************************************
TTpContr::TTpContr(string name) : TTipDAQ(MOD_ID)
{
    mod	= this;

    mName	= MOD_NAME;
    mType	= MOD_TYPE;
    mVers	= MOD_VER;
    mAutor	= AUTORS;
    mDescr	= DESCRIPTION;
    mLicense	= LICENSE;
    mSource	= name;
}

TTpContr::~TTpContr()
{

}

void TTpContr::postEnable( int flag )
{
    TTipDAQ::postEnable( flag );

    //> Controler's bd structure
    fldAdd(new TFld("PRM_BD",_("Parameters table"),TFld::String,TFld::NoFlag,"30",""));
    fldAdd(new TFld("PERIOD",_("Gather data period (s)"),TFld::Real,TFld::NoFlag,"6.2","1","0.01;100"));
    fldAdd(new TFld("PRIOR",_("Gather task priority"),TFld::Integer,TFld::NoFlag,"2","0","-1;99"));
    fldAdd(new TFld("ADDR",_("Serial transport"),TFld::String,TFld::NoFlag,"30",""));
    fldAdd(new TFld("REQ_TRY",_("Request tries"),TFld::Integer,TFld::NoFlag,"1","3","1;10"));

    //> Parameter type bd structure
    int t_prm = tpParmAdd("std","PRM_BD",_("Standard"));
    tpPrmAt(t_prm).fldAdd(new TFld("MOD_ADDR",_("Module address"),TFld::Integer,TFld::NoFlag|TCfg::NoVal,"20","1","0;255"));
    tpPrmAt(t_prm).fldAdd(new TFld("CRC_CTRL",_("CRC control"),TFld::Boolean,TFld::NoFlag|TCfg::NoVal,"1","1"));
    tpPrmAt(t_prm).fldAdd(new TFld("HOST_SIGNAL",_("Host signal"),TFld::Integer,TFld::Selected|TCfg::NoVal,"1","0",
		"0;1",_("No signal;HostOK")));
    tpPrmAt(t_prm).fldAdd(new TFld("AI_METHOD",_("AI method"),TFld::Integer,TFld::Selected|TCfg::NoVal,"1","0",
		"0;1;2;3;4;6;8;10;20;116",
		_("No AI;1AI (#AA);2AI (#AA);3AI (#AA);4AI (#AA);6AI (#AA);8AI (#AA);10AI (#AA);20AI (#AA);16AI (#AA^AA)")));
    tpPrmAt(t_prm).fldAdd(new TFld("AI_RANGE",""/*_("AI range")*/,TFld::Integer,TFld::Selected|TCfg::NoVal,"1","0",
		"0;1;2",_("Engeneer or percent;Hexadecimal (0000 FFFF);Hexadecimal (8000 7FFF)")));
    tpPrmAt(t_prm).fldAdd(new TFld("AO_METHOD",_("AO method"),TFld::Integer,TFld::Selected|TCfg::NoVal,"1","0",
		"0;1;2;4",_("No AO;1AO (#AA);2AO (#AA);4AO (#AA)")));
    tpPrmAt(t_prm).fldAdd(new TFld("AO_RANGE",""/*_("AO range")*/,TFld::Integer,TFld::Selected|TCfg::NoVal,"1","0",
		"0;1;2;3;4;5;6;7;8;9;10",
		_("Engeneer (00.000 20.000);Engeneer (04.000 20.000);Engeneer (00.000 10.000);Engeneer (+00.000 +20.000);"
		  "Engeneer (+04.000 +20.000);Engeneer (+00.000 +10.000);Engeneer (-10.000 +10.000);Engeneer (+00.000 +05.000);"
		  "Engeneer (-05.000 +05.000);Percent (+000.00 +100.00);Hexadecimal (000 FFF)")));
    tpPrmAt(t_prm).fldAdd(new TFld("DI_METHOD",_("DI method"),TFld::Integer,TFld::Selected|TCfg::NoVal,"1","0",
		"0;3;4;7;8;14;16;101;201",
		_("No DI;3DI (@AA);4DI (@AA);7DI (@AA);8DI (@AA);14DI (@AA);16DI (@AA);1DI (@AADI);8DI (@AA,FF00)")) );
    tpPrmAt(t_prm).fldAdd(new TFld("DO_METHOD",_("DO method"),TFld::Integer,TFld::Selected|TCfg::NoVal,"1","0",
		"0;2;3;4;5;7;8;12;13;16;102;103;202;204;306;402;504;604;608)",
		_("No DO;2DO (@AA,0300);3DO (@AA,7);4DO (@AA,F);5DO (@AA,1F);7DO (@AA,7F);8DO (@AA,FF);12DO (@AA,0FFF);"
		  "13DO (@AA,1FFF);16DO (@AA,FFFF);2DO (^AADOVVV);3DO (^AADOVVV);2DO (@AADO);4DO (@AADO);6DO (@AADODD);"
		  "2DO (@AADO0D);4DO (@(^)AADO0D);4DO (@AA,0F00);8DO (@AA,FF00)")));
    tpPrmAt(t_prm).fldAdd(new TFld("CI_METHOD",_("CI method"),TFld::Integer,TFld::Selected|TCfg::NoVal,"1","0",
		"0;2;3",_("No CI;2CI (#AA);3CI (#AA)")));
}

void TTpContr::load_( )
{
    //> Load parameters from command line
}

void TTpContr::save_()
{

}

TController *TTpContr::ContrAttach( const string &name, const string &daq_db )
{
    return new TMdContr(name,daq_db,this);
}


//******************************************************
//* TMdContr                                           *
//******************************************************
TMdContr::TMdContr( string name_c, const string &daq_db, TElem *cfgelem ) :
    TController( name_c, daq_db, cfgelem ), m_per(cfg("PERIOD").getRd()), m_prior(cfg("PRIOR").getId()),
    connTry(cfg("REQ_TRY").getId()), m_addr(cfg("ADDR").getSd()), prc_st(false), endrun_req(false), tm_gath(0)
{
    cfg("PRM_BD").setS("DCONPrm_"+name_c);
}

TMdContr::~TMdContr()
{
    if(run_st) stop();
}

string TMdContr::getStatus( )
{
    string val = TController::getStatus( );

    if(startStat() && !redntUse()) val += TSYS::strMess(_("Spent time: %s. "),TSYS::time2str(tm_gath).c_str());

    return val;
}


TParamContr *TMdContr::ParamAttach( const string &name, int type )
{
    return new TMdPrm(name, &owner().tpPrmAt(type));
}

void TMdContr::disable_( )
{

}

void TMdContr::start_( )
{
    if(!prc_st)
    {
	SYS->transport().at().at("Serial").at().outAt(m_addr).at().start();

    	//> Start the gathering data task
    	SYS->taskCreate(nodePath('.',true), m_prior, TMdContr::Task, this, &prc_st);
    }
}

void TMdContr::stop_( )
{
    //> Stop the request and calc data task
    if(prc_st) SYS->taskDestroy(nodePath('.',true), &prc_st, &endrun_req);
}

bool TMdContr::cfgChange( TCfg &icfg )
{
    TController::cfgChange(icfg);
    return true;
}

void TMdContr::prmEn( const string &id, bool val )
{
    ResAlloc res( en_res, true );

    unsigned i_prm;
    for(i_prm = 0; i_prm < p_hd.size(); i_prm++)
	if(p_hd[i_prm].at().id() == id) break;

    if(val && i_prm >= p_hd.size())	p_hd.push_back(at(id));
    if(!val && i_prm < p_hd.size())	p_hd.erase(p_hd.begin()+i_prm);
}

string TMdContr::DCONCRC( string str )
{
    unsigned char CRC = 0;
    for(unsigned i = 0; i < str.size(); i++) CRC += (unsigned char)str[i];
    return TSYS::strMess("%02X",CRC);
}

string TMdContr::DCONReq( string &pdu, bool CRC, unsigned acqLen )
{
    ResAlloc res(req_res, true);
    char buf[1000];
    string rez, err;

    try
    {
	AutoHD<TTransportOut> tr = SYS->transport().at().at("Serial").at().outAt(m_addr);
	if(!tr.at().startStat()) tr.at().start();
	if(CRC) pdu += DCONCRC(pdu);
	pdu += "\r";

	ResAlloc resN( tr.at().nodeRes(), true );

	for(int i_tr = 0; i_tr < vmax(1,vmin(10,connTry)); i_tr++)
	{
	    int resp_len = tr.at().messIO( pdu.data(), pdu.size(), buf, sizeof(buf), 0, true );
	    rez.assign(buf,resp_len);

	    //> Wait tail
	    while(rez.size() < 2 || rez[rez.size()-1] != '\r')
	    {
		try{ resp_len = tr.at().messIO(NULL, 0, buf, sizeof(buf), 0, true); } catch(TError er){ break; }
		rez.append(buf, resp_len);
	    }
	    if(rez.size() < 2 || rez[rez.size()-1] != '\r') { err = _("13:Error respond: Not full."); continue; }
    	    pdu = rez.substr(0,rez.size()-1);
	    if(CRC && pdu.substr(pdu.size()-2) != DCONCRC(pdu.substr(0,pdu.size()-2)))
	    { err = _("21:Invalid module CRC"); continue; }
	    else if(!pdu.size() || (acqLen && !CRC && acqLen != pdu.size()) || (acqLen && CRC && (acqLen+2) != pdu.size()))
	    {
		if(pdu.size() && pdu[0] == '?')		err = _("24:Module out of range");
		else if(pdu.size() && pdu[0] == '!')	err = _("25:Command ignored (host watchdog)");
		else if(pdu.size() && pdu[0] != '>')	err = _("22:Invalid module response");
		else err = _("20:Waited respond length error");
		break;
	    }
	    err = "0";
	    break;
	}
    }
    catch(TError er) { err = _("10:Transport error: ")+er.mess; }

    if(err != "0") pdu = "";

    return err;
}

void *TMdContr::Task( void *icntr )
{
    string str, pdu, ai_txterr, ao_txterr, di_txterr, do_txterr, ci_txterr;
    int acq_len;
    int n, m, i;
    TMdContr &cntr = *(TMdContr *)icntr;

    cntr.endrun_req = false;
    cntr.prc_st = true;

    try
    {
	while(!cntr.endrun_req)
	{
	    if(!cntr.redntUse())
	    {
		long long t_cnt = TSYS::curTime();

		//> Update controller's data
		ResAlloc res(cntr.en_res, false);
		for(unsigned i_p = 0; i_p < cntr.p_hd.size(); i_p++)
		{
		    if(cntr.endrun_req) break;
		    //> Reset errors
		    ai_txterr = ao_txterr = di_txterr = do_txterr = ci_txterr = "0";

		    //AI
		    n = 0;
		    switch(cntr.p_hd[i_p].at().ai_method)
		    {
			case 1: case 2: case 3: case 4: case 6: case 8: case 10: case 20:
			    n = cntr.p_hd[i_p].at().ai_method;
			case 116: if(!n) n = 8;
			    //#AA
			    //> Request with module
		    	    pdu = TSYS::strMess("#%02X",cntr.p_hd[i_p].at().mod_addr);
			    m = (cntr.p_hd[i_p].at().ai_range == 0) ? 7 : 4;
			    if((ai_txterr=cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,(m*n+1))) == "0")
			        for(i = 0; i < n; i++)
    				{
				    if(m == 7) cntr.p_hd[i_p].at().AI[i] = atof(pdu.substr(i*m+1,m).c_str());
				    if(m == 4) cntr.p_hd[i_p].at().AI[i] = strtol(pdu.substr(i*m+1,m).c_str(),NULL,16);
	    			    if(cntr.p_hd[i_p].at().ai_range == 2 && cntr.p_hd[i_p].at().AI[i] > 0x7fff)
					cntr.p_hd[i_p].at().AI[i]=cntr.p_hd[i_p].at().AI[i]-0x10000;
				}

			    if(cntr.p_hd[i_p].at().ai_method == 116)
			    {
				//^AA
				//> Request with module
				pdu = TSYS::strMess("^%02X",cntr.p_hd[i_p].at().mod_addr);
				m = (cntr.p_hd[i_p].at().ai_range == 0) ? 7 : 4;
				if((ai_txterr=cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,1)) == "0")
                                    for(i = 0; i < n; i++)
				    {
					if(m == 7) cntr.p_hd[i_p].at().AI[i+8] = atof(pdu.substr(i*m+1,m).c_str());
	    				if(m == 4) cntr.p_hd[i_p].at().AI[i+8] = strtol(pdu.substr(i*m+1,m).c_str(),NULL,16);
	    				if(cntr.p_hd[i_p].at().ai_range == 2 && cntr.p_hd[i_p].at().AI[i+8] > 0x7fff)
					    cntr.p_hd[i_p].at().AI[i+8] = cntr.p_hd[i_p].at().AI[i+8]-0x10000;
				    }
			    }
			    break;
		    }

		    //AO
		    switch(cntr.p_hd[i_p].at().ao_method)
		    {
			case 1: case 2: case 4:
			    for(i = 0; i < cntr.p_hd[i_p].at().ao_method; i++)
			    {
				//Control range
				switch(cntr.p_hd[i_p].at().ao_range)
				{
				    case 0://Engeneer (00.000 20.000)
					str = TSYS::strMess("%+07.3f",vmax(0,vmin(20,cntr.p_hd[i_p].at().AO[i])));
					break;
				    case 1://Engeneer (04.000 20.000)
					str = TSYS::strMess("%+07.3f",vmax(4,vmin(20,cntr.p_hd[i_p].at().AO[i])));
					break;
				    case 2://Engeneer (00.000 10.000)
					str = TSYS::strMess("%+07.3f",vmax(0,vmin(10,cntr.p_hd[i_p].at().AO[i])));
					break;
				    case 3://Engeneer (+00.000 +20.000)
					str = TSYS::strMess("%+07.3f",vmax(0,vmin(20,cntr.p_hd[i_p].at().AO[i])));
					break;
				    case 4://Engeneer (+04.000 +20.000)
					str = TSYS::strMess("%+07.3f",vmax(4,vmin(20,cntr.p_hd[i_p].at().AO[i])));
					break;
				    case 5://Engeneer (+00.000 +10.000)
					str = TSYS::strMess("%+07.3f",vmax(0,vmin(10,cntr.p_hd[i_p].at().AO[i])));
					break;
				    case 6://Engeneer (-10.000 +10.000)
					str = TSYS::strMess("%+07.3f",vmax(-10,vmin(10,cntr.p_hd[i_p].at().AO[i])));
					break;
				    case 7://Engeneer (+00.000 +05.000)
					str = TSYS::strMess("%+07.3f",vmax(0,vmin(5,cntr.p_hd[i_p].at().AO[i])));
					break;
				    case 8://Engeneer (-05.000 +05.000)
					str = TSYS::strMess("%+07.3f",vmax(-5,vmin(5,cntr.p_hd[i_p].at().AO[i])));
					break;
				    case 9://Percent (+000.00 +100.00)
					str = TSYS::strMess("%+07.2f",vmax(0,vmin(100,cntr.p_hd[i_p].at().AO[i])));
					break;
				    case 10://Hexadecimal (000 FFF)
					str = TSYS::strMess("%03X",(int)vmax(0,vmin(0xFFF,cntr.p_hd[i_p].at().AO[i])));
					break;
				    default:	ao_txterr = _("23:Undefined range");	break;
				}
				//> Request with module
				pdu = TSYS::strMess("#%02X%d%s",cntr.p_hd[i_p].at().mod_addr,i,str.c_str());
    				ao_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,1);
			    }
		    }

		    //DI
		    unsigned int DI;
		    switch(cntr.p_hd[i_p].at().di_method)
		    {
			case 3: case 4: case 7: case 8: case 14: case 16: case 101: case 201:
    			{
			    //> Request with module
			    pdu = TSYS::strMess("@%02X",cntr.p_hd[i_p].at().mod_addr);
			    acq_len = 5;
		    	    if(cntr.p_hd[i_p].at().di_method == 101) { pdu += "DI"; acq_len = 7; }
			    if((di_txterr=cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,acq_len)) == "0")
			    {
    				n = 0;
				switch(cntr.p_hd[i_p].at().di_method)
				{
    				    case 3: if(!n) n = 3;	//3DI (@AA)
    				    case 4: if(!n) n = 4;	//4DI (@AA)
    				    case 7: if(!n) n = 7;	//7DI (@AA)
    				    case 8: if(!n) n = 8;	//8DI (@AA)
    				    case 14:if(!n) n = 14;	//14DI (@AA)
    				    case 16:if(!n) n = 16;	//16DI (@AA)
    				    case 101:if(!n) n = 1;	//1DI (@AADI)
    				    case 201:if(!n) n = 8;	//8DI (@AA,FF00)
				        DI = strtoul(pdu.substr(1,4).c_str(),NULL,16);	//???? substring length 4 for a request?
				        for(int i_n = 0; i_n < n; i_n++)
					    cntr.p_hd[i_p].at().DI[i_n] = (DI>>i_n)&0x01;
					break;
				}
			    }
			    break;
			}
		    }

		    //DO
		    unsigned code = 0;
		    n = 0;
		    pdu = "";
		    switch(cntr.p_hd[i_p].at().do_method)
		    {
			case 2://2DO (@AA,0300)
			    //> Request with module
			    code = (cntr.p_hd[i_p].at().DO[1]?2:0)+(cntr.p_hd[i_p].at().DO[0]?1:0);
			    pdu = TSYS::strMess("@%02X%02X00",cntr.p_hd[i_p].at().mod_addr,code);
		    	    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,1);
		    	    break;
			case 604://4DO (@AA,0F00)
			    for(unsigned i_n = 0; i_n < 4; i_n++) if(cntr.p_hd[i_p].at().DO[i_n]) code += (1<<i_n);
			    pdu = TSYS::strMess("@%02X%02X00",cntr.p_hd[i_p].at().mod_addr,code);
			    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,1);
			    break;
			case 608://8DO (@AA,FF00)
			    for(unsigned i_n = 0; i_n < 8; i_n++) if(cntr.p_hd[i_p].at().DO[i_n]) code += (1<<i_n);
			    pdu = TSYS::strMess("@%02X%02X00",cntr.p_hd[i_p].at().mod_addr,code);
			    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,1);
			    break;
			case 3://3DO (@AA,7)
			    for(unsigned i_n = 0; i_n < 3; i_n++) if(cntr.p_hd[i_p].at().DO[i_n]) code += (1<<i_n);
			    pdu = TSYS::strMess("@%02X%01X",cntr.p_hd[i_p].at().mod_addr,code);
			    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,1);
			    break;
			case 4://4DO (@AA,F)
			    for(unsigned i_n = 0; i_n < 4; i_n++) if(cntr.p_hd[i_p].at().DO[i_n]) code += (1<<i_n);
			    pdu = TSYS::strMess("@%02X%01X",cntr.p_hd[i_p].at().mod_addr,code);
			    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,1);
			    break;
			case 5://5DO (@AA,1F)
			    for(unsigned i_n = 0; i_n < 5; i_n++) if(cntr.p_hd[i_p].at().DO[i_n]) code += (1<<i_n);
			    pdu = TSYS::strMess("@%02X%02X",cntr.p_hd[i_p].at().mod_addr,code);
			    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,1);
			    break;
			case 7://7DO (@AA,7F)
			    for(unsigned i_n = 0; i_n < 7; i_n++) if(cntr.p_hd[i_p].at().DO[i_n]) code += (1<<i_n);
			    pdu = TSYS::strMess("@%02X%02X",cntr.p_hd[i_p].at().mod_addr,code);
			    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,1);
			    break;
       			case 8://8DO (@AA,FF)
			    for(unsigned i_n = 0; i_n < 8; i_n++) if(cntr.p_hd[i_p].at().DO[i_n]) code += (1<<i_n);
		    	    pdu = TSYS::strMess("@%02X%02X",cntr.p_hd[i_p].at().mod_addr,code);
		    	    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,1);
		    	    break;
       			case 12://12DO (@AA,0FFF)
      			    for(unsigned i_n = 0; i_n < 12; i_n++) if(cntr.p_hd[i_p].at().DO[i_n]) code += (1<<i_n);
       			    pdu = TSYS::strMess("@%02X%04X",cntr.p_hd[i_p].at().mod_addr,code);
       			    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,1);
       			    break;
       			case 13://13DO (@AA,1FFF)
       			    for(unsigned i_n = 0; i_n < 13; i_n++) if(cntr.p_hd[i_p].at().DO[i_n]) code += (1<<i_n);
       			    pdu = TSYS::strMess("@%02X%04X",cntr.p_hd[i_p].at().mod_addr,code);
       			    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,1);
       			    break;
       			case 16://16DO (@AA,FFFF)
       			    for(unsigned i_n = 0; i_n < 16; i_n++) if(cntr.p_hd[i_p].at().DO[i_n]) code += (1<<i_n);
       			    pdu = TSYS::strMess("@%02X%04X",cntr.p_hd[i_p].at().mod_addr,code);
		    	    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,1);
	    		    break;
       			case 102://2DO (^AADOVVV)
			    //> Request with module
			    pdu = TSYS::strMess("^%02XDO",cntr.p_hd[i_p].at().mod_addr);
		    	    pdu += cntr.p_hd[i_p].at().DO[0] ? "1" : "0";
		    	    pdu += cntr.p_hd[i_p].at().DO[1] ? "1" : "0";
		    	    pdu += "0";
		    	    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,1);
		    	    break;
			case 103://3DO (^AADOVVV)
			    pdu = TSYS::strMess("^%02XDO",cntr.p_hd[i_p].at().mod_addr);
		    	    pdu += cntr.p_hd[i_p].at().DO[0] ? "1" : "0";
		    	    pdu += cntr.p_hd[i_p].at().DO[1] ? "1" : "0";
		    	    pdu += cntr.p_hd[i_p].at().DO[2] ? "1" : "0";
			    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,1);
	    		    break;
			case 202://2DO (@AADO)
			    n = 2;
			case 204://4DO (@AADO)
			    if(!n) n = 4;
			    for(int i_r = 0; i_r < n/2; i_r++)
			    {
				//> Request with module
				for(unsigned i_n = 0; i_n < 2; i_n++) if(cntr.p_hd[i_p].at().DO[i_r*2+i_n]) code += (1<<i_n);
				pdu = TSYS::strMess("@%02XDO%01X%01X",cntr.p_hd[i_p].at().mod_addr,i_r,code);
				do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,3);
	    		    }
			    break;
			case 306://6DO (@AADODD)
			    //> Request with module
			    for(unsigned i_n = 0; i_n < 6; i_n++) if(cntr.p_hd[i_p].at().DO[i_n]) code += (1<<i_n);
			    pdu = TSYS::strMess("@%02XDO%02X",cntr.p_hd[i_p].at().mod_addr,code);
			    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,3);
			    break;
			case 402://2DO (@AADO0D)
			    for(unsigned i_n = 0; i_n < 2; i_n++) if(cntr.p_hd[i_p].at().DO[i_n]) code += (1<<i_n);
			    pdu = TSYS::strMess("@%02XDO%02X",cntr.p_hd[i_p].at().mod_addr,code);
	    		    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,3);
			    break;
			case 504://4DO (@(^)AADO0D)
			    //> Request with module
			    code = (cntr.p_hd[i_p].at().DO[1]?2:0)+(cntr.p_hd[i_p].at().DO[0]?1:0);
			    pdu = TSYS::strMess("@%02XDO%02X",cntr.p_hd[i_p].at().mod_addr,code);
			    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,3);
			    //> Request with module
			    code = (cntr.p_hd[i_p].at().DO[3]?2:0)+(cntr.p_hd[i_p].at().DO[2]?1:0);
			    pdu = TSYS::strMess("^%02XDO%02X",cntr.p_hd[i_p].at().mod_addr,code);
			    do_txterr = cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,3);
			    break;
		    }

		    //CI
		    n = 0;
		    switch(cntr.p_hd[i_p].at().ci_method)
		    {
			case 2://2CI (#AA)
			    n = 2;
			case 3://3CI (#AA)
			    if(!n) n = 3;

			    for(int i_n = 0; i_n < n; i_n++)
			    {
				//> Request with module
				pdu = TSYS::strMess("#%02X%d",cntr.p_hd[i_p].at().mod_addr,i_n);
				if((ci_txterr=cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl,9)) == "0")
			            cntr.p_hd[i_p].at().CI[i_n] = strtol(pdu.substr(1,8).c_str(),NULL,16);
			    }
			    break;
		    }

		    //Host signal
		    switch (cntr.p_hd[i_p].at().host_signal)
		    {
			case 1://HostOK signal
			{
			    pdu = "~**";
	    		    cntr.DCONReq(pdu,cntr.p_hd[i_p].at().crc_ctrl);
			    break;
			}
		    }
		    cntr.p_hd[i_p].at().ai_err.setVal(ai_txterr);
		    cntr.p_hd[i_p].at().ao_err.setVal(ao_txterr);
		    cntr.p_hd[i_p].at().di_err.setVal(di_txterr);
		    cntr.p_hd[i_p].at().do_err.setVal(do_txterr);
		    cntr.p_hd[i_p].at().ci_err.setVal(ci_txterr);
		}
		res.release();

		//> Calc acquisition process time
		cntr.tm_gath = TSYS::curTime()-t_cnt;
	    }

	    //> Calc next work time and sleep
	    TSYS::taskSleep((long long)(1e9*cntr.period()));
	}
    }
    catch(TError err)	{ mess_err(err.cat.c_str(), err.mess.c_str()); }

    cntr.prc_st = false;

    return NULL;
}

void TMdContr::cntrCmdProc( XMLNode *opt )
{
    //> Get page info
    if(opt->name() == "info")
    {
	TController::cntrCmdProc(opt);
	ctrMkNode("fld",opt,-1,"/cntr/cfg/ADDR",cfg("ADDR").fld().descr(),RWRWR_,"root",SDAQ_ID,3,"tp","str","dest","select","select","/cntr/cfg/serDevLst");
	return;
    }
    //> Process command to page
    string a_path = opt->attr("path");
    if(a_path == "/cntr/cfg/serDevLst" && ctrChkNode(opt))
    {
	vector<string> sls;
	if(SYS->transport().at().modPresent("Serial"))
	    SYS->transport().at().at("Serial").at().outList(sls);
	for(unsigned i_s = 0; i_s < sls.size(); i_s++)
	    opt->childAdd("el")->setText(sls[i_s]);
    }
    else TController::cntrCmdProc(opt);
}

//******************************************************
//* TMdPrm                                             *
//******************************************************
TMdPrm::TMdPrm( string name, TTipParam *tp_prm ) :
    TParamContr( name, tp_prm ), p_el("w_attr"), mod_addr(cfg("MOD_ADDR").getId()), crc_ctrl(cfg("CRC_CTRL").getBd()),
    host_signal(cfg("HOST_SIGNAL").getId()), ai_method(cfg("AI_METHOD").getId()), ai_range(cfg("AI_RANGE").getId()), ao_method(cfg("AO_METHOD").getId()),
    ao_range(cfg("AO_RANGE").getId()), di_method(cfg("DI_METHOD").getId()), do_method(cfg("DO_METHOD").getId()), ci_method(cfg("CI_METHOD").getId())
{
    ai_err.setVal("0");
    ao_err.setVal("0");
    di_err.setVal("0");
    do_err.setVal("0");
    ci_err.setVal("0");
    // Default input/output data
    for(int i = 0; i < 32; i++)
    {
	AI[i] = AO[i] = CI[i] = 0;
	DI[i] = DO[i] = false;
    }
}

TMdPrm::~TMdPrm( )
{
    nodeDelAll( );
}

void TMdPrm::postEnable( int flag )
{
    TParamContr::postEnable(flag);
    if(!vlElemPresent(&p_el))	vlElemAtt(&p_el);
}

TMdContr &TMdPrm::owner( )	{ return (TMdContr&)TParamContr::owner(); }

void TMdPrm::enable()
{
    if(enableStat())	return;

    TParamContr::enable();

    //> Delete DAQ parameter's attributes
    for(unsigned i_f = 0; i_f < p_el.fldSize(); )
    {
	try { p_el.fldDel(i_f); continue; }
	catch(TError err) { mess_warning(err.cat.c_str(),err.mess.c_str()); }
	i_f++;
    }

    //> Make DAQ parameter's attributes
    unsigned itCnt = 0;
    //>> AI
    switch(ai_method)
    {
	case 1: itCnt = 1;	break;	//1AI (#AA)
	case 2: itCnt = 2;	break;	//2AI (#AA)
	case 3: itCnt = 3;	break;	//3AI (#AA)
	case 4: itCnt = 4;	break;	//4AI (#AA)
	case 6: itCnt = 6;	break;	//6AI (#AA)
	case 8: itCnt = 8;	break;	//8AI (#AA)
	case 10:itCnt = 10;	break;	//10AI (#AA)
	case 20:itCnt = 20;	break;	//20AI (#AA)
	case 16:itCnt = 16;	break;	//16AI (#AA^AA)
	default:itCnt = 0;	break;
    }
    if(itCnt)
    {
    	string sai_method = TSYS::int2str(ai_method);
    	p_el.fldAdd(new TFld("ai_err","ai_err",TFld::Boolean,TFld::NoWrite|TVal::DirRead,"","","","",sai_method.c_str()));
 	for(unsigned i_p = 0; i_p < itCnt; i_p++)
	    p_el.fldAdd(new TFld(TSYS::strMess("AI%d",i_p).c_str(),TSYS::strMess("AI%d",i_p).c_str(),
		    TFld::Real,TFld::NoWrite|TVal::DirRead,"","","","",sai_method.c_str()));
    }

    //>> AO
    string ao_d;
    switch(ao_range)
    {
	case 0:	ao_d = "0;20";	break;	//Engeneer (00.000 20.000)
	case 1:	ao_d = "4;20";	break;	//Engeneer (04.000 20.000)
	case 2:	ao_d = "0;10";	break;	//Engeneer (00.000 10.000)
	case 3:	ao_d = "0;20";	break;	//Engeneer (+00.000 +20.000)
	case 4:	ao_d = "4;20";	break;	//Engeneer (+04.000 +20.000)
	case 5:	ao_d = "0;10";	break;	//Engeneer (+00.000 +10.000)
	case 6:	ao_d = "-10;10";break;	//Engeneer (-10.000 +10.000)
	case 7:	ao_d = "0;5";	break;	//Engeneer (+00.000 +05.000)
	case 8:	ao_d = "-5;5";	break;	//Engeneer (-05.000 +05.000)
	case 9:	ao_d = "0;100";	break;	//Percent (+000.00 +100.00)
	case 10:ao_d = "0;4095";break;	//Hexadecimal (000 FFF)
	default:ao_d = "";	break;
    }

    switch(ao_method)
    {
	case 1: itCnt = 1;	break;	//1AO (#AA)
	case 2: itCnt = 2;	break;	//2AO (#AA)
	case 4: itCnt = 4;	break;	//4AO (#AA)
	default:itCnt = 0;      break;
    }
    if(itCnt)
    {
    	string sao_method = TSYS::int2str(ao_method);
	p_el.fldAdd(new TFld("ao_err","ao_err",TFld::Boolean,TFld::NoWrite|TVal::DirRead,"","","","",sao_method.c_str()) );
    	for(unsigned i_p = 0; i_p < itCnt; i_p++)
	    p_el.fldAdd(new TFld(TSYS::strMess("AO%d",i_p).c_str(),TSYS::strMess("AO%d",i_p).c_str(),
	    	    TFld::Real,TVal::DirRead|TVal::DirWrite,"20","1",ao_d.c_str(),"",sao_method.c_str())); 
    }

    //>> DI
    switch(di_method)
    {
	case 3:	itCnt = 3;	break;	//3DI (@AA)
	case 4: itCnt = 4;	break;	//4DI (@AA)
	case 7: itCnt = 7;	break;	//7DI (@AA)
	case 8: itCnt = 8;	break;	//8DI (@AA)
	case 14:itCnt = 14;	break;	//14DI (@AA)
	case 16:itCnt = 16;	break;	//16DI (@AA)
	case 101:itCnt = 1;	break;	//1DI (@AADI)
	case 201:itCnt = 8;	break;	//8DI (@AA,FF00)
	default:itCnt = 0;      break;
    }
    if(itCnt)
    {
    	string sdi_method = TSYS::int2str(di_method);
	p_el.fldAdd(new TFld("di_err","di_err",TFld::Boolean,TFld::NoWrite|TVal::DirRead,"","","","",sdi_method.c_str()));
	for(unsigned i_p = 0; i_p < itCnt; i_p++)
	    p_el.fldAdd(new TFld(TSYS::strMess("DI%d",i_p).c_str(),TSYS::strMess("DI%d",i_p).c_str(),
		    TFld::Boolean,TFld::NoWrite|TVal::DirRead,"","","","",sdi_method.c_str()));
    }

    //>> DO
    switch(do_method)
    {
	case 2:	itCnt = 2;	break;	//2DO (@AA,0300)
	case 3:	itCnt = 3;	break;	//3DO (@AA,7)
	case 4: itCnt = 4;	break;	//4DO (@AA,F)
	case 5: itCnt = 5;	break;	//5DO (@AA,1F)
	case 7: itCnt = 7;	break;	//7DO (@AA,7F)
	case 8: itCnt = 8;	break;	//8DO (@AA,FF)
	case 12:itCnt = 12;	break;	//12DO (@AA,0FFF)
	case 13:itCnt = 13;	break;	//13DO (@AA,1FFF)
	case 16:itCnt = 16;	break;	//16DO (@AA,FFFF)
	case 102:itCnt = 2;	break;	//2DO (^AADOVVV)
	case 103:itCnt = 3;	break;	//3DO (^AADOVVV)
	case 202:itCnt = 2;	break;	//2DO (@AADO)
	case 204:itCnt = 4;	break;	//4DO (@AADO)
	case 306:itCnt = 6;	break;	//@AADODD
	case 402:itCnt = 2;	break;	//@AADO0D
	case 504:itCnt = 4;	break;	//4DO (@(^)AADO0D)
	case 604:itCnt = 4;	break;	//4DO (@AA,0F00)
	case 608:itCnt = 8;	break;	//8DO (@AA,FF00)
    	default:itCnt = 0;      break;
    }
    if(itCnt)
    {
   	string sdo_method = TSYS::int2str(do_method);
	p_el.fldAdd(new TFld("do_err","do_err",TFld::Boolean,TFld::NoWrite|TVal::DirRead,"","","","",sdo_method.c_str()) );
	for(unsigned i_p = 0; i_p < itCnt; i_p++)
	    p_el.fldAdd(new TFld(TSYS::strMess("DO%d",i_p).c_str(),TSYS::strMess("DO%d",i_p).c_str(),
		    TFld::Boolean,TVal::DirRead|TVal::DirWrite,"","","","",sdo_method.c_str()));
    }

    //>> CI
    switch(ci_method)
    {
	case 2:	itCnt = 2;	break;	//2CI (#AA)
	case 3:	itCnt = 3;	break;	//3CI (#AA)
	default:itCnt = 0;      break;
    }
    if(itCnt)
    {
    	string sci_method = TSYS::int2str(ci_method); 
	p_el.fldAdd(new TFld("ci_err","ci_err",TFld::Boolean,TFld::NoWrite|TVal::DirRead,"","","","",sci_method.c_str()));
	for(unsigned i_p = 0; i_p < itCnt; i_p++)
	    p_el.fldAdd( new TFld(TSYS::strMess("CI%d",i_p).c_str(),TSYS::strMess("CI%d",i_p).c_str(),
		    TFld::Real,TFld::NoWrite|TVal::DirRead,"","","","",sci_method.c_str()) );
    }

    owner().prmEn( id(), true );
}

void TMdPrm::disable()
{
    if(!enableStat())  return;

    owner().prmEn(id(), false);

    TParamContr::disable();

    //> Set EVAL to parameter attributes
    vector<string> ls;
    elem().fldList(ls);
    for(unsigned i_el = 0; i_el < ls.size(); i_el++)
	vlAt(ls[i_el]).at().setS(EVAL_STR, 0, true);
}

void TMdPrm::vlGet( TVal &val )
{
    if(!enableStat() || !owner().startStat())
    {
	if(val.name() == "err")
	{
	    if(!enableStat())			val.setS(_("1:Parameter is disabled."),0,true);
	    else if(!owner().startStat())	val.setS(_("2:Acquisition is stoped."),0,true);
	}
	else val.setS(EVAL_STR,0,true);
	return;
    }

    if(owner().redntUse()) return;
    if(val.name() == "ai_err") val.setB(ai_err.getVal() != "0",0,true);
    if(val.name() == "ao_err") val.setB(ao_err.getVal() != "0",0,true);
    if(val.name() == "di_err") val.setB(di_err.getVal() != "0",0,true);
    if(val.name() == "do_err") val.setB(do_err.getVal() != "0",0,true);
    if(val.name() == "ci_err") val.setB(ci_err.getVal() != "0",0,true);
    if(val.name() == "err")
    {
	if(ai_err.getVal() != "0")	val.setS(ai_err.getVal(),0,true);
	else if(ao_err.getVal() != "0")	val.setS(ao_err.getVal(),0,true);
	else if(di_err.getVal() != "0")	val.setS(di_err.getVal(),0,true);
    	else if(do_err.getVal() != "0")	val.setS(do_err.getVal(),0,true);
	else if(ci_err.getVal() != "0")	val.setS(ci_err.getVal(),0,true);
	else val.setS("0",0,true);
    }
    if(val.name().compare(0,2,"AI") == 0) val.setR(AI[atoi(val.name().substr(2,val.name().size()-2).c_str())],0,true);
    if(val.name().compare(0,2,"AO") == 0) val.setR(AO[atoi(val.name().substr(2,val.name().size()-2).c_str())],0,true);
    if(val.name().compare(0,2,"DI") == 0) val.setB(DI[atoi(val.name().substr(2,val.name().size()-2).c_str())],0,true);
    if(val.name().compare(0,2,"DO") == 0) val.setB(DO[atoi(val.name().substr(2,val.name().size()-2).c_str())],0,true);
    if(val.name().compare(0,2,"CI") == 0) val.setR(CI[atoi(val.name().substr(2,val.name().size()-2).c_str())],0,true);
}

void TMdPrm::vlSet( TVal &valo, const TVariant &pvl )
{
    if(!enableStat())	valo.setI(EVAL_INT, 0, true);

    //> Send to active reserve station
    if(owner().redntUse())
    {
	if(valo.getS() == pvl.getS()) return;
	XMLNode req("set");
	req.setAttr("path",nodePath(0,true)+"/%2fserv%2fattr")->childAdd("el")->setAttr("id",valo.name())->setText(valo.getS());
	SYS->daq().at().rdStRequest(owner().workId(),req);
	return;
    }

    //> Direct write
    if(valo.name().compare(0,2,"AO") == 0) AO[atoi(valo.name().substr(2,valo.name().size()-2).c_str())]=valo.getR(NULL,true);
    if(valo.name().compare(0,2,"DO") == 0) DO[atoi(valo.name().substr(2,valo.name().size()-2).c_str())]=valo.getB(NULL,true);
}

void TMdPrm::vlArchMake( TVal &val )
{
    if(val.arch().freeStat()) return;
    val.arch().at().setSrcMode(TVArchive::ActiveAttr, val.arch().at().srcData());
    val.arch().at().setPeriod((long long)(owner().period()*1000000));
    val.arch().at().setHardGrid(true);
    val.arch().at().setHighResTm(true);
}

bool TMdPrm::cfgChange( TCfg &icfg )
{
    TParamContr::cfgChange(icfg);

    if(enableStat() && (icfg.fld().name() == "AI_METHOD" || icfg.fld().name() == "AO_METHOD" || 
	    icfg.fld().name() == "DI_METHOD" || icfg.fld().name() == "DO_METHOD" || icfg.fld().name() == "CI_METHOD"))
        disable();

    return true;
}

void TMdPrm::cntrCmdProc( XMLNode *opt )
{
    //> Get page info
    if(opt->name() == "info")
    {
        TParamContr::cntrCmdProc(opt);
        if(ai_method == 0) ctrRemoveNode(opt,"/prm/cfg/AI_RANGE");
        if(ao_method == 0) ctrRemoveNode(opt,"/prm/cfg/AO_RANGE");
        return;
    }
    //> Process command to page
    TParamContr::cntrCmdProc(opt);
}
