#include "iosource.h"
#include "glibmm/iochannel.h"
#include "glibmm/refptr.h"
#include "glibmm/main.h" //timeout and other small glib classes

IoSource::IoSource(int fd):
  fd(fd){
  //#nada
}

IoSource::~IoSource(){
  //created for breakpoint
}


sigc::connection IoSource::watcher(int opts, sigc::slot<bool, int> action){
  //todo:1 document what an invalid fd does here.
  Glib::RefPtr< Glib::IOSource > source = Glib::IOSource::create(fd, Glib::IOCondition(opts));
  sigc::connection connection = source->connect(action);
  source->attach();//this is supposed to keep the Glib::IOSource in existence.
  return connection;
}

sigc::connection IoSource::input(Slot reader){
  return watcher(Glib::IO_IN,sigc::hide(reader));
}

sigc::connection IoSource::output(Slot writeable){
  return watcher(Glib::IO_OUT,sigc::hide(writeable));
}

sigc::connection IoSource::hangup(IoSource::Slot handler){
  return watcher(Glib::IO_HUP,sigc::hide(handler));
}


bool IoSource::isValid() const {
  return fd!=~0;//unix standard, >=0 would probably be fine
}

#if ArtWithPosix
ssize_t IoSource::read(void *__buf, size_t __nbytes){
  //todo:1 create breakpoints for using invalid fd.
  return  recode(::read(fd,__buf,__nbytes));
}

ssize_t IoSource::write(const void *__buf, size_t __n){
  return recode(::write (fd, __buf, __n));
}

int IoSource::recode(ssize_t rwreturn){
  if(rwreturn==~0){
    int errnum(errno);
    if(EINTR == errnum || EAGAIN == errnum || EWOULDBLOCK == errnum) {
      return 0;//still more perhaps.
    }
    else {
      return -errnum;
    }
  } else {
    return rwreturn;
  }
}

void IoSource::close(){
  ::close(fd);
  fd = ~0;
}
#endif

IoConnections::IoConnections(IoSource &source):
  source(source){
  //#nada
}

void IoConnections::disconnect(){
  incoming.disconnect();
  hangup.disconnect();
  outgoing.disconnect();
}

bool IoConnections::writeInterest(IoSource::Slot action){
  if(!outgoing) {
    outgoing= source.output(action);
    return true;
  } else {
    return false;
  }
}

void IoConnections::hookup(IoSource::Slot readAction, IoSource::Slot hangupAction){
  if(readAction){
    incoming=source.input(readAction);
  }
  if(hangupAction){
    hangup=source.hangup(hangupAction);
  }
}


