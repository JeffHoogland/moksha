#ifndef __TRANSLATION_H__
#define __TRANSLATION_H__

#include <libintl.h>


#define _(str) gettext(str)
#define d_(str, dom) dgettext(dom, str)
#define P_(str, str_p, n) ngettext(str, str_p, n)
#define dP_(str, str_p, n, dom) dngettext(dom, str, str_p, n)
#define N_(str) str
#define NP_(str, str_p) str, str_p

#endif /* __TRANSLATION_H__ */
