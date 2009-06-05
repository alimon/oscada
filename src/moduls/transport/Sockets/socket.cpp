
//OpenSCADA system module Transport.Sockets file: socket.cpp
/***************************************************************************
 *   Copyright (C) 2003-2008 by Roman Savochenko                           *
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

#include <sys/types.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <string>
#include <errno.h>

#include <tsys.h>
#include <tmess.h>
#include <tprotocols.h>
#include <tmodule.h>
#include "socket.h"

//************************************************
//* Modul info!                                  *
#define MOD_ID		"Sockets"
#define MOD_NAME	"Sockets"
#define MOD_TYPE	"Transport"
#define VER_TYPE	VER_TR
#define VERSION		"1.4.0"
#define AUTORS		"Roman Savochenko"
#define DESCRIPTION	"Allow sockets based transport. Support inet and unix sockets. Inet socket use TCP and UDP protocols."
#define LICENSE		"GPL"
//************************************************

Sockets::TTransSock *Sockets::mod;

extern "C"
{
    TModule::SAt module( int n_mod )
    {
	if( n_mod==0 ) return TModule::SAt(MOD_ID,MOD_TYPE,VER_TYPE);
	return TModule::SAt("");
    }

    TModule *attach( const TModule::SAt &AtMod, const string &source )
    {
	if( AtMod == TModule::SAt(MOD_ID,MOD_TYPE,VER_TYPE) )
	    return new Sockets::TTransSock( source );
	return NULL;
    }
}

using namespace Sockets;

//************************************************
//* TTransSock					 *
//************************************************
TTransSock::TTransSock( string name )
{
    mId		= MOD_ID;
    mName	= MOD_NAME;
    mType	= MOD_TYPE;
    mVers	= VERSION;
    mAutor	= AUTORS;
    mDescr	= DESCRIPTION;
    mLicense	= LICENSE;
    mSource	= name;

    mod		= this;
}

TTransSock::~TTransSock()
{

}

void TTransSock::postEnable( int flag )
{
    TModule::postEnable( flag );

    if( flag&TCntrNode::NodeConnect )
    {
	//> Add self DB-fields to input transport
	owner().inEl().fldAdd( new TFld("BufLen",_("Input buffer length (kB)"),TFld::Integer,0,"4","5") );
	owner().inEl().fldAdd( new TFld("MaxClients",_("Maximum clients process"),TFld::Integer,0,"3","10") );
	owner().inEl().fldAdd( new TFld("SocketsMaxQueue",_("Maximum queue of input socket"),TFld::Integer,0,"2","10") );

	//> Add self DB-fields to output transport
	owner().outEl().fldAdd( new TFld("TMS",_("Timeout (ms)"),TFld::String,0,"30") );
    }
}

string TTransSock::optDescr( )
{
    char buf[STR_BUF_LEN];
    snprintf(buf,sizeof(buf),_(
	"======================= The module <%s:%s> options =======================\n"
	"---------- Parameters of the module section <%s> in config file ----------\n\n"),
	MOD_TYPE,MOD_ID,nodePath().c_str());

    return buf;
}

void TTransSock::load_( )
{
    //> Load parameters from command line
    int next_opt;
    const char *short_opt="h";
    struct option long_opt[] =
    {
	{"help"    ,0,NULL,'h'},
	{NULL      ,0,NULL,0  }
    };

    optind=opterr=0;
    do
    {
	next_opt=getopt_long(SYS->argc,(char * const *)SYS->argv,short_opt,long_opt,NULL);
	switch(next_opt)
	{
	    case 'h': fprintf(stdout,TSYS::strEncode(optDescr(),TSYS::FormatPrint).c_str()); break;
	    case -1 : break;
	}
    } while(next_opt != -1);
}

TTransportIn *TTransSock::In( const string &name, const string &idb )
{
    return new TSocketIn(name,idb,&owner().inEl());
}

TTransportOut *TTransSock::Out( const string &name, const string &idb )
{
    return new TSocketOut(name,idb,&owner().outEl());
}

void TTransSock::cntrCmdProc( XMLNode *opt )
{
    //- Get page info -
    if( opt->name() == "info" )
    {
	TTipTransport::cntrCmdProc(opt);
	ctrMkNode("fld",opt,-1,"/help/g_help",_("Options help"),0440,"root","root",3,"tp","str","cols","90","rows","5");
	return;
    }

    //- Process command to page -
    string a_path = opt->attr("path");
    if( a_path == "/help/g_help" && ctrChkNode(opt,"get",0440) )   opt->setText(optDescr());
    else TTipTransport::cntrCmdProc(opt);
}

//************************************************
//* TSocketIn                                    *
//************************************************
TSocketIn::TSocketIn( string name, const string &idb, TElem *el ) :
    TTransportIn(name,idb,el), cl_free(true), max_queue(cfg("SocketsMaxQueue").getId()),
    mMaxFork(cfg("MaxClients").getId()), mBufLen(cfg("BufLen").getId())
{
    setAddr("TCP:localhost:10002:0");
}

TSocketIn::~TSocketIn()
{
    try{ stop(); }catch(...){ }
}

string TSocketIn::getStatus( )
{
    string rez = TTransportIn::getStatus( );

    if( startStat() )
	rez += TSYS::strMess(_("Connections %d, opened %d. Traffic in %.4g kb, out %.4g kb. Closed connections by limit %d."),
	connNumb,cl_id.size(),trIn,trOut,clsConnByLim);

    return rez;
}

void TSocketIn::start()
{
    pthread_attr_t pthr_attr;

    if( run_st ) return;

    //- Status clear -
    trIn = trOut = 0;
    connNumb = clsConnByLim = 0;

    //- Socket init -
    string s_type = TSYS::strSepParse(addr(),0,':');

    if( s_type == S_NM_TCP )
    {
	if( (sock_fd = socket(PF_INET,SOCK_STREAM,0) )== -1 )
	    throw TError(nodePath().c_str(),_("Error create %s socket!"),s_type.c_str());
	int vl = 1;
	setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&vl,sizeof(int));
	type = SOCK_TCP;
    }
    else if( s_type == S_NM_UDP )
    {
	if( (sock_fd = socket(PF_INET,SOCK_DGRAM,0) )== -1 )
	    throw TError(nodePath().c_str(),_("Error create %s socket!"),s_type.c_str());
	type = SOCK_UDP;
    }
    else if( s_type == S_NM_UNIX )
    {
	if( (sock_fd = socket(PF_UNIX,SOCK_STREAM,0) )== -1)
	    throw TError(nodePath().c_str(),_("Error create %s socket!"),s_type.c_str());
	type = SOCK_UNIX;
    }
    else throw TError(nodePath().c_str(),_("Socket type <%s> error!"),s_type.c_str());

    if( type == SOCK_TCP || type == SOCK_UDP )
    {
	struct sockaddr_in  name_in;
	struct hostent *loc_host_nm;
	memset(&name_in,0,sizeof(name_in));
	name_in.sin_family = AF_INET;

	host	= TSYS::strSepParse(addr(),1,':');
	port	= TSYS::strSepParse(addr(),2,':');
	if( host.size() )
	{
	    loc_host_nm = gethostbyname(host.c_str());
	    if(loc_host_nm == NULL || loc_host_nm->h_length == 0)
		throw TError(nodePath().c_str(),_("Socket name <%s> error!"),host.c_str());
	    name_in.sin_addr.s_addr = *( (int *) (loc_host_nm->h_addr_list[0]) );
	}
	else name_in.sin_addr.s_addr = INADDR_ANY;
	if( type == SOCK_TCP )
	{
	    mode	= atoi( TSYS::strSepParse(addr(),3,':').c_str() );
	    //Get system port for "oscada" /etc/services
	    struct servent *sptr = getservbyname(port.c_str(),"tcp");
	    if( sptr != NULL )                       name_in.sin_port = sptr->s_port;
	    else if( htons(atol(port.c_str())) > 0 ) name_in.sin_port = htons( atol(port.c_str()) );
	    else name_in.sin_port = 10001;

	    if( bind(sock_fd,(sockaddr *)&name_in,sizeof(name_in) ) == -1)
	    {
		shutdown( sock_fd,SHUT_RDWR );
		close( sock_fd );
		throw TError(nodePath().c_str(),_("TCP socket doesn't bind to <%s>!"),addr().c_str());
	    }
	    listen(sock_fd,max_queue);
	}
	else if(type == SOCK_UDP )
	{
	    //Get system port for "oscada" /etc/services
	    struct servent *sptr = getservbyname(port.c_str(),"udp");
	    if( sptr != NULL )                       name_in.sin_port = sptr->s_port;
	    else if( htons(atol(port.c_str())) > 0 ) name_in.sin_port = htons( atol(port.c_str()) );
	    else name_in.sin_port = 10001;

	    if( bind(sock_fd,(sockaddr *)&name_in,sizeof(name_in) ) == -1) 
	    {
		shutdown( sock_fd,SHUT_RDWR );
		close( sock_fd );
                throw TError(nodePath().c_str(),_("UDP socket doesn't bind to <%s>!"),addr().c_str());
	    }
	}
    }
    else if( type == SOCK_UNIX )
    {
	path	= TSYS::strSepParse(addr(),1,':');
	mode	= atoi( TSYS::strSepParse(addr(),2,':').c_str() );
	if( !path.size() ) path = "/tmp/oscada";
	remove( path.c_str());
	struct sockaddr_un  name_un;
	memset(&name_un,0,sizeof(name_un));
	name_un.sun_family = AF_UNIX;
	strncpy( name_un.sun_path,path.c_str(),sizeof(name_un.sun_path) );
	if( bind(sock_fd,(sockaddr *)&name_un,sizeof(name_un) ) == -1)
	{
	    close( sock_fd );
            throw TError(nodePath().c_str(),_("UNIX socket doesn't bind to <%s>!"),addr().c_str());
	}
	listen(sock_fd,max_queue);
    }

    pthread_attr_init(&pthr_attr);
    pthread_attr_setschedpolicy(&pthr_attr,SCHED_OTHER);
    pthread_create(&pthr_tsk,&pthr_attr,Task,this);
    pthread_attr_destroy(&pthr_attr);
    if( TSYS::eventWait( run_st, true,nodePath()+"open",5) )
	throw TError(nodePath().c_str(),_("Not opened!"));
}

void TSocketIn::stop()
{
    if( !run_st ) return;

    //- Status clear -
    trIn = trOut = 0;
    connNumb = clsConnByLim = 0;

    endrun = true;
    if( TSYS::eventWait( run_st, false, nodePath()+"close",5) )
	throw TError(nodePath().c_str(),_("Not closed!"));
    pthread_join( pthr_tsk, NULL );

    shutdown(sock_fd,SHUT_RDWR);
    close(sock_fd); 
    if( type == SOCK_UNIX ) remove( path.c_str() );
}

void *TSocketIn::Task(void *sock_in)
{
    char		*buf;
    fd_set		rd_fd;
    struct timeval	tv;
    TSocketIn *sock = (TSocketIn *)sock_in;
    AutoHD<TProtocolIn> prot_in;

#if OSC_DEBUG >= 2
    mess_debug(sock->nodePath().c_str(),_("Thread <%u> is started. TID: %ld"),pthread_self(),(long int)syscall(224));
#endif

    pthread_t      th;
    pthread_attr_t pthr_attr;
    pthread_attr_init(&pthr_attr);
    pthread_attr_setschedpolicy(&pthr_attr,SCHED_OTHER);
    pthread_attr_setdetachstate(&pthr_attr, PTHREAD_CREATE_DETACHED);

    sock->run_st    = true;
    sock->endrun_cl = false;
    sock->endrun    = false;

    if( sock->type == SOCK_UDP )
	buf = new char[sock->bufLen()*1024 + 1];

    while( !sock->endrun )
    {
	tv.tv_sec  = 0; tv.tv_usec = STD_WAIT_DELAY*1000;
	FD_ZERO(&rd_fd); FD_SET(sock->sock_fd,&rd_fd);

	int kz = select(sock->sock_fd+1,&rd_fd,NULL,NULL,&tv);
	if( kz == 0 || (kz == -1 && errno == EINTR) || kz < 0 || !FD_ISSET(sock->sock_fd, &rd_fd) ) continue;

	struct sockaddr_in name_cl;
	socklen_t          name_cl_len = sizeof(name_cl);
	if( sock->type == SOCK_TCP )
	{
	    int sock_fd_CL = accept(sock->sock_fd, (sockaddr *)&name_cl, &name_cl_len);
	    if( sock_fd_CL != -1 )
	    {
		if( sock->maxFork() <= (int)sock->cl_id.size() )
		{
		    sock->clsConnByLim++;
		    close(sock_fd_CL);
		    continue;
		}
		if( pthread_create( &th, &pthr_attr, ClTask, new SSockIn( sock, sock_fd_CL, inet_ntoa(name_cl.sin_addr) ) ) < 0)
		    mess_err(sock->nodePath().c_str(),_("Error creation of the thread!"));
		else sock->connNumb++;
	    }
	}
	else if( sock->type == SOCK_UNIX )
	{
	    int sock_fd_CL = accept(sock->sock_fd, NULL, NULL);
	    if( sock_fd_CL != -1 )
	    {
		if( sock->maxFork() <= (int)sock->cl_id.size() )
		{
		    sock->clsConnByLim++;
		    close(sock_fd_CL);
		    continue;
		}
		if( pthread_create( &th, &pthr_attr, ClTask, new SSockIn(sock,sock_fd_CL,"") ) < 0 )
		    mess_err(sock->nodePath().c_str(),_("Error creation of the thread!"));
		else sock->connNumb++;
	    }
	}
	else if( sock->type == SOCK_UDP )
	{
	    int r_len;
	    string  req, answ;

	    r_len = recvfrom(sock->sock_fd, buf, sock->bufLen()*1000, 0,(sockaddr *)&name_cl, &name_cl_len); sock->trIn += (float)r_len/1024;
	    if( r_len <= 0 ) continue;
	    req.assign(buf,r_len);

#if OSC_DEBUG >= 5
	    mess_debug( sock->nodePath().c_str(), _("Socket received datagram <%d> from <%s>!"), r_len, inet_ntoa(name_cl.sin_addr) );
#endif

	    sock->messPut(sock->sock_fd, req, answ, inet_ntoa(name_cl.sin_addr),prot_in);
	    if( !prot_in.freeStat() ) continue;

#if OSC_DEBUG >= 5
	    mess_debug( sock->nodePath().c_str(), _("Socket replied datagram <%d> to <%s>!"), answ.size(), inet_ntoa(name_cl.sin_addr) );
#endif

	    r_len = sendto(sock->sock_fd,answ.c_str(),answ.size(),0,(sockaddr *)&name_cl, name_cl_len); sock->trOut += (float)r_len/1024;
	}
    }
    pthread_attr_destroy(&pthr_attr);

    if( sock->type == SOCK_UDP ) delete []buf;
    //- Client tasks stop command -
    sock->endrun_cl = true;
    ResAlloc res(sock->sock_res,false);
    //- Find already registry -
    for( int i_id = 0; i_id < sock->cl_id.size(); i_id++)
        pthread_kill(sock->cl_id[i_id].cl_id,SIGALRM);
    res.release();
    TSYS::eventWait( sock->cl_free, true, string(MOD_ID)+": "+sock->id()+" client task is stopping....");

    sock->run_st = false;

    return NULL;
}

void *TSocketIn::ClTask( void *s_inf )
{
    SSockIn &s = *(SSockIn*)s_inf;

#if OSC_DEBUG >= 2
    mess_debug(s.s->nodePath().c_str(),_("Thread <%u> is started. TID: %ld"),pthread_self(),(long int)syscall(224));
#endif
#if OSC_DEBUG >= 3
    mess_debug(s.s->nodePath().c_str(),_("Socket has been connected by <%s>!"),s.sender.c_str() );
#endif

    s.s->clientReg( pthread_self(), s.cSock );

    //> Client socket process
    struct  timeval tv;
    fd_set  rd_fd;
    int     r_len;
    string  req, answ;
    char    buf[s.s->bufLen()*1000 + 1];
    AutoHD<TProtocolIn> prot_in;
    bool sessOk = false;

    do
    {
	tv.tv_sec  = 0; tv.tv_usec = STD_WAIT_DELAY*1000;
	FD_ZERO(&rd_fd); FD_SET(s.cSock,&rd_fd);

	int kz = select(s.cSock+1,&rd_fd,NULL,NULL,&tv);
	if( kz == 0 || (kz == -1 && errno == EINTR) || kz < 0 || !FD_ISSET(s.cSock, &rd_fd) ) continue;
	r_len = read(s.cSock,buf,s.s->bufLen()*1000);
	if(r_len <= 0) break;
	s.s->trIn += (float)r_len/1024;

#if OSC_DEBUG >= 5
	mess_debug(s.s->nodePath().c_str(),_("Socket received message <%d> from <%s>."), r_len, s.sender.c_str() );
#endif
	req.assign(buf,r_len);

	s.s->messPut(s.cSock,req,answ,s.sender,prot_in);
	if( answ.size() )
	{
#if OSC_DEBUG >= 5
	    mess_debug(s.s->nodePath().c_str(),_("Socket replied message <%d> to <%s>."), answ.size(), s.sender.c_str() );
#endif
	    r_len = write(s.cSock,answ.c_str(),answ.size()); s.s->trOut += (float)r_len/1024;
	    answ = "";
	}
	sessOk = true;
    }while( !s.s->endrun_cl && (!sessOk || s.s->mode || !prot_in.freeStat()) );

    //> Close protocol on broken connection
    if( !prot_in.freeStat() )
    {
	string n_pr = prot_in.at().name();
	prot_in.free();
	AutoHD<TProtocol> proto = SYS->protocol().at().modAt(s.s->protocol());
	if( proto.at().openStat(n_pr) ) proto.at().close(n_pr);
    }

    s.s->clientUnreg( pthread_self( ) );

#if OSC_DEBUG >= 3
    mess_debug(s.s->nodePath().c_str(),_("Socket has been disconnected by <%s>!"),s.sender.c_str() );
#endif

    delete (SSockIn*)s_inf;

    pthread_exit(NULL);
}

void TSocketIn::messPut( int sock, string &request, string &answer, string sender, AutoHD<TProtocolIn> &prot_in )
{
    AutoHD<TProtocol> proto;
    string n_pr = id()+TSYS::int2str(sock);
    try
    {
	proto = SYS->protocol().at().modAt(protocol());
	if( prot_in.freeStat() )
	{
	    if( !proto.at().openStat(n_pr) ) proto.at().open( n_pr, workId() );
	    prot_in = proto.at().at( n_pr );
	}
	if( prot_in.at().mess(request,answer,sender) ) return;
	prot_in.free();
	if( proto.at().openStat(n_pr) ) proto.at().close(n_pr);
    }catch(TError err)
    {
	prot_in.free();
	if( !proto.freeStat() && proto.at().openStat(n_pr) ) proto.at().close( n_pr );

	mess_err(nodePath().c_str(),"%s",err.mess.c_str() );
	mess_err(nodePath().c_str(),_("Error request to protocol."));
    }
}

void TSocketIn::clientReg( pthread_t thrid, int i_sock )
{
    ResAlloc res(sock_res,true);
    //- Find already registry -
    for( unsigned i_id = 0; i_id < cl_id.size(); i_id++)
	if( cl_id[i_id].cl_id == thrid ) return;
    SSockCl scl = { thrid, i_sock };
    cl_id.push_back(scl);
    cl_free = false;
}

void TSocketIn::clientUnreg( pthread_t thrid )
{
    ResAlloc res(sock_res,true);
    for( unsigned i_id = 0; i_id < cl_id.size(); i_id++ )
	if( cl_id[i_id].cl_id == thrid )
	{
	    shutdown(cl_id[i_id].cl_sock,SHUT_RDWR);
	    close(cl_id[i_id].cl_sock);
	    cl_id.erase(cl_id.begin() + i_id);
	    if( !cl_id.size() ) cl_free = true;
	    break;
	}
}

void TSocketIn::cntrCmdProc( XMLNode *opt )
{
    //> Get page info
    if( opt->name() == "info" )
    {
	TTransportIn::cntrCmdProc(opt);
	ctrMkNode("fld",opt,-1,"/prm/cfg/addr",cfg("ADDR").fld().descr(),0664,"root","root",2,"tp","str","help",
	    _("Socket's input transport has address format:\n"
	    "  TCP:[addr]:[port]:[mode] - TCP socket:\n"
	    "    addr - address for socket to be opened, empty address opens socket for all interfaces;\n"
	    "    port - network port (/etc/services);\n"
	    "    mode - work mode (0 - break connection; 1 - keep alive).\n"
	    "  UDP:[addr]:[port] - UDP socket:\n"
	    "    addr - address for socket to be opened, empty address opens socket for all interfaces;\n"
	    "    port - network port (/etc/services).\n"
	    "  UNIX:[name]:[mode] - UNIX socket:\n"
	    "    name - UNIX-socket's file name;\n"
	    "    mode - work mode (0 - break connection; 1 - keep alive)."));
	ctrMkNode("fld",opt,-1,"/prm/cfg/q_ln",_("Queue length"),0664,"root","root",2,"tp","dec","help",_("Used for TCP and UNIX sockets."));
	ctrMkNode("fld",opt,-1,"/prm/cfg/cl_n",_("Clients maximum"),0664,"root","root",2,"tp","dec","help",_("Used for TCP and UNIX sockets."));
	ctrMkNode("fld",opt,-1,"/prm/cfg/bf_ln",_("Input buffer (kbyte)"),0664,"root","root",1,"tp","dec");
	return;
    }
    //- Process command to page -
    string a_path = opt->attr("path");
    if( a_path == "/prm/cfg/q_ln" )
    {
	if( ctrChkNode(opt,"get",0664,"root","root",SEQ_RD) )	opt->setText( TSYS::int2str(maxQueue()) );
	if( ctrChkNode(opt,"set",0664,"root","root",SEQ_WR) )	setMaxQueue( atoi(opt->text().c_str()) );
    }
    else if( a_path == "/prm/cfg/cl_n" )
    {
	if( ctrChkNode(opt,"get",0664,"root","root",SEQ_RD) )	opt->setText( TSYS::int2str(maxFork()) );
	if( ctrChkNode(opt,"set",0664,"root","root",SEQ_WR) )	setMaxFork( atoi(opt->text().c_str()) );
    }
    else if( a_path == "/prm/cfg/bf_ln" )
    {
	if( ctrChkNode(opt,"get",0664,"root","root",SEQ_RD) )	opt->setText( TSYS::int2str(bufLen()) );
	if( ctrChkNode(opt,"set",0664,"root","root",SEQ_WR) )	setBufLen( atoi(opt->text().c_str()) );
    }
    else TTransportIn::cntrCmdProc(opt);
}

//************************************************
//* TSocketOut                                   *
//************************************************
TSocketOut::TSocketOut(string name, const string &idb, TElem *el) : TTransportOut(name,idb,el), sock_fd(-1)
{
    setAddr("TCP:localhost:10002");
    setTimeout(1000);
}

TSocketOut::~TSocketOut()
{
    if( startStat() )	stop();
}

string TSocketOut::getStatus( )
{
    string rez = TTransportOut::getStatus( );

    if( startStat() )	rez += TSYS::strMess(_("Traffic in %.4g kb, out %.4g kb."),trIn,trOut);

    return rez;
}

int TSocketOut::timeout( )	{ return vmax(1,vmin(100000,cfg("TMS").getI())); }

void TSocketOut::setTimeout( int vl )	{ cfg("TMS").setI(vl); modif(); }

void TSocketOut::start()
{
    ResAlloc res( wres, true );

    if( run_st ) return;

    //> Status clear
    trIn = trOut = 0;

    //> Connect to remote host
    string s_type = TSYS::strSepParse(addr(),0,':');

    if( s_type == S_NM_TCP )		type = SOCK_TCP;
    else if( s_type == S_NM_UDP ) 	type = SOCK_UDP;
    else if( s_type == S_NM_UNIX )	type = SOCK_UNIX;
    else throw TError(nodePath().c_str(),_("Type socket <%s> error!"),s_type.c_str());

    if( type == SOCK_TCP || type == SOCK_UDP )
    {
	memset(&name_in,0,sizeof(name_in));
	name_in.sin_family = AF_INET;

	string host = TSYS::strSepParse(addr(),1,':');
	string port = TSYS::strSepParse(addr(),2,':');
	if( host.size() )
	{
	    struct hostent *loc_host_nm = gethostbyname(host.c_str());
	    if(loc_host_nm == NULL || loc_host_nm->h_length == 0)
		throw TError(nodePath().c_str(),_("Socket name <%s> error!"),host.c_str());
	    name_in.sin_addr.s_addr = *( (int *) (loc_host_nm->h_addr_list[0]) );
	}
	else name_in.sin_addr.s_addr = INADDR_ANY;
	//> Get system port for "oscada" /etc/services
	struct servent *sptr = getservbyname(port.c_str(),(type == SOCK_TCP)?"tcp":"udp");
	if( sptr != NULL )                       name_in.sin_port = sptr->s_port;
	else if( htons(atol(port.c_str())) > 0 ) name_in.sin_port = htons( atol(port.c_str()) );
	else name_in.sin_port = 10001;

	//> Create socket
	if( type == SOCK_TCP )
	{
	    if( (sock_fd = socket(PF_INET,SOCK_STREAM,0) )== -1 )
		throw TError(nodePath().c_str(),_("Error creation TCP socket: %s!"),strerror(errno));
	    int vl = 1;
	    setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&vl,sizeof(int));
	}
	else if( type == SOCK_UDP )
	{
	    if( (sock_fd = socket(PF_INET,SOCK_DGRAM,0) )== -1 )
		throw TError(nodePath().c_str(),_("Error creation UDP socket: %s!"),strerror(errno));
	}
	//> Connect to socket
	int flags = fcntl(sock_fd,F_GETFL,0);
	fcntl(sock_fd,F_SETFL,flags|O_NONBLOCK);
	int res = ::connect(sock_fd, (sockaddr *)&name_in, sizeof(name_in));
	if( res == -1 && errno == EINPROGRESS )
	{
	    struct timeval tv;
	    socklen_t slen;
	    fd_set fdset;
	    tv.tv_sec = timeout()/1000; tv.tv_usec = 1000*(timeout()%1000);
	    FD_ZERO( &fdset ); FD_SET( sock_fd, &fdset );
	    if( (res=select( sock_fd+1, NULL, &fdset, NULL, &tv )) > 0 && !getsockopt(sock_fd,SOL_SOCKET,SO_ERROR,&res,&slen) && !res ) res = 0;
	    else res = -1;
	}
	if( res )
	{
	    close(sock_fd);
	    sock_fd = -1;
	    throw TError(nodePath().c_str(),_("Connect to Internet socket error: %s!"),strerror(errno));
	}

	/*time_t wTm = time(NULL);
	int res = -1;
	do
	{
	    res = ::connect(sock_fd, (sockaddr *)&name_in, sizeof(name_in));
	    if( res == 0 || (res == -1 && !(errno == EINPROGRESS || errno == EALREADY)) ) break;
	    usleep(10000);
	}while( (time(NULL)-wTm) < timeout()/1000 );
	if( res == -1 )
	{
	    close(sock_fd);
	    sock_fd = -1;
	    throw TError(nodePath().c_str(),_("Connect to Internet socket error: %s!"),strerror(errno));
	}*/
    }
    else if( type == SOCK_UNIX )
    {
	string path = TSYS::strSepParse(addr(),1,':');
	if( !path.size() ) path = "/tmp/oscada";
	memset(&name_un,0,sizeof(name_un));
	name_un.sun_family = AF_UNIX;
	strncpy( name_un.sun_path,path.c_str(),sizeof(name_un.sun_path) );
	
	//- Create socket -
	if( (sock_fd = socket(PF_UNIX,SOCK_STREAM,0) )== -1)
	    throw TError(nodePath().c_str(),_("Error creation UNIX socket: %s!"),strerror(errno));
	if( ::connect(sock_fd, (sockaddr *)&name_un, sizeof(name_un)) == -1 )
	{
	    close(sock_fd);
	    sock_fd = -1;
	    throw TError(nodePath().c_str(),_("Connect to UNIX error: %s!"),strerror(errno));
	}
    }
    run_st = true;
}

