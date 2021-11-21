#include "processrunner.h"
#include "logger.h"

bool ProcessRunner::run(const std::vector<std::string> &argv){
  for(size_t ai=0;ai<argv.size();++ai){
    packed.append(argv[ai]);
    packed.append(" ");
  }
//our out is the shell'd process's in
  pid = Shell::run_async(argv, &out.fd, &in.fd,  &err.fd);
  if(~0 == pid) {
    dbg("Error starting external process %s",packed.c_str());
    return false;
  }
  in.input(sigc::bind(MyHandler(ProcessRunner::readable),0));
  in.hangup(sigc::bind(MyHandler(ProcessRunner::hangup),-1));
  //we don't out.output until there is a writeInterest else we get spammed with writes.

  if(mergErr){
    err.input(sigc::bind(MyHandler(ProcessRunner::readable),1));
//the following may be gratuitous, for normal completion conditions it is redundant
    err.hangup(sigc::bind(MyHandler(ProcessRunner::hangup),-2));
  }
  return true;
}

/** dummy implementation discards data and returns true */
bool ProcessRunner::readChunk(ByteScanner &incoming){
  incoming.dump();//mostly just to breakpoint, and for future line stitching.
  return true;
}

/** dummy implementation writes 0's over the data to send, always returns false */
bool ProcessRunner::writeSome(ByteScanner &outgoing){
  while(outgoing.hasNext()){
    outgoing.next()=0;
    return false;
  }
  return false;
}

bool ProcessRunner::readable(bool which) {
  u8 buff[4096];
  IoSource &ios(which?err:in);
  int len = ios.read(buff, sizeof(buff));
  if(len > 0) {//if we got some chars
    ByteScanner stuff(buff,len);
    if(readChunk(stuff)){
      return true;//expect to be called again
    }
    //else kill task
  } else if(len==0){
    return true;
  }
  //kill task:
  ios.close();
  hangup(which?-2:-1);
  return false;
}


/** no argument needed, we only allow for one type of ioc */
bool ProcessRunner::writeable() {
  //append to unsent stuff
  ByteScanner stuff;
  stuff.wrap(&sender.peek(),sender.freespace());
  bool partial=writeSome(stuff);
  sender.skip(stuff.used());
  //write what target will accept

  int length = out.write(sender.internalBuffer(), sender.used());
  if(length<0) {
    hangup(+1);
    out.close();
    return false; //failure, might call the hangup stuff from here
  } else if(length>0) {
    sender.skip(length);
  }
  return sender.hasNext()||partial;
}

/** expect this when far end of socket spontaneously closes*/
bool ProcessRunner::hangup(int which){
  dbg("hangup on %s from %d (%d-th)",packed.c_str(), which,++hangups);
  return false;
}

void ProcessRunner::writeInterest() {
  out.output(MyHandler(ProcessRunner::writeable));
}

ProcessRunner::ProcessRunner(bool mergeErr):
  sender(sendbuff,sizeof(sendbuff)),
  mergErr(mergeErr),
  hangups(0){
  //#nada, can't run yet as this class is often used as a base class and the extended class would not be ready to receive.
}
