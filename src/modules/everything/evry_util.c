#include "e_mod_main.h"
#include "md5.h"

#define MAX_FUZZ  100
#define MAX_WORDS 5

static const char *home_dir = NULL;
static int home_dir_len;
static char dir_buf[1024];
static char thumb_buf[4096];


/* Functions are too complex to cover UTF8, UTF16 and UTF32 to work weeks on it. Code taken from:
   https://stackoverflow.com/questions/36897781/how-to-uppercase-lowercase-utf-8-characters-in-c */

unsigned char * 
StrToLwrExt(unsigned char* pString)
{
   unsigned char* p = pString;
   unsigned char* pExtChar = 0;

   if (pString && *pString) {
            while (*p) {
            if ((*p >= 0x41) && (*p <= 0x5a)) /* US ASCII */
                (*p) += 0x20;
            else if (*p > 0xc0) {
                pExtChar = p;
                p++;
                switch (*pExtChar) {
                case 0xc3: /* Latin 1 */
                    if ((*p >= 0x80)
                        && (*p <= 0x9e)
                        && (*p != 0x97))
                        (*p) += 0x20; /* US ASCII shift */
                    break;
                case 0xc4: /* Latin ext */
                    if (((*p >= 0x80)
                        && (*p <= 0xb7)
                        && (*p != 0xb0))
                        && (!(*p % 2))) /* Even */
                        (*p)++; /* Next char is lwr */
                    else if ((*p >= 0xb9)
                        && (*p <= 0xbe)
                        && (*p % 2)) /* Odd */
                        (*p)++; /* Next char is lwr */
                    else if (*p == 0xbf) {
                        *pExtChar = 0xc5;
                        (*p) = 0x80;
                    }
                    break;
                case 0xc5: /* Latin ext */
                    if ((*p >= 0x81)
                        && (*p <= 0x88)
                        && (*p % 2)) /* Odd */
                        (*p)++; /* Next char is lwr */
                    else if ((*p >= 0x8a)
                        && (*p <= 0xb7)
                        && (!(*p % 2))) /* Even */
                        (*p)++; /* Next char is lwr */
                    else if (*p == 0xb8) {
                        *pExtChar = 0xc3;
                        (*p) = 0xbf;
                    }
                    else if ((*p >= 0xb9)
                        && (*p <= 0xbe)
                        && (*p % 2)) /* Odd */
                        (*p)++; /* Next char is lwr */
                    break;
                case 0xc6: /* Latin ext */
                    switch (*p) {
                    case 0x81:
                        *pExtChar = 0xc9;
                        (*p) = 0x93;
                        break;
                    case 0x86:
                        *pExtChar = 0xc9;
                        (*p) = 0x94;
                        break;
                    case 0x89:
                        *pExtChar = 0xc9;
                        (*p) = 0x96;
                        break;
                    case 0x8a:
                        *pExtChar = 0xc9;
                        (*p) = 0x97;
                        break;
                    case 0x8e:
                        *pExtChar = 0xc9;
                        (*p) = 0x98;
                        break;
                    case 0x8f:
                        *pExtChar = 0xc9;
                        (*p) = 0x99;
                        break;
                    case 0x90:
                        *pExtChar = 0xc9;
                        (*p) = 0x9b;
                        break;
                    case 0x93:
                        *pExtChar = 0xc9;
                        (*p) = 0xa0;
                        break;
                    case 0x94:
                        *pExtChar = 0xc9;
                        (*p) = 0xa3;
                        break;
                    case 0x96:
                        *pExtChar = 0xc9;
                        (*p) = 0xa9;
                        break;
                    case 0x97:
                        *pExtChar = 0xc9;
                        (*p) = 0xa8;
                        break;
                    case 0x9c:
                        *pExtChar = 0xc9;
                        (*p) = 0xaf;
                        break;
                    case 0x9d:
                        *pExtChar = 0xc9;
                        (*p) = 0xb2;
                        break;
                    case 0x9f:
                        *pExtChar = 0xc9;
                        (*p) = 0xb5;
                        break;
                    case 0xa9:
                        *pExtChar = 0xca;
                        (*p) = 0x83;
                        break;
                    case 0xae:
                        *pExtChar = 0xca;
                        (*p) = 0x88;
                        break;
                    case 0xb1:
                        *pExtChar = 0xca;
                        (*p) = 0x8a;
                        break;
                    case 0xb2:
                        *pExtChar = 0xca;
                        (*p) = 0x8b;
                        break;
                    case 0xb7:
                        *pExtChar = 0xca;
                        (*p) = 0x92;
                        break;
                    case 0x82:
                    case 0x84:
                    case 0x87:
                    case 0x8b:
                    case 0x91:
                    case 0x98:
                    case 0xa0:
                    case 0xa2:
                    case 0xa4:
                    case 0xa7:
                    case 0xac:
                    case 0xaf:
                    case 0xb3:
                    case 0xb5:
                    case 0xb8:
                    case 0xbc:
                        (*p)++; /* Next char is lwr */
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xc7: /* Latin ext */
                    if (*p == 0x84)
                        (*p) = 0x86;
                    else if (*p == 0x85)
                        (*p)++; /* Next char is lwr */
                    else if (*p == 0x87)
                        (*p) = 0x89;
                    else if (*p == 0x88)
                        (*p)++; /* Next char is lwr */
                    else if (*p == 0x8a)
                        (*p) = 0x8c;
                    else if (*p == 0x8b)
                        (*p)++; /* Next char is lwr */
                    else if ((*p >= 0x8d)
                        && (*p <= 0x9c)
                        && (*p % 2)) /* Odd */
                        (*p)++; /* Next char is lwr */
                    else if ((*p >= 0x9e)
                        && (*p <= 0xaf)
                        && (!(*p % 2))) /* Even */
                        (*p)++; /* Next char is lwr */
                    else if (*p == 0xb1)
                        (*p) = 0xb3;
                    else if (*p == 0xb2)
                        (*p)++; /* Next char is lwr */
                    else if (*p == 0xb4)
                        (*p)++; /* Next char is lwr */
                    else if (*p == 0xb6) {
                        *pExtChar = 0xc6;
                        (*p) = 0x95;
                    }
                    else if (*p == 0xb7) {
                        *pExtChar = 0xc6;
                        (*p) = 0xbf;
                    }
                    else if ((*p >= 0xb8)
                        && (*p <= 0xbf)
                        && (!(*p % 2))) /* Even */
                        (*p)++; /* Next char is lwr */
                    break;
                case 0xc8: /* Latin ext */
                    if ((*p >= 0x80)
                        && (*p <= 0x9f)
                        && (!(*p % 2))) /* Even */
                        (*p)++; /* Next char is lwr */
                    else if (*p == 0xa0) {
                        *pExtChar = 0xc6;
                        (*p) = 0x9e;
                    }
                    else if ((*p >= 0xa2)
                        && (*p <= 0xb3)
                        && (!(*p % 2))) /* Even */
                        (*p)++; /* Next char is lwr */
                    else if (*p == 0xbb)
                        (*p)++; /* Next char is lwr */
                    else if (*p == 0xbd) {
                        *pExtChar = 0xc6;
                        (*p) = 0x9a;
                    }
                    /* 0xba three byte small 0xe2 0xb1 0xa5 */
                    /* 0xbe three byte small 0xe2 0xb1 0xa6 */
                    break;
                case 0xc9: /* Latin ext */
                    if (*p == 0x81)
                        (*p)++; /* Next char is lwr */
                    else if (*p == 0x83) {
                        *pExtChar = 0xc6;
                        (*p) = 0x80;
                    }
                    else if (*p == 0x84) {
                        *pExtChar = 0xca;
                        (*p) = 0x89;
                    }
                    else if (*p == 0x85) {
                        *pExtChar = 0xca;
                        (*p) = 0x8c;
                    }
                    else if ((*p >= 0x86)
                        && (*p <= 0x8f)
                        && (!(*p % 2))) /* Even */
                        (*p)++; /* Next char is lwr */
                    break;
                case 0xcd: /* Greek & Coptic */
                    switch (*p) {
                    case 0xb0:
                    case 0xb2:
                    case 0xb6:
                        (*p)++; /* Next char is lwr */
                        break;
                    case 0xbf:
                        *pExtChar = 0xcf;
                        (*p) = 0xb3;
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xce: /* Greek & Coptic */
                    if (*p == 0x86)
                        (*p) = 0xac;
                    else if (*p == 0x88)
                        (*p) = 0xad;
                    else if (*p == 0x89)
                        (*p) = 0xae;
                    else if (*p == 0x8a)
                        (*p) = 0xaf;
                    else if (*p == 0x8c) {
                        *pExtChar = 0xcf;
                        (*p) = 0x8c;
                    }
                    else if (*p == 0x8e) {
                        *pExtChar = 0xcf;
                        (*p) = 0x8d;
                    }
                    else if (*p == 0x8f) {
                        *pExtChar = 0xcf;
                        (*p) = 0x8e;
                    }
                    else if ((*p >= 0x91)
                        && (*p <= 0x9f))
                        (*p) += 0x20; /* US ASCII shift */
                    else if ((*p >= 0xa0)
                        && (*p <= 0xab)
                        && (*p != 0xa2)) {
                        *pExtChar = 0xcf;
                        (*p) -= 0x20;
                    }
                    break;
                case 0xcf: /* Greek & Coptic */
                    if (*p == 0x8f)
                        (*p) = 0x97;
                    else if ((*p >= 0x98)
                        && (*p <= 0xaf)
                        && (!(*p % 2))) /* Even */
                        (*p)++; /* Next char is lwr */
                    else if (*p == 0xb4) {
                        (*p) = 0x91;
                    }
                    else if (*p == 0xb7)
                        (*p)++; /* Next char is lwr */
                    else if (*p == 0xb9)
                        (*p) = 0xb2;
                    else if (*p == 0xba)
                        (*p)++; /* Next char is lwr */
                    else if (*p == 0xbd) {
                        *pExtChar = 0xcd;
                        (*p) = 0xbb;
                    }
                    else if (*p == 0xbe) {
                        *pExtChar = 0xcd;
                        (*p) = 0xbc;
                    }
                    else if (*p == 0xbf) {
                        *pExtChar = 0xcd;
                        (*p) = 0xbd;
                    }
                    break;
                case 0xd0: /* Cyrillic */
                    if ((*p >= 0x80)
                        && (*p <= 0x8f)) {
                        *pExtChar = 0xd1;
                        (*p) += 0x10;
                    }
                    else if ((*p >= 0x90)
                        && (*p <= 0x9f))
                        (*p) += 0x20; /* US ASCII shift */
                    else if ((*p >= 0xa0)
                        && (*p <= 0xaf)) {
                        *pExtChar = 0xd1;
                        (*p) -= 0x20;
                    }
                    break;
                case 0xd1: /* Cyrillic supplement */
                    if ((*p >= 0xa0)
                        && (*p <= 0xbf)
                        && (!(*p % 2))) /* Even */
                        (*p)++; /* Next char is lwr */
                    break;
                case 0xd2: /* Cyrillic supplement */
                    if (*p == 0x80)
                        (*p)++; /* Next char is lwr */
                    else if ((*p >= 0x8a)
                        && (*p <= 0xbf)
                        && (!(*p % 2))) /* Even */
                        (*p)++; /* Next char is lwr */
                    break;
                case 0xd3: /* Cyrillic supplement */
                    if (*p == 0x80)
                        (*p) = 0x8f;
                    else if ((*p >= 0x81)
                        && (*p <= 0x8e)
                        && (*p % 2)) /* Odd */
                        (*p)++; /* Next char is lwr */
                    else if ((*p >= 0x90)
                        && (*p <= 0xbf)
                        && (!(*p % 2))) /* Even */
                        (*p)++; /* Next char is lwr */
                    break;
                case 0xd4: /* Cyrillic supplement & Armenian */
                    if ((*p >= 0x80)
                        && (*p <= 0xaf)
                        && (!(*p % 2))) /* Even */
                        (*p)++; /* Next char is lwr */
                    else if ((*p >= 0xb1)
                        && (*p <= 0xbf)) {
                        *pExtChar = 0xd5;
                        (*p) -= 0x10;
                    }
                    break;
                case 0xd5: /* Armenian */
                    if ((*p >= 0x80)
                        && (*p <= 0x8f)) {
                        (*p) += 0x30;
                    }
                    else if ((*p >= 0x90)
                        && (*p <= 0x96)) {
                        *pExtChar = 0xd6;
                        (*p) -= 0x10;
                    }
                    break;
                case 0xe1: /* Three byte code */
                    pExtChar = p;
                    p++;
                    switch (*pExtChar) {
                    case 0x82: /* Georgian asomtavruli */
                        if ((*p >= 0xa0)
                            && (*p <= 0xbf)) {
                            *pExtChar = 0x83;
                            (*p) -= 0x10;
                        }
                        break;
                    case 0x83: /* Georgian asomtavruli */
                        if (((*p >= 0x80)
                            && (*p <= 0x85))
                            || (*p == 0x87)
                            || (*p == 0x8d))
                            (*p) += 0x30;
                        break;
                    case 0x8e: /* Cherokee */
                        if ((*p >= 0xa0)
                            && (*p <= 0xaf)) {
                            *(p - 2) = 0xea;
                            *pExtChar = 0xad;
                            (*p) += 0x10;
                        }
                        else if ((*p >= 0xb0)
                            && (*p <= 0xbf)) {
                            *(p - 2) = 0xea;
                            *pExtChar = 0xae;
                            (*p) -= 0x30;
                        }
                        break;
                    case 0x8f: /* Cherokee */
                        if ((*p >= 0x80)
                            && (*p <= 0xaf)) {
                            *(p - 2) = 0xea;
                            *pExtChar = 0xae;
                            (*p) += 0x10;
                        }
                        else if ((*p >= 0xb0)
                            && (*p <= 0xb5)) {
                            (*p) += 0x08;
                        }
                        /* 0xbe three byte small 0xe2 0xb1 0xa6 */
                        break;
                    case 0xb2: /* Georgian mtavruli */
                        if (((*p >= 0x90)
                            && (*p <= 0xba))
                            || (*p == 0xbd)
                            || (*p == 0xbe)
                            || (*p == 0xbf))
                            *pExtChar = 0x83;
                        break;
                    case 0xb8: /* Latin ext */
                        if ((*p >= 0x80)
                            && (*p <= 0xbf)
                            && (!(*p % 2))) /* Even */
                            (*p)++; /* Next char is lwr */
                        break;
                    case 0xb9: /* Latin ext */
                        if ((*p >= 0x80)
                            && (*p <= 0xbf)
                            && (!(*p % 2))) /* Even */
                            (*p)++; /* Next char is lwr */
                        break;
                    case 0xba: /* Latin ext */
                        if ((*p >= 0x80)
                            && (*p <= 0x94)
                            && (!(*p % 2))) /* Even */
                            (*p)++; /* Next char is lwr */
                        else if ((*p >= 0xa0)
                            && (*p <= 0xbf)
                            && (!(*p % 2))) /* Even */
                            (*p)++; /* Next char is lwr */
                        /* 0x9e Two byte small 0xc3 0x9f */
                        break;
                    case 0xbb: /* Latin ext */
                        if ((*p >= 0x80)
                            && (*p <= 0xbf)
                            && (!(*p % 2))) /* Even */
                            (*p)++; /* Next char is lwr */
                        break;
                    case 0xbc: /* Greek ex */
                        if ((*p >= 0x88)
                            && (*p <= 0x8f))
                            (*p) -= 0x08;
                        else if ((*p >= 0x98)
                            && (*p <= 0x9d))
                            (*p) -= 0x08;
                        else if ((*p >= 0xa8)
                            && (*p <= 0xaf))
                            (*p) -= 0x08;
                        else if ((*p >= 0xb8)
                            && (*p <= 0xbf))
                            (*p) -= 0x08;
                        break;
                    case 0xbd: /* Greek ex */
                        if ((*p >= 0x88)
                            && (*p <= 0x8d))
                            (*p) -= 0x08;
                        else if ((*p == 0x99)
                            || (*p == 0x9b)
                            || (*p == 0x9d)
                            || (*p == 0x9f))
                            (*p) -= 0x08;
                        else if ((*p >= 0xa8)
                            && (*p <= 0xaf))
                            (*p) -= 0x08;
                        break;
                    case 0xbe: /* Greek ex */
                        if ((*p >= 0x88)
                            && (*p <= 0x8f))
                            (*p) -= 0x08;
                        else if ((*p >= 0x98)
                            && (*p <= 0x9f))
                            (*p) -= 0x08;
                        else if ((*p >= 0xa8)
                            && (*p <= 0xaf))
                            (*p) -= 0x08;
                        else if ((*p >= 0xb8)
                            && (*p <= 0xb9))
                            (*p) -= 0x08;
                        else if ((*p >= 0xba)
                            && (*p <= 0xbb)) {
                            *(p - 1) = 0xbd;
                            (*p) -= 0x0a;
                        }
                        else if (*p == 0xbc)
                            (*p) -= 0x09;
                        break;
                    case 0xbf: /* Greek ex */
                        if ((*p >= 0x88)
                            && (*p <= 0x8b)) {
                            *(p - 1) = 0xbd;
                            (*p) += 0x2a;
                        }
                        else if (*p == 0x8c)
                            (*p) -= 0x09;
                        else if ((*p >= 0x98)
                            && (*p <= 0x99))
                            (*p) -= 0x08;
                        else if ((*p >= 0x9a)
                            && (*p <= 0x9b)) {
                            *(p - 1) = 0xbd;
                            (*p) += 0x1c;
                        }
                        else if ((*p >= 0xa8)
                            && (*p <= 0xa9))
                            (*p) -= 0x08;
                        else if ((*p >= 0xaa)
                            && (*p <= 0xab)) {
                            *(p - 1) = 0xbd;
                            (*p) += 0x10;
                        }
                        else if (*p == 0xac)
                            (*p) -= 0x07;
                        else if ((*p >= 0xb8)
                            && (*p <= 0xb9)) {
                            *(p - 1) = 0xbd;
                        }
                        else if ((*p >= 0xba)
                            && (*p <= 0xbb)) {
                            *(p - 1) = 0xbd;
                            (*p) += 0x02;
                        }
                        else if (*p == 0xbc)
                            (*p) -= 0x09;
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xe2: /* Three byte code */
                    pExtChar = p;
                    p++;
                    switch (*pExtChar) {
                    case 0xb0: /* Glagolitic */
                        if ((*p >= 0x80)
                            && (*p <= 0x8f)) {
                            (*p) += 0x30;
                        }
                        else if ((*p >= 0x90)
                            && (*p <= 0xae)) {
                            *pExtChar = 0xb1;
                            (*p) -= 0x10;
                        }
                        break;
                    case 0xb1: /* Latin ext */
                        switch (*p) {
                        case 0xa0:
                        case 0xa7:
                        case 0xa9:
                        case 0xab:
                        case 0xb2:
                        case 0xb5:
                            (*p)++; /* Next char is lwr */
                            break;
                        case 0xa2: /* Two byte small 0xc9 0xab */
                        case 0xa4: /* Two byte small 0xc9 0xbd */
                        case 0xad: /* Two byte small 0xc9 0x91 */
                        case 0xae: /* Two byte small 0xc9 0xb1 */
                        case 0xaf: /* Two byte small 0xc9 0x90 */
                        case 0xb0: /* Two byte small 0xc9 0x92 */
                        case 0xbe: /* Two byte small 0xc8 0xbf */
                        case 0xbf: /* Two byte small 0xc9 0x80 */
                            break;
                        case 0xa3:
                            *(p - 2) = 0xe1;
                            *(p - 1) = 0xb5;
                            *(p) = 0xbd;
                            break;
                        default:
                            break;
                        }
                        break;
                    case 0xb2: /* Coptic */
                        if ((*p >= 0x80)
                            && (*p <= 0xbf)
                            && (!(*p % 2))) /* Even */
                            (*p)++; /* Next char is lwr */
                        break;
                    case 0xb3: /* Coptic */
                        if (((*p >= 0x80)
                            && (*p <= 0xa3)
                            && (!(*p % 2))) /* Even */
                            || (*p == 0xab)
                            || (*p == 0xad)
                            || (*p == 0xb2))
                            (*p)++; /* Next char is lwr */
                        break;
                    case 0xb4: /* Georgian nuskhuri */
                        if (((*p >= 0x80)
                            && (*p <= 0xa5))
                            || (*p == 0xa7)
                            || (*p == 0xad)) {
                            *(p - 2) = 0xe1;
                            *(p - 1) = 0x83;
                            (*p) += 0x10;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xea: /* Three byte code */
                    pExtChar = p;
                    p++;
                    switch (*pExtChar) {
                    case 0x99: /* Cyrillic */
                        if ((*p >= 0x80)
                            && (*p <= 0xad)
                            && (!(*p % 2))) /* Even */
                            (*p)++; /* Next char is lwr */
                        break;
                    case 0x9a: /* Cyrillic */
                        if ((*p >= 0x80)
                            && (*p <= 0x9b)
                            && (!(*p % 2))) /* Even */
                            (*p)++; /* Next char is lwr */
                        break;
                    case 0x9c: /* Latin ext */
                        if ((((*p >= 0xa2)
                            && (*p <= 0xaf))
                            || ((*p >= 0xb2)
                                && (*p <= 0xbf)))
                            && (!(*p % 2))) /* Even */
                            (*p)++; /* Next char is lwr */
                        break;
                    case 0x9d: /* Latin ext */
                        if ((((*p >= 0x80)
                            && (*p <= 0xaf))
                            && (!(*p % 2))) /* Even */
                            || (*p == 0xb9)
                            || (*p == 0xbb)
                            || (*p == 0xbe))
                            (*p)++; /* Next char is lwr */
                        else if (*p == 0xbd) {
                            *(p - 2) = 0xe1;
                            *(p - 1) = 0xb5;
                            *(p) = 0xb9;
                        }
                        break;
                    case 0x9e: /* Latin ext */
                        if (((((*p >= 0x80)
                            && (*p <= 0x87))
                            || ((*p >= 0x96)
                                && (*p <= 0xa9))
                            || ((*p >= 0xb4)
                                && (*p <= 0xbf)))
                            && (!(*p % 2))) /* Even */
                            || (*p == 0x8b)
                            || (*p == 0x90)
                            || (*p == 0x92))
                            (*p)++; /* Next char is lwr */
                        else if (*p == 0xb3) {
                            *(p - 2) = 0xea;
                            *(p - 1) = 0xad;
                            *(p) = 0x93;
                        }
                        /* case 0x8d: // Two byte small 0xc9 0xa5 */
                        /* case 0xaa: // Two byte small 0xc9 0xa6 */
                        /* case 0xab: // Two byte small 0xc9 0x9c */
                        /* case 0xac: // Two byte small 0xc9 0xa1 */
                        /* case 0xad: // Two byte small 0xc9 0xac */
                        /* case 0xae: // Two byte small 0xc9 0xaa */
                        /* case 0xb0: // Two byte small 0xca 0x9e */
                        /* case 0xb1: // Two byte small 0xca 0x87 */
                        /* case 0xb2: // Two byte small 0xca 0x9d */
                        break;
                    case 0x9f: /* Latin ext */
                        if ((*p == 0x82)
                            || (*p == 0x87)
                            || (*p == 0x89)
                            || (*p == 0xb5))
                            (*p)++; /* Next char is lwr */
                        else if (*p == 0x84) {
                            *(p - 2) = 0xea;
                            *(p - 1) = 0x9e;
                            *(p) = 0x94;
                        }
                        else if (*p == 0x86) {
                            *(p - 2) = 0xe1;
                            *(p - 1) = 0xb6;
                            *(p) = 0x8e;
                        }
                        /* case 0x85: // Two byte small 0xca 0x82 */
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xef: /* Three byte code */
                    pExtChar = p;
                    p++;
                    switch (*pExtChar) {
                    case 0xbc: /* Latin fullwidth */
                        if ((*p >= 0xa1)
                            && (*p <= 0xba)) {
                            *pExtChar = 0xbd;
                            (*p) -= 0x20;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xf0: /* Four byte code */
                    pExtChar = p;
                    p++;
                    switch (*pExtChar) {
                    case 0x90:
                        pExtChar = p;
                        p++;
                        switch (*pExtChar) {
                        case 0x90: /* Deseret */
                            if ((*p >= 0x80)
                                && (*p <= 0x97)) {
                                (*p) += 0x28;
                            }
                            else if ((*p >= 0x98)
                                && (*p <= 0xa7)) {
                                *pExtChar = 0x91;
                                (*p) -= 0x18;
                            }
                            break;
                        case 0x92: /* Osage  */
                            if ((*p >= 0xb0)
                                && (*p <= 0xbf)) {
                                *pExtChar = 0x93;
                                (*p) -= 0x18;
                            }
                            break;
                        case 0x93: /* Osage  */
                            if ((*p >= 0x80)
                                && (*p <= 0x93))
                                (*p) += 0x28;
                            break;
                        case 0xb2: /* Old hungarian */
                            if ((*p >= 0x80)
                                && (*p <= 0xb2))
                                *pExtChar = 0xb3;
                            break;
                        default:
                            break;
                        }
                        break;
                    case 0x91:
                        pExtChar = p;
                        p++;
                        switch (*pExtChar) {
                        case 0xa2: /* Warang citi */
                            if ((*p >= 0xa0)
                                && (*p <= 0xbf)) {
                                *pExtChar = 0xa3;
                                (*p) -= 0x20;
                            }
                            break;
                        default:
                            break;
                        }
                        break;
                    case 0x96:
                        pExtChar = p;
                        p++;
                        switch (*pExtChar) {
                        case 0xb9: /* Medefaidrin */
                            if ((*p >= 0x80)
                                && (*p <= 0x9f)) {
                                (*p) += 0x20;
                            }
                            break;
                        default:
                            break;
                        }
                        break;
                    case 0x9E:
                        pExtChar = p;
                        p++;
                        switch (*pExtChar) {
                        case 0xA4: /* Adlam */
                            if ((*p >= 0x80)
                                && (*p <= 0x9d))
                                (*p) += 0x22;
                            else if ((*p >= 0x9e)
                                && (*p <= 0xa1)) {
                                *(pExtChar) = 0xa5;
                                (*p) -= 0x1e;
                            }
                            break;
                        default:
                            break;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                default:
                    break;
                }
                pExtChar = 0;
            }
            p++;
        }
    }
   return pString;
}

int 
StrnCiCmp(const char* s1, const char* s2, size_t ztCount)
{
   unsigned char* pStr1Low = 0;
   unsigned char* pStr2Low = 0;
   unsigned char* p1 = 0; 
   unsigned char* p2 = 0; 

   if (s1 && *s1 && s2 && *s2) {
      //~ char cExtChar = 0;
      pStr1Low = (unsigned char*)calloc(strlen(s1) + 1, sizeof(unsigned char));
      if (pStr1Low) {
        pStr2Low = (unsigned char*)calloc(strlen(s2) + 1, sizeof(unsigned char));
          if (pStr2Low) {
             p1 = pStr1Low;
             p2 = pStr2Low;
             strcpy((char*)pStr1Low, s1);
             strcpy((char*)pStr2Low, s2);
             StrToLwrExt(pStr1Low);
             StrToLwrExt(pStr2Low);
             for (; ztCount--; p1++, p2++) {
                int iDiff = *p1 - *p2;
                if (iDiff != 0 || !*p1 || !*p2) {
                  free(pStr1Low);
                  free(pStr2Low);
                  return iDiff;
                }
              }
              free(pStr1Low);
              free(pStr2Low);
              return 0;
          }
          free(pStr1Low);
          return -1;
      }
      return -1;
   }
   return -1;
}

int 
StrCiCmp(const char* s1, const char* s2)
{
   return StrnCiCmp(s1, s2, (size_t)(-1));
}

char* 
StrCiStr(const char* s1, const char* s2)
{
   char* p = (char*)s1;
   size_t len = 0; 

   if (s1 && *s1 && s2 && *s2) {
     len = strlen(s2);
     while (*p) {
       if (StrnCiCmp(p, s2, len) == 0)
         return (char*)p;
       p++;
       }
     }
   return (0);
}

/* end of taken code */

void
evry_util_file_detail_set(Evry_Item_File *file)
{
   char *dir = NULL;
   const char *tmp;

   if (EVRY_ITEM(file)->detail)
     return;

   if (!home_dir)
     {
        home_dir = e_user_homedir_get();
        home_dir_len = strlen(home_dir);
     }

   dir = ecore_file_dir_get(file->path);
   if (!dir || !home_dir) return;

   if (!strncmp(dir, home_dir, home_dir_len))
     {
        tmp = dir + home_dir_len;

        if (*(tmp) == '\0')
          snprintf(dir_buf, sizeof(dir_buf), "~%s", tmp);
        else
          snprintf(dir_buf, sizeof(dir_buf), "~%s/", tmp);

        EVRY_ITEM(file)->detail = eina_stringshare_add(dir_buf);
     }
   else
     {
        if (!strncmp(dir, "//", 2))
          EVRY_ITEM(file)->detail = eina_stringshare_add(dir + 1);
        else
          EVRY_ITEM(file)->detail = eina_stringshare_add(dir);
     }

   E_FREE(dir);
}

int
evry_fuzzy_match(const char *str, const char *match)
{
   const char *p, *m, *next;
   int sum = 0;

   unsigned int last = 0;
   unsigned int offset = 0;
   unsigned int min = 0;
   unsigned char first = 0;
   /* ignore punctuation */
   unsigned char ip = 1;

   unsigned int cnt = 0;
   /* words in match */
   unsigned int m_num = 0;
   unsigned int m_cnt = 0;
   unsigned int m_min[MAX_WORDS];
   unsigned int m_len = 0;
   unsigned int s_len = 0;

   if (!match || !str || !match[0] || !str[0])
     return 0;

   /* remove white spaces at the beginning */
   for (; (*match != 0) && isspace(*match); match++) ;
   for (; (*str != 0) && isspace(*str); str++) ;

   /* SUBSTRING search mode */
   if (evry_conf->fuzzy_search == 0)
     {
        if (StrCiStr(str, match) != NULL)
          return 10;
     }
   
   /* FUZZY search mode */

   /* count words in match */
   for (m = match; (*m != 0) && (m_num < MAX_WORDS); )
     {
        for (; (*m != 0) && !isspace(*m); m++) ;
        for (; (*m != 0) && isspace(*m); m++) ;
        m_min[m_num++] = MAX_FUZZ;
     }
   for (m = match; ip && (*m != 0); m++)
     if (ip && ispunct(*m)) ip = 0;

   m_len = strlen(match);
   s_len = strlen(str);

   /* with less than 3 chars match must be a prefix */
   if (m_len < 3) m_len = 0;

   next = str;
   m = match;

   while ((m_cnt < m_num) && (*next != 0))
     {
        int ii;

        /* reset match */
        if (m_cnt == 0) m = match;

        /* end of matching */
        if (*m == 0) break;

        offset = 0;
        last = 0;
        min = 1;
        first = 0;
        /* m_len = 0; */

        /* match current word of string against current match */
        for (p = next; *next != 0; p++)
          {
             /* new word of string begins */
             if ((*p == 0) || isspace(*p) || (ip && ispunct(*p)))
               {
                  if (m_cnt < m_num - 1)
                    {
                       /* test next match */
                       for (; (*m != 0) && !isspace(*m); m++) ;
                       for (; (*m != 0) && isspace(*m); m++) ;
                       m_cnt++;
                       break;
                    }
                  else
                    {
                       ii = 0;
                       /* go to next word */
                       for (; (*p != 0) && ((isspace(*p) || (ip && ispunct(*p)))); p += ii)
                         if (!eina_unicode_utf8_next_get(p, &ii)) break;
                       cnt++;
                       next = p;
                       m_cnt = 0;
                       break;
                    }
               }

             /* current char matches? */
             if (tolower(*p) != tolower(*m))
               {
                  if (!first)
                    offset += 1;
                  else
                    offset += 3;

                   m_len++; 

                  if (offset <= m_len * 3)
                    continue;
               }

             if (min < MAX_FUZZ && offset <= m_len * 3)
               {
                  /* first offset of match in word */
                  if (!first)
                    {
                       first = 1;
                       last = offset;
                    }

                  min += offset + (offset - last) * 5;
                  last = offset;

                  /* try next char of match */
                  ii = 0;
                  if (!eina_unicode_utf8_next_get(m, &ii)) continue;
                  m += ii;
                  if (*m != 0 && !isspace(*m))
                    continue;

                  /* end of match: store min weight of match */
                  min += (cnt - m_cnt) > 0 ? (cnt - m_cnt) : 0;

                  if (min < m_min[m_cnt])
                    m_min[m_cnt] = min;
               }
             else
               {
                  ii = 0;
                  /* go to next match */
                  for (; (m[0] && m[ii]) && !isspace(*m); m += ii)
                    if (!eina_unicode_utf8_next_get(m, &ii)) break;
               }

             if (m_cnt < m_num - 1)
               {
                  ii = 0;
                  /* test next match */
                  for (; (m[0] && m[ii]) && !isspace(*m); m += ii)
                    if (!eina_unicode_utf8_next_get(m, &ii)) break;
                  m_cnt++;
                  break;
               }
             else if (*p != 0)
               {
                  ii = 0;
                  /* go to next word */
                  for (; (p[0] && (s_len - (p - str) >= (unsigned int)ii)) &&
                       !((isspace(*p) || (ip && ispunct(*p))));
                       p += ii)
                    {
                       if (!eina_unicode_utf8_next_get(p, &ii)) break;
                    }
                  ii = 0;
                  for (; (p[0] && (s_len - (p - str) >= (unsigned int)ii)) &&
                       ((isspace(*p) || (ip && ispunct(*p))));
                       p += ii)
                    {
                       if (!eina_unicode_utf8_next_get(p, &ii)) break;
                    }
                  cnt++;
                  next = p;
                  m_cnt = 0;
                  break;
               }
             else
               {
                  next = p;
                  break;
               }
          }
     }

   for (m_cnt = 0; m_cnt < m_num; m_cnt++)
     {
        sum += m_min[m_cnt];

        if (sum >= MAX_FUZZ)
          {
             sum = 0;
             break;
          }
     }

   if (sum > 0)
     {
        /* exact match ? */
        if (strcmp(match, str))
          sum += 10;
     }
   
   return sum;
}

static int
_evry_fuzzy_match_sort_cb(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   if (it1->priority - it2->priority)
     return it1->priority - it2->priority;

   if (it1->fuzzy_match || it2->fuzzy_match)
     {
        if (it1->fuzzy_match && !it2->fuzzy_match)
          return -1;

        if (!it1->fuzzy_match && it2->fuzzy_match)
          return 1;

        if (it1->fuzzy_match - it2->fuzzy_match)
          return it1->fuzzy_match - it2->fuzzy_match;
     }

   return 0;
}

Eina_List *
evry_fuzzy_match_sort(Eina_List *items)
{
   return eina_list_sort(items, -1, _evry_fuzzy_match_sort_cb);
}

static int _sort_flags = 0;

static int
_evry_items_sort_func(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   /* if (!((!_sort_flags) &&
    *       (it1->type == EVRY_TYPE_ACTION) &&
    *       (it2->type == EVRY_TYPE_ACTION)))
    *   { */
   /* only sort actions when there is input otherwise show default order */

   if (((it1->type == EVRY_TYPE_ACTION) || (it1->subtype == EVRY_TYPE_ACTION)) &&
       ((it2->type == EVRY_TYPE_ACTION) || (it2->subtype == EVRY_TYPE_ACTION)))
     {
        const Evry_Action *act1 = data1;
        const Evry_Action *act2 = data2;

        /* sort actions that match the specific type before
           those matching general type */
        if (act1->it1.item && act2->it1.item)
          {
             if ((act1->it1.type == act1->it1.item->type) &&
                 (act2->it1.type != act2->it1.item->type))
               return -1;

             if ((act1->it1.type != act1->it1.item->type) &&
                 (act2->it1.type == act2->it1.item->type))
               return 1;
          }

        /* sort context specific actions before
           general actions */
        if (act1->remember_context)
          {
             if (!act2->remember_context)
               return -1;
          }
        else
          {
             if (act2->remember_context)
               return 1;
          }
     }
   /* } */

   if (_sort_flags)
     {
        /* when there is no input sort items with higher
         * plugin priority first */
        if (it1->type != EVRY_TYPE_ACTION &&
            it2->type != EVRY_TYPE_ACTION)
          {
             int prio1 = it1->plugin->config->priority;
             int prio2 = it2->plugin->config->priority;

             if (prio1 - prio2)
               return prio1 - prio2;
          }
     }

   /* sort items which match input or which
      match much better first */
   if (it1->fuzzy_match > 0 || it2->fuzzy_match > 0)
     {
        if (it2->fuzzy_match <= 0)
          return -1;

        if (it1->fuzzy_match <= 0)
          return 1;

        if (abs (it1->fuzzy_match - it2->fuzzy_match) > 5)
          return it1->fuzzy_match - it2->fuzzy_match;
     }

   /* sort recently/most frequently used items first */
   if (it1->usage > 0.0 || it2->usage > 0.0)
     {
        return it1->usage > it2->usage ? -1 : 1;
     }

   /* sort items which match input better first */
   if (it1->fuzzy_match > 0 || it2->fuzzy_match > 0)
     {
        if (it1->fuzzy_match - it2->fuzzy_match)
          return it1->fuzzy_match - it2->fuzzy_match;
     }

   /* sort itemswith higher priority first */
   if ((it1->plugin == it2->plugin) &&
       (it1->priority - it2->priority))
     return it1->priority - it2->priority;

   /* sort items with higher plugin priority first */
   if (it1->type != EVRY_TYPE_ACTION &&
       it2->type != EVRY_TYPE_ACTION)
     {
        int prio1 = it1->plugin->config->priority;
        int prio2 = it2->plugin->config->priority;

        if (prio1 - prio2)
          return prio1 - prio2;
     }

   /* user has a broken system: -╯□）╯︵-┻━┻ */
   if ((!it1->label) || (!it2->label)) return -1;
   return strcasecmp(it1->label, it2->label);
}

void
evry_util_items_sort(Eina_List **items, int flags)
{
   _sort_flags = flags;
   *items = eina_list_sort(*items, -1, _evry_items_sort_func);
   _sort_flags = 0;
}

int
evry_util_plugin_items_add(Evry_Plugin *p, Eina_List *items, const char *input,
                           int match_detail, int set_usage)
{
   Eina_List *l;
   Evry_Item *it;
   int match = 0;

   EINA_LIST_FOREACH (items, l, it)
     {
        it->fuzzy_match = 0;

        if (set_usage)
          evry_history_item_usage_set(it, input, NULL);

        if (!input)
          {
             p->items = eina_list_append(p->items, it);
             continue;
          }

        it->fuzzy_match = evry_fuzzy_match(it->label, input);

        if (match_detail)
          {
             match = evry_fuzzy_match(it->detail, input);

             if (!(it->fuzzy_match) || (match && (match < it->fuzzy_match)))
               it->fuzzy_match = match;
          }

        if (it->fuzzy_match)
          p->items = eina_list_append(p->items, it);
     }

   p->items = eina_list_sort(p->items, -1, _evry_items_sort_func);

   return !!(p->items);
}

Evas_Object *
evry_icon_theme_get(const char *icon, Evas *e)
{
   Evas_Object *o = NULL;

   if (!icon)
     return NULL;

   o = e_icon_add(e);
   e_icon_scale_size_set(o, 128);
   e_icon_preload_set(o, 1);

   if (icon[0] == '/')
     {
        e_icon_file_set(o, icon);
     }
   else if (!e_util_icon_theme_set(o, icon))
     {
        char grp[1024];

        snprintf(grp, sizeof(grp), "fileman/mime/%s", icon);
        if (!e_util_icon_theme_set(o, grp))
          {
             evas_object_del(o);
             o = NULL;
          }
     }

   return o;
}

Evas_Object *
evry_util_icon_get(Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;

   if (it->icon_get)
     {
        o = it->icon_get(it, e);
        if (o) return o;
     }

   if ((it->icon) && (it->icon[0] == '/'))
     {
        o = evry_icon_theme_get(it->icon, e);
        if (o) return o;
     }

   if (CHECK_TYPE(it, EVRY_TYPE_FILE))
     {
        const char *icon;
        char *sum;

        GET_FILE(file, it);

        if (it->browseable)
          {
             o = evry_icon_theme_get("folder", e);
             if (o) return o;
          }

        if ((!it->icon) && (file->mime) &&
            ( /*(!strncmp(file->mime, "image/", 6)) || */
              (!strncmp(file->mime, "video/", 6)) ||
              (!strncmp(file->mime, "application/pdf", 15))) &&
            (evry_file_url_get(file)))
          {
             sum = evry_util_md5_sum(file->url);

             snprintf(thumb_buf, sizeof(thumb_buf),
                      "%s/.thumbnails/normal/%s.png",
                      e_user_homedir_get(), sum);
             free(sum);

             if ((o = evry_icon_theme_get(thumb_buf, e)))
               {
                  it->icon = eina_stringshare_add(thumb_buf);
                  return o;
               }
          }

        if ((!it->icon) && (file->mime))
          {
             icon = efreet_mime_type_icon_get(file->mime, e_config->icon_theme, 128);
             /* XXX can do _ref ?*/
             if ((o = evry_icon_theme_get(icon, e)) || (o = evry_icon_theme_get(file->mime, e)))
               {
                  /* it->icon = eina_stringshare_add(icon); */
                  return o;
               }
          }

        if ((icon = efreet_mime_type_icon_get("unknown", e_config->icon_theme, 128)))
          it->icon = eina_stringshare_add(icon);
        else
          it->icon = eina_stringshare_add("");
     }

   if (CHECK_TYPE(it, EVRY_TYPE_APP))
     {
        GET_APP(app, it);

        o = e_util_desktop_icon_add(app->desktop, 128, e);
        if (o) return o;

        o = evry_icon_theme_get("system-run", e);
        if (o) return o;
     }

   if (it->icon)
     {
        o = evry_icon_theme_get(it->icon, e);
        if (o) return o;
     }

   if (it->browseable)
     {
        o = evry_icon_theme_get("folder", e);
        if (o) return o;
     }

   o = evry_icon_theme_get("unknown", e);
   return o;
}

int
evry_util_exec_app(const Evry_Item *it_app, const Evry_Item *it_file)
{
   E_Zone *zone;
   Eina_List *files = NULL;
   char *exe = NULL;
   char *tmp = NULL;

   if (!it_app) return 0;
   GET_APP(app, it_app);
   GET_FILE(file, it_file);

   zone = e_util_zone_current_get(e_manager_current_get());

   if (app->desktop)
     {
        if (file && evry_file_path_get(file))
          {
             Eina_List *l;
             char *mime;
             int open_folder = 0;

             /* when the file is no a directory and the app
                opens folders, pass only the dir */
             if (!IS_BROWSEABLE(file))
               {
                  EINA_LIST_FOREACH (app->desktop->mime_types, l, mime)
                    {
                       if (!mime)
                         continue;

                       if (!strcmp(mime, "x-directory/normal"))
                         open_folder = 1;

                       if (file->mime && !strcmp(mime, file->mime))
                         {
                            open_folder = 0;
                            break;
                         }
                    }
               }

             if (open_folder)
               {
                  tmp = ecore_file_dir_get(file->path);
                  files = eina_list_append(files, tmp);
               }
             else
               {
                  files = eina_list_append(files, file->path);
               }

             e_exec(zone, app->desktop, NULL, files, NULL);

             if (file && file->mime && !open_folder)
               e_exehist_mime_desktop_add(file->mime, app->desktop);

             if (files)
               eina_list_free(files);

             E_FREE(tmp);
          }
        else if (app->file)
          {
             files = eina_list_append(files, app->file);
             e_exec(zone, app->desktop, NULL, files, NULL);
             eina_list_free(files);
          }
        else
          {
             e_exec(zone, app->desktop, NULL, NULL, NULL);
          }
     }
   else if (app->file)
     {
        if (file && evry_file_path_get(file))
          {
             int len;
             len = strlen(app->file) + strlen(file->path) + 4;
             exe = malloc(len);
             snprintf(exe, len, "%s \'%s\'", app->file, file->path);
             e_exec(zone, NULL, exe, NULL, NULL);
             E_FREE(exe);
          }
        else
          {
             exe = (char *)app->file;
             e_exec(zone, NULL,  strtok(exe, "%"), NULL, NULL);
          }
     }

   return 1;
}

/* taken from curl:
 *
 * Copyright (C) 1998 - 2010, Daniel Stenberg, <daniel@haxx.se>, et
 * al.
 *
 * Unescapes the given URL escaped string of given length. Returns a
 * pointer to a malloced string with length given in *olen.
 * If length == 0, the length is assumed to be strlen(string).
 * If olen == NULL, no output length is stored.
 */
#define ISXDIGIT(x) (isxdigit((int)((unsigned char)x)))

char *
evry_util_url_unescape(const char *string, int length)
{
   int alloc = (length ? length : (int)strlen(string)) + 1;
   char *ns = malloc(alloc);
   unsigned char in;
   int strindex = 0;
   unsigned long hex;

   if ( !ns )
     return NULL;

   while (--alloc > 0)
     {
        in = *string;
        if (('%' == in) && ISXDIGIT(string[1]) && ISXDIGIT(string[2]))
          {
             /* this is two hexadecimal digits following a '%' */
             char hexstr[3];
             char *ptr;
             hexstr[0] = string[1];
             hexstr[1] = string[2];
             hexstr[2] = 0;

             hex = strtoul(hexstr, &ptr, 16);
             in = (unsigned char)(hex & (unsigned long)0xFF);
             // in = ultouc(hex); /* this long is never bigger than 255 anyway */

             string += 2;
             alloc -= 2;
          }

        ns[strindex++] = in;
        string++;
     }
   ns[strindex] = 0; /* terminate it */

   return ns;
}

#undef ISXDIGIT

static Eina_Bool
_isalnum(unsigned char in)
{
   switch (in)
     {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
      case 'g':
      case 'h':
      case 'i':
      case 'j':
      case 'k':
      case 'l':
      case 'm':
      case 'n':
      case 'o':
      case 'p':
      case 'q':
      case 'r':
      case 's':
      case 't':
      case 'u':
      case 'v':
      case 'w':
      case 'x':
      case 'y':
      case 'z':
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
      case 'G':
      case 'H':
      case 'I':
      case 'J':
      case 'K':
      case 'L':
      case 'M':
      case 'N':
      case 'O':
      case 'P':
      case 'Q':
      case 'R':
      case 'S':
      case 'T':
      case 'U':
      case 'V':
      case 'W':
      case 'X':
      case 'Y':
      case 'Z':
        return EINA_TRUE;

      default:
        break;
     }
   return EINA_FALSE;
}

char *
evry_util_url_escape(const char *string, int inlength)
{
   size_t alloc = (inlength ? (size_t)inlength : strlen(string)) + 1;
   char *ns;
   char *testing_ptr = NULL;
   unsigned char in; /* we need to treat the characters unsigned */
   size_t newlen = alloc;
   int strindex = 0;
   size_t length;

   ns = malloc(alloc);
   if (!ns)
     return NULL;

   length = alloc - 1;
   while (length--)
     {
        in = *string;

        if (_isalnum(in))
          {
             /* just copy this */
             ns[strindex++] = in;
          }
        else {
             /* encode it */
             newlen += 2; /* the size grows with two, since this'll become a %XX */
             if (newlen > alloc)
               {
                  alloc *= 2;
                  testing_ptr = realloc(ns, alloc);
                  if (!testing_ptr)
                    {
                       free(ns);
                       return NULL;
                    }
                  else
                    {
                       ns = testing_ptr;
                    }
               }

             snprintf(&ns[strindex], 4, "%%%02X", in);

             strindex += 3;
          }
        string++;
     }
   ns[strindex] = 0; /* terminate it */
   return ns;
}

const char *
evry_file_path_get(Evry_Item_File *file)
{
   const char *tmp;
   char *path;

   if (file->path)
     return file->path;

   if (!file->url)
     return NULL;

   if (!strncmp(file->url, "file://", 7))
     tmp = file->url + 7;
   else return NULL;

   if (!(path = evry_util_url_unescape(tmp, 0)))
     return NULL;

   file->path = eina_stringshare_add(path);

   E_FREE(path);

   return file->path;
}

const char *
evry_file_url_get(Evry_Item_File *file)
{
   char dest[PATH_MAX * 3 + 7];
   const char *p;
   int i;

   if (file->url)
     return file->url;

   if (!file->path)
     return NULL;

   memset(dest, 0, sizeof(dest));

   snprintf(dest, 8, "file://");

   /* Most app doesn't handle the hostname in the uri so it's put to NULL */
   for (i = 7, p = file->path; *p != '\0'; p++, i++)
     {
        if (isalnum(*p) || strchr("/$-_.+!*'()", *p))
          dest[i] = *p;
        else
          {
             snprintf(&(dest[i]), 4, "%%%02X", (unsigned char)*p);
             i += 2;
          }
     }

   file->url = eina_stringshare_add(dest);

   return file->url;
}

static void
_cb_free_item_changed(void *data __UNUSED__, void *event)
{
   Evry_Event_Item_Changed *ev = event;

   evry_item_free(ev->item);
   E_FREE(ev);
}

void
evry_item_changed(Evry_Item *it, int icon, int selected)
{
   Evry_Event_Item_Changed *ev;
   ev = E_NEW(Evry_Event_Item_Changed, 1);
   ev->item = it;
   ev->changed_selection = selected;
   ev->changed_icon = icon;
   evry_item_ref(it);
   ecore_event_add(_evry_events[EVRY_EVENT_ITEM_CHANGED], ev, _cb_free_item_changed, NULL);
}

static char thumb_buf[4096];
static const char hex[] = "0123456789abcdef";

char *
evry_util_md5_sum(const char *str)
{
   MD5_CTX ctx;
   unsigned char hash[MD5_HASHBYTES];
   int n;
   char md5out[(2 * MD5_HASHBYTES) + 1];
   MD5Init (&ctx);
   MD5Update (&ctx, (unsigned char const *)str,
              (unsigned)strlen (str));
   MD5Final (hash, &ctx);

   for (n = 0; n < MD5_HASHBYTES; n++)
     {
        md5out[2 * n] = hex[hash[n] >> 4];
        md5out[2 * n + 1] = hex[hash[n] & 0x0f];
     }
   md5out[2 * n] = '\0';

   return strdup(md5out);
}
