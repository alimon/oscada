
//OpenSCADA system module DAQ.LogicLev file: logiclev.h
/***************************************************************************
 *   Copyright (C) 2006-2010 by Roman Savochenko                           *
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

#ifndef LOGICLEV_H
#define LOGICLEV_H

#include <tmodule.h>
#include <tcontroller.h>
#include <ttipdaq.h>
#include <tparamcontr.h>

#include <string>
#include <vector>

#undef _
#define _(mess) mod->I18N(mess)

using std::string;
using std::vector;
using namespace OSCADA;

namespace LogicLev
{

//*************************************************
//* TMdPrm                                        *
//*************************************************
class TMdContr;

class TMdPrm : public TParamContr
{
    public:
	//Data
	enum Mode { Free, DirRefl, Template };

	//Methods
	TMdPrm( string name, TTipParam *tp_prm );
	~TMdPrm( );

	TCntrNode &operator=( TCntrNode &node );

	Mode mode( )	{ return m_wmode; }
	void mode( Mode md, const string &prm = "" );

	void enable( );
	void disable( );

	void calc( bool first, bool last, double frq );	//Calc template's algoritmes

    protected:
	//Methods
	void load_( );
	void save_( );

	TMdContr &owner( );

    private:
	//Data
	class SLnk
	{
	    public:
		SLnk( int iid, const string &iprm_attr = "" ) : io_id(iid), prm_attr(iprm_attr) { }
		int	io_id;
		string	prm_attr;
		AutoHD<TVal> aprm;
	};

	struct STmpl
	{
	    TValFunc	val;
	    vector<SLnk> lnk;
	};

	union
	{
	    AutoHD<TValue> *prm_refl;		//Direct reflection
	    STmpl *tmpl;			//Template
	};

	//Methods
	void postEnable( int flag );
	void postDisable( int flag );

	void cntrCmdProc( XMLNode *opt );	//Control interface command process

	void vlGet( TVal &val );
	void vlSet( TVal &val, const TVariant &pvl );
	void vlArchMake( TVal &val );

	//> Template link operations
	int lnkSize( );
	int lnkId( int id );
	int lnkId( const string &id );
	SLnk &lnk( int num );

	void loadIO( );
	void saveIO( );
	void initTmplLnks( bool checkNoLink = false );

	//Attributes
	ResString &m_prm;
	string	m_wprm;
	int	&m_mode;			//Config parameter mode
	Mode	m_wmode;			//Work parameter mode

	TElem	p_el;				//Work atribute elements

	bool	chk_lnk_need;			//Check lnk need flag
	Res	moderes, calcRes;		//Resource
	int	id_freq, id_start, id_stop, id_err, id_sh, id_nm, id_dscr;	//Fixed system attributes identifiers
};

//*************************************************
//* TMdContr                                      *
//*************************************************
class TMdContr: public TController
{
    friend class TMdPrm;
    public:
	//Methods
	TMdContr( string name_c, const string &daq_db, ::TElem *cfgelem);
	~TMdContr( );

	string getStatus( );
	double period( )			{ return mPer; }
        string cron( )				{ return mSched; }

	AutoHD<TMdPrm> at( const string &nm )	{ return TController::at(nm); }

	void redntDataUpdate( );

    protected:
	//Methods
	void prmEn( const string &id, bool val );
	void postDisable( int flag );		//Delete all DB if flag 1

	void load_( );
	void start_( );
	void stop_( );
	void cntrCmdProc( XMLNode *opt );       //Control interface command process

    private:
	//Methods
	TParamContr *ParamAttach( const string &name, int type );
	static void *Task( void *icntr );

	//Attributes
	Res	en_res;				//Resource for enable params
	double	&mPer;				// ms
	int	&mPrior;			// Process task priority
	ResString &mSched;      		// Calc schedule

	bool	prc_st,				// Process task active
		endrun_req;			// Request to stop of the Process task
	vector< AutoHD<TMdPrm> >  p_hd;

	double	tm_calc;			// Template functions calc time
};

//*************************************************
//* TTpContr                                      *
//*************************************************
class TTpContr: public TTipDAQ
{
    public:
	//Methods
	TTpContr( string name );
	~TTpContr( );

	TElem	&prmIOE( )	{ return el_prm_io; }

    protected:
	//Methods
	void postEnable( int flag );
	void load_( );

	bool redntAllow( )	{ return true; }

    private:
	//Methods
	TController *ContrAttach( const string &name, const string &daq_db );

	//Attributes
	TElem	el_prm_io;
};

extern TTpContr *mod;

} //End namespace

#endif //LOGICLEV_H
