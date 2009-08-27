
//OpenSCADA system module Special.FLibComplex1 file: libcompl1.h
/***************************************************************************
 *   Copyright (C) 2005-2008 by Roman Savochenko                           *
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

#ifndef LIBCOMPL1_H
#define LIBCOMPL1_H

#include <math.h>

#include <string>
#include <vector>

#include <tfunction.h>

#include "statfunc.h"

using std::string;
using std::vector;

namespace FLibComplex1
{

//*********************************************************************
//* Complex1 functions:                                               *

//*********************************************************************
//* DigitBlock                                                        *
//* Support digital blocks: 0-Open;1-Start;2-Enable;3-Norm;4-Block    *
//*********************************************************************
class DigitBlock : public TFunction
{
    public:
	DigitBlock( ) : TFunction("digitBlock")
	{
	    ioAdd( new IO("cmdOpen",_("Command \"Open\""),IO::Boolean,IO::Output,"0") );
	    ioAdd( new IO("cmdClose",_("Command \"Close\""),IO::Boolean,IO::Output,"0") );
	    ioAdd( new IO("cmdStop",_("Command \"Stop\""),IO::Boolean,IO::Output,"0") );
	    ioAdd( new IO("stOpen",_("Stat \"Opened\""),IO::Boolean,IO::Default,"0") );
	    ioAdd( new IO("stClose",_("Stat \"Closed\""),IO::Boolean,IO::Default,"0") );
	    ioAdd( new IO("tCmd",_("Command hold time (s)"),IO::Integer,IO::Default,"5") );
	    ioAdd( new IO("frq",_("Calc period (ms)"),IO::Integer,IO::Default,"1000") );
	    ioAdd( new IO("w_tm",_("Process command clock"),IO::Real,IO::Output,"0",true) );
	    ioAdd( new IO("last_cmd",_("Last command"),IO::Integer,IO::Output,"0",true) );
	}

	string name( )	{ return _("Digital block"); }
	string descr( )	{ return _("Digital assemble block."); }	//!!!! make full description

	void calc( TValFunc *val )
	{
	    bool set = false;
	
	    if(val->getB(0) && val->getI(8)!=1)	{ val->setI(8,1); set = true; }
	    if(val->getB(1) && val->getI(8)!=2)	{ val->setI(8,2); set = true; }
	    if(val->getB(2) && val->getI(8)!=3)	{ val->setI(8,3); set = true; }
	    if(set && val->getI(5)>0)	val->setR(7,val->getI(5));
	    if(val->getR(7)>0)	val->setR(7,val->getR(7)-0.001*val->getI(6));
	    else
	    {
		val->setR(7,0);
		if(val->getI(5)>0)
		{
		    if(val->getI(8)==1)	val->setB(0,0);
		    if(val->getI(8)==2) val->setB(1,0);
		    if(val->getI(8)==3) val->setB(2,0);
		    val->setI(8,0);
		}
	    }
	}
};

//*********************************************************************
//* Simple summator                                                   *
//* Formula: out=in1_1*in1_2+in2_1*in2_2+in3_1*in3_2+in4_1*in4_2+     *
//*              in5_1*in5_2+in6_1*in6_2+in7_1*in7_2+in8_1*in8_2;     *
//*********************************************************************
class Sum : public TFunction
{
    public:
	Sum( ) : TFunction("sum")
	{
	    char id_buf[10], nm_buf[20];

	    ioAdd( new IO("out",_("Output"),IO::Real,IO::Return,"1") );
	    for( int i_in=1; i_in <= 8; i_in++ )
	    {
		snprintf(id_buf,sizeof(id_buf),"in%d_1",i_in);
		snprintf(nm_buf,sizeof(nm_buf),_("Input %d.1"),i_in);
		ioAdd( new IO(id_buf,nm_buf,IO::Real,IO::Default,"0") );
		snprintf(id_buf,sizeof(id_buf),"in%d_2",i_in);
		snprintf(nm_buf,sizeof(nm_buf),_("Input %d.2"),i_in);
		ioAdd( new IO(id_buf,nm_buf,IO::Real,IO::Default,"0") );
	    }
	}

	string name( )	{ return _("Simple summator"); }
	string descr( )
	{
	    return _("Simple summator per formula:\n"
		"out=in1_1*in1_2+in2_1*in2_2+in3_1*in3_2+in4_1*in4_2+\n"
		"    in5_1*in5_2+in6_1*in6_2+in7_1*in7_2+in8_1*in8_2;"); 
	}

	void calc( TValFunc *val )
	{
	    val->setR(0,
		val->getR(1)*val->getR(2)+
		val->getR(3)*val->getR(4)+
		val->getR(5)*val->getR(6)+
		val->getR(7)*val->getR(8)+
		val->getR(9)*val->getR(10)+
		val->getR(11)*val->getR(12)+
		val->getR(13)*val->getR(14)+
		val->getR(15)*val->getR(16));
	}
};

//*********************************************************************
//* Simple moltiplicator                                              *
//* Formula: out=(in1_1*in1_2*in1_3*in1_4*in1_5*in1_6)/               *
//*              (in2_1*in2_2*in2_3*in2_4);                           *
//*********************************************************************
class Mult : public TFunction
{
    public:
	Mult( ) : TFunction("mult")
	{
	    ioAdd( new IO("out",_("Output"),IO::Real,IO::Return,"1") );
	    for( int i_c = 1; i_c <= 6; i_c++ )
		ioAdd( new IO(("in1_"+TSYS::int2str(i_c)).c_str(),(_("Input 1.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in2_"+TSYS::int2str(i_c)).c_str(),(_("Input 2.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	}

	string name( )	{ return _("Simple multiplicator"); }
	string descr( )	
	{
	    return _("Simple moltiplicator per formula:\n"
		"out=(in1_1*in1_2*in1_3*in1_4*in1_5*in1_6)/(in2_1*in2_2*in2_3*in2_4);");
	}

	void calc( TValFunc *val )
	{
	    val->setR(0,
		(val->getR(1)*val->getR(2)*val->getR(3)*val->getR(4)*val->getR(5)*val->getR(6))/
		(val->getR(7)*val->getR(8)*val->getR(9)*val->getR(10)));
	}
};

//*********************************************************************
//* Multiplicator+divider                                             *
//* Formula: out=(in1_1*in1_2*in1_3*in1_4*in1_5*(in2_1*in2_2*in2_3*in2_4*in2_5+
//*              (in3_1*in3_2*in3_3*in3_4*in3_5)/(in4_1*in4_2*in4_3*in4_4*in4_5)))
//*********************************************************************
class MultDiv : public TFunction
{
    public:
	MultDiv( ) : TFunction("multDiv")
	{
	    ioAdd( new IO("out",_("Output"),IO::Real,IO::Return,"0") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in1_"+TSYS::int2str(i_c)).c_str(),(_("Input 1.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in2_"+TSYS::int2str(i_c)).c_str(),(_("Input 2.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in3_"+TSYS::int2str(i_c)).c_str(),(_("Input 3.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in4_"+TSYS::int2str(i_c)).c_str(),(_("Input 4.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	}

	string name( )	{ return _("Multiplicator+divider"); }
	string descr( )
	{
	    return _("Multiplicator+divider per formula:\n"
		"out=in1_1*in1_2*in1_3*in1_4*in1_5*(in2_1*in2_2*in2_3*in2_4*in2_5+\n"
		"          (in3_1*in3_2*in3_3*in3_4*in3_5)/(in4_1*in4_2*in4_3*in4_4*in4_5));");
	}

	void calc( TValFunc *val )
	{
	    double tmp1 = val->getR(16)*val->getR(17)*val->getR(18)*val->getR(19)*val->getR(20);
	    double tmp2 = val->getR(1)*val->getR(2)*val->getR(3)*val->getR(4)*val->getR(5);
	    double tmp3 = val->getR(6)*val->getR(7)*val->getR(8)*val->getR(9)*val->getR(10);
	    double tmp4 = val->getR(11)*val->getR(12)*val->getR(13)*val->getR(14)*val->getR(15);
	    val->setR(0,tmp2*(tmp3+tmp4/tmp1));
	}
};

//*********************************************************************
//* Exponent                                                          *
//* Formula: out=exp (in1_1*in1_2*in1_3*in1_4*in1_5 +                 *
//*                    (in2_1*in2_2*in2_3*in2_4*in2_5+in3) /          *
//*                    (in4_1*in4_2*in4_3*in4_4*in4_5+in5) )          *
//*********************************************************************
class Exp : public TFunction
{
    public:
	Exp( ) : TFunction("exp")
	{
	    ioAdd( new IO("out",_("Output"),IO::Real,IO::Return,"0") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in1_"+TSYS::int2str(i_c)).c_str(),(_("Input 1.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in2_"+TSYS::int2str(i_c)).c_str(),(_("Input 2.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    ioAdd( new IO("in3",_("Input 3"),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in4_"+TSYS::int2str(i_c)).c_str(),(_("Input 4.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    ioAdd( new IO("in5",_("Input 5"),IO::Real,IO::Default,"1") );
	}

	string name( )	{ return _("Exponent"); }
	string descr( )
	{
	    return _("Exponent per formula:\n"
		"out=exp (in1_1*in1_2*in1_3*in1_4*in1_5 +\n"
		"         (in2_1*in2_2*in2_3*in2_4*in2_5+in3) / (in4_1*in4_2*in4_3*in4_4*in4_5+in5) );");
	}

	void calc( TValFunc *v )
	{
	    double tmp1=v->getR(12)*v->getR(13)*v->getR(14)*v->getR(15)*v->getR(16)+v->getR(17);
	    double tmp2=v->getR(1)*v->getR(2)*v->getR(3)*v->getR(4)*v->getR(5);
	    double tmp3=v->getR(6)*v->getR(7)*v->getR(8)*v->getR(9)*v->getR(10)+v->getR(11);
	    v->setR(0,exp(tmp2+tmp3/tmp1));
	}
};

//*********************************************************************
//* Power                                                             *
//* Formula: out=(in1_1*in1_2*in1_3*in1_4*in1_5)^                     *
//*                (in2_1*in2_2*in2_3*in2_4*in2_5 +                   *
//*                   (in3_1*in3_2*in3_3*in3_4*in3_5)/                *
//*                   (in4_1*in4_2*in4_3*in4_4*in4_5) )               *
//*********************************************************************
class Pow : public TFunction
{
    public:
	Pow( ) : TFunction("pow")
	{
	    ioAdd( new IO("out",_("Output"),IO::Real,IO::Return,"0") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in1_"+TSYS::int2str(i_c)).c_str(),(_("Input 1.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in2_"+TSYS::int2str(i_c)).c_str(),(_("Input 2.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in3_"+TSYS::int2str(i_c)).c_str(),(_("Input 3.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in4_"+TSYS::int2str(i_c)).c_str(),(_("Input 4.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	}

	string name( )	{ return _("Power"); }
	string descr( )
	{
	    return _("Power per formula:\n"
		"out=(in1_1*in1_2*in1_3*in1_4*in1_5)^(in2_1*in2_2*in2_3*in2_4*in2_5 +\n"
		"	(in3_1*in3_2*in3_3*in3_4*in3_5)/(in4_1*in4_2*in4_3*in4_4*in4_5));");
	}

	void calc( TValFunc *v )
	{
	    double tmp1=v->getR(16)*v->getR(17)*v->getR(18)*v->getR(19)*v->getR(20);
	    double tmp2=v->getR(6)*v->getR(7)*v->getR(8)*v->getR(9)*v->getR(10);
	    double tmp3=v->getR(11)*v->getR(12)*v->getR(13)*v->getR(14)*v->getR(15);
	    tmp2=tmp2+tmp3/tmp1;
	    tmp3=v->getR(1)*v->getR(2)*v->getR(3)*v->getR(4)*v->getR(5);
	    v->setR(0,pow(tmp3,tmp2));
	}
};

//*********************************************************************
//* Condition <                                                       *
//* Formula: out=if( in1<(in2_1*in2_2*in2_3*in2_4) )                  *
//*                then in3_1*in3_2*in3_3*in3_4;                      *
//*                else in4_1*in4_2*in4_3*in4_4;                      *
//*********************************************************************
class Cond1 : public TFunction
{
    public:
	Cond1( ) : TFunction("cond_lt")
	{
	    ioAdd( new IO("out",_("Output"),IO::Real,IO::Return,"0") );
	    ioAdd( new IO("in1",_("Input 1"),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in2_"+TSYS::int2str(i_c)).c_str(),(_("Input 2.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in3_"+TSYS::int2str(i_c)).c_str(),(_("Input 3.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in4_"+TSYS::int2str(i_c)).c_str(),(_("Input 4.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	}

	string name( )	{ return _("Condition '<'"); }
	string descr( )
	{
	    return _("Condition '<' per formula:\n"
		"out=if( in1<(in2_1*in2_2*in2_3*in2_4) ) then in3_1*in3_2*in3_3*in3_4;\n"
		"    else in4_1*in4_2*in4_3*in4_4;");
	}

	void calc( TValFunc *v )
	{
	    double tmp1=v->getR(2)*v->getR(3)*v->getR(4)*v->getR(5);
	    if( v->getR(1)<tmp1 )	v->setR(0,v->getR(6)*v->getR(7)*v->getR(8)*v->getR(9));
	    else			v->setR(0,v->getR(10)*v->getR(11)*v->getR(12)*v->getR(13));
	}
};

//*********************************************************************
//* Condition >                                                       *
//* Formula: out=if( in1>(in2_1*in2_2*in2_3*in2_4) )                  *
//*                then in3_1*in3_2*in3_3*in3_4;                      *
//*                else in4_1*in4_2*in4_3*in4_4;                      *
//*********************************************************************
class Cond2 : public TFunction
{
    public:
	Cond2( ) : TFunction("cond_gt")
	{
	    ioAdd( new IO("out",_("Output"),IO::Real,IO::Return,"0") );
	    ioAdd( new IO("in1",_("Input 1"),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in2_"+TSYS::int2str(i_c)).c_str(),(_("Input 2.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in3_"+TSYS::int2str(i_c)).c_str(),(_("Input 3.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in4_"+TSYS::int2str(i_c)).c_str(),(_("Input 4.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	}

	string name( )	{ return _("Condition '>'"); }
	string descr( )
	{
	    return _("Condition '>' per formula:\n"
		"out=if( in1>(in2_1*in2_2*in2_3*in2_4) ) then in3_1*in3_2*in3_3*in3_4;\n"
		"    else in4_1*in4_2*in4_3*in4_4;");
	}

	void calc( TValFunc *v )
	{
	    double tmp1=v->getR(2)*v->getR(3)*v->getR(4)*v->getR(5);
	    if( v->getR(1)>tmp1 )	v->setR(0,v->getR(6)*v->getR(7)*v->getR(8)*v->getR(9));
	    else			v->setR(0,v->getR(10)*v->getR(11)*v->getR(12)*v->getR(13));
	}
};

//*********************************************************************
//* Condition x>, x<, >x>                                             *
//* Formula: out = if( in1<(in2_1*in2_2*in2_3*in2_4) )                *
//*                  then in3_1*in3_2*in3_3*in3_4;                    *
//*                  else if( in1>(in4_1*in4_2*in4_3*in4_4)           *
//*                    then in5_1*in5_2*in5_3*in5_4;                  *
//*                    else in6_1*in6_2*in6_3*in6_4;                  *
//*********************************************************************
class Cond3 : public TFunction
{
    public:
	Cond3( ) : TFunction("cond_full")
	{
	    ioAdd( new IO("out",_("Output"),IO::Real,IO::Return,"0") );
	    ioAdd( new IO("in1",_("Input 1"),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in2_"+TSYS::int2str(i_c)).c_str(),(_("Input 2.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in3_"+TSYS::int2str(i_c)).c_str(),(_("Input 3.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in4_"+TSYS::int2str(i_c)).c_str(),(_("Input 4.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in5_"+TSYS::int2str(i_c)).c_str(),(_("Input 5.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in6_"+TSYS::int2str(i_c)).c_str(),(_("Input 6.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	}

	string name( )	{ return _("Full condition"); }
	string descr( )
	{
	    return _("Full condition per formula:\n"
		"out = if( in1<(in2_1*in2_2*in2_3*in2_4) )    then in3_1*in3_2*in3_3*in3_4;\n"
		"      else if( in1>(in4_1*in4_2*in4_3*in4_4) then in5_1*in5_2*in5_3*in5_4;\n"
		"      else in6_1*in6_2*in6_3*in6_4;");
	}

	void calc( TValFunc *v )
	{
	    double tmp1=v->getR(2)*v->getR(3)*v->getR(4)*v->getR(5);
	    double tmp2=v->getR(10)*v->getR(11)*v->getR(12)*v->getR(13);
	    double tmp3=v->getR(6)*v->getR(7)*v->getR(8)*v->getR(9);
	    double tmp4=v->getR(18)*v->getR(19)*v->getR(20)*v->getR(21);
	    double tmp5=v->getR(14)*v->getR(15)*v->getR(16)*v->getR(17);
	    if(v->getR(1)<tmp1)		v->setR(0,tmp3);
	    else if( v->getR(1)>tmp2)	v->setR(0,tmp5);
	    else			v->setR(0,tmp4);
	}
};

//*********************************************************************
//* Selector                                                          *
//* Formula: out = if( sel = 1 )  then in1_1*in1_2*in1_3*in1_4;       *
//*                if( sel = 2 )  then in2_1*in2_2*in2_3*in2_4;       *
//*                if( sel = 3 )  then in3_1*in3_2*in3_3*in3_4;       *
//*                if( sel = 4 )  then in4_1*in4_2*in4_3*in4_4;       *
//*********************************************************************
class Select : public TFunction
{
    public:
	Select( ) : TFunction("select")
	{
	    ioAdd( new IO("out",_("Output"),IO::Real,IO::Return,"0") );
	    ioAdd( new IO("sel",_("Select"),IO::Integer,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in1_"+TSYS::int2str(i_c)).c_str(),(_("Input 1.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in2_"+TSYS::int2str(i_c)).c_str(),(_("Input 2.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in3_"+TSYS::int2str(i_c)).c_str(),(_("Input 3.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 4; i_c++ )
		ioAdd( new IO(("in4_"+TSYS::int2str(i_c)).c_str(),(_("Input 4.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	}

	string name( )	{ return _("Selector"); }
	string descr( )	
	{
	    return _("Selector per formula:\n"
		"out = if( sel = 1 )  then in1_1*in1_2*in1_3*in1_4;\n"
		"      if( sel = 2 )  then in2_1*in2_2*in2_3*in2_4;\n"
		"      if( sel = 3 )  then in3_1*in3_2*in3_3*in3_4;\n"
		"      if( sel = 4 )  then in4_1*in4_2*in4_3*in4_4;");
	}

	void calc( TValFunc *v )
	{
	    switch(v->getI(1))
	    {
		case  1: v->setR(0,v->getR(2)*v->getR(3)*v->getR(4)*v->getR(5)); break;
		case  2: v->setR(0,v->getR(6)*v->getR(7)*v->getR(8)*v->getR(9)); break;
		case  3: v->setR(0,v->getR(10)*v->getR(11)*v->getR(12)*v->getR(13)); break;
		case  4: v->setR(0,v->getR(14)*v->getR(15)*v->getR(16)*v->getR(17)); break;
		default: v->setR(0,0.0);
	    }
	}
};

//*********************************************************************
//* Incrementator                                                     *
//* Formula: out = if( in > prev )                                    *
//*                  then prev + k_pos*(in-prev);                     *
//*                  else prev - k_neg*(prev-in);                     *
//*********************************************************************
class Increm : public TFunction
{
    public:
	Increm( ) : TFunction("increment")
	{
	    ioAdd( new IO("out",_("Output"),IO::Real,IO::Return,"0") );
	    ioAdd( new IO("in",_("Input"),IO::Real,IO::Default,"1") );
	    ioAdd( new IO("prev",_("Previous"),IO::Real,IO::Default,"1") );
	    ioAdd( new IO("k+",_("Positive koef"),IO::Real,IO::Default,"0.1") );
	    ioAdd( new IO("k-",_("Negative koef"),IO::Real,IO::Default,"0.1") );
	}

	string name( )	{ return _("Incrementator"); }
	string descr( )
	{
	    return _("Incrementator per formula:\n"
		"out = if( in > prev ) then prev + (k+)*(in-prev); else prev - (k-)*(prev-in);");
	}

	void calc( TValFunc *v )
	{
	    if( v->getR(1) > v->getR(2) )
		v->setR(0,v->getR(2) + v->getR(3)*(v->getR(1) - v->getR(2)));
	    else v->setR(0,v->getR(2) - v->getR(4)*(v->getR(2) - v->getR(1)));
	}
};

//*********************************************************************
//* Divider                                                           *
//* Formula: out = (in1_1*in1_2*in1_3*in1_4*in1_5 + in2_1*in2_2*in2_3*in2_4*in2_5 + in3) /
//*		   (in4_1*in4_2*in4_3*in4_4*in4_5 + in5_1*in5_2*in5_3*in5_4*in5_5 + in6);
//*********************************************************************
class Divider : public TFunction
{
    public:
	Divider( ) : TFunction("div")
	{
	    ioAdd( new IO("out",_("Output"),IO::Real,IO::Return,"0") );

	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in1_"+TSYS::int2str(i_c)).c_str(),(_("Input 1.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in2_"+TSYS::int2str(i_c)).c_str(),(_("Input 2.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    ioAdd( new IO("in3",_("Input 3"),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in4_"+TSYS::int2str(i_c)).c_str(),(_("Input 4.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in5_"+TSYS::int2str(i_c)).c_str(),(_("Input 5.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    ioAdd( new IO("in6",_("Input 6"),IO::Real,IO::Default,"1") );
	}

	string name( )	{ return _("Divider"); }
	string descr( )
	{
	    return _("Divider per formula:\n"
		"out = (in1_1*in1_2*in1_3*in1_4*in1_5 + in2_1*in2_2*in2_3*in2_4*in2_5 + in3) /\n"
		"      (in4_1*in4_2*in4_3*in4_4*in4_5 + in5_1*in5_2*in5_3*in5_4*in5_5 + in6);");
	}

	void calc( TValFunc *v )
	{
	    double t1 =	v->getR(1)*v->getR(2)*v->getR(3)*v->getR(4)*v->getR(5) + 
			v->getR(6)*v->getR(7)*v->getR(8)*v->getR(9)*v->getR(10) + v->getR(11);
	    double t2 =	v->getR(12)*v->getR(13)*v->getR(14)*v->getR(15)*v->getR(16) +
			v->getR(17)*v->getR(18)*v->getR(19)*v->getR(20)*v->getR(21) + v->getR(22);
	    v->setR(0,t1/t2);
	}
};

//*********************************************************************
//* PID                                                               *
//* Inputs:                                                           *
//*   0:val   - Value                                                 *
//*   1:sp    - Setpoint                                              *
//*   2:max   - Maximum scale                                         *
//*   3:min   - Minimum scale                                         *
//*   4:out   - Output                                                *
//*   5:auto  - Automatic mode                                        *
//*   6:casc  - Cascade mode                                          *
//*   7:Kp    - Gain                                                  *
//*   8:Ti    - Integral time (ms)                                    *
//*   9:Td    - Differencial time (ms)                                *
//*   10:Tf   - Filter (lag) time (ms)                                *
//*   11:Hup  - Up output limit (%)                                   *
//*   12:Hdwn - Down output limit (%)                                 *
//*   13:Zi   - Insensibility zone (%)                                *
//*   14:K1   - Scale input 1                                         *
//*   15:in1  - Add input 1                                           *
//*   16:K2   - Scale input 2                                         *
//*   17:in2  - Add input 2                                           *
//*   18:K3   - Scale input 3                                         *
//*   19:in3  - Add input 3                                           *
//*   20:K4   - Scale input 4                                         *
//*   21:in4  - Add input 4                                           *
//*   22:cycle- Calc cycle (ms)                                       *
//* Internal data:                                                    *
//*   23:#int - Curent integral value                                 *
//*   24:#dif - Curent differencial value                             *
//*   25:#lag - Curent lag value                                      *
//*********************************************************************
class PID : public TFunction
{
    public:
	PID( ) : TFunction("pid")
	{
	    //Inputs
	    ioAdd( new IO("var",_("Variable"),IO::Real,IO::Default,"0") );
	    ioAdd( new IO("sp",_("Setpoint"),IO::Real,IO::Default,"0") );
	    ioAdd( new IO("max",_("Max scale"),IO::Real,IO::Default,"100") );
	    ioAdd( new IO("min",_("Min scale"),IO::Real,IO::Default,"0") );
	    ioAdd( new IO("out",_("Output (%)"),IO::Real,IO::Return,"0") );
	    ioAdd( new IO("auto",_("Auto mode"),IO::Boolean,IO::Default,"0") );
	    ioAdd( new IO("casc",_("Cascade mode"),IO::Boolean,IO::Default,"0") );
	    ioAdd( new IO("Kp",_("Kp"),IO::Real,IO::Default,"1") );
	    ioAdd( new IO("Ti",_("Ti (ms)"),IO::Integer,IO::Default,"1000") );
	    ioAdd( new IO("Td",_("Td (ms)"),IO::Integer,IO::Default,"0") );
	    ioAdd( new IO("Tf",_("Tf-lag (ms)"),IO::Integer,IO::Default,"0") );
	    ioAdd( new IO("Hup",_("Out up limit (%)"),IO::Real,IO::Default,"100") );
	    ioAdd( new IO("Hdwn",_("Out down limit (%)"),IO::Real,IO::Default,"0") );
	    ioAdd( new IO("Zi",_("Insensibility (%)"),IO::Real,IO::Default,"1") );

	    ioAdd( new IO("K1",_("K input 1"),IO::Real,IO::Default,"0") );
	    ioAdd( new IO("in1",_("Input 1"),IO::Real,IO::Default,"0") );
	    ioAdd( new IO("K2",_("K input 2"),IO::Real,IO::Default,"0") );
	    ioAdd( new IO("in2",_("Input 2"),IO::Real,IO::Default,"0") );
	    ioAdd( new IO("K3",_("K input 3"),IO::Real,IO::Default,"0") );
	    ioAdd( new IO("in3",_("Input 3"),IO::Real,IO::Default,"0") );
	    ioAdd( new IO("K4",_("K input 4"),IO::Real,IO::Default,"0") );
	    ioAdd( new IO("in4",_("Input 4"),IO::Real,IO::Default,"0") );

	    ioAdd( new IO("cycle",_("Calc cycle (ms)"),IO::Integer,IO::Default,"1000") );

	    //Internal data:
	    ioAdd( new IO("int",_("Integral value"),IO::Real,IO::Output,"0",true) );
	    ioAdd( new IO("dif",_("Differencial value"),IO::Real,IO::Output,"0",true) );
	    ioAdd( new IO("lag",_("Lag value"),IO::Real,IO::Output,"0",true) );
	}

	string name( )	{ return _("PID regulator"); }
	string descr( )	{ return _("PID regulator"); }

	void calc( TValFunc *v )
	{
	    double	val	= v->getR(0),
			sp	= v->getR(1),
			max	= v->getR(2),
			min	= v->getR(3),
			out	= v->getR(4),
			kp	= v->getR(7),
			h_up	= v->getR(11),
			h_dwn	= v->getR(12),
			zi	= v->getR(13),
			k1	= v->getR(14),
			in1	= v->getR(15),
			k2	= v->getR(16),
			in2	= v->getR(17),
			k3	= v->getR(18),
			in3	= v->getR(19),
			k4	= v->getR(20),
			in4	= v->getR(21),
			cycle	= v->getI(22),
			integ	= v->getR(23),
			difer	= v->getR(24),
			lag	= v->getR(25);

	    double	Kf	= (v->getI(10)>cycle)?cycle/v->getI(10):1.;
	    double	Kint	= (v->getI(8)>cycle)?cycle/v->getI(8):1.;
	    double	Kdif	= (v->getI(9)>cycle)?cycle/v->getI(9):1.;

	    //> Scale error
	    if( max <= min )	return;

	    //> Prepare values
	    sp = 100.*(sp+min)/(max-min);
	    val = 100.*(val+min)/(max-min);
	    val += k1*in1 + k2*in2;
	    val=vmin(100.,vmax(-100.,val));

	    //> Error
	    double err = sp - val;

	    //> Insensibility
	    err = (fabs(err)<zi)?0:((err>0)?err-zi:err+zi);

	    //> Gain
	    err*=kp;
	    err=vmin(100.,vmax(-100.,err));

	    //> Input filter lag
	    lag+=Kf*(err-lag);
	    integ+=Kint*lag;		//Integral
	    difer-=Kdif*(difer-lag);	//Differecial lag

	    //> Automatic mode enabled
	    if( v->getB(5) ) out = (2.*lag + integ - difer) + k3*in3 + k4*in4;
	    else { v->setB(6,false); v->setR(1,v->getR(0)); }

	    //> Check output limits
	    out=vmin(h_up,vmax(h_dwn,out));

	    //> Fix integral for manual and limits
	    integ = out - 2.*lag + difer - k3*in3 + k4*in4;

	    //> Write outputs
	    v->setR(4,out);
	    v->setR(23,integ);
	    v->setR(24,difer);
	    v->setR(25,lag);
	}
};

//*********************************************************************
//* Alarm                                                             *
//* Formula: out = if (val>max || val<min) then true; else false;     *
//*********************************************************************
class Alarm : public TFunction
{
    public:
	Alarm( ) : TFunction("alarm")
	{
	    ioAdd( new IO("out",_("Output"),IO::Boolean,IO::Return,"0") );
	    ioAdd( new IO("val",_("Value"),IO::Real,IO::Default,"0") );
	    ioAdd( new IO("max",_("Maximum"),IO::Real,IO::Default,"100") );
	    ioAdd( new IO("min",_("Minimum"),IO::Real,IO::Default,"0") );
	}

	string name( )	{ return _("Alarm"); }
	string descr( )
	{
	    return _("Scale parameter alarm:\n"
		"out = if (val>max || val<min) then true; else false;");
	}

	void calc( TValFunc *v )
	{
	    double max = v->getR(2),
		min = v->getR(3),
		val = v->getR(1);

	    if( max != min && ( val < min || val > max ) )
		v->setB(0,true);
	    else v->setB(0,false);
	}
};

//*********************************************************************
//* Flow                                                              *
//* Formula: f = k1*((k3+k4*x)^k2)+Krnd*RAND                          *
//*********************************************************************
class Flow : public TFunction
{
    public:
	Flow( ) : TFunction("flow")
	{
	    ioAdd( new IO("f",_("Flow"),IO::Real,IO::Return,"0") );
	    ioAdd( new IO("x",_("X positon"),IO::Real,IO::Default,"0") );
	    ioAdd( new IO("K1",_("K1"),IO::Real,IO::Default,"1") );
	    ioAdd( new IO("K2",_("K2"),IO::Real,IO::Default,"1") );
	    ioAdd( new IO("K3",_("K3"),IO::Real,IO::Default,"0") );
	    ioAdd( new IO("K4",_("K4"),IO::Real,IO::Default,"1") );
	}

	string name( )	{ return _("Flow"); }
	string descr( )
	{
	    return _("Flow calc per formule:\n"
		"f = K1*((K3+K4*x)^K2);");
	}

	void calc( TValFunc *v )
	{
	    v->setR(0,v->getR(2)*pow(v->getR(4)+v->getR(5)*v->getR(1),v->getR(3)));
	}
};

//*********************************************************************
//* Sum+mult                                                          *
//* Formula: out = in1_1*in1_2*(in1_3*in1_4+in1_5) +                  *
//*                in2_1*in2_2*(in2_3*in2_4+in2_5) +                  *
//*                in3_1*in3_2*(in3_3*in3_4+in3_5) +                  *
//*                in4_1*in4_2*(in4_3*in4_4+in4_5);                   *
//*********************************************************************
class SumMult : public TFunction
{
    public:
	SumMult( ) : TFunction("sum_mult")
	{
	    ioAdd( new IO("out",_("Output"),IO::Real,IO::Return,"0") );

	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in1_"+TSYS::int2str(i_c)).c_str(),(_("Input 1.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in2_"+TSYS::int2str(i_c)).c_str(),(_("Input 2.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in3_"+TSYS::int2str(i_c)).c_str(),(_("Input 3.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in4_"+TSYS::int2str(i_c)).c_str(),(_("Input 4.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	}

	string name( )	{ return _("Sum and mult"); }
	string descr( )
	{
	    return _("Sum and mult per formule:\n"
		"out = in1_1*in1_2*(in1_3*in1_4+in1_5) + in2_1*in2_2*(in2_3*in2_4+in2_5) +\n"
		"      in3_1*in3_2*(in3_3*in3_4+in3_5) + in4_1*in4_2*(in4_3*in4_4+in4_5);");
	}

	void calc( TValFunc *v )
	{
	    double t1 = v->getR(1)*v->getR(2)*(v->getR(3)*v->getR(4)+v->getR(5));
	    double t2 = v->getR(6)*v->getR(7)*(v->getR(8)*v->getR(9)+v->getR(10));
	    double t3 = v->getR(11)*v->getR(12)*(v->getR(13)*v->getR(14)+v->getR(15));
	    double t4 = v->getR(16)*v->getR(17)*(v->getR(18)*v->getR(19)+v->getR(20));

	    v->setR(0,t1+t2+t3+t4);
	}
};

//*********************************************************************
//* Sum+div                                                           *
//* Formula: out = in1_1*in1_2*(in1_3+in1_4/in1_5) +                  *
//*                in2_1*in2_2*(in2_3+in2_4/in2_5) +                  *
//*                in3_1*in3_2*(in3_3+in3_4/in3_5) +                  *
//*                in4_1*in4_2*(in4_3+in4_4/in4_5);                   *
//*********************************************************************
class SumDiv : public TFunction
{
    public:
	SumDiv( ) : TFunction("sum_div")
	{
	    ioAdd( new IO("out",_("Output"),IO::Real,IO::Return,"0") );

	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in1_"+TSYS::int2str(i_c)).c_str(),(_("Input 1.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in2_"+TSYS::int2str(i_c)).c_str(),(_("Input 2.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in3_"+TSYS::int2str(i_c)).c_str(),(_("Input 3.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	    for( int i_c = 1; i_c <= 5; i_c++ )
		ioAdd( new IO(("in4_"+TSYS::int2str(i_c)).c_str(),(_("Input 4.")+TSYS::int2str(i_c)).c_str(),IO::Real,IO::Default,"1") );
	}

	string name( )	{ return _("Sum and divide"); }
	string descr( )
	{
	    return _("Sum and divide per formule:\n"
		"out = in1_1*in1_2*(in1_3+in1_4/in1_5) + in2_1*in2_2*(in2_3+in2_4/in2_5) +\n"
		"      in3_1*in3_2*(in3_3+in3_4/in3_5) + in4_1*in4_2*(in4_3+in4_4/in4_5);");
	}

	void calc( TValFunc *v )
	{
	    double t1 =	v->getR(1)*v->getR(2)*(v->getR(3)+v->getR(4)/v->getR(5));
	    double t2 =	v->getR(6)*v->getR(7)*(v->getR(8)+v->getR(9)/v->getR(10));
	    double t3 =	v->getR(11)*v->getR(12)*(v->getR(13)+v->getR(14)/v->getR(15));
	    double t4 =	v->getR(16)*v->getR(17)*(v->getR(18)+v->getR(19)/v->getR(20));

	    v->setR(0,t1+t2+t3+t4);
	}
};

//*********************************************************************
//* Lag                                                               *
//* Formula: y = y - Klag*( y - x );                                  *
//*********************************************************************
class Lag : public TFunction
{
    public:
	Lag( ) : TFunction("lag")
	{
	    ioAdd( new IO("y",_("Y"),IO::Real,IO::Return,"0") );
	    ioAdd( new IO("x",_("X"),IO::Real,IO::Default,"0") );
	    ioAdd( new IO("Klag",_("Klag"),IO::Real,IO::Default,"0.1") );
	}

	string name( )	{ return _("Lag"); }
	string descr( )
	{
	    return _("Lag per formule:\n"
		"y = y - Klag*( y - x );");
	}

	void calc( TValFunc *v )
	{
	    double y = v->getR(0);
	    v->setR(0,y - v->getR(2)*(y - v->getR(1)));
	}
};

} //End namespace virtual

#endif //LIBCOMPL1_H

