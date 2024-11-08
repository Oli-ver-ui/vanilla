static char rcsver[] = "$Id: dir.c,v 1.9 2002/06/06 02:20:13 saadat Exp $";
 
/**
 ** $Source: /tes/cvs/vanilla/dir.c,v $
 **
 ** $Log: dir.c,v $
 ** Revision 1.9  2002/06/06 02:20:13  saadat
 ** Added support for detached label data files.
 **
 ** Revision 1.8  2002/06/04 23:55:40  saadat
 ** Absolute paths starting with drive letters were not working correctly in
 ** DOS/Windows DATASET files.
 **
 ** Revision 1.7  2001/12/20 00:18:38  saadat
 ** File sorting now happens independent of case.
 **
 ** Revision 1.6  2001/12/19 22:42:36  saadat
 ** The compare function used in qsort was causing vanilla to terminate
 ** abnormally. It has been fixed as per spec of Visual C++.
 **
 ** Revision 1.5  2001/11/28 20:22:39  saadat
 ** Merged branch vanilla-3-3-13-key-alias-fix at vanilla-3-3-13-4 with
 ** vanilla-3-4-6.
 **
 ** Revision 1.4.2.1  2001/05/22 03:58:07  saadat
 ** Windows 95 stat() fails if the path is suffixed with a "/" (e.g. stat
 ** on "/vanilla/data/" will yield -1). Windows NT does not complain
 ** however.
 **
 ** Revision 1.4  2000/07/12 10:34:40  gorelick
 ** Moves SortFiles out of LoadFilenames
 **
 ** Revision 1.3  2000/07/07 17:15:22  saadat
 ** Added support for Windows MS Visual C++ memory-mapped file support.
 ** Removed some unused variables from various files.
 **
 ** Revision 1.2  1999/11/19 21:19:43  gorelick
 ** Version 3.1, post PDS delivery of 3.0
 **
 ** Revision 1.1.1.1  1999/10/15 19:30:35  gorelick
 ** Version 3.0
 **
 ** Revision 2.1  1999/02/10 04:00:50  gorelick
 ** *** empty log message ***
 **
 ** Revision 2.0  1998/12/22 22:47:04  gorelick
 ** release version
 **
 ** Revision 2.0  1998/12/18 01:26:03  gorelick
 ** release version
 **
 ** Revision 1.3  1998/12/18 01:04:48  gorelick
 ** *** empty log message ***
 **
 ** Revision 1.2  1998/11/12 22:58:55  gorelick
 ** first release version
 **
 **/

#ifdef _WINDOWS
#include "dos.h"
#include <io.h>
#else
#include <dirent.h>
#include <libgen.h>
#endif /* _WINDOWS */

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "tools.h"
#include "system.h"


#ifdef _WINDOWS
/* Some old Microsoft Visual C compilers need the "__cdecl" */
#define __CDECL __cdecl
#else
#define __CDECL
#endif /* _WINDOWS */

#define qsortptr int (__CDECL *)(const void *, const void *)

int mystrcmp(const void *a, const void *b);
static const char *my_basename(const char *path);
void SortFiles(LIST *list);
int IsDataFile(char *test, char *type);
int IsLblFile(char *test, char *type);
int IsAPhantomLink(const char *fpath);
void PruneByNamePriority(LIST **org);


#ifdef _WINDOWS
/**
 ** LoadFilenames()
 **     Given a directory and table prefix, load the name of all the files
 **
 ** Wed Jul 12 00:16:55 MST 2000 (NsG)
 **     Removed the SortFiles from here, since it is done again by the
 **     caller to merge file lists.
 */

