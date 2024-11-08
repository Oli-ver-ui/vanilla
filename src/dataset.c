static char rcsver[] = "$Id: dataset.c,v 1.16 2004/05/25 20:32:01 saadat Exp $";
 
/**
 ** $Source: /tes/cvs/vanilla/dataset.c,v $
 **
 ** $Log: dataset.c,v $
 ** Revision 1.16  2004/05/25 20:32:01  saadat
 ** When opening a fragment with detached label with explicit filename in
 ** ^TABLE, vanilla is case-sensitive. I've added an crude method to match
 ** the case with the ".lbl" file. This should work for most of the cases.
 **
 ** If the START and STOP key-values belong to a character/string
 ** key-field, any enclosing double-quotes are removed from these
 ** key-values.
 **
 ** When table name is enclosed in quotes within table fragments, vanilla
 ** choked on the "tbl.fld_name" construction in the "-fields" and
 ** "-select".
 **
 ** Fixed unnecessary LoadTable for blank lines in the DATASET.
 **
 ** Revision 1.15  2002/10/08 22:30:29  saadat
 ** Fake table name is new extracted from the first file specified with the
 ** "-files" option.
 **
 ** Revision 1.14  2002/06/14 15:51:59  saadat
 ** Logging code fixed.
 **
 ** Revision 1.13  2002/06/13 20:25:25  saadat
 ** Added a new command-line option, "-files" which allows vanilla to
 ** interpret a list of files specified on the command-line as a table.
 **
 ** Logging code still needs fixing and the indexing code needs checks put
 ** in to avoid users use indexing options incorrectly.
 **
 ** Revision 1.12  2002/06/06 02:20:13  saadat
 ** Added support for detached label data files.
 **
 ** Revision 1.11  2002/06/04 23:55:40  saadat
 ** Absolute paths starting with drive letters were not working correctly in
 ** DOS/Windows DATASET files.
 **
 ** Revision 1.10  2002/01/12 01:44:00  saadat
 ** vanilla failed on a non-existent directory in the dataset.
 **
 ** Revision 1.9  2001/11/28 20:22:39  saadat
 ** Merged branch vanilla-3-3-13-key-alias-fix at vanilla-3-3-13-4 with
 ** vanilla-3-4-6.
 **
 ** Revision 1.8  2000/08/04 01:50:52  saadat
 ** Rename STATS to LOGGING.
 ** Rename hstats.c to logging.c
 **
 ** Revision 1.7  2000/08/04 00:52:14  saadat
 ** Added a preliminary version of logging/statistics gathering on normal
 ** termination through exit() and on receipt of the following signals:
 **              SIGSEGV, SIGBUS, SIGPIPE, SIGXCPU, SIGXFSZ
 **
 ** Revision 1.6.2.1  2001/05/22 03:58:07  saadat
 ** Windows 95 stat() fails if the path is suffixed with a "/" (e.g. stat
 ** on "/vanilla/data/" will yield -1). Windows NT does not complain
 ** however.
 **
 ** Revision 1.6  2000/07/12 10:34:39  gorelick
 ** Moves SortFiles out of LoadFilenames
 **
 ** Revision 1.5  2000/07/07 17:15:22  saadat
 ** Added support for Windows MS Visual C++ memory-mapped file support.
 ** Removed some unused variables from various files.
 **
 ** Revision 1.4  2000/05/26 15:04:51  asbms
 ** Added fixes for smoother return from errors
 **
 ** Revision 1.3  2000/05/25 02:24:23  saadat
 ** Fixed IRTM temperature generation.
 **
 ** Revision 1.2  1999/11/19 21:19:43  gorelick
 ** Version 3.1, post PDS delivery of 3.0
 **/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "header.h"
#include "proto.h"

#ifdef _WINDOWS
#include "dos.h"
#include <direct.h>
#else
#include <libgen.h>
#endif

#include "system.h"


int LoadTable(DATASET *dataset, char *path, char *name);
void rtrim(char *str, char *trim_chars);
int isAbsPathName(const char *path);
int isDriveRelPathName(const char *path);

