#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <glibmm.h>
#include "sigcuser.h"

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
