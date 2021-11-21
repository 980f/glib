#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include "sigcuser.h"
#include "charscanner.h"
#include "iosource.h"
#include "stopwatch.h" //for blocking connection timeout use

/** common part of client and server sockets. server socket itself is declared elsewhere.
  made sgc::trackable as its extensions all want to be. */
class TcpSocketBase: SIGCTRACKABLE {
public:
    /** diagnostic counters */
    struct Stats {
      /** successful connection */
      u32 connects;
      /** failed attempt or lost connection after succeeded */
      u32 disconnects;
      /** read events */
      u32 reads;
      /** write events */
      u32 writes;
      /** returned values from last io read command */
      int lastRead;
       /** returned values from last io write command */
      int lastWrote;
      Stats();
      void clear();
    } stats;
    /** a slot to track connects and disconnects */
    void connectionEvent(bool connected);
  /** host ordered connection parameters */
  struct ConnectArgs {
    u32 ipv4;
    u16 port;
    /** request that the local tcp stack not try to accumulate bytes to send before sending. */
    bool noDelay;
    /** request that we block on connection, i.e. even on explicit failure to connect do not retry until timeout has expired */
    bool block;
    ConnectArgs(int ipv4=0, int port=0, bool noDelay=false,bool block=false);//defaults are the same values as set by @see erase

    /** sets to blatantly ridiculous values, @see isPossible()*/
    void erase();
    /** @returns whether neither critical parameter is 0 */
    bool isPossible();
  } connectArgs;

protected:
  IoSource sock;
  IoConnections source;
  /** create from an fd of an open (connected) socket or use the default arg and call connect() */
  TcpSocketBase(int fd=~0,u32 remoteAddress=0,int port=0);

public:
  virtual ~TcpSocketBase();
  /** @returns address of other end of link */
  u32 remoteIpv4();
  /** @returns whether socket seems to be open */
  bool isConnected() const;
  /** call this to disconnect the socket.
 @returns false so that it easily drops into a TcpSocket::Slot*/
  virtual bool disconnect();
};

//////////////////////////////////////////////

/** client socket */
class TcpSocket: public TcpSocketBase, SIGCTRACKABLE {
protected:
  /** create from an fd of an open (connected) socket or use the default arg and call connect() */
  TcpSocket(int fd=~0,u32 remoteAddress=0,int port=0);

  /** enables attempts to reconnect */
  bool autoConnect;
  /** pointer into actual sending buffer, which is NOT part of this object. */
  ByteScanner sendbuf;
  /** called by glib when there is something to read (after startReception has once been called) */
  bool readable();
  /** called by glib when remote disconnects (after startReception has once been called) */
  bool hangup();
  /** call this when you would like to write something, get called back for the data inside writeable*/
  void writeInterest();
  /** called by glib when data can be written (after writeInterest has recently been called) */
  bool writeable();
public:
  virtual ~TcpSocket();
  /** @returns isConnected() */
  bool connect(unsigned ipv4, unsigned port, bool noDelay, bool block);
  /** disconnect and release socket, if @param andNotify then call 'onConnectionChange' actions*/
  void disconnect(bool andNotify);
  /** throw away all pending input, best effort-some bytes might sneak in right after it returns.
   * this is BLOCKING, but due to the way Glib works it will leave in a finite amount of time.
   * added to dump the qchardware incoming queue when the logic is way behind */
  void flush();
  BooleanSignal notifyConnected;//todo:1 our usual hiding behind a funciton call that returns a connection object.
protected:
  /** called when some data has arrived. You MUST copy the data, the underlying pointer of @param raw is to a piece of the stack. */
  virtual void reader(ByteScanner&raw)=0;
  /** called when can write, should set ByteScanner to point to data, and return true if should be sent.
  The data YOU point to by modifying @param raw must stay allocated until the next call to writer(). You could poll the TcpSocket to see if it is done with the write, we should probably add a callback for 'transmit buffer empty'.*/
  virtual bool writer(ByteScanner&raw)=0;

public:
  sigc::connection whenConnectionChanges(const BooleanSlot &nowConnected, bool kickme=false);
  bool reconnect();
  void startReception();
  /** temporary (hah!) debug flags, tracking writeInterest calling */
  int eagerToWrite;
  int newConnections;
  /** true from calling ::connect() until first writeable event */
  bool connectionInProgress;
  /** how long we have been waiting for a connection to complete */
  StopWatch connectionTimer;
protected:
  /** called when connection failes, @param error is an errno value */
  void connectionFailed(int error);
};

#include <netinet/in.h>
/** address etc will be maintained in network order, with accessors for host order.*/
struct SocketAddress{
  sockaddr_in sin;
  /** for one that is to be filled by a posix call*/
  SocketAddress();
  /** for one that is to be sent to a posix call */
  SocketAddress(TcpSocket::ConnectArgs &cargs);
  u16 hostPort()const;
  u32 hostAddress()const;
  /** address for posix functions */
  const sockaddr *addr()const;
  sockaddr *addr();

  /** struct size for posix functions*/
  int len()const;
  /** @returns errno if an ERROR occured (else 0), @param fd is value returned from your call to socket() */
  int connect(int fd);
};


#endif // TCPSOCKET_H
