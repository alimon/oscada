
//OpenSCADA system module UI.WebVision file: web_vision.cpp
/***************************************************************************
 *   Copyright (C) 2007 by Roman Savochenko                                *
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
#include <string>

#include <config.h>
#include <tsys.h>
#include <tmess.h>

#include "web_vision.h"

//************************************************
//* Modul info!                                  *
#define MOD_ID	    "WebVision"
#define MOD_NAME    "WEB vision"
#define MOD_TYPE    "UI"
#define VER_TYPE    VER_UI
#define SUB_TYPE    "WWW"
#define MOD_VERSION "0.1.0"
#define AUTORS      "Roman Savochenko"
#define DESCRIPTION "Web vision for visual control area (VCA) projects playing."
#define LICENSE     "GPL"
//************************************************

WebVision::TWEB *WebVision::mod;

extern "C" 
{
    TModule::SAt module( int n_mod )
    {
	if( n_mod==0 )	return TModule::SAt(MOD_ID,MOD_TYPE,VER_TYPE);
	return TModule::SAt("");
    }

    TModule *attach( const TModule::SAt &AtMod, const string &source )
    {
	if( AtMod == TModule::SAt(MOD_ID,MOD_TYPE,VER_TYPE) )
	    return new WebVision::TWEB( source );
	return NULL;
    }    
}

using namespace WebVision;

//************************************************
//* TWEB                                         *
//************************************************
TWEB::TWEB( string name ) : m_t_auth(10), lst_ses_chk(0)
{
    mId		= MOD_ID;
    mName       = MOD_NAME;
    mType	= MOD_TYPE;
    mVers	= MOD_VERSION;
    mAutor	= AUTORS;
    mDescr	= DESCRIPTION;
    mLicense	= LICENSE;
    mSource	= name;

    mod		= this;
    
    //- Reg export functions -
    modFuncReg( new ExpFunc("void HttpGet(const string&,string&,const string&,vector<string>&);",
        "Process Get comand from http protocol's!",(void(TModule::*)( )) &TWEB::HttpGet) );
    modFuncReg( new ExpFunc("void HttpPost(const string&,string&,const string&,vector<string>&,const string&);",
        "Process Set comand from http protocol's!",(void(TModule::*)( )) &TWEB::HttpPost) );
	
    //- Default CSS init -
    m_CSStables =
        "hr {width:100%; size:3}\n"
        "body {background-color:#818181; alink:#33ccff; link:#3366ff; text:#000000; vlink:#339999}\n"
        "h1.head {text-align:center; color:#ffff00 }\n"
        "h2.title {text-align:center; font-style:italic; margin:0; padding:0; border-width:0 }\n"
        "table.page_head {background-color:#cccccc; border:3px ridge blue; width:100% }\n"
        "table.page_head td.tool {text-align:center; border:1px solid blue; width:120px;  white-space: nowrap }\n"
        "table.page_head td.user {text-align:left; border:1px solid blue; width:120px; white-space: nowrap }\n"
        "table.page_area {background-color:#9999ff; border:3px ridge #a9a9a9; width:100%; padding:2px }\n"
        "table.page_area tr.content {background-color:#cccccc; border:5px ridge #9999ff; padding:5px }\n"
        "table.page_auth {background-color:#9999ff; border:3px ridge #a9a9a9; padding:2px }\n"
        "table.page_auth tr.content {background-color:#cccccc; border:5px ridge #9999ff; padding:5px }\n";
    
    m_VCAjs = 
	"/***************************************************\n"
	" * pathLev - Path parsing function.                *\n"
	" ***************************************************/\n"
	"pathLev.off = 0;\n"
	"function pathLev( path, level )\n"
	"{\n"
	"  var an_dir = pathLev.off;\n"
	"  var t_lev = 0;\n"
	"  var t_dir;\n"
	"  while( an_dir<path.length && path.charAt(an_dir)=='/' ) an_dir++;\n"
	"  if( an_dir >= path.length ) return '';\n"
	"  while( true )\n"
	"  {\n"
	"    t_dir = path.indexOf('/',an_dir);\n"
	"    if( t_dir < 0 ) { pathLev.off=path.length; return (t_lev==level)?path.substr(an_dir):''; }\n"
        "    if( t_lev==level ) { pathLev.off=t_dir; return path.substr(an_dir,t_dir-an_dir); }\n"
        "    an_dir = t_dir;\n"
	"    t_lev++;\n"
	"    while( an_dir<path.length && path.charAt(an_dir)=='/' ) an_dir++;\n"
	"  }\n"
	"}\n"
	"/***************************************************\n"
	" * getXmlHttp - Check and return XMLHttpRequest for*\n"
	" *  various brousers.                              *\n"
	" ***************************************************/\n"
	"function getXmlHttp()\n"
	"{\n"
	"  var xmlhttp;\n"
	"  try { xmlhttp = new ActiveXObject(\"Msxml2.XMLHTTP\"); }\n"
    	"  catch (e) {\n"
	"    try { xmlhttp = new ActiveXObject(\"Microsoft.XMLHTTP\"); }\n"
	"    catch (e) { xmlhttp = false; } }\n"
	"    if( !xmlhttp && typeof XMLHttpRequest != 'undefined' )\n"
	"    { xmlhttp = new XMLHttpRequest(); }\n"
	"  return xmlhttp;\n"
	"}\n"
	"/***************************************************\n"
	" * servGet - XML get request to server             *\n"
	" ***************************************************/\n"
	"function servGet( adr, prm, full )\n"
	"{\n"
	"  var req = getXmlHttp();\n"
	"  req.open('GET',encodeURI((full?'':'/"MOD_ID"')+adr+'?'+prm),false);\n"
	"  req.send(null);\n"
	"  if( req.status == 200 && req.responseXML.childNodes.length )\n"
	"    return req.responseXML.childNodes[0];\n"
	"}\n"
	"/***************************************************\n"
	" * servSet - XML set request to server             *\n"
	" ***************************************************/\n"
	"function servSet( adr, prm, body )\n"
	"{\n"
 	"  var req = getXmlHttp();\n"
	"  req.open('POST',encodeURI('/"MOD_ID"'+adr+'?'+prm),true);\n"
	"  req.send(body);\n"
	"}\n"
	"/***************************************************\n"
	" * getWAttrs - request page/widget attributes      *\n"
	" ***************************************************/\n"
	"function getWAttrs( wId, tm )\n"
	"{\n"
	"  var rNode = servGet(wId,'com=attrs'+(tm?('&tm='+tm):''))\n"
	"  if( !rNode ) return;\n"
	"  var atrLs = new Object();\n"	
	"  for( var i = 0; i < rNode.childNodes.length; i++ )\n"
	"    atrLs[rNode.childNodes[i].getAttribute('id')] = rNode.childNodes[i].textContent;\n"
	"  return atrLs;\n"
	"}\n"
	"/***************************************************\n"
	" * setWAttrs - set page/widget attributes          *\n"
	" ***************************************************/\n"
	"function setWAttrs( wId, attrs )\n"
	"{\n"
	"  var body = '<set>';\n"
	"  for( var i in attrs ) body+='<el id=\\''+i+'\\'>'+attrs[i]+'</el>';\n"
	"  body+='</set>';\n"
	"  servSet(wId,'com=attrs',body);\n"
	"}\n"
	"/***************************************************\n"
	" * pwDescr - Page/widget descriptor object         *\n"
	" ***************************************************/\n"
	"function callPage( pgId, pgAttrs, upW )\n"
	"{\n"
	"  if( !pgId ) return true;\n"
	"  if( this == masterPage )\n"
	"  {\n"
	"    var opPg = this.findOpenPage(pgId);\n"
	"    if( opPg && upW )\n"
	"      for( var i = 0; i < upW.length; i++ )\n"
	"      {\n"
	"        pathLev.off = 0;\n"
	"        pathLev(upW[i],0);\n"
	"        opPg.makeEl(false,(upW[i].length<=pathLev.off)?'/':upW[i].substr(pathLev.off));\n"
	"      }\n"
	"    if( opPg ) return true;\n"
	"  }\n"
	"  if( !pgAttrs ) pgAttrs = getWAttrs(pgId);\n"
	"  var pgGrp = pgAttrs['pgGrp'];\n"
	"  var pgOpenSrc = pgAttrs['pgOpenSrc'];\n"
	"  if( !this.addr.length || (this == masterPage && pgGrp == 'main') || pgGrp == this.attrs['pgGrp'] )\n"
	"  {\n"
	"    this.addr  = pgId;\n"
	"    this.attrs = pgAttrs;\n"
	"    this.place = document.createElement('div');\n"
	"    this.makeEl(true,'');\n"
	"    document.body.appendChild(this.place);\n"
	"    //- Set project's icon and RunTime page title -\n"
 	"    document.getElementsByTagName(\"link\")[0].setAttribute(\"href\",location.pathname+'?com=ico');\n" 
	"    return true;\n"
	"  }\n"
	"  //- Find for include page creation -\n"
	"  for( var i in this.wdgs )\n"
	"    if( this.wdgs[i].tp == 'Box' && pgGrp == this.wdgs[i].attrs['pgGrp'] && pgId != this.wdgs[i].attrs['pgOpenSrc'] )\n"
	"    {\n"
	"      this.wdgs[i].attrs['pgOpenSrc'] = pgId;\n"
	"      this.wdgs[i].makeEl(true,'');\n"
	"      return true;\n"
	"    }\n"
	"  //- Put checking to child pages -\n"
	"  for( var i = 0; i < this.pages.length; i++ )\n"
	"    if( this.pages[i].callPage(pgId,pgAttrs,upW) ) return true;\n"
	"  return false;\n"
	"}\n"
	"function findOpenPage( pgId )\n"
	"{\n"
	"  if( pgId == this.addr ) return this;\n"
	"  //- Check from included widgets -\n"
 	"  for( var i in this.wdgs )\n"
	"    if( this.wdgs[i].tp == 'Box' && pgId == this.wdgs[i].attrs['pgOpenSrc'] )\n"
	"      return this.wdgs[i].pages[pgId];\n"
	"  //- Put checking to child pages -\n"
	"  for( var i = 0; i < this.pages.length; i++ )\n"
	"  {\n"
	"    var opPg = this.pages[i].findOpenPage(pgId);\n"
	"    if( opPg ) return opPg;\n"
	"  }\n"
	"}\n"
	"function makeEl( hot, el_addr )\n"
	"{\n"
	"  if( el_addr && el_addr != '/' )\n"
	"  {\n"
	"    pathLev.off = 0;\n"
	"    var pthEl = pathLev(el_addr,0);\n"
	"    this.wdgs[pthEl].makeEl(false,(el_addr.length<=pathLev.off)?'/':el_addr.substr(pathLev.off));\n"
	"    return;\n"
	"  }\n"
	"  var rNode;\n"
	"  if( !el_addr )\n"
	"  {\n"
	"    rNode = servGet(this.addr,'com=root'); if( !rNode ) return;\n"
	"    this.tp = rNode.textContent;\n"
	"  }\n"
	"  if( !hot )\n"
	"  {\n"
	"    var tAttrs = getWAttrs(this.addr,el_addr?tmCnt:0);\n"
	"    for( var i in tAttrs ) this.attrs[i] = tAttrs[i];\n"
	"  }\n"
	"  var elStyle = '';\n"
	"  if( this.pg ) elStyle+='position: absolute; left: 0px; top: 0px; '; \n"
	"  else elStyle+='position: absolute; left: '+this.attrs['geomX']+'px; top: '+this.attrs['geomY']+'px; ';\n"
	"  elStyle+='width: '+this.attrs['geomW']+'px; height: '+this.attrs['geomH']+'px; ';\n"
	"  elStyle+='z-index: '+this.attrs['geomZ']+'; margin: '+this.attrs['geomMargin']+'; ';\n"	
	"  if( this.tp == 'Box' )\n"
	"  {\n"
	"    if( this.attrs['backColor'] ) elStyle+='background-color: '+this.attrs['backColor']+'; ';\n"
	"    if( this.attrs['backImg'] )   elStyle+='background-image: url(\\'/"MOD_ID"'+this.addr+'?com=res&val='+this.attrs['backImg']+'\\'); ';\n"
	"    elStyle+='border-style: solid; border-width: '+this.attrs['bordWidth']+'px; ';\n"
	"    if( this.attrs['bordColor'] ) elStyle+='border-color: '+this.attrs['bordColor']+'; ';\n"
	"    if( this.attrs['pgOpenSrc'] && !this.pages[this.attrs['pgOpenSrc']] )\n"
	"    {\n"
	"      if( this.inclOpen )\n"
	"      {\n"
	"	 var atrs = new Object(); atrs.pgOpen = '0'; setWAttrs(this.inclOpen,atrs);\n"
	"        pgCache[this.inclOpen] = this.place.removeChild(this.pages[this.inclOpen].place);\n"
	"        delete this.pages[this.inclOpen];\n"
	"      }\n"
	"      this.inclOpen = this.attrs['pgOpenSrc'];\n"
	"      var iPg = new pwDescr(this.inclOpen,true,this);\n"
	"      iPg.place = pgCache[this.inclOpen];\n"
	"      if( true ) { iPg.place = document.createElement('div'); iPg.makeEl(false,''); }\n"
	"      this.pages[this.inclOpen] = iPg;\n"
	"      this.place.appendChild(iPg.place);\n"
	"    }\n"
	"  }\n"
	"  else if( this.tp == 'Text' )\n"
	"  {\n"
	"    if( this.attrs['backColor'] ) elStyle+='background-color: '+this.attrs['backColor']+'; ';\n"
	"    if( this.attrs['backImg'] )   elStyle+='background-image: url(\\'/"MOD_ID"'+this.addr+'?com=res&val='+this.attrs['backImg']+'\\'); ';\n"
	"    elStyle+='border-style: solid; border-width: '+this.attrs['bordWidth']+'px; ';\n"
	"    elStyle+='overflow: hidden; ';\n"	
	"    if( this.attrs['bordColor'] ) elStyle+='border-color: '+this.attrs['bordColor']+'; ';\n"	
	"    var txtAlign = parseInt(this.attrs['alignment']);\n"
	"    switch(txtAlign&0x3)\n"
	"    {\n"
	"      case 0: elStyle+='text-align: left; '; break;\n"
	"      case 1: elStyle+='text-align: right; '; break;\n"
	"      case 2: elStyle+='text-align: center; '; break;\n"
	"      case 3: elStyle+='text-align: justify; '; break;\n"	
	"    }\n"
	"    switch(txtAlign>>2)\n"
	"    {\n"
	"      case 0: elStyle+='vertical-align: top; '; break;\n"
	"      case 1: elStyle+='vertical-align: bottom; '; break;\n"
	"      case 2: elStyle+='vertical-align: middle; '; break;\n"
	"    }\n"
	"    var allFnt = this.attrs['font'].split(' ');\n"
	"    if( allFnt.length >= 1 ) elStyle+='font-family: '+allFnt[0]+'; ';\n"
	"    else if( this.attrs['fontFamily'] ) elStyle+='font-family: '+this.attrs['fontFamily']+'; ';\n"
	"    if( allFnt.length >= 2 ) elStyle+='font-size: '+allFnt[1]+'px; ';\n"
	"    else elStyle+='font-size: '+this.attrs['fontSize']+'px; ';\n"
	"    if( this.attrs['fontBold'] == 1 ) elStyle+='font-weight: bold; ';\n"
	"    if( this.attrs['fontItalic'] == 1 ) elStyle+='font-style: italic; ';\n"
	"    elStyle+='color: '+this.attrs['color']+'; ';\n"
	"    var txtVal = this.attrs['text'];\n"
	"    for( var i = 0; i < parseInt(this.attrs['numbArg']); i++ )\n"
	"    {\n"
	"      var argVal;\n"
	"      var argCfg = new Array();\n"
	"      switch(parseInt(this.attrs['arg'+i+'tp']))\n"
	"      {\n"
	"        case 0: case 2:\n"
	"          argCfg[0]=this.attrs['arg'+i+'cfg'];\n"
	"          argVal=this.attrs['arg'+i+'val'];\n"
    	"          break;\n"
	"        case 1:\n"
	"          argCfg=this.attrs['arg'+i+'cfg'].split(';');\n"
	"          if( argCfg[1] == 'g' )      argVal=parseFloat(this.attrs['arg'+i+'val']).toPrecision(argCfg[2]);\n"
	"          else if( argCfg[1] == 'f' ) argVal=parseFloat(this.attrs['arg'+i+'val']).toFixed(argCfg[2]);\n"
	"          else argVal=this.attrs['arg'+i+'val'];\n"
	"          break;\n"
	"      }\n"
	"      txtVal = txtVal.replace('%'+(i+1),argVal);\n"
	"      if( !argCfg.length ) continue;\n"
	"      var argSize = parseInt(argCfg[0]);\n"
	"      while( txtVal.legth < Math.abs(argSize) )\n"
	"        if( argSize > 0 ) txtVal=' '+txtVal; else txtVal+=' ';\n"
	"    }\n"
	"    var txtNode = this.place.childNodes[0];\n"
	"    if( !txtNode ) this.place.appendChild(document.createTextNode(txtVal));\n"
	"    else txtNode.nodeValue = txtVal;\n"
	"  }\n"
	"  else if( this.tp == 'Media' )\n"
	"  {\n"
	"    if( this.attrs['backColor'] ) elStyle+='background-color: '+this.attrs['backColor']+'; ';\n"
	"    if( this.attrs['backImg'] )   elStyle+='background-image: url(\\'/"MOD_ID"'+this.addr+'?com=res&val='+this.attrs['backImg']+'\\'); ';\n"
	"    elStyle+='border-style: solid; border-width: '+this.attrs['bordWidth']+'px; ';\n"
	"    elStyle+='overflow: hidden; ';\n"
	"    if( this.attrs['bordColor'] ) elStyle+='border-color: '+this.attrs['bordColor']+'; ';\n"
	"    var medObj = this.place.childNodes[0];\n"
	"    if( !medObj )\n"
	"    {\n"
	"      medObj = document.createElement('img');\n"
	"      this.place.appendChild(medObj);\n"
	"    }\n"
	"    medObj.src = '/"MOD_ID"'+this.addr+'?com=res&val='+this.attrs['src'];\n"
	"    if( this.attrs['fit'] == 1 )\n"
	"    {\n"
	"      medObj.width = this.attrs['geomW'];\n"
	"      medObj.height = this.attrs['geomH'];\n"
	"    }\n"
	"  }\n"
	"  else if( this.tp == 'FormEl' )\n"
	"  {\n"
	"    var elTp = parseInt(this.attrs['elType']);\n"
	"    if( this.elTp && this.elTp != elTp )\n"
	"      while(this.place.childNodes.length) this.place.removeChild(this.place.childNodes[0]);\n"
	"    this.elTp = elTp;\n"
	"    switch(elTp)\n"
	"    {\n"
	"      case 0:\n"
	"        var formObj = this.place.childNodes[0];\n"
	"        if( !formObj )\n"
	"        {\n"
	"          formObj = document.createElement('input');\n"
	"          this.place.appendChild(formObj);\n"
	"        }\n"
	"        formObj.setAttribute('style','width: '+this.attrs['geomW']+'px; ');\n"
	"        formObj.type='text'; formObj.value=this.attrs['value'];\n"
	"        break;\n"
	"      case 1:\n"
 	"        var formObj = this.place.childNodes[0];\n"
	"        if( !formObj )\n"
	"        {\n"
	"          formObj = document.createElement('textarea');\n"
	"          this.place.appendChild(formObj);\n"
	"        }\n" 
	"        formObj.setAttribute('style','width: '+this.attrs['geomW']+'px; height: '+this.attrs['geomH']+'px; ');\n"
	"        formObj.textContent = this.attrs['value'];\n"
	"        break;\n"
	"      case 2:\n"
 	"        var formObj = this.place.childNodes[0];\n"
 	"        var textObj = this.place.childNodes[1];\n"
	"        if( !formObj )\n"
	"        {\n"
	"          formObj = document.createElement('input');\n"
	"          this.place.appendChild(formObj);\n"
	"        }\n" 
 	"        if( !textObj )\n"
	"        {\n"
	"          textObj = document.createTextNode('');\n"
	"          this.place.appendChild(textObj);\n"
	"        }\n"  
	"        formObj.type='checkbox'; formObj.checked=parseInt(this.attrs['value']);\n"
	"        textObj.nodeValue = this.attrs['name'];\n"
	"        break;\n"
	"      case 3:\n"
 	"        var formObj = this.place.childNodes[0];\n"
 	"        if( !formObj )\n"
	"        {\n"
	"          formObj = document.createElement('button');\n"
	"          this.place.appendChild(formObj);\n"
	"          formObj.wdgLnk = this;\n"
	"        }\n"
	"        else while(formObj.childNodes.length) formObj.removeChild(formObj.childNodes[0]);\n"
	"        formObj.setAttribute('style','width: '+this.attrs['geomW']+'px; ');\n"
	"        formObj.onclick = function()\n"
	"        {\n"
	"          var atrs = new Object();\n"
	"          atrs.event = 'ws_BtPress';\n"
	"          setWAttrs(this.wdgLnk.addr,atrs);\n"
        "        };\n"
	"        if( this.attrs['color'] )\n"
	"          formObj.setAttribute('style',formObj.getAttribute('style')+'background-color: '+this.attrs['color']+'; ');\n"
	"        if( this.attrs['img'] )\n"
	"        {\n"
	"          var btImg = document.createElement('IMG');\n"
	"          btImg.src = '/"MOD_ID"'+this.addr+'?com=res&val='+this.attrs['img'];\n"
	"          formObj.appendChild(btImg);\n"
	"        }\n"
	"        formObj.appendChild(document.createTextNode(this.attrs['name']));\n"
	"        break;\n"
	"      case 4: case 5:\n"
  	"        var formObj = this.place.childNodes[0];\n"
 	"        if( !formObj )\n"
	"        {\n"
	"          formObj = document.createElement('select');\n"
	"          this.place.appendChild(formObj);\n"
	"        }\n"
	"        else while(formObj.childNodes.length) formObj.removeChild(formObj.childNodes[0]);\n"
 	"        if( elTp == 4 )\n"
	"	   formObj.setAttribute('style','width: '+this.attrs['geomW']+'px; ');\n"
	"        else\n"
	"        {\n"
	"          formObj.setAttribute('style','width: '+this.attrs['geomW']+'px; height: '+this.attrs['geomH']+'px; ');\n"
	"          formObj.setAttribute('size',100);\n"
	"        }\n"
	"        var selVal = this.attrs['value'];\n"
	"        var elLst = this.attrs['items'].split('\\n');\n"
	"        var optEl;\n"
	"        var selOk = false;\n"	
	"        for( var i = 0; i < elLst.length; i++ )\n"
	"        {\n"
	"          optEl = document.createElement('option');\n"
	"          optEl.textContent = elLst[i];\n"
	"          if( selVal == elLst[i] ) optEl.selected=selOk=true;\n"
	"          formObj.appendChild(optEl);\n"
	"        }\n"
	"        if( !selOk && elTp == 4 )\n"
	"        {\n" 
	"          optEl = document.createElement('option');\n"
	"          optEl.textContent = selVal;\n"
	"          optEl.selected = true;\n"
	"          formObj.appendChild(optEl);\n"
	"        }\n"
	"        break;\n"
	"    }\n"
	"  }\n"
	"  this.place.setAttribute('style',elStyle);\n"
	"  if( el_addr ) return;\n"
	"  rNode = servGet(this.addr,'com=chlds'); if( !rNode ) return;\n"
	"  for( var i = 0; i < rNode.childNodes.length; i++ )\n"
	"  {\n"
	"    var chEl = rNode.childNodes[i].getAttribute('id');\n"
	"    var wdgO = new pwDescr(this.addr+'/wdg_'+chEl,false,this);\n"	
	"    wdgO.place = document.createElement('div');\n"	
	"    wdgO.makeEl(false,'');\n"
	"    this.place.appendChild(wdgO.place);\n"
	"    this.wdgs[chEl] = wdgO;\n"
	"  }\n"
	"}\n"
	"function pwDescr( pgAddr, pg, parent )\n"
	"{\n"
	"  this.addr = pgAddr;\n"
	"  this.pages = new Object();\n"
	"  this.wdgs = new Object();\n"
	"  this.attrs = new Object();\n"
	"  this.pg = pg;\n"
	"  this.tp = null;\n"
	"  this.parent = parent;\n"
	"  this.place = null;\n"
	"  this.callPage = callPage;\n"
	"  this.findOpenPage = findOpenPage;\n"
	"  this.makeEl = makeEl;\n"
	"}\n"
        "/***************************************************\n"
	" * makeUI                                          *\n"
	" ***************************************************/\n"
        "function makeUI()\n"
	"{\n"
	"  //- Get open pages list -\n"
 	"  var pgNode = servGet(location.pathname,'com=pgOpen&tm='+tmCnt,true)\n"
        "  if( pgNode )\n"
	"  {\n"
	"    var pags = pgNode.getElementsByTagName('pg');\n"
 	"    for( var i = 0; i < pags.length; i++ )\n"
	"    {\n"
	"      var pgnm;\n"
	"      var upW = new Array();\n"
	"      for( var j = 0; j < pags[i].childNodes.length; j++ )\n"
	"        if( pags[i].childNodes[j].nodeName == 'el' ) upW.push(pags[i].childNodes[j].textContent);\n"
	"        else pgnm = pags[i].childNodes[j].textContent;\n"
	"      masterPage.callPage(pgnm,null,upW);\n"
	"    }\n"
	"    tmCnt = parseInt(pgNode.getAttribute('tm'));\n"
	"  }\n"
	"  setTimeout(makeUI,1000);\n"
	"}\n"
	"/***************************************************\n"
	" * Main start code                                 *\n"
	" ***************************************************/\n"
	"tmCnt = 0;\n"
	"pgCache = new Object();\n"
	"masterPage = new pwDescr('',true);\n"
	"setTimeout(makeUI,10);\n";
}

