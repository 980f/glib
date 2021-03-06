#ifndef IOSOURCE_H
#define IOSOURCE_H  (C) 2020 Andrew Heilveil (github/980f) 

#include "sigcuser.h"

#include <unistd.h>
#include <errno.h>  //errno isn't necessarily a simple extern'd global any more, it is a macro.
#include <netinet/in.h>

/** collation of Gio stuff that interacts with int filedescriptors.*/
struct IoSource: SIGCTRACKABLE {
  int fd;
public:
  IoSource(int fd=~0);
  /** todo: debate closing the file when we delete this source */
  ~IoSource();
  /** in all the slot<bool>s below the return value if false drops the connection (normally return true) */
  sigc::connection watcher(int opts, sigc::slot<bool, int /*Glib::Iocondition enum */ > action);
  typedef sigc::slot<bool> Slot;//IoSource::Slot
  sigc::connection input(Slot reader);
  sigc::connection output(Slot writeable);
  sigc::connection hangup(Slot handler);
  
  /** wrap fd access */
  void close();
  /** @returns whether fd looks like it goes with a usable thing */
  bool isValid()const;
  ssize_t read (void *__buf, size_t __nbytes);
  ssize_t write (const void *__buf, size_t __n);
  
  /** merge return from read or write with errno, @return negative errno for most errors, 0 for errors worthy of simple retry (same as 0 bytes read or written) else the number of bytes successfully operated upon */
  static int recode(ssize_t rwreturn);


	/** set network option */
  template <typename OptionArgument> int setOption (int optFamily, int optname, OptionArgument optval){
    if(setsockopt(fd, optFamily, optname, &optval, sizeof(OptionArgument))){
      return errno;
    } else {
      return 0;
    }
  }

	/** get network option value */
  template <typename OptionArgument> int getOption(int optFamily, int optname, OptionArgument &optval){
    socklen_t length(sizeof(OptionArgument));
    if(getsockopt(fd, optFamily,optname, &optval, &length)){
      return errno;
    } else {
      return 0;
    }
  }

};

/** attach demons to IoSource events. */
struct IoConnections: SIGCTRACKABLE {
  IoConnections(IoSource &source);
  IoSource &source;
  sigc::connection incoming;
  sigc::connection outgoing;
  sigc::connection hangup;
  /** drop all callback registrations.
Note: that is automatic on destruction of each member, we don't need an explicit destructor.*/
  void disconnect();
  /** call this to set hook that will be called when the channel can accept write data.
 @returns whether it took some action to enable the write notification */
  bool whenWritable(IoSource::Slot action);
  
  /** set hooks for reception of data and hangup notifications. */
  void hookup(IoSource::Slot readAction,IoSource::Slot hangupAction);
  //write slurpInput() -- calls input function until we run out of input
};

#endif // IOSOURCE_H
