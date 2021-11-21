#ifndef UICORE_H
#define UICORE_H

#include <glibmm/ustring.h>
#include <sigc++/sigc++.h>
#include "textkey.h"

/** marker for internal tag for an icon */
typedef const char * IconKey;

/** abstract enumeration, convert index into text. */
typedef sigc::slot < Glib::ustring, int /*ordinal*/ > Textifier;

/** Translate @param prompt and @see addColon(Glib::ustring) */
Glib::ustring addColon(TextKey prompt);
/** add a colon, with perhaps some space, and only if there isn't one already.*/
Glib::ustring addColon(const Glib::ustring &prompt);

/** translate a fixed string */
Glib::ustring translate(TextKey prompt); //probably in l10n.cpp

/** .95 should report -5.0%,  1.1 should return +10% */
Glib::ustring showFactorAsPercentChange(double factor,bool withSymbol=false);


//number concept not instantiated
#define NODATA "N/A"
//number concept not printable as number
#define BADDATA  "###"
//data depends on bad data
#define NOSOURCE "---"

#define EMPTYCELL NODATA
#define BADCELL BADDATA

#endif // UICORE_H
