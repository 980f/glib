#ifndef CYCLICACTION_H
#define CYCLICACTION_H

#include "sigcuser.h"
/** a restartable, run only one instance action */
class CyclicAction {
public:
  /** if action returns false cycling stops */
  typedef sigc::slot<bool> Runnable;
  sigc::connection cycler;
protected:
  Runnable action;
  double seconds;
  bool paused;//simple one, doesn't tolerate nesting.
public:
  CyclicAction(Runnable action,double seconds=0,bool startNow=false);
  ~CyclicAction();
  /** starts cycling if period>0. @param andInvoke true will run the action regardless of whether cycling starts. */
  void run(bool andInvoke=false);
  /** @returns whether it (timing, not the action) was running before this instance of calling stop*/
  bool stop();
  /** @returns whether it (timing, not the action) was running before this instance of calling pause, which is false if already paused.*/
  bool pause();
  /** start again if was paused, @returns whether it was blocked. */
  bool resume(bool immediately);
  void setCycleTime(double seconds);
  /** @returns whether cycler is not stopped*/
  operator bool()const;
  /** @returns whether cycler is fully stopped.*/
  bool isStopped()const{
    return cycler.empty();//perhaps disconnected() is slightly different?
  }
  /** run the action now */
  void trigger();
private:
  BooleanSignal cyclingChangedTo;
public:
  /** inform someone of state change.
   * Will sometimes call the action when there isn't a change, if you must strictly react to changes then your action will locally have to do a change detect.*/
  sigc::connection whenRunOrStopped(BooleanSlot action, bool kickme=true);
};

/** uses RIAA to pause and resume a cycler*/
class CyclerPauser{
  CyclicAction &cycler;
  bool immediately;
public:
  CyclerPauser(CyclicAction &cycler,bool immediately=true);
  ~CyclerPauser();
  /** unpause now*/
  void resume();
};

#endif // CYCLICACTION_H