TWEB::~TWEB()
{

}

string TWEB::modInfo( const string &name )
{
    if( name == "SubType" ) return SUB_TYPE;
    else return TModule::modInfo(name);
}

void TWEB::modInfo( vector<string> &list )
{
    TModule::modInfo(list);
    list.push_back("SubType");
}

string TWEB::optDescr( )
{
    char buf[STR_BUF_LEN];

    snprintf(buf,sizeof(buf),_(
	"======================= The module <%s:%s> options =======================\n"
	"---------- Parameters of the module section <%s> in config file ----------\n\n"),
        "SessTimeLife <time>      Time of the sesion life, minutes (default 10).\n"
        "CSSTables    <CSS>       CSS for creating pages.\n\n",
	MOD_TYPE,MOD_ID,nodePath().c_str());

    return buf;
}

void TWEB::modLoad( )
{
    //- Load parameters from command line -
    int next_opt;
    char *short_opt="h";
    struct option long_opt[] =
    {
	{"help"      ,0,NULL,'h'},
	{NULL        ,0,NULL,0  }
    };

    optind=opterr=0;
    do
    {
	next_opt=getopt_long(SYS->argc,(char * const *)SYS->argv,short_opt,long_opt,NULL);
	switch(next_opt)
	{
	    case 'h': fprintf(stdout,optDescr().c_str()); break;
	    case -1 : break;
	}
    } while(next_opt != -1);
    
    //- Load parameters from config file -
    m_t_auth = atoi( TBDS::genDBGet(nodePath()+"SessTimeLife",TSYS::int2str(m_t_auth)).c_str() );
    m_CSStables = TBDS::genDBGet(nodePath()+"CSSTables",m_CSStables);
}