/**
 ** Load dataset stuff
 **     Load all the table names, and all the files for each table.
 **
 ** Options:
 **
 **     table
 **     path/table
 **     path
 **
 **/

DATASET *
LoadDataset(DATASET *dataset, char *fname)
{
    char *path, *file, *dir, *tbl_name, *dir_name;
    struct stat sbuf;
    char tmppath[512], newpath[256], buf[256], *p;
    FILE *fp;

    if (dataset == NULL) {
        dataset = calloc(1, sizeof(DATASET));
        dataset->tables = new_list();
        dataset->tablenames = new_list();
    }
    
    path = strdup(fname);
    /**
    ** Split fname into path/file
    **/
    if ((file = strrchr(path, '/')) == NULL) {
        dir = ".";
        file = path;
    } else {
        *file = '\0';
        file++;
        dir = path;
    }

	/* >> THERE ARE A BUNCH OF MEMORY LEAKS HERE << */

	/*
	** Remove trailing slashes and white spaces.
	** Windows 95/98 chokes on stat() on a path with
	** a slash at the end.
	*/
	fname = strdup(fname);
	rtrim(fname, "\\/ \t");

    if (stat(fname, &sbuf) != 0) {
        fname = find_file(fname);
        if (stat(fname, &sbuf) != 0) {
            fprintf(stderr, "Bad path: %s\n", fname);
            return (NULL);
        }
    }

    if ((sbuf.st_mode & S_IFDIR) != 0) {
        dir = fname;
        file = "DATASET";
    }

    sprintf(newpath, "%s/%s", dir, file);
    fname = find_file(newpath);

    if (access(fname, F_OK)) {
        sprintf(newpath, "%s/%s.LST", dir, file);
        fname = find_file(newpath);
    }

    if (access(fname, F_OK)) {
        sprintf(newpath, "%s/%s.TXT", dir, file);
        fname = find_file(newpath);
    }

#ifdef LOGGING
	invocation_log_dataset(fname);
#endif /* LOGGING */

    /**
    ** Open the DATASET file and extract its contents
    **/
    if ((fp = fopen(fname, "r")) == NULL) {
        fprintf(stderr, "Unable to open DATASET file: %s\n", fname);
        return (NULL);
    }

    while (fgets(buf, 256, fp) != NULL) {
		/*
        if ((p = strrchr(buf, '\r')) != NULL) *p = '\0';
        if ((p = strrchr(buf, '\n')) != NULL) *p = '\0';
		*/
		rtrim(buf, " \t\r\n");

        if (strcmp(buf, "") == 0){
            /* do absolutely nothing on an empty line */
        } else if (buf[strlen(buf) -1] == '/') {
            /**
            ** This is a path to another dataset in another directory
            **/
            LoadDataset(dataset, buf);
        } else if (strchr(buf, '/')) {
            /**
            ** This is an explicit path to another table
            **/
            tbl_name = (char *)basename(buf);
            dir_name = (char *)dirname(buf);

				if (isAbsPathName(buf)){
                if(LoadTable(dataset, dir_name, tbl_name)){
                    fclose(fp);
                    return(NULL);
                }
            } else {
#ifdef _WINDOWS
                if(isDriveRelPathName(buf)){
					 	_getdcwd(toupper(buf[0])-'A'+1, tmppath, sizeof(tmppath)-1);
						strcat(tmppath,"/");
						strcat(tmppath, dir_name);
					 }
#else  /* UNIX */
                sprintf(tmppath, "%s/%s", dir, dir_name);
#endif /* _WINDOWS */
                if(LoadTable(dataset, tmppath, tbl_name)){
                    fclose(fp);
                    return(NULL);
                }
            }
        } else {
            /**
            ** This is the name of a table, load it directly.
            **/
            if(LoadTable(dataset, dir, buf)){
                fclose(fp);
                return(NULL);
            }
        }
    }
    fclose(fp);
    return(dataset);
}

