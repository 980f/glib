#include "filelister.h"

#include "iterate.h"
#include "logger.h"

using namespace Glib;
using namespace Gio;

void FileLister::queryCompleted(Glib::RefPtr<Gio::AsyncResult> &result) {
  RefPtr< File > dir(RefPtr< File >::cast_dynamic(result->get_source_object_base()));
  try {
    RefPtr< FileEnumerator > enumerator(dir->enumerate_children_finish(result));
    enumerator->next_files_async(MyHandler(FileLister::listSome));
  } catch(Glib::Error err) {
    IgnoreGlib(err);
  }
}
//NB:  This needs to be tested for leaks.
void FileLister::listSome(Glib::RefPtr<Gio::AsyncResult> &result){
  RefPtr< FileEnumerator > enumerator(RefPtr< FileEnumerator >::cast_dynamic(result->get_source_object_base()));
  try {
    ListHandle< RefPtr< FileInfo > > list(enumerator->next_files_finish(result));
    int chunk=list.size();
    //dbg("FileLister chunk:%d",chunk);  //note: debug text, this will spam you
    if(chunk) {
      ITERATE(ListHandle< RefPtr< FileInfo > >::const_iterator, it, list) {
        ++numFiles;
        if(!listener((*it).operator ->())){//gotta just love RefPtr syntax (not)
          return;//if we stored a cancellable here is where we would invoke it.
        }
      }
      enumerator->next_files_async(MyHandler(FileLister::listSome));
    } else {
      listener(nullptr);//# and ignore 'end early' return as we are ending now :)
    }
  } catch(Glib::Error err) {
    IgnoreGlib(err);
  }
}

FileLister::FileLister(Glib::RefPtr<File> dir, sigc::slot<bool, FileInfo *> listener):dir(dir),listener(listener), numFiles(0){
  //#nada
}

Glib::RefPtr<File> FileLister::file(FileInfo *fileinfo){
  return dir->get_child(fileinfo->get_name());
}

void FileLister::run(){
  dir->enumerate_children_async( MyHandler(FileLister::queryCompleted),                                            ustring::format(G_FILE_ATTRIBUTE_STANDARD_NAME, ",", G_FILE_ATTRIBUTE_TIME_MODIFIED));
}
