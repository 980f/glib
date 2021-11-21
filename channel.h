#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <glibmm.h>
#include "sigcuser.h"

/** wraps an asynch message queue 
	it has a refcount that includes itself.
	it delete's itself if you unref() when there are no messages pending.
	
@deprecated	no known users.
*/
class Channel: SIGCTRACKABLE {
public:
  typedef sigc::slot< void, gpointer > t_slot;
  typedef sigc::signal< void, gpointer > t_signal;

private:
  Glib::Dispatcher dispatcher;
  t_signal sig;
  Glib::Mutex mutex;
  int refcount;
  GAsyncQueue *queue;

  void dispatch(void);
public:
  Channel(void);
  ~Channel(void);
  void unref(void);
  void send(gpointer data);
  void sendNow(gpointer data);
  sigc::connection connect(const t_slot &slot);
};


#endif // _CHANNEL_H_