LIST *
LoadFilenames(char *path, char *prefix)
{
    LIST *list;
    char buf[2048];
    const char *phantom_link_msg = "Ignoring phantom link named %s.\n";
    long fhand;
    struct _finddata_t finfo;
    long rc;
	
    sprintf(buf, "%s/%s*", path, prefix);

    fhand = _findfirst(buf, &finfo);
    if (fhand == -1 && errno == EINVAL){ return NULL; }
    rc = fhand;
    list = new_list();
    while(rc != -1){
        if (IsDataFile(finfo.name, prefix) ||
			IsLblFile(finfo.name, prefix)){
            sprintf(buf, "%s/%s", path, finfo.name);
            if (!IsAPhantomLink(buf)){
                /**
                ** Store the file name
                **/
                list_add(list, strdup(buf));
            }
            else {
                fprintf(stderr, phantom_link_msg, buf);
            }
        }
        rc = _findnext(fhand, &finfo);
    }
	
    if (fhand != -1) { _findclose(fhand); }

	/**
	 ** If a fragment has both a ".lbl" and ".dat" file
	 ** then ".lbl" get priority and ".dat" is removed
	 ** from the list.
	 **/
	PruneByNamePriority(&list);

    return(list);
}

#else

LIST *
LoadFilenames(char *path, char *prefix)
{
    LIST *list;
    char buf[2048];
    const char *phantom_link_msg = "Ignoring phantom link named %s.\n";
    DIR *d;
    struct dirent *d_ent;

    if ((d = opendir(path)) == NULL)
        return (NULL);

    list = new_list();
    while ((d_ent = readdir(d)) != NULL) {
        if (IsDataFile(d_ent->d_name, prefix) ||
			IsLblFile(d_ent->d_name, prefix)){
            sprintf(buf, "%s/%s", path, d_ent->d_name);

            if(!IsAPhantomLink(buf)) {
                /**
                ** Store the filename
                **/
                list_add(list, strdup(buf));
            }
            else {
                fprintf(stderr, phantom_link_msg, buf);
            }
        }
    }
    closedir(d);

	/**
	 ** If a fragment has both a ".lbl" and ".dat" file
	 ** then ".lbl" get priority and ".dat" is removed
	 ** from the list.
	 **/
	PruneByNamePriority(&list);

    return(list);
}

#endif /* _WINDOWS */


/**
 ** Prunes the input LIST of path names according to heuristics
 ** based on file extensions and names. On return the original
 ** LIST is replaced with a new LIST with fewer or equal number
 ** of members in it.
 **
 ** Heuristics are as follows:
 **
 ** 1) If there is a file, say file1.lbl, and another file, say
 **    file1.dat or file1.tab, then only file1.lbl makes it to the
 **    output list. This is assuming that the ".lbl" file is the 
 **    detached label of the ".dat"/".tab" file.
 **
 ** 2) For any other file, say file3, without an associated ".lbl"
 **    file. file3 makes it to the output list.
 ** 
 **/
void
PruneByNamePriority(LIST **org)
{
	LIST *pruned = new_list(); /* result after the pruning operation */
	char **fname, *last = NULL;
	int n, i;
	const char *last_bn, *last_ext, *curr_bn, *curr_ext;

	/* sorted list makes it easier to find (".lbl",".dat") pairs */
	SortFiles(*org);

	fname = (char **)list_data(*org);
	n = list_count(*org);

	/**
	 ** ASSUME for now that names of ".lbl" and their corresponding
	 ** ".dat" files are the same. This assumption may change in the
	 ** future.
	 **/
	for(i = 0; i < n; i++){
		if (last == NULL){
			last = fname[i];
			continue;
		}

		last_bn = my_basename(last);
		curr_bn = my_basename(fname[i]);

		last_ext = strrchr(last, '.');
		curr_ext = strrchr(fname[i], '.');

		if (last_ext && strcasecmp(last_ext,".dat") == 0 &&
			curr_ext && strcasecmp(curr_ext,".lbl") == 0 &&
			(long)(last_ext - last_bn) == (long)(curr_ext - curr_bn) &&
			strncasecmp(last_bn, curr_bn, last_ext - last_bn) == 0){
			
			/** 
			 ** The ".dat" file will be used by way of ".lbl" (or a 
			 ** detached label) file so don't put it in the list. Instead
			 ** put the ".lbl" file in the list.
			 **/
			list_add(pruned,strdup(fname[i]));
			last = NULL;
		}
		else if (last_ext && strcasecmp(last_ext, ".lbl") == 0 &&
				 curr_ext && strcasecmp(curr_ext, ".tab") == 0 &&
				 (long)(last_ext - last_bn) == (long)(curr_ext - curr_bn) &&
				 strncasecmp(last_bn, curr_bn, last_ext - last_bn) == 0){
			/** 
			 ** The ".tab" file will be used by way of ".lbl" (or a 
			 ** detached label) file so don't put it in the list. Instead
			 ** put the ".lbl" file in the list.
			 **/
			list_add(pruned,strdup(last));
			last = NULL;
		}
		else {
			/**
			 ** The ".dat/.tab" file does not have a detached ".lbl"
			 ** file associated with it. So, it must be our old
			 ** fashioned (attached label) vanilla file. Put it in the
			 ** list and move on.
			 **/
			list_add(pruned,strdup(last));
			last = fname[i];
		}
	}

	if (last != NULL){
		list_add(pruned,strdup(last));
	}

	for(i = 0; i < n; i++){
		free(fname[i]);
	}
	list_free(*org);

	/* returned the pruned results */
	*org = pruned;
}


