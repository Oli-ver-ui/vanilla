static char rcsver[] = "$Id: system.c,v 1.3 2000/07/07 17:15:27 saadat Exp $";
 
/**
 ** $Source: /tes/cvs/vanilla/system.c,v $
 **
 ** $Log: system.c,v $
 ** Revision 1.3  2000/07/07 17:15:27  saadat
 ** Added support for Windows MS Visual C++ memory-mapped file support.
 ** Removed some unused variables from various files.
 **
 ** Revision 1.2  1999/11/19 21:19:51  gorelick
 ** Version 3.1, post PDS delivery of 3.0
 **
 ** Revision 1.1.1.1  1999/10/15 19:30:35  gorelick
 ** Version 3.0
 **
 ** Revision 1.3  1999/07/12 23:14:08  gorelick
 ** *** empty log message ***
 **
 ** Revision 1.2  1998/11/12 22:58:55  gorelick
 ** first release version
 **
 **/

/* #define NEED_BASENAME	if you need basename() */
/* #define NEED_DIRNAME		if you need dirname() */
/* #define NEED_STRTOUL		if you need strtoul() */


#ifdef NEED_BASENAME
char *
basename(const char *name)
{
	const char *base = name;

	while (*name) {
		if (*name++ == '/') {
			base = name;
		}
	}
	return (char *) base;
}
#endif /* NEED_BASENAME */

#ifdef NEED_DIRNAME
#include <string.h>

char *
dirname(char *path)
{
	char *slash;
	int length;		/* Length of result, not including NUL.  */
	/* char *newpath; */

	slash = strrchr(path, '/');
	if (slash == 0) {
		/* File is in the current directory.  */
		path = ".";
		length = 1;
	} else {
		/* Remove any trailing slashes from the result.  */
		while (slash > path && *slash == '/')
			--slash;

		length = slash - path + 1;
	}

	/*
	newpath = (char *) malloc(length + 1);
	if (newpath == 0)
		return 0;
	strncpy(newpath, path, length);
	newpath[length] = 0;
	return newpath;
	*/

	path[length] = 0;
	return path;
}
#endif /* NEED_DIRNAME */

#ifdef NEED_STRTOUL
#include <limits.h>
#include <errno.h>
/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
unsigned long
strtoul(nptr, endptr, base)
        const char *nptr;
        char **endptr;
        register int base;
{
        register const char *s = nptr;
        register unsigned long acc;
        register int c;
        register unsigned long cutoff;
        register int neg = 0, any, cutlim;
 
        /*
         * See strtol for comments as to the logic used.
         */
        do {
                c = *s++;
        } while (isspace(c));
        if (c == '-') {
                neg = 1;
                c = *s++;
        } else if (c == '+')
                c = *s++;
        if ((base == 0 || base == 16) &&
            c == '0' && (*s == 'x' || *s == 'X')) {
                c = s[1];
                s += 2;
                base = 16;
        }
        if (base == 0)
                base = c == '0' ? 8 : 10;
        cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
        cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;
        for (acc = 0, any = 0;; c = *s++) {
                if (isdigit(c))
                        c -= '0';
                else if (isalpha(c))
                        c -= isupper(c) ? 'A' - 10 : 'a' - 10;
                else
                        break;
                if (c >= base)
                        break;
                if (any < 0 || acc > cutoff || acc == cutoff && c > cutlim)
                        any = -1;
                else {
                        any = 1;
                        acc *= base;
                        acc += c;
                }
        }
        if (any < 0) {
                acc = ULONG_MAX;
                errno = ERANGE;
        } else if (neg)
                acc = -acc;
        if (endptr != 0)
                *endptr = any ? (char *)(s - 1) : (char *)nptr;
        return (acc);
}

#endif

