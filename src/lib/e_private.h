#ifndef _E_PRIVATE_H
#define _E_PRIVATE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE___ATTRIBUTE__
#define __UNUSED__ __attribute__((unused))
#else
#define __UNUSED__
#endif

#define E_TYPEDEFS 1
#include "e_ipc.h"
#undef E_TYPEDEFS
#include "e_ipc.h"

#endif
