/*---------------------------------------------------------------------------/
/  FatFs - FAT file system module configuration file  R0.11 (C)ChaN, 2015
/---------------------------------------------------------------------------*/

#ifndef _FFCONF
#define _FFCONF 32020	/* Revision ID */

#define _FS_TINY                0   /* 0: Normal or 1: Tiny */
#define _FS_READONLY            0   /* 0: Read/Write */
#define _FS_MINIMIZE            0   /* 0: All functions enabled */
#define _USE_STRFUNC            0   /* 0: Disable string functions */
#define _USE_FIND               1   /* 1: Enable f_findfirst and f_findnext */
#define _USE_MKFS               0   /* 0: Disable f_mkfs */
#define _USE_FASTSEEK           1   /* 1: Enable fast seek function */
#define _USE_LABEL              0   /* 0: Disable volume label functions */
#define _USE_FORWARD            0   /* 0: Disable f_forward */

/* Locale and Namespace Configurations */
#define _CODE_PAGE              437 /* U.S. */
#define _USE_LFN                0   /* 0: 8.3 filenames (nhanh, nhẹ, không tốn RAM buffer) */
#define _MAX_LFN                255
#define _LFN_UNICODE            0
#define _STRF_ENCODE            3
#define _FS_RPATH               0   /* 0: Disable relative path */

/* Volume / Drive Configurations */
#define _VOLUMES                1   /* 1 logical drive */
#define _STR_VOLUME_ID          0
#define _MULTI_PARTITION        0   /* Single partition */
#define _MIN_SS                 512
#define _MAX_SS                 512
#define _USE_TRIM               0
#define _FS_NOFSINFO            0

/* System Configurations */
#define _FS_NORTC               1   /* 1: No RTC */
#define _NORTC_MON              9
#define _NORTC_MDAY             12
#define _NORTC_YEAR             2026
#define _FS_LOCK                0
#define _FS_REENTRANT           0
#define _FS_TIMEOUT             1000

#endif /* _FFCONF */
