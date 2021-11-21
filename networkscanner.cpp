#include "networkscanner.h"

NetworkScanner::NetworkScanner(bool run, SimpleSlot onDone){
  if(run){
    scan(onDone);
  }
}

sigc::connection NetworkScanner::scan(SimpleSlot onDone){
  Args argv;
  argv.push_back(FileName("bin").folder("arp"));
  argv.push_back("-Dan"); //'D': format for easy parsing; 'a': the info we want; 'n':no hostnames, keep addresses numerical

  if(run(argv)){
    return this->onDone.connect(onDone);
  } else {
    return sigc::connection();
  }
}

bool NetworkScanner::readChunk(ByteScanner &incoming){
  if(viaARP){
    return parseArp(incoming);
  } else {
    return parseIp(incoming);
  }
}

bool NetworkScanner::parseArp(ByteScanner &incoming){
	//todo: actually parse arp content.
}

