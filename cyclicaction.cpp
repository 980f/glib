#include "cyclicaction.h"
#include "glibmm.h"
#include "cheapTricks.h"

CyclicAction::CyclicAction(Runnable action, double seconds, bool startNow):
  action(action),seconds(seconds),paused(false){
  if(startNow){
    run();
  }
}

/** this implementation is probably not needed, the cycler's destructor is supposed to do a disconnect.*/
CyclicAction::~CyclicAction(){
  cycler.disconnect();
}

void CyclicAction::run(bool andInvoke){
  bool wasRunning=cycler;//failed to notify stopped when cancelled when paused.
  if(seconds>0.0){
    cycler.disconnect();//#don't call stop here, don't want to signal stop for this nanoscopic one.
    cycler=Glib::signal_timeout().connect(action,seconds*1000);
    if(!wasRunning){
      cyclingChangedTo(true);
    }
  }
  if(andInvoke){
    action();
  }
}

bool CyclicAction::stop(){
  bool wasRunning=cycler;
  cycler.disconnect();
  if(wasRunning){
    cyclingChangedTo(paused);
  }
  return wasRunning;
}

bool CyclicAction::pause(){
  paused=cycler;//setting this here keeps stop from triggering an offline indication.
  return stop();
}

bool CyclicAction::resume(bool immediately){
  if(flagged(paused)){
    run(immediately);
    return true;
  }
  return false;  //cycler.unblock();
}

void CyclicAction::setCycleTime(double seconds){
  if(changed(this->seconds,seconds)){
    if(cycler){//if already running need to jerk the timer logic
      run();
    }
  }
}

CyclicAction::operator bool() const {
  return paused || cycler.connected();//first term may be gratuitous
}

sigc::connection CyclicAction::whenRunOrStopped(BooleanSlot action,bool kickme){
  if(kickme){
    action(cycler.connected());
  }
  return cyclingChangedTo.connect(action);
}

void CyclicAction::trigger(){
  action();
}
//////////////////////////

CyclerPauser::CyclerPauser(CyclicAction &cycler, bool immediately):
  cycler(cycler),
  immediately(immediately){
  cycler.pause();
}

CyclerPauser::~CyclerPauser(){
  resume();//todo: constructor arg for this.
}

void CyclerPauser::resume(){
  cycler.resume(immediately);
  immediately=false;//but not again on exit.
}