int
IsAPhantomLink(const char *fpath)
{
    struct stat sbuf;

    if (stat(fpath, &sbuf) < 0){
        if (errno == ENOENT || errno == ENOTDIR){
            return 1; /* Phantom link */
        }
    }

    /* Not a phantom link or not accessable */
    return 0;
}

int
IsDataFile(char *test, char *type)
{
    char *p;
    if (!strncasecmp(test, type, strlen(type))) {
        p = strrchr(test, '.');
        if (p && (!strcasecmp(p, ".DAT") || !strcasecmp(p, ".TAB")))
            return (1);
    }
    return (0);
}

int
IsLblFile(char *test, char *type)
{
	char *p;
    if (!strncasecmp(test, type, strlen(type))) {
        p = strrchr(test, '.');
        if (p && !strcasecmp(p, ".LBL"))
            return (1);
    }
    return (0);
}


#ifdef _WINDOWS

int
isAbsPathName(const char *path)
{
	if (strlen(path) > 3 &&
		isalpha(path[0]) && path[1] == ':' && 
		(path[2] == '/' || path[2] == '\\')){
		return 1;
	}
	else if (path[0] == '/' || path[0] == '\\'){
		return 1;
	}
	return 0;
}

int
isDriveRelPathName(const char *path)
{
	if (strlen(path) >= 2 &&
		isalpha(path[0]) && path[1] == ':' &&
		path[2] != '/' && path[2] != '\\'){
		return 1;
	}
	return 0;
}

static const char *
my_basename(const char *path)
{
    const char *name, *p1, *p2;

    if (path == NULL){ return NULL; }

    p1 = strrchr(path, '/');
    p2 = strrchr(path, '\\');

    /* name follows the last "/" or "\" - if any! */
    if (p1 && p2){ name = (p1 > p2)? p1: p2; name ++; }
    else if (p1) { name = p1 + 1; }
    else if (p2) { name = p2 + 1; }
    else         {
        /* name follows the drive letter - if any! */
        p1 = strchr(path, ':');
        if ((p1 - path) == 2){ name = p1 + 1; }
        else                 { name = path; }
    }

    return name;	
}

#else /* UNIX */

int
isAbsPathName(const char *path)
{
	if (path[0] == '/'){ return 1; }
	return 0;
}

static const char *
my_basename(const char *path)
{
    const char *name;

    if (path == NULL){ return NULL; }

    name = strrchr(path, '/');
    if (name){ name++; }
    else { name = path; }

    return name;
}

#endif /* _WINDOWS */

static int __CDECL 
my_filename_cmp(const void *a, const void *b)
{
    return strcasecmp(my_basename(*(char **)a), my_basename(*(char **)b));
}

void
SortFiles(LIST *list)
{
    if (list->number > 1)
        qsort(list->ptr, list->number, sizeof(char *), my_filename_cmp);
}
