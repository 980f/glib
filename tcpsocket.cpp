#include "tcpsocket.h"
#include "glibmm/refptr.h"
#include "glibmm/main.h" //timeout
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include "logger.h"
#include <errno.h>
#include "netinet/tcp.h"
#include "cheapTricks.h"

void TcpSocketBase::connectionEvent(bool connected){
  if(connected){
    ++stats.connects;
  } else {
    ++stats.disconnects;
    dbg("Disconnected from head.");
  }
}

TcpSocketBase::TcpSocketBase(int fd, u32 remoteAddress, int port):
  stats(),
  connectArgs(remoteAddress,port),
  sock(fd),
  source(sock){
  //#nada
}

TcpSocketBase::~TcpSocketBase(){
  //#nada
}

bool TcpSocketBase::isConnected() const {
  return sock.isValid();
}

////

TcpSocket::TcpSocket(int fd, u32 remoteAddress, int port):
  TcpSocketBase(fd,remoteAddress,port),
  autoConnect(false),
  sendbuf(),
  eagerToWrite(0),
  newConnections(0),
  connectionInProgress(false){
  whenConnectionChanges(MyHandler(TcpSocket::connectionEvent),false);//use notifier to manage connection counters
  if(sock.isValid()){
    startReception();
  }
}

u32 TcpSocketBase::remoteIpv4(){
  return connectArgs.ipv4;
}

bool TcpSocketBase::disconnect(){
  source.disconnect();
  if(isConnected()){//#checking for debug purposes
    sock.close();
  }
  return false;
}

//////////////////////////////

bool TcpSocket::connect(unsigned ipv4, unsigned port,bool noDelay,bool block){
  connectArgs.ipv4=ipv4;
  connectArgs.port=port;
  connectArgs.noDelay=noDelay;
  connectArgs.block=block;

  autoConnect=connectArgs.isPossible();
  return autoConnect&&reconnect();
}

void TcpSocket::startReception(){
  source.incoming= sock.input(MyHandler(TcpSocket::readable));
  source.hangup= sock.hangup(MyHandler(TcpSocket::hangup));
}

void TcpSocket::connectionFailed(int error){
  stats.lastWrote=-error;
  if(connectArgs.block){
    dbg("Connection Failed after %g seconds",connectionTimer.elapsed());
    //todo:0 ?if fail immediately then spin some before admitting defeat?
  }
  disconnect(true);
}

bool TcpSocket::reconnect(){
  bool wasConnected=isConnected();
  if(wasConnected) {
    disconnect(false);//don't notify since notify is likely to call connect and hence infinite loop.
  }
  sock.fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if(sock.isValid()) {
    //Setting the socket to reuse the address if we fail and restart
    sock.setOption(SOL_SOCKET, SO_REUSEADDR, int(1));

    if(wasConnected){//then now we can let the world know.
      notifyConnected(false);
    }
  } else {
    return false;
  }

  if(connectArgs.noDelay){
    dbg("Setting TCP No Delay on");
    if(int ernum=sock.setOption( IPPROTO_TCP, TCP_NODELAY, 1)){
      dbg("Set TCP no delay returned %d",ernum);
    }
  }

  //todo:1 research whether the default so_linger is false, we'd like a reset() rather than a gentle close() when we disconnect.
  SocketAddress sad(connectArgs);  // :(
  connectionTimer.start();
  if(int error=sad.connect(sock.fd)) {
    switch(error){
    case EALREADY://todo:1 research why we would ever get this
      break;
    case EINPROGRESS:
      break;//treat the same as a return of 0
    case ECONNREFUSED:
      //#join
    default:
      connectionFailed(error);
      return false;
    }
  }
  //following dbg()s will spam with useless information unless you're debugging socket stuff
  if(connectArgs.block){//new way
    //dbg("TCPsocket Waiting for positive connection before sending data");
    connectionInProgress=true;
    writeInterest();
  } else { //old way: let connection errors fold into write errors
    //dbg("TCPsocket doing legacy sloppy connection, will be spamming data while connecting is in progress");
    connectionInProgress=false;//in case we change 'block' after some gross failure
    startReception();
    notifyConnected(true);
  }
  return true; //we didn't fail, doesn't mean 'actually connected'
}

void TcpSocket::disconnect(bool andNotify){
  TcpSocketBase::disconnect();
  if(andNotify){
    notifyConnected(false);
  }
}

void TcpSocket::flush(){
  //there is no flush in TCP.
  u8 bytes[4096] = {0};
  //can we stat a socket fd?
  int notforever=10000;
  while(sock.read(bytes, sizeof(bytes))==sizeof(bytes)){
    //a debug message here might slow us down enough to never catch up with the source and hence make this an infinite loop.
    if(--notforever<=0){
      return;
    }
  }
}

