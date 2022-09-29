#ifndef __COMMON_H__
#define __COMMON_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <Ecore.h>
#include <Evas.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_ELEMENTARY
# include <Elementary.h>
#endif

#   ifdef HAVE_GETTEXT
#define _(str) gettext(str)
#define d_(str, dom) dgettext(PACKAGE dom, str)
#define P_(str, str_p, n) ngettext(str, str_p, n)
#define dP_(str, str_p, n, dom) dngettext(PACKAGE dom, str, str_p, n)
#   else
#define _(str) (str)
#define d_(str, dom) (str)
#define P_(str, str_p, n) (str_p)
#define dP_(str, str_p, n, dom) (str_p)
#   endif

/* These macros are used to just mark strings for translation, this is useful
 * for string lists which are not dynamically allocated
 */
#define N_(str) (str)
#define NP_(str, str_p) str, str_p

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "translation.h"

#define EPULSE_THEME PACKAGE_DATA_DIR"default.edj"
#define BASE_VOLUME_STEP 10

EAPI extern int _log_domain;

#undef DBG
#undef INF
#undef WRN
#undef ERR
#undef CRI
#define CRIT(...)     EINA_LOG_DOM_CRIT(_log_domain, __VA_ARGS__)
#define ERR(...)      EINA_LOG_DOM_ERR(_log_domain, __VA_ARGS__)
#define WRN(...)      EINA_LOG_DOM_WARN(_log_domain, __VA_ARGS__)
#define INF(...)      EINA_LOG_DOM_INFO(_log_domain, __VA_ARGS__)
#define DBG(...)      EINA_LOG_DOM_DBG(_log_domain, __VA_ARGS__)


EAPI Eina_Bool epulse_common_init(const char *domain);
EAPI void epulse_common_shutdown(void);
EAPI Evas_Object *epulse_layout_add(Evas_Object *parent, const char *group,
                                    const char *style);

#endif /* __COMMON_H__ */
