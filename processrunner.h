#ifndef PROCESSRUNNER_H
#define PROCESSRUNNER_H

#include "iosource.h"
#include "shell.h"
#include "charscanner.h"

class ProcessRunner {
  /** buffer for outgoing data, although later changes made it possible to push this back into using classes.*/
  u8 sendbuff[4096];
  ByteScanner sender;
  /** whether to make the data on the err stream seem as if it comes from the in.*/
  bool mergErr;
protected:
  int hangups;
  Glib::Pid pid;
  /** in to this module from process out*/
  IoSource in;
  /** in to this module from process err*/
  IoSource err;
  /** out from this module to process in*/
  IoSource out;
  /** just for debug: assembled commandline */
  Glib::ustring packed;
private: //internal linkage, @see readChunk
    /** @returns true if more input is desired/expected, false to kill task */
  bool readable(bool which);
  /** called by glib when data can be written, @return true is the internal buffer isn't empty after a write to the stream, @see writeSome */
  bool writeable();
protected:
  /** hooked to signal termination. @param which: -1 read error, +1 write error, 0=ctrl-C or the like*/
  virtual bool hangup(int which);
  /** called by this module when actual read data has arrived.
   * reserved for future signalling: a chunk of 0 might be sent to indicate normal end-of-input, presently that gets sent via hangup.
   *@return true if more input is desired/expected, false to kill task */
  virtual bool readChunk(ByteScanner &incoming);
  /** called by this module to get data to send to process.
   *@return true if there is already more data to send beyond what you stuff into outgoing*/
  virtual bool writeSome(ByteScanner &outgoing);

public:
  /** just prepares to run the process @param mergErr @see mergErr member. default of true is statistical by textual occurence */
  ProcessRunner(bool mergeErr=true);
  typedef std::vector<std::string> Args;
  /** @returns whether process launched successfully, e.g. will return false if command not found */
  bool run(const Args &argv);
  /** call this when you would like to write something, writeSome will get called to sak for the data to send*/
  void writeInterest();
};

#endif // PROCESSRUNNER_H