bool TcpSocket::readable() {
  u8 bytes[4096] = {0};
  stats.lastRead = sock.read(bytes, sizeof(bytes));
  if(stats.lastRead < 0) {
    //    dbg("read err: %d on %08X:%d",-stats.lastRead,connectArgs.ipv4, connectArgs.port); //useful for debug, but it will spam you
    disconnect(true);
    return false;
  } else if (stats.lastRead == 0) {//read returns zero when a client cleanly disconnects
    dbg("The remote client has disconnected");
    disconnect(true);
    return false;
  } else { //must be>0
    ByteScanner chunk(bytes,stats.lastRead);
    ++stats.reads;
    reader(chunk);
  }
  return true;
}

bool TcpSocket::writeable() {

  if(flagged(connectionInProgress)){//then we are finishing up connecting
    connectionTimer.stop();
  dbg("Completing connection.");
    int socketError(0);
    if(int error=sock.getOption(SOL_SOCKET,SO_ERROR,socketError)){
      connectionFailed(error);
      return false;
    }
    //leave this and the above separate for debug.
    if(socketError){
      connectionFailed(socketError);
      return false;
    }
    startReception();
    notifyConnected(true);
    return eagerToWrite;
  }
  //else socket is ready for write data
  eagerToWrite=0;
  if(sendbuf.hasNext() || writer(sendbuf)){
    stats.lastWrote = write(sock.fd, &sendbuf.peek(), sendbuf.freespace());
    if(stats.lastWrote<0) {
      connectionFailed(errno);
      dbg("write errno: %d on %08X:%d", -stats.lastWrote,connectArgs.ipv4, connectArgs.port);
      return false; //failure, lose data, drop connection and sleep until reconnected and writeInterest is called.
    } else if(stats.lastWrote>0) {
      ++stats.writes;//logs attempts
      sendbuf.skip(stats.lastWrote);
      bool more=sendbuf.hasNext();//usually false
      if(more){
        ++eagerToWrite;
      }
      return more;
    } else {
      //wtf? shouldn't be possible
      return true; //more please
    }
  } else {
    sendbuf.dump();//in case writer() was sloppy.
    return false;//nothing to send, sleep until writeInterest is called.
  }
}

/** expect this when far end of socket spontaneously closes*/
bool TcpSocket::hangup(){
  disconnect(true);
  return true;//todo:0 this probably should be false, may be moot.
}

void TcpSocket::writeInterest() {
  ++eagerToWrite; //we actually do want to send data
  if(!connectionInProgress){//in case we use a seperate callback for connection.
    if(source.writeInterest(MyHandler(TcpSocket::writeable))){
      ++newConnections;
    }
  }
}

TcpSocket::~TcpSocket(){
  disconnect(false);
}

sigc::connection TcpSocket::whenConnectionChanges(const BooleanSlot &nowConnected,bool kickme){
  if(kickme){
    nowConnected(isConnected());
  }
  return notifyConnected.connect(nowConnected);
}
///////////////////////////
TcpSocketBase::Stats::Stats(){
  clear();
}

void TcpSocketBase::Stats::clear(){
  connects=0;
  disconnects=0;
  lastRead=0;
  lastWrote=0;
  reads=0;
  writes=0;
}

///////////////////////////
bool TcpSocket::ConnectArgs::isPossible(){
  return port || ipv4;
}

TcpSocket::ConnectArgs::ConnectArgs(int ipv4, int port, bool noDelay, bool block):
  ipv4(ipv4),
  port(port),
  noDelay(noDelay),
  block(block){
  //#nada
}

void TcpSocket::ConnectArgs::erase(){
  ipv4=0;
  port=0;
  noDelay=false;
}

//////////////////////////////
SocketAddress::SocketAddress(){
  EraseThing(sin);
}

SocketAddress::SocketAddress(TcpSocket::ConnectArgs &cargs){
  EraseThing(sin);
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(cargs.ipv4);
  sin.sin_port = htons(cargs.port);
}

u16 SocketAddress::hostPort() const {
  return ntohs(sin.sin_port);
}

u32 SocketAddress::hostAddress() const {
  return ntohl(sin.sin_addr.s_addr);
}

const sockaddr *SocketAddress::addr() const {
  return reinterpret_cast<const sockaddr*>(&sin);
}

sockaddr *SocketAddress::addr() {
  return reinterpret_cast<sockaddr*>(&sin);
}

int SocketAddress::len() const {
  return sizeof(sin);
}

int SocketAddress::connect(int fd){
  return ::connect(fd, reinterpret_cast<sockaddr *>(&sin), sizeof(sin))?errno:0;
}
