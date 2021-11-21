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

}



/* snippet from running command on a real instrument. the arp command was copied over from an ubuntu 12.04 system.
root@OL1001:~# /nexqc/bin/arp -Dan
? (10.7.2.79) at 00:15:58:2c:89:67 [ether] on eth0
? (10.101.11.101) at c8:aa:cc:10:10:02 [ether] on eth1
? (10.101.11.102) at c8:aa:cc:10:10:01 [ether] on eth1

root@OL1001:~# bin/ipaddress.sh
IP address:  10.7.2.80/24
MAC address:  00:0b:ab:67:98:f1

ipaddress.sh:
echo "IP address: " `ip -4 addr show eth0 | grep inet | cut -d' ' -f6`
echo "MAC address: " `ip -0 addr show eth0 | grep ether | cut -d' ' -f6`

root@OL1001:~# ip -4 addr show eth0
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast qlen 1000
    inet 10.7.2.80/24 scope global eth0

root@OL1001:~# ip -0 addr show eth0
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast qlen 1000
    link/ether 00:0b:ab:67:98:f1 brd ff:ff:ff:ff:ff:ff

*/