void TSocketOut::stop()
{
    ResAlloc res( wres, true );

    if( !run_st ) return;

    //> Status clear
    trIn = trOut = 0;

    //> Close connection
    if( sock_fd >= 0 )
    {
	shutdown(sock_fd,SHUT_RDWR);
	close(sock_fd);
    }
    run_st = false;
}

int TSocketOut::messIO( const char *obuf, int len_ob, char *ibuf, int len_ib, int time )
{
    int kz = 0, reqTry = 0;

    ResAlloc res( wres, true );
    int prevTmOut = timeout( );
    if( time ) setTimeout(time);

    if( !run_st ) throw TError(nodePath().c_str(),_("Transport is not started!"));

repeate:
    if( reqTry++ >= 2 ) throw TError(nodePath().c_str(),_("Read reply error: %s"),strerror(errno));
    //> Write request
    if( obuf != NULL && len_ob > 0 && (kz=write(sock_fd,obuf,len_ob)) <= 0 )
    {
	res.release();
	stop(); start();
	res.request(true);
	goto repeate;
    }
    trOut += (float)kz/1024;

    //> Read reply
    int i_b = 0;
    if( ibuf != NULL && len_ib > 0 )
    {
	fd_set rd_fd;
	struct timeval tv;

	do
	{
	    tv.tv_sec  = timeout()/1000; tv.tv_usec = 1000*(timeout()%1000);
	    FD_ZERO(&rd_fd); FD_SET(sock_fd,&rd_fd);
	    kz = select(sock_fd+1,&rd_fd,NULL,NULL,&tv);
	}
	while( kz == -1 && errno == EINTR );
	if( kz == 0 )	{ res.release(); stop( ); throw TError(nodePath().c_str(),_("Timeouted!")); }
	else if( kz < 0){ res.release(); stop( ); throw TError(nodePath().c_str(),_("Socket error!")); }
	else if( FD_ISSET(sock_fd, &rd_fd) )
	{
	    i_b = read(sock_fd,ibuf,len_ib);
	    if(i_b <= 0) { res.release(); stop(); start(); res.request(true); goto repeate; }
	    trIn += (float)i_b/1024;
	}
    }

    if( time ) setTimeout(prevTmOut);

    return i_b;
}

