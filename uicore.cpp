#include "uicore.h"
#include "l10n.h"
#include "minimath.h"
#include "string.h"

/////////////////
/** globalized in case we implement ltr vs rtl*/
Glib::ustring addColon(const Glib::ustring &prompt){
  //todo:0 #353 if prompt already ends in colon or colon and whitespace then don't add extra
  return prompt + ": ";
}

Glib::ustring showFactorAsPercentChange(double factor, bool withSymbol) {
  if(isSignal(factor)){
    return BADDATA;
  } else {
    Glib::ustring deltaPercent(Glib::ustring::format(100 * (factor - 1.0)));
    return withSymbol? deltaPercent.append(" %") : deltaPercent;
  }
}

Glib::ustring addColon(TextKey prompt){
  return addColon(l10n->lookupText(prompt));
}



