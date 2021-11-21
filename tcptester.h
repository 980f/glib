#ifndef TCPTESTER_H
#define TCPTESTER_H
//////////////////////////////////////////

#include "tcpserver.h"
#include "chain.h"

#include "deferedexecutor.h"

class TcpTester: public TcpServer {
  class TestService : public TcpSocket {
    friend class TcpTester;
    //notify on disconnects, including on destructor
    sigc::slot<void,TestService *> onDisconnect;

    /** expect this to be called with connected==false only when remote disconnects. */
    void connectionChanged(bool connected);

    u8 bufferAllocation[100];
    ByteScanner buffer;
    DeferedExecutor idleDetector;

    void reader(ByteScanner&raw);

    /** called when can write, should set ByteScanner to point to data, and return true if should be sent.*/
    bool writer(ByteScanner&raw);

    /** remote has gone quiet*/
    void goneQuiet();
  public:
    TestService(int fd,u32 ipv4,sigc::slot<void,TestService *>container);

    /** */
    bool disconnect(void);

  };
  ///////////////////////////////
  Chain<TestService> spawned;//todo:1 intermediate class of MultiTcpServer with templated list and factory.

  /** called by @param dead when it loses its remote connection. */
  void disconnect(TestService *dead);
public:
  TcpTester(int port,int backlog=3);

  TcpSocket *spawnClient(int client_fd, u32 ipv4);
  bool runMiniServer(int port);
};


#endif // TCPTESTER_H
