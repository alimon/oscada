
//OpenSCADA system file: tprotocols.cpp
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
#include "tmess.h"
#include "tprotocols.h"

using namespace OSCADA;

//************************************************
//* TProtocolS                                   *
//************************************************
TProtocolS::TProtocolS( ) : TSubSYS("Protocol",_("Transport protocols"),true)
{

}

TProtocolS::~TProtocolS( )
{

}

string TProtocolS::optDescr(  )
{
    return(_(
	"=============== Subsystem \"Transport protocols\" options =================\n\n"));
}

void TProtocolS::load_()
{
    //> Load parameters from command line
    string argCom, argVl;
    for(int argPos = 0; (argCom=SYS->getCmdOpt(argPos,&argVl)).size(); )
        if(argCom == "h" || argCom == "help")	fprintf(stdout,"%s",optDescr().c_str());

    //> Load parameters from config-file
}

void TProtocolS::cntrCmdProc( XMLNode *opt )
{
    //> Get page info
    if(opt->name() == "info")
    {
	TSubSYS::cntrCmdProc(opt);
	ctrMkNode("fld",opt,-1,"/help/g_help",_("Options help"),R_R___,"root",SPRT_ID,3,"tp","str","cols","90","rows","10");
	return;
    }
    //> Process command to page
    string a_path = opt->attr("path");
    if(a_path == "/help/g_help" && ctrChkNode(opt,"get",R_R___,"root",SPRT_ID))	opt->setText(optDescr());
    else TSubSYS::cntrCmdProc(opt);
}


//************************************************
//* TProtocol                                    *
//************************************************
TProtocol::TProtocol( const string &id ) : TModule(id)
{
    m_pr = grpAdd("pr_");
}

TProtocol::~TProtocol()
{

}

void TProtocol::open( const string &name, const string &tr )
{
    chldAdd(m_pr,in_open(name));
    at(name).at().setSrcTr(tr);
}

void TProtocol::close( const string &name )
{
    chldDel(m_pr,name);
}

void TProtocol::itemListIn( vector<string> &ls, const string &curIt )	{ ls.clear(); }

//************************************************
//* TProtocolIn                                  *
//************************************************
TProtocolIn::TProtocolIn( const string &name ) : mName(name)
{
    modifClr( );
}

TProtocolIn::~TProtocolIn()
{

}



TProtocol &TProtocolIn::owner( )	{ return *(TProtocol*)nodePrev(); }