void TSocketOut::cntrCmdProc( XMLNode *opt )
{
    //> Get page info
    if( opt->name() == "info" )
    {
	TTransportOut::cntrCmdProc(opt);
	ctrMkNode("fld",opt,-1,"/prm/cfg/addr",cfg("ADDR").fld().descr(),0664,"root","root",2,"tp","str","help",
	    _("Socket's output transport has address format:\n"
	    "  TCP:[addr]:[port] - TCP socket:\n"
	    "    addr - address for remote socket to be opened;\n"
	    "    port - network port (/etc/services);\n"
	    "  UDP:[addr]:[port] - UDP socket:\n"
	    "    addr - address for remote socket to be opened;\n"
	    "    port - network port (/etc/services).\n"
	    "  UNIX:[name] - UNIX socket:\n"
	    "    name - UNIX-socket's file name."));
	ctrMkNode("fld",opt,-1,"/prm/cfg/tmOut",_("Timeout (ms)"),0664,"root","root",1,"tp","dec");
	return;
    }

    //> Process command to page
    string a_path = opt->attr("path");
    if( a_path == "/prm/cfg/tmOut" )
    {
	if( ctrChkNode(opt,"get",0664,"root","root",SEQ_RD) )	opt->setText(TSYS::int2str(timeout()));
	if( ctrChkNode(opt,"set",0664,"root","root",SEQ_WR) )	setTimeout(atoi(opt->text().c_str()));
    }
    else TTransportOut::cntrCmdProc(opt);
}
