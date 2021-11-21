#ifndef NETWORKSCANNER_H
#define NETWORKSCANNER_H

#include "eztypes.h"
#include "chain.h"
#include "processrunner.h"

class NetworkScanner: public ProcessRunner {
  SimpleSignal onDone;//who to call when list is refreshed
  /** which command was issued and hence how to parse the return data */
  bool viaARP;//, else host and slaves, using arp
public:
  struct Pair {
    u32 ipv4;
    u64 mac;
  };
  Chain<Pair> resolved;
public:
  NetworkScanner(bool run,SimpleSlot onDone);
  /** invoke a scan, @returns a connection with which to disable notifications */
  sigc::connection scan(SimpleSlot onDone);
private:
  /** called by this module when actual read data has arrived.
   * reserved for future signalling: a chunk of 0 might be sent to indicate normal end-of-input, presently that gets sent via hangup.
   *@return true if more input is desired/expected, false to kill task */
  bool readChunk(ByteScanner &incoming);
  /** receives line per host */
  bool parseArp(ByteScanner &incoming);
  /** only receives localhost info, one item per line */
  bool parseIp(ByteScanner &incoming);

};

#endif // NETWORKSCANNER_H
