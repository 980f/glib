#include <glibmm.h>

#include "cheapTricks.h"
#include "deferedexecutor.h"
#include "logger.h"

using namespace Glib;

DeferedExecutor::DeferedExecutor(int milliseconds, SimpleSlot action, int relativePriority):
  action(action),
  inProgress(false),
  holding(false),
  held(false),
  milliseconds(milliseconds),
  relativePriority(relativePriority) {
  //#do NOT trigger on creation.
}

DeferedExecutor& DeferedExecutor::setDelay(int milliseconds){
  this->milliseconds=milliseconds;
  return *this;
}

void DeferedExecutor::cancel(){
  conn.disconnect();
}

bool DeferedExecutor::sync(bool force){
  ConnectionLocker locked(conn);
  if(locked&&(force||conn)){
    doAction();//disconnecting before acting allows for a retrigger, usually a confusing thing for an implementation to do but not this guy's problem.
    return true;
  } else {
    return false;
  }
}

void DeferedExecutor::retrigger(){
  if(milliseconds<0){
    doAction();//run immediately if not configured for delay.
  } else {
    if(conn){//checking for debug reasons.
      conn.disconnect();//not using cancel for debug reasons.
    }
    held=false;
    conn = signal_timeout().connect(MyHandler(DeferedExecutor::doAction), milliseconds, PRIORITY_DEFAULT_IDLE - relativePriority - 1);
  }
}

void DeferedExecutor::hold(){
  ++holding;
}

/** all holds and releases must execute on the same thread, and you must release for each hold */
void DeferedExecutor::release(){
  if(--holding==0){
    if(flagged(held)){//then we fired while holding
      conn = signal_timeout().connect(MyHandler(DeferedExecutor::doAction), 0, PRIORITY_DEFAULT_IDLE - relativePriority -1);
    }
  } else if(holding<0){
    holding=0;
    wtf("more DE releases than holds");
  }
}

bool DeferedExecutor::doAction(){
  cancel();
  if(holding){
    held=true;
  } else {
    action();
  }
  return false; //we run once then are quiet until after the next retrigger.
}

bool DeferedExecutor::isPending(){
  return conn;
}

SimpleSlot DeferedExecutor::makeTrigger(){
  return MyHandler(DeferedExecutor::retrigger);
}

///////////////////
DeferedExecutorHolder::DeferedExecutorHolder(DeferedExecutor &defexec):defexec(defexec){
  defexec.hold();
}

DeferedExecutorHolder::~DeferedExecutorHolder(){
  defexec.release();
}