void TWEB::modSave( )
{
    TBDS::genDBSet(nodePath()+"SessTimeLife",TSYS::int2str(m_t_auth));
    TBDS::genDBSet(nodePath()+"CSSTables",m_CSStables);
}

string TWEB::httpHead( const string &rcode, int cln, const string &cnt_tp, const string &addattr )
{
    return  "HTTP/1.0 "+rcode+"\n"
        "Server: "+PACKAGE_STRING+"\n"
        "Accept-Ranges: bytes\n"
        "Content-Length: "+TSYS::int2str(cln)+"\n"
        "Connection: close\n"
        "Content-type: "+cnt_tp+"\n"
        "Charset="+Mess->charset()+"\n"+addattr+"\n";										
}

string TWEB::pgHead( string head_els )
{
    string shead =
    	"<?xml version='1.0' ?>\n"
	"<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN'\n"
	"'DTD/xhtml1-transitional.dtd'>\n"
	"<html xmlns='http://www.w3.org/1999/xhtml'>\n"
	"<head>\n"
        "  <meta http-equiv='Content-Type' content='text/html; charset="+Mess->charset()+"'/>\n"
	"  <link rel='shortcut icon' href='/"MOD_ID"/ico' type='image' />\n"
	"  <title>"+PACKAGE_NAME+". "+_(MOD_NAME)+"</title>\n"
	"  <style type='text/css'>\n"+m_CSStables+"</style>\n"+
	head_els+
        "</head>\n"
        "<body bgcolor='#818181' text='#000000' link='#3366ff' vlink='#339999' alink='#33ccff'>\n";	
	
    return shead;
}

