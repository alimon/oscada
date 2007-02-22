
//OpenSCADA system module UI.VCSEngine file: libwidgbase.cpp
/***************************************************************************
 *   Copyright (C) 2006 by Roman Savochenko
 *   rom_as@diyaorg.dp.ua
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 ***************************************************************************/

#include <tsys.h>

#include "widget.h"
#include "vcaengine.h"
#include "origwidg.h"

using namespace VCA;

//==================================
//=== Elementary figures element ===
//==================================
OrigElFigure::OrigElFigure( const string &iid, bool lib_loc ) : Widget(iid,lib_loc)
{

}

void OrigElFigure::attrProc( Widget *uwdg, bool init )
{
    Widget::attrProc(uwdg,init);
    if(init)
    {
	uwdg->attrAdd( new TFld("lineWdth",_("Line:width"),TFld::Integer,TFld::NoFlag,"2","1","0;99") );
	uwdg->attrAdd( new TFld("lineClr",_("Line:color"),TFld::String,Attr::Color,"20","#000000") );
	uwdg->attrAdd( new TFld("lineDecor",_("Line:decorate"),TFld::Integer,TFld::Selected,"1","0","0;1",
	    _("No decor;Pipe")) );
	uwdg->attrAdd( new TFld("bordWdth",_("Border:width"),TFld::Integer,TFld::NoFlag,"2","0","0;99") );
	uwdg->attrAdd( new TFld("bordClr",_("Border:color"),TFld::String,Attr::Color,"20","#000000") );
	uwdg->attrAdd( new TFld("backgClr",_("Background:color"),TFld::String,Attr::Color,"20","") );
	uwdg->attrAdd( new TFld("backgImg",_("Background:image"),TFld::String,Attr::Image,"20","") );
	uwdg->attrAdd( new TFld("arrowBeginType",_("Arrow begin:type"),TFld::Integer,TFld::Selected,"2","0","0;1;2;3;4;5;6;7;8",
	    _("No arrow;Arrow;Arc arrow;Line arrow;Two arrow;Rectangle;Rhomb;Cyrcle;Size line")) );
	uwdg->attrAdd( new TFld("arrowBeginWidth",_("Arrow begin:width"),TFld::Integer,TFld::NoFlag,"2","0","0;99") );
	uwdg->attrAdd( new TFld("arrowBeginHeight",_("Arrow begin:height"),TFld::Integer,TFld::NoFlag,"2","0","0;99") );
	uwdg->attrAdd( new TFld("arrowEndType",_("Arrow end:type"),TFld::Integer,TFld::Selected,"2","0","0;1;2;3;4;5;6;7;8",
	    _("No arrow;Arrow;Arc arrow;Line arrow;Two arrow;Rectangle;Rhomb;Cyrcle;Size line")) );	
	uwdg->attrAdd( new TFld("arrowEndWidth",_("Arrow end:width"),TFld::Integer,TFld::NoFlag,"2","0","0;99") );
	uwdg->attrAdd( new TFld("arrowEndHeight",_("Arrow end:height"),TFld::Integer,TFld::NoFlag,"2","0","0;99") );	    
	//Elements: line, arc, besie, grad(line,biline, radial, square decLine, atForm)
	uwdg->attrAdd( new TFld("elLst",_("Element's list"),TFld::String,TFld::NoFlag,"300","") );
	//Next is dynamic created Element's points attributes
    }
}

//====================
//=== Form element ===
//====================
OrigFormEl::OrigFormEl( const string &iid, bool lib_loc ) : Widget(iid,lib_loc)
{

}

void OrigFormEl::attrProc( Widget *uwdg, bool init )
{
    Widget::attrProc(uwdg,init);
    if(init)
    {
	uwdg->attrAdd( new TFld("elType",_("Element type"),TFld::Integer,TFld::Selected,"2","0","0;1;2;3;4;5",
	    _("Line edit;Text edit;Chek box;Button;Combo box;List")) );
	//Next is dynamic created individual elements attributes    
    }
}

//====================
//=== Text element ===
//==================== 
OrigText::OrigText( const string &iid, bool lib_loc ) : Widget(iid,lib_loc)
{

}

