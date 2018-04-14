#ifndef __COMMON_H__
#define __COMMON_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <Ecore.h>
#include <Evas.h>
#include <Elementary.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "translation.h"

#define EPULSE_THEME PACKAGE_DATA_DIR"default.edj"
#define BASE_VOLUME_STEP 10

EAPI extern int _log_domain;

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