DATASET *
FakeDataset(LIST *files_list)
{
    char *tbl_name;
    struct stat sbuf;
	DATASET *dataset;
	char **files;
	int n, i;

	files = (char **)list_data(files_list);
	n = list_count(files_list);

	dataset = calloc(1, sizeof(DATASET));
	dataset->tables = new_list();
	dataset->tablenames = new_list();

	for (i = 0; i < n; i++){
		if (stat(files[i], &sbuf) != 0) {
			perror(files[0]);
			return NULL;
		}
		if ((sbuf.st_mode & S_IFDIR) != 0) {
			fprintf(stderr, "%s is a directory - it should be a file instead.\n", files[i]);
			return NULL;
		}
	}

#ifdef LOGGING
	/* invocation_log_dataset(path); */
#endif /* LOGGING */

    if (n > 0){
        char *p;
        int len;
        
        p = strpbrk(files[0], "0123456789.");
        if (p == NULL){
            tbl_name = strdup(files[0]);
        }
        else {
            len = (long)p-(long)files[0];
            tbl_name = (char *)calloc(1, len+1);
            strncpy(tbl_name, files[0], len);
        }
    }
    else {
        tbl_name = strdup("x");
    }

	if (FakeTable(dataset, files_list, tbl_name)){
        free(tbl_name);
		return NULL;
	}

    free(tbl_name);
    return(dataset);
}

int
FakeTable(DATASET *dataset, LIST *files, char *name)
{
	TABLE *table;

	table = calloc(1, sizeof(TABLE));
	table->files = files;
        
	if (table->files == NULL || table->files->number == 0) {
		/**
		 ** No fragments found for the table, so free the 
		 ** table structure. It is a liability to keep it
		 ** around.
		 **/
		free(table);        /* this should be FreeTable */
	}
	else {
		SortFiles(table->files);
		list_add(dataset->tables, table);
		list_add(dataset->tablenames, (void *)strdup(name));

		table->label = LoadLabel((table->files->ptr)[0]);
		if (table->label == NULL) {
			free(table);
			return(1);
		}
		table->label->table = table;
	}

    return(0); /* return ok */
}


/*
** rtrim()
**
** Trims trim_chars from the right-end of the specified string
*/
void
rtrim(char *str, char *trim_chars)
{
	char *p;

	if (str){
		p = &str[strlen(str)];

		while(p != str && strchr(trim_chars, *--p)){
			*p = '\0';
		}
	}
}


TABLE *
FindTable(DATASET *dataset, char *name)
{
    int i;

    for (i = 0 ; i < dataset->tables->number ; i++) {
        if (!strcmp(((char **)dataset->tablenames->ptr)[i], name)) {
            return(((TABLE **)dataset->tables->ptr)[i]);
        }
    }
    return(NULL);
}

/**
 ** Search the dataset to see if this table already exists in it.
 ** If not, make a new TABLE
 ** Add the file entries from <path> and sort.
 **
 ** Wed Jul 12 00:16:55 MST 2000
 **     SortFiles was moved out of LoadFilenames and into here
 **
 ** Thu May 25 15:47:25 MST 2000
 **     Added check on LoadLabel return 
 **     (file can be rejected resulting in a NULL label
 **
 ** Thu May 25 15:46:38 MST 2000
 **     Changed function from void to int for error checking purposes
 **/

int
LoadTable(DATASET *dataset, char *path, char *name)
{
    TABLE *table = FindTable(dataset, name);

    if (table == NULL) {
        table = calloc(1, sizeof(TABLE));
        table->files = LoadFilenames(path, name);
        
        if (table->files == NULL || table->files->number == 0) {
			/**
			 ** No fragments found for the table, so free the 
			 ** table structure. It is a liability to keep it
			 ** around.
			 **/
            free(table);        /* this should be FreeTable */
        } else {
            SortFiles(table->files);
            list_add(dataset->tables, table);
            list_add(dataset->tablenames, (void *)strdup(name));
            table->label = LoadLabel((table->files->ptr)[0]);

            if (table->label == NULL) {
                free(table);
                return(1);
            }
            table->label->table = table;

        }
    } else {
        LIST *files = LoadFilenames(path, name);
        list_merge(table->files, files);
        SortFiles(table->files);

        /* Check for duplicate file names? */
    }

    return(0);
}