string TWEB::pgTail( )
{
    return
        "</body>\n"
        "</html>";
}

void TWEB::HttpGet( const string &url, string &page, const string &sender, vector<string> &vars )
{
    map< string, string >::iterator prmEl;
    SSess ses(TSYS::strDecode(url,TSYS::HttpURL),page,sender,vars,"");
    ses.page = pgHead();

    try
    {
        string zero_lev = TSYS::pathLev(ses.url,0);
	//- Get about module page -
	if( zero_lev == "about" )       getAbout(ses);
	//- Get module icon -	
	else if( zero_lev == "ico" )
	{
            string itp;
            ses.page=TUIS::icoGet("UI."MOD_ID,&itp);
            page = httpHead("200 OK",ses.page.size(),string("image/")+itp)+ses.page;
            return;
	}
        else
        {
            sesCheck(ses);
	    //- Auth dialog preparing -	    
            if( !ses.user.size() )
	    {
		ses.page = ses.page+"<h1 class='head'>"+PACKAGE_NAME+". "+_(MOD_NAME)+"</h1>\n<hr/><br/>\n";
		getAuth( ses );
	    }
	    //- Main VCA page JavaScript programm text request process -
	    else if( zero_lev == "VCA.js" )
	    {
        	page = httpHead("200 OK",m_VCAjs.size(),"application/x-javascript")+m_VCAjs;
        	return;	    
	    }
	    //- Session select or new session for project creation -	    
            else if( zero_lev.empty() )
            {
		bool sesPrjOk = false;
		ses.page = ses.page+
		    "<h1 class='head'>"+PACKAGE_NAME+". "+_(MOD_NAME)+"</h1>\n<hr/><br/>\n"
		    "<center><table class='page_auth'>\n";
		//-- Get present sessions list --
		XMLNode req("get");
		req.setAttr("path","/%2fses%2fses");
		cntrIfCmd(req,ses);
		if( req.childSize() )
		{
		    ses.page = ses.page+
			"<tr><td><b>"+_("Connect to opened session")+"</b></td></tr>\n"
	        	"<tr class='content'><td align='center'>\n"
			"<table border='0'>\n";
		    for( int i_ch = 0; i_ch < req.childSize(); i_ch++ )
			ses.page += "<tr><td><a href='/"MOD_ID"/ses_"+req.childGet(i_ch)->text()+"'>"+
			    req.childGet(i_ch)->text()+"</a></td></tr>";
    		    ses.page += "</table></td></tr>\n";
		    sesPrjOk = true;
		}
		//-- Get present projects list --
		req.clear()->setAttr("path","/%2fprm%2fcfg%2fprj");
		cntrIfCmd(req,ses);
		if( req.childSize() )
		{
		    ses.page = ses.page +
			"<tr><td><b>"+_("Create new session for present project")+"</b></td></tr>\n"
                	"<tr class='content'><td align='center'>\n"
	        	"<table border='0'>\n";
		    for( int i_ch = 0; i_ch < req.childSize(); i_ch++ )
			ses.page += "<tr><td><a href='/"MOD_ID"/prj_"+req.childGet(i_ch)->attr("id")+"'>"+
			    req.childGet(i_ch)->text()+"</a></td></tr>";
		    ses.page += "</table></td></tr>\n";
		    sesPrjOk = true;
		}
		ses.page += "</table></center>";		
		if( !sesPrjOk )	messPost(ses.page,nodePath(),_("No one sessions and projects VCA engine present!"),TWEB::Warning);
    	    }
	    //- New session creation -	    
	    else if( zero_lev.size() > 4 && zero_lev.substr(0,4) == "prj_" )
	    {
		XMLNode req("connect");
		req.setAttr("path","/%2fserv%2f0")->setAttr("prj",zero_lev.substr(4));
		if( cntrIfCmd(req,ses) )
		    messPost(ses.page,req.attr("mcat").c_str(),req.text().c_str(),TWEB::Error);
		else
		    ses.page = pgHead("<META HTTP-EQUIV='Refresh' CONTENT='0; URL=/"MOD_ID"/ses_"+req.attr("sess")+"'/>")+
			"<center>Open new session '"+req.attr("sess")+"' for project: '"+zero_lev.substr(4)+"'</center>\n<br/>";
	    }
	    //- Main session page data prepare -	    
	    else if( zero_lev.size() > 4 && zero_lev.substr(0,4) == "ses_" )
	    {
		string first_lev = TSYS::pathLev(ses.url,1);
		prmEl = ses.prm.find("com");
		string wp_com = (prmEl!=ses.prm.end()) ? prmEl->second : "";
		if( wp_com.empty() )	ses.page = pgHead("<SCRIPT type='text/javascript' src='/"MOD_ID"/VCA.js'/>");	
		//-- Session/projects icon --
		else if( wp_com == "ico" )
		{
		    XMLNode req("get");
	    	    req.setAttr("path",ses.url+"/%2fico");
		    cntrIfCmd(req,ses);
		    ses.page = TSYS::strDecode(req.text(),TSYS::base64);
        	    page = httpHead("200 OK",ses.page.size(),"image/png")+ses.page;
		    return;
		}
	    	//- Get open pages list -		
		else if( wp_com == "pgOpen" && first_lev.empty() )
		{
		    prmEl = ses.prm.find("tm");
		    XMLNode req("openlist");
		    req.setAttr("path",ses.url+"/%2fserv%2f0")->
			setAttr("tm",(prmEl!=ses.prm.end())?prmEl->second:"0");
	    	    cntrIfCmd(req,ses);
		    ses.page = req.save();
        	    page = httpHead("200 OK",ses.page.size(),"text/xml")+ses.page;
		    return;
		}
		//- Page and widget attributes request -
		else if( wp_com == "attrs" )
		{
		    prmEl = ses.prm.find("tm");
		    XMLNode req("get");
		    req.setAttr("path",ses.url+"/%2fserv%2f0")->
			setAttr("tm",(prmEl!=ses.prm.end())?prmEl->second:"0");
		    cntrIfCmd(req,ses);
		    ses.page = req.save();
        	    page = httpHead("200 OK",ses.page.size(),"text/xml")+ses.page;
		    return;
		}
		//- Resources request (images and other files) -
		else if( wp_com == "res" )
		{
		    prmEl = ses.prm.find("val");
		    if( prmEl != ses.prm.end() )
		    {
			XMLNode req("get");
			req.setAttr("path",ses.url+"/%2fwdg%2fres")->setAttr("id",prmEl->second);
			cntrIfCmd(req,ses);
			ses.page = TSYS::strDecode(req.text(),TSYS::base64);
        		page = httpHead("200 OK",ses.page.size(),req.attr("mime"))+ses.page;
		    } else page = httpHead("404 Not Found");
		    return;
		}
		//- Page or widget child widgets request -
		else if( wp_com == "chlds" )
		{
		    XMLNode req("get");
		    req.setAttr("path",ses.url+"/%2fwdg%2fcfg%2fpath")->setAttr("resLink","1");
	    	    if( !cntrIfCmd(req,ses) )
		    {
			req.clear()->setAttr("path",req.text()+"/%2finclwdg%2fwdg");
		        cntrIfCmd(req,ses);
		    }
		    ses.page = req.save();
        	    page = httpHead("200 OK",ses.page.size(),"text/xml")+ses.page;
		    return;
		}
		//- Widget root element identifier (primitive) -
		else if( wp_com == "root" )
		{
	    	    XMLNode req("get");
		    req.setAttr("path",ses.url+"/%2fwdg%2fcfg%2froot");
		    cntrIfCmd(req,ses);
		    ses.page = req.save();
        	    page = httpHead("200 OK",ses.page.size(),"text/xml")+ses.page;
		    return;		    
		}
		else
		{
		    mess_warning(nodePath().c_str(),_("Unknown command: %s."),wp_com.c_str());
		    ses.page = pgHead()+"<center>Call page/widget '"+ses.url+"' command: '"+wp_com+"'</center>\n<br/>";
		}
	    }
	    else mess_err(nodePath().c_str(),_("No permit request is received: '%s'"),zero_lev.c_str());
        }
    }catch(TError err)
    {
        ses.page = "Page <"+ses.url+"> error: "+err.mess;
        page = httpHead("404 Not Found",ses.page.size())+ses.page;
        return;
    }
    
    ses.page += pgTail();    
    page = httpHead("200 OK",ses.page.size())+ses.page;
}

