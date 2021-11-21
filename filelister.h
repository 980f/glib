#ifndef FILELISTER_H
#define FILELISTER_H

#include <giomm/file.h>
#include "sigcuser.h"

/** list a directory asynchronously.
 *todo: add "and watch" functionality */
class FileLister: SIGCTRACKABLE {
public:
  /** directory to list */
  Glib::RefPtr< Gio::File > dir;
protected:
  /** @return false to stop enumeration, called with @param FileInfo as nullptr after last file is delivered */
  sigc::slot<bool,Gio::FileInfo*> listener;

  /** called back by enumerate_children_async */
  void queryCompleted(Glib::RefPtr< Gio::AsyncResult > &result);

  /** called back by next_files_async */
  void listSome(Glib::RefPtr< Gio::AsyncResult > &result) ;

public:
  FileLister (Glib::RefPtr< Gio::File > dir,sigc::slot<bool,Gio::FileInfo*> listener);
  /** get File object for info that typically is passed to the listener */
  Glib:: RefPtr<Gio:: File > file(Gio::FileInfo* fileinfo);
  void run();
  int numFiles;

};

#endif // FILELISTER_H
