#include "tcptester.h"
#include "logger.h"
void TcpTester::TestService::connectionChanged(bool connected){
  if(connected){
    dbg("connected another test service, %p",this);
  } else {
    //if remote disconnects we get here, which via the callback will delete this, so we can't call any functions on this object after calling onDisconnect.
    dbg("disconnected a test service, %p",this);
    onDisconnect(this);
  }
}

void TcpTester::TestService::reader(ByteScanner &raw){
  stats.reads++;
  idleDetector.retrigger();
  buffer.next()='<';
  while(raw.hasNext()){
    u8 character=raw.next();
    if(character==3){
      disconnect();
      return;
    }
    buffer.next()=toupper(character);
  }
  buffer.next()='>';
  //  if(buffer.hasNext()){
  writeInterest();
  //  }
}

bool TcpTester::TestService::writer(ByteScanner &raw){
  raw.grab(buffer);
  buffer.rewind();
  return true;
}

void TcpTester::TestService::goneQuiet(){
  dbg("service gone quiet for port: %d, fd: %d",this->connectArgs.port,sock.fd);
  //todo:2 can we get remote ip?
}

TcpTester::TestService::TestService(int fd, u32 ipv4, sigc::slot<void, TcpTester::TestService *> container):
  TcpSocket(fd,ipv4),
  onDisconnect(container),
  buffer(bufferAllocation,sizeof(bufferAllocation)),
  idleDetector(5000,MyHandler(TestService::goneQuiet))
{
  whenConnectionChanges(MyHandler(TestService::connectionChanged));
  idleDetector.makeTrigger();
  startReception();
}

bool TcpTester::TestService::disconnect(){
  TcpSocket::disconnect(true);
  return false;
}


void TcpTester::disconnect(TcpTester::TestService *dead){
  //the removal does a delete which in turn first disconnects. While the disconnect is futile when called from here it is not futile when shutting down this program.
  spawned.remove(dead);
}

TcpTester::TcpTester(int port, int backlog):TcpServer(port,0x7F000001,backlog){
  startup();
  //runMiniServer(port);
}

TcpSocket * TcpTester::spawnClient(int client_fd,u32 ipv4){//called on accept, needs to deploy
  dbg("TcpTester::spawn fd:%d",client_fd);
  return spawned.append(new TestService(client_fd,ipv4,MyHandler(TcpTester::disconnect)));
}

//todo:0 the following is apparently pasted in from somewhere on the web. Needs to be reworked to use our classes so that we are testing our classes.
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
bool TcpTester::runMiniServer(int port){
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  int n;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0){
    dbg("ERROR opening socket");
  }else{
    int optval(1);
    setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = port;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &serv_addr,
           sizeof(serv_addr)) < 0)
    dbg("ERROR on binding");
  listen(sockfd,5);
  clilen = sizeof(cli_addr);
  newsockfd = accept(sockfd,
                     (struct sockaddr *) &cli_addr,
                     &clilen);
  if (newsockfd < 0)
    dbg("ERROR on accept");
  while(true){
    bzero(buffer,256);
    n = read(newsockfd,buffer,255);
    if (n < 0){
      dbg("ERROR reading from socket");
      break;
    }else if(n==0){
      dbg("The connection was closed by the remote client");
      break;
    }
    dbg("Here is the message: %s",buffer);
    n = write(newsockfd,"I got your message\n",20);
    if (n < 0){
      dbg("ERROR writing to socket");
      break;
    }
  }
    close(newsockfd);
    close(sockfd);
    if (::shutdown(sockfd, SHUT_RDWR)){
      dbg("shutdown the socket.");}
    else{
      dbg("failed to shutdown the connection");
    }
  return 0;
}