void TWEB::getAbout( SSess &ses )
{
    ses.page = ses.page+"<center><table class='page_auth'>\n"
        "<TR><TD>"+PACKAGE+" "+VERSION+"</TD></TR>\n"
	"<TR class='content'><TD>\n"
	"<table border='0'>\n"
	"<TR><TD><font color='Blue'>"+_("Name: ")+"</font></TD><TD>OpenSCADA</TD></TR>\n"
	"<TR><TD><font color='Blue'>"+_("License: ")+"</font></TD><TD>GPL</TD></TR>\n"
        "<TR><TD><font color='Blue'>"+_("Author: ")+"</font></TD><TD>Roman Savochenko</TD></TR>\n"
        "</table>\n"
        "</TD></TR></table><br/>\n"
        "<table class='page_auth'>\n"
        "<TR><TD>"MOD_ID" "MOD_VERSION"</TD></TR>\n"
        "<TR class='content'><TD>\n"
        "<table border='0'>\n"
        "<TR><TD><font color='Blue'>"+_("Name: ")+"</font></TD><TD>"+_(MOD_NAME)+"</TD></TR>"
	"<TR><TD><font color='Blue'>"+_("Desription: ")+"</font></TD><TD>"+_(DESCRIPTION)+"</TD></TR>"
	"<TR><TD><font color='Blue'>"+_("License: ")+"</font></TD><TD>"+_(LICENSE)+"</TD></TR>"
        "<TR><TD><font color='Blue'>"+_("Author: ")+"</font></TD><TD>"+_(AUTORS)+"</TD></TR>"
        "</table>\n"
        "</TD></TR>\n</table><br/></center>\n";
}