void OrigText::attrProc( Widget *uwdg, bool init )
{
    Widget::attrProc(uwdg,init);
    if(init)
    {
	uwdg->attrAdd( new TFld("text",_("Text"),TFld::String,TFld::NoFlag,"1000","Text") );
	uwdg->attrAdd( new TFld("font",_("Font:full"),TFld::String,TFld::NoFlag,"50","Arial 11") );
	uwdg->attrAdd( new TFld("fontFamily",_("Font:family"),TFld::String,TFld::NoFlag,"10","Arial") );
	uwdg->attrAdd( new TFld("fontSize",_("Font:size"),TFld::Integer,TFld::NoFlag,"2","11") );
	uwdg->attrAdd( new TFld("fontBold",_("Font:bold"),TFld::Boolean,TFld::NoFlag,"1","0") );
	uwdg->attrAdd( new TFld("fontItalic",_("Font:italic"),TFld::Boolean,TFld::NoFlag,"1","0") );
	uwdg->attrAdd( new TFld("fontUnderline",_("Font:underline"),TFld::Boolean,TFld::NoFlag,"1","0") );
	uwdg->attrAdd( new TFld("fontStrikeout",_("Font:strikeout"),TFld::Boolean,TFld::NoFlag,"1","0") );
	uwdg->attrAdd( new TFld("color",_("Color"),TFld::String,Attr::Color,"20","#000000") );
	uwdg->attrAdd( new TFld("orient",_("Orientation angle"),TFld::Integer,TFld::NoFlag,"3","0","-180;180") );
	uwdg->attrAdd( new TFld("wordWrap",_("Word wrap"),TFld::Boolean,TFld::NoFlag,"1","1") );
	uwdg->attrAdd( new TFld("alignment",_("Alignment"),TFld::Integer,TFld::Selected,"1","0","0;1;2;3",
	    _("Left;Right;Center;Justify")) );
    }
}

//=====================
//=== Media element ===
//=====================
OrigMedia::OrigMedia( const string &iid, bool lib_loc ) : Widget(iid,lib_loc)
{

}

void OrigMedia::attrProc( Widget *uwdg, bool init )
{
    Widget::attrProc(uwdg,init);
    if(init)
    {
	uwdg->attrAdd( new TFld("src",_("Source"),TFld::String,TFld::NoFlag,"50","") );
	uwdg->attrAdd( new TFld("play",_("Media play"),TFld::Boolean,TFld::NoFlag,"1","0") );
	uwdg->attrAdd( new TFld("cycle",_("Media cyclic play"),TFld::Boolean,TFld::NoFlag,"1","0") );
    }
}

//=====================
//=== Trend element ===
//=====================
OrigTrend::OrigTrend( const string &iid, bool lib_loc ) : Widget(iid,lib_loc)
{

}

void OrigTrend::attrProc( Widget *uwdg, bool init )
{
    Widget::attrProc(uwdg,init);
    if(init)
    {
	uwdg->attrAdd( new TFld("type",_("Type"),TFld::Integer,TFld::Selected,"1","0","0;1",
	    _("Tradition;Cyrcle")) );
	uwdg->attrAdd( new TFld("widthTime",_("Width time (ms)"),TFld::Integer,TFld::Selected,"6","60000","10;360000") );
	uwdg->attrAdd( new TFld("number",_("Number"),TFld::Integer,TFld::Selected,"1","0","1;2;3;4;5;6;7;8","1;2;3;4;5;6;7;8") );
	//Next is dynamic created individual trend's item attributes
    }
}

//========================
//=== Protocol element ===
//========================
OrigProtocol::OrigProtocol( const string &iid, bool lib_loc ) : Widget(iid,lib_loc)
{

}

void OrigProtocol::attrProc( Widget *uwdg, bool init )
{
    Widget::attrProc(uwdg,init);
    if(init)
    {
	
    
    }
}

//=============================
//=== User function element ===
//=============================
OrigFunction::OrigFunction( const string &iid, bool lib_loc ) : Widget(iid,lib_loc)
{

}

void OrigFunction::attrProc( Widget *uwdg, bool init )
{
    Widget::attrProc(uwdg,init);
    if(init)
    {
    
	    
    }
}

//====================
//=== User element ===
//====================
OrigUserEl::OrigUserEl( const string &iid, bool lib_loc ) : Widget(iid,lib_loc,"",true)
{

}

OrigUserEl::~OrigUserEl( )
{

}

void OrigUserEl::attrProc( Widget *uwdg, bool init )
{
    Widget::attrProc(uwdg,init);
    if(init)
    {
	uwdg->attrAdd( new TFld("backColor",_("Background:color"),TFld::String,Attr::Color,"","#FFFFFF") );
	uwdg->attrAdd( new TFld("backImg",_("Background:image"),TFld::String,Attr::Image,"","") );
	//Next is user created attributes
    }
}

/*void OrigUserEl::cntrCmdProc( XMLNode *opt )
{

}*/

//======================================================
//=== The association line element for User elements ===
//======================================================
OrigAssocLine::OrigAssocLine( const string &iid, bool lib_loc ) : Widget(iid,lib_loc)
{

}

void OrigAssocLine::attrProc( Widget *uwdg, bool init )
{
    Widget::attrProc(uwdg,init);
    if(init)
    {
	uwdg->attrAdd( new TFld("out",_("Output"),TFld::String,TFld::NoFlag,"50","") );	
	uwdg->attrAdd( new TFld("in",_("Input"),TFld::String,TFld::NoFlag,"50","") );
	uwdg->attrAdd( new TFld("lineWdth",_("Line:width"),TFld::Integer,TFld::NoFlag,"2","1","0;99") );
        uwdg->attrAdd( new TFld("lineClr",_("Line:color"),TFld::String,Attr::Color,"20","#000000") );
	uwdg->attrAdd( new TFld("lineSquare",_("Line:square angle"),TFld::Boolean,TFld::NoFlag,"1","0") );
	//Next is dynamic created internal points
    }
}