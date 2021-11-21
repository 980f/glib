#ifndef DEFEREDEXECUTOR_H
#define DEFEREDEXECUTOR_H

#include "sigcuser.h"

/** do something later, in a way that can be used by a gui response.*/
class DeferedExecutor: SIGCTRACKABLE {
  sigc::connection conn;
  /** this is how we add a boolean return to a void slot.*/
  bool doAction();
  SimpleSlot action;
  /** had some problems with action taking longer than milliseconds.*/
  bool inProgress;
  int holding;//counted boolean, perhaps a barrier?
  bool held;
public:
  int milliseconds;
  int relativePriority;
public:
  /** @param relativePriority higher is sooner ie if a 3 and a 2 are both runnable the 3 will run.*/
  DeferedExecutor(int milliseconds,SimpleSlot action,int relativePriority=0);
  /** @param delay applied at time of next retrigger. <br>A delay of 0 is still delayed, but happens really soon, a negative delay executes within the retrigger call.
 @returns *this (so you can code: thing.setDelay(localvalue).retrigger())*/
  DeferedExecutor &setDelay(int milliseconds);
  /** start or restart a delay timer, upon whose expiration the action executes.*/
  void retrigger();
private:
  friend class DeferedExecutorHolder;
  /** if fires after hold don't execute until release*/
  void hold();
  /** will reschedule for zero delay if fired while held*/
  void release();
public:
  /** @return a slot that will retrigger this DE.*/
  SimpleSlot makeTrigger();
  /** cancel a deferred execution.*/
  void cancel();
  /** if running/pending or @param force execute now, @return whether action was invoked (ignores potential triviality of action)*/
  bool sync(bool force=false);
  /** is pending? may fire after the return and before you act on that return value.*/
  bool isPending();
};

/**
 *construction freezes execution, destruction executes (if triggered).
 *@see GatedSignal for instances where many slots for the same signal are one of these.
*/
class DeferedExecutorHolder {
  DeferedExecutor &defexec;
public:
  DeferedExecutorHolder (DeferedExecutor &defexec);
  ~DeferedExecutorHolder();
};

#endif // DEFEREDEXECUTOR_H