void TWEB::getAuth( SSess &ses )
{
    ses.page = ses.page+"<center><table class='page_auth'>"
	"<tr><td><b>"+_("Enter to module")+"</b></td></tr>\n"
        "<tr class='content'> <td align='center'>\n"
        "<form method='post' action='/"MOD_ID+ses.url+"' enctype='multipart/form-data'>\n"
	"<table cellpadding='3'>\n"
	"<tr><td><b>"+_("User name")+"</b></td><td><input type='text' name='user' size='20'/></td></tr>\n"
	"<tr><td><b>"+_("Password")+"</b></td><td><input type='password' name='pass' size='20'/></td></tr>\n"
        "<tr><td colspan='2' align='center'><input type='submit' name='auth_enter' value='"+_("Enter")+"'/>\n"
        "<input type='reset' name='clean' value='"+_("Clean")+"'/>\n"
        "</td></tr></table>\n</form>\n"
        "</td></tr></table></center>\n";
}
																			  
string TWEB::getCookie( string name, vector<string> &vars )
{
    for( unsigned i_var = 0; i_var < vars.size(); i_var++)
        if( vars[i_var].substr(0, vars[i_var].find(":",0)) == "Cookie" )
        {
            int i_beg = vars[i_var].find(name+"=",0);
            if( i_beg == string::npos ) return "";
            i_beg += name.size()+1;
            return vars[i_var].substr(i_beg,vars[i_var].find(";",i_beg)-i_beg);
        }
    return "";
}

