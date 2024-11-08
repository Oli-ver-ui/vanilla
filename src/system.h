/*
** Header for functions from system.c
*/

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#ifdef NEED_BASENAME
char *basename(const char *name);
#endif /* NEED_BASENAME */

#ifdef NEED_DIRNAME
char *dirname(char *path);
#endif /* NEED_DIRNAME */

#ifdef NEED_STRTOUL
unsigned long strtoul(const char *nptr, char **endptr, register int base);
#endif /* NEED_STRTOUL */

#endif /* _SYSTEM_H_ */

