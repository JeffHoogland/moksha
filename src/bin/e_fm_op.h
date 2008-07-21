/*
 * vim:cindent:ts=8:sw=3:sts=8:expandtab:cino=>5n-3f0^-2{2
 */
#ifndef _E_FM_OP_H
#define _E_FM_OP_H

#define E_FM_OP_DEBUG(...) fprintf(stderr, __VA_ARGS__)

#define E_FM_OP_MAGIC 314

typedef enum _E_Fm_Op_Type
{
   E_FM_OP_COPY = 0,
   E_FM_OP_MOVE = 1,
   E_FM_OP_REMOVE = 2,
   E_FM_OP_ABORT = 3,
   E_FM_OP_ERROR = 4,
   E_FM_OP_ERROR_RESPONSE_IGNORE_THIS = 5,
   E_FM_OP_ERROR_RESPONSE_IGNORE_ALL = 6,
   E_FM_OP_ERROR_RESPONSE_ABORT = 7,
   E_FM_OP_PROGRESS = 8,
   E_FM_OP_NONE = 9,
   E_FM_OP_ERROR_RESPONSE_RETRY = 10,
   E_FM_OP_OVERWRITE = 11,
   E_FM_OP_OVERWRITE_RESPONSE_NO = 12,
   E_FM_OP_OVERWRITE_RESPONSE_NO_ALL = 13,
   E_FM_OP_OVERWRITE_RESPONSE_YES = 14,
   E_FM_OP_OVERWRITE_RESPONSE_YES_ALL = 15,
   E_FM_OP_COPY_STAT_INFO = 16
} E_Fm_Op_Type;

#endif