int TWEB::sesOpen( string name )
{
    int sess_id;
    ResAlloc res(m_res,true);

    //- Get free identifier -
    do{ sess_id = rand(); }
    while( sess_id == 0 || m_auth.find(sess_id) != m_auth.end() );
    
    //- Add new session authentification -
    m_auth[sess_id] = SAuth(name,time(NULL));
    
    return sess_id;
}

void TWEB::sesCheck( SSess &ses )
{
    time_t cur_tm = time(NULL);
    map< int, SAuth >::iterator authEl;
    
    ResAlloc res(m_res,false);    
    
    //- Check for close old sessions -
    if( cur_tm > lst_ses_chk+10 )
    {
	for( authEl = m_auth.begin(); authEl != m_auth.end(); )
	    if( cur_tm > authEl->second.t_auth+m_t_auth*60 )
		m_auth.erase(authEl++);
	    else authEl++;
	lst_ses_chk = cur_tm;
    }
    
    //- Check for session and close old sessions -
    authEl = m_auth.find(atoi(getCookie( "oscd_u_id", ses.vars ).c_str()));
    if( authEl != m_auth.end() )
    {
	ses.user = authEl->second.name;
	authEl->second.t_auth = cur_tm;
    }
}

void TWEB::HttpPost( const string &url, string &page, const string &sender, vector<string> &vars, const string &contein )
{
    map< string, string >::iterator cntEl;
    SSess ses(TSYS::strDecode(url,TSYS::HttpURL),page,sender,vars,contein);

    //- Check for autentification POST requests -
    if( ses.cnt.find("auth_enter") != ses.cnt.end() )
    {
	string pass;    
	if( (cntEl=ses.cnt.find("user")) != ses.cnt.end() )	ses.user = cntEl->second;
	if( (cntEl=ses.cnt.find("pass")) != ses.cnt.end() )	pass = cntEl->second;
	if( SYS->security().at().usrPresent(ses.user) && SYS->security().at().usrAt(ses.user).at().auth(pass) )
	{
	    ses.page = pgHead("<META HTTP-EQUIV='Refresh' CONTENT='0; URL=/"MOD_ID"/"+url+"'/>")+pgTail();
	    page=httpHead("200 OK",ses.page.size(),"text/html",
		"Set-Cookie: oscd_u_id="+TSYS::int2str(sesOpen(ses.user))+"; path=/;\n")+ses.page;
	    return;
	}
	ses.page = pgHead()+"<h1 class='head'>"+PACKAGE_NAME+". "+_(MOD_NAME)+"</h1>\n<hr/><br/>\n";
    	messPost(ses.page,nodePath(),_("Auth wrong! Retry please."),TWEB::Error);
        ses.page += "\n";
        getAuth( ses );
	ses.page += pgTail();	
	page = httpHead("200 OK",ses.page.size(),"text/html")+ses.page;		
        return;
    }
    //- Session check -
    sesCheck( ses );    
    if( !ses.user.size() || ses.cnt.find("auth_ch") != ses.cnt.end() )
    {
	ses.page = pgHead("<META HTTP-EQUIV='Refresh' CONTENT='0; URL="MOD_ID"/"+url+"'/>")+pgTail();
	page=httpHead("200 OK",ses.page.size(),"text/html",
	    "Set-Cookie: oscd_u_id=""; path=/;\n")+ses.page;
	return;
    }
 
    //- Commands process -
    cntEl = ses.prm.find("com");
    string wp_com = (cntEl!=ses.prm.end()) ? cntEl->second : ""; 
    if( wp_com == "attrs" )
    {
	XMLNode req("set");
	req.load(contein);
	req.setAttr("path",ses.url+"/%2fserv%2f0");
	cntrIfCmd(req,ses);
    }
    
    page = httpHead("200 OK",ses.page.size(),"text/html")+ses.page;
}

