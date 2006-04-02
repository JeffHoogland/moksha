/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_SIGNALS_H
#define E_SIGNALS_H

EAPI void e_sigseg_act(int x, siginfo_t *info, void *data);
EAPI void e_sigill_act(int x, siginfo_t *info, void *data);
EAPI void e_sigfpe_act(int x, siginfo_t *info, void *data);
EAPI void e_sigbus_act(int x, siginfo_t *info, void *data);
EAPI void e_sigabrt_act(int x, siginfo_t *info, void *data);

#endif
#endif
