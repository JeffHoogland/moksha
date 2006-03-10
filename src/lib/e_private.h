#ifndef E_PRIVATE_H
#define E_PRIVATE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef USE_IPC
  
#if HAVE___ATTRIBUTE__
#define __UNUSED__ __attribute__((unused))
#else
#define __UNUSED__
#endif

#define E_REMOTE_OPTIONS 1
#define E_REMOTE_OUT     2
#define E_WM_IN          3
#define E_REMOTE_IN      4
#define E_ENUM           5
#define E_LIB_IN         6

#define E_TYPEDEFS 1
#include "e_ipc.h"
#undef E_TYPEDEFS
#include "e_ipc.h"

#endif

#endif


