#ifndef _DOS_H
#define _DOS_H

/* This file must be included in _WINDOWS define */

#include <string.h>

/*
** DOS/Windows have stricmp & strnicmp in place of UNIX strcasecmp & 
** strncasecmp respectively.
*/
#define strcasecmp stricmp
#define strncasecmp strnicmp

/*
** Following convenince defines have been taken from help pages and defined for
** a uniform implementation across DOS/UNIX
*/
#define  R_OK  4  /* Test for Read permission */
#define  W_OK  2  /* Test for Write permission */
#define  F_OK  0  /* Test for existence of File */
#define  RW_OK (R_OK|W_OK) /* Test for Read/Write permission */


/*
** To keep the memory mapping consistent across platforms, I am currently using
** UNIX/mmap() enumerations. These may have to be changed later on.
*/

/* Following are the values taken by the "mmap_prot" parameter of MemMapFile() */
#define PROT_READ       0x1             /* pages can be read */
#define PROT_WRITE      0x2             /* pages can be written */
#define PROT_EXEC       0x4             /* pages can be executed */
#define PROT_USER       0x8             /* pages are user accessable */
#define PROT_ZFOD       (PROT_READ | PROT_WRITE | PROT_EXEC | PROT_USER)
#define PROT_ALL        (PROT_READ | PROT_WRITE | PROT_EXEC | PROT_USER)
#define PROT_NONE       0x0             /* pages cannot be accessed */
#define PROC_TEXT       (PROT_EXEC | PROT_READ)
#define PROC_DATA       (PROT_READ | PROT_WRITE | PROT_EXEC)
#define VALID_ATTR  (PROT_READ|PROT_WRITE|PROT_EXEC|SHARED|PRIVATE)
#define PROT_EXCL       0x20

/* Following are the values taken by the "mmap_flags" parameter of MemMapFile() */
#define MAP_SHARED      1               /* share changes */
#define MAP_PRIVATE     2               /* changes are private */
#define MAP_TYPE        0xf             /* mask for share type */
/* other flags to mmap (or-ed in to MAP_SHARED or MAP_PRIVATE) */
#define MAP_FIXED       0x10            /* user assigns address */
#define MAP_NORESERVE   0x40            /* don't reserve needed swap area */
#define MAP_FAILED      ((void *) -1)

/* Include basename and dirname */
#ifndef NEED_BASENAME
#define NEED_BASENAME
#endif /* NEED_BASENAME */

#ifndef NEED_DIRNAME
#define NEED_DIRNAME
#endif /* NEED_DIRNAME */

#include "system.h"

#endif