void TWEB::messPost( string &page, const string &cat, const string &mess, MessLev type )
{
    //- Put system message. -
    message(cat.c_str(), (type==Error) ? TMess::Error : 
			 (type==Warning) ? TMess::Warning : 
			 TMess::Info,"%s",mess.c_str());

    //- Prepare HTML messages -
    page = page+"<table border='2' width='40%' align='center'><tbody>\n";
    if(type == Warning )
        page = page+"<tr bgcolor='yellow'><td align='center'><b>Warning!</b></td></tr>\n";
    else if(type == Error )
        page = page+"<tr bgcolor='red'><td align='center'><b>Error!</b></td></tr>\n";
    else page = page+"<tr bgcolor='#9999ff'><td align='center'><b>Message!</b></td></tr>\n";
	page = page+"<tr bgcolor='#cccccc'> <td align='center'>"+TSYS::strEncode(mess,TSYS::Html)+"</td></tr>\n";
    page = page+"</tbody></table>\n";
}

int TWEB::cntrIfCmd( XMLNode &node, SSess &ses )
{
    node.setAttr("user",ses.user);
    node.setAttr("path","/UI/VCAEngine"+node.attr("path"));
    SYS->cntrCmd(&node);
    return atoi(node.attr("rez").c_str());
}

void TWEB::cntrCmdProc( XMLNode *opt )
{
    //- Get page info -
    if( opt->name() == "info" )
    {
        TUI::cntrCmdProc(opt);
        if(ctrMkNode("area",opt,1,"/prm/cfg",_("Module options")))
        {
            ctrMkNode("fld",opt,-1,"/prm/cfg/lf_tm",_("Life time of auth sesion(min)"),0660,"root","root",1,"tp","dec");
            ctrMkNode("fld",opt,-1,"/prm/cfg/CSS",_("CSS"),0660,"root","root",3,"tp","str","cols","90","rows","7");
            ctrMkNode("comm",opt,-1,"/prm/cfg/load",_("Load"),0660);
            ctrMkNode("comm",opt,-1,"/prm/cfg/save",_("Save"),0660);
        }
        ctrMkNode("fld",opt,-1,"/help/g_help",_("Options help"),0440,"root","root",3,"tp","str","cols","90","rows","5");
        return;
    }

    //- Process command to page -
    string a_path = opt->attr("path");
    if( a_path == "/prm/cfg/lf_tm" )
    {
        if( ctrChkNode(opt,"get",0660,"root","root",SEQ_RD) )   opt->setText(TSYS::int2str(m_t_auth));
        if( ctrChkNode(opt,"set",0660,"root","root",SEQ_WR) )   m_t_auth = atoi(opt->text().c_str());
    }
    else if( a_path == "/prm/cfg/CSS" )
    {
        if( ctrChkNode(opt,"get",0660,"root","root",SEQ_RD) )   opt->setText(m_CSStables);
        if( ctrChkNode(opt,"set",0660,"root","root",SEQ_WR) )   m_CSStables = opt->text();
    }
    else if( a_path == "/help/g_help" && ctrChkNode(opt,"get",0440) )   opt->setText(optDescr());
    else if( a_path == "/prm/cfg/load" && ctrChkNode(opt,"set",0660,"root","root",SEQ_WR) )  modLoad();
    else if( a_path == "/prm/cfg/save" && ctrChkNode(opt,"set",0660,"root","root",SEQ_WR) )  modSave();
    else TUI::cntrCmdProc(opt);
}		    

//*************************************************
//* SSess                                         *
//*************************************************
SSess::SSess( const string &iurl, const string &ipage, const string &isender,
	vector<string> &ivars, const string &icontent ) :
    url(iurl), page(ipage), sender(isender), vars(ivars), content(icontent)
{  
    //- URL parameters parse -
    int prmSep = iurl.find("?");
    if( prmSep != string::npos )
    {
	url = iurl.substr(0,prmSep);
	string prms = iurl.substr(prmSep+1);
	string sprm;
	for( int iprm = 0; (sprm=TSYS::strSepParse(prms,0,'&',&iprm)).size(); )
	{
	    prmSep = sprm.find("=");
	    if( prmSep == string::npos ) prm[sprm] = "true";
	    else prm[sprm.substr(0,prmSep)] = sprm.substr(prmSep+1);
	}
    }
    
    //- Content parse -
    int pos = 0, i_bnd;
    string boundary;
    char *c_bound = "boundary=";
    char *c_term = "\r\n";
    char *c_end = "--";
    char *c_fd = "Content-Disposition: form-data;";
    char *c_name = "name=\"";
    char *c_file = "filename=\"";

    for( int i_vr = 0; i_vr < vars.size(); i_vr++ )
        if( vars[i_vr].substr(0,vars[i_vr].find(":",0)) == "Content-Type" )
	{
	    int pos = vars[i_vr].find(c_bound,0)+strlen(c_bound);
            boundary = vars[i_vr].substr(pos,vars[i_vr].size()-pos);
        }
    if( !boundary.size() ) return;

    while(true)
    {
	pos = content.find(boundary,pos);
        if( pos == string::npos || content.substr(pos+boundary.size(),2) == "--" ) break;
        pos += boundary.size()+strlen(c_term);
        string c_head = content.substr(pos, content.find(c_term,pos)-pos);
        if( c_head.find(c_fd,0) == string::npos ) continue;
						
        //-- Get name --
        i_bnd = c_head.find(c_name,0)+strlen(c_name);
        string c_name = c_head.substr(i_bnd,c_head.find("\"",i_bnd)-i_bnd);
        i_bnd = c_head.find(c_file,0);
        if( i_bnd == string::npos )
        {
    	    //--- Get value ---
            pos += c_head.size()+(2*strlen(c_term));
            if(pos >= content.size()) break;
            string c_val  = content.substr(pos, content.find(string(c_term)+c_end+boundary,pos)-pos);
	    cnt[c_name] = c_val;
        }
        else
        {
            i_bnd += strlen(c_file);
	    cnt[c_name] = c_head.substr(i_bnd,c_head.find("\"",i_bnd)-i_bnd);
        }
    }
}
						    