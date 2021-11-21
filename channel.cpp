#include "channel.h"

using namespace sigc;
using namespace Glib;

Channel::Channel(void):
  refcount(1),
  queue(g_async_queue_new()) {
  dispatcher.connect(MyHandler(Channel::dispatch));
}

Channel::~Channel(void) {
  Mutex::Lock lock(mutex);
  if(refcount > 0) {
    lock.release();
    g_error("Channel destructed with undelivered messages");
  }
  lock.release();
  g_async_queue_unref(queue);
}

void Channel::unref(void) {//todo:1 prove no use after unref, need a ChannelRef class
  Mutex::Lock lock(mutex);
  --refcount;
  if(refcount <= 0) {
    lock.release();
    delete this;//NEW@ all constructions store reference to a new Channel
  }
}

void Channel::send(gpointer data) {
  if(!data) { //
    g_error("The queue doesn't like NULLs");
  }
  Mutex::Lock lock(mutex);
  ++refcount;
  lock.release();
  g_async_queue_push(queue, data);
  dispatcher();
}

void Channel::sendNow(gpointer data) {
  sig(data);
}

connection Channel::connect(const t_slot &slot) {
  return sig.connect(slot);
}

void Channel::dispatch(void) {
  sig(g_async_queue_pop(queue));
  unref();
}
