/* 
** If LOGGING is defined, this module logs invocation info
** in /tes/lib/vanilla.log (if it exists already and is
** writable).
*/

#ifdef LOGGING

#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include "version.h"

/*
** CAUTION:
**
** Don't include "proto.h" here, as the definition of
** exit() will cause an infinite recursion in
** invocation_log_common_exit_handler().
*/

#define INVOCATION_LOG "/tes/lib/vanilla.log"
#define MAX_PATHS 100

struct {
	char   exec_path[PATH_MAX];
	struct timeval start_time, end_time;
	char   start_time_str[32];
	char   end_time_str[32];
	char   dataset_path[MAX_PATHS][PATH_MAX];
	int    npaths; /* number of paths stored in "dataset_path" */
	int    npaths_overflow; /* number of paths overflowed */
	int    process_files; /* vanilla processed files instead of a DATASET */
	char   **args;
	int    nargs;
} loginfo;       /* items to be logged */


typedef void (*sighandler)(int);
sighandler sighandlers[NSIG]; /* some arbitrary number */

#define INSTALL_HANDLER(handlers, sig, handler) \
	if (((handlers)[(sig)] = signal((sig), (handler))) == SIG_IGN){ \
		signal((sig), SIG_IGN); \
		(handlers)[(sig)] = SIG_IGN; \
	}

static void
invocation_log_end()
{
	time_t clock;
	char *p;

	gettimeofday(&loginfo.end_time, NULL);

	clock = time(NULL);
	/* ctime_r(&clock, loginfo.end_time_str, sizeof(loginfo.end_time_str)); */
	strcpy(loginfo.end_time_str, ctime(&clock));
	if (p = strchr(loginfo.end_time_str, '\n')){ *p = 0; }
}

/*
** issignal = 0 -- called by exit()
** issignal = 1 -- called via a signal
*/
static void
invocation_log_common_exit_handler(int issignal, int exit_status)
{
	const  char separator[] = 
		"=====================================================================";
	const  char invocation_log_file[] = INVOCATION_LOG;
	char   null_str[] = "(null)";
	FILE  *logfile;
	int    i;
	struct timeval tdiff;
	struct flock sflock;
	int    fd;
	char   hostnamestr[1024];
	char   loginname[L_cuserid];

	/*
	** Make sure the log file already exits and is writable.
	** If not, we don't want to create it.
	*/
	if ((fd = open(invocation_log_file, O_WRONLY | O_APPEND, 0666)) >= 0){
		/* accumulate "end" statistics */
		invocation_log_end();

		/* evalulate time consumed during processing */
		tdiff.tv_usec = loginfo.end_time.tv_usec - loginfo.start_time.tv_usec;
		tdiff.tv_sec = loginfo.end_time.tv_sec - loginfo.start_time.tv_sec;
		if (tdiff.tv_usec < 0){
			tdiff.tv_sec --;
			tdiff.tv_usec += 1000000UL;
		}

		/* 
		** Obtain a write-lock on the file, so that log from one vanilla 
		** call does not get jumbled with other concurrent calls.
		*/
		sflock.l_type = F_WRLCK;
		sflock.l_whence = SEEK_CUR;
		sflock.l_start = 0;
		sflock.l_len = 0; /* till EOF */
		while(fcntl(fd, F_SETLKW, &sflock) == EINTR);

		/* open the file in buffered mode, so that we can use fprintf() */
		if ((logfile = fdopen(fd, "a")) != NULL){
			/* format and print statistics */

			fprintf(logfile, "\n%s\n", separator);

			fprintf(logfile, "%s: %s\n", loginfo.exec_path, version);

			strcpy(hostnamestr, "");
			gethostname(hostnamestr, sizeof(hostnamestr));

			strcpy(loginname, "");
			cuserid(loginname);

			fprintf(logfile, "user: %d (%s)    system: %s\n",
				getuid(), loginname, hostnamestr);

			fprintf(logfile, "started: %s\nfinished: %s\ntook: %ld.%ld sec\n",
				loginfo.start_time_str, loginfo.end_time_str,
				tdiff.tv_sec, (long)(0.5 + tdiff.tv_usec / 1E3));

			fprintf(logfile, "exit-status: %d %s\n", 
				exit_status,
				(issignal? "(signal)": ""));

			if (loginfo.process_files){
				fprintf(logfile, "Files: [");
				for(i = 0; i < loginfo.npaths; i++){
					fprintf(logfile, "%s%s", loginfo.dataset_path[i],
							(i<(loginfo.npaths-1))?",":"");
				}
				if (loginfo.npaths_overflow){ fprintf(logfile,"...]\n"); }
				else { fprintf(logfile,"]\n"); }
			}
			else {
				fprintf(logfile, "DATASET-full-path: \"%s\"\n",
						loginfo.dataset_path[0]);
			}

			fprintf(logfile, "command-line arguments: \n");

			for(i = 0; i < loginfo.nargs; i++){
				fprintf(logfile, "   \"%s\"\n", 
					(loginfo.args[i]? loginfo.args[i]: null_str));
			}

			fprintf(logfile, "%s\n", separator);

			fclose(logfile);
		}

		while(fcntl(fd, F_UNLCK, &sflock) == EINTR);
		close(fd);
	}

	if (!issignal) exit(exit_status);
	else { 
		INSTALL_HANDLER(sighandlers, exit_status, sighandlers[exit_status]);
		raise(exit_status);
	}
}

void
invocation_log_on_exit(int exit_status)
{
	invocation_log_common_exit_handler(0, exit_status);
}

void
invocation_log_on_signal(int sig)
{
	invocation_log_common_exit_handler(1, sig);
}

void
invocation_log_start(int ac, char **av)
{
	int i;
	time_t clock;
	char *p;

	if (av[0]){ realpath(av[0], loginfo.exec_path); }
	else { strcpy(loginfo.exec_path, "(null)"); }

	strcpy(loginfo.dataset_path[0], "");
	loginfo.npaths = 0;
	loginfo.npaths_overflow = 0;
	loginfo.process_files = 0;

	clock = time(NULL);
	/* ctime_r(&clock, loginfo.start_time_str, sizeof(loginfo.start_time_str)); */
	strcpy(loginfo.start_time_str, ctime(&clock));
	if (p = strchr(loginfo.start_time_str, '\n')){ *p = 0; }

	gettimeofday(&loginfo.start_time, NULL);

	loginfo.nargs = ac;
	loginfo.args = (char **)calloc(sizeof(char *), loginfo.nargs);
	for(i = 0; i < ac; i++){ loginfo.args[i] = strdup(av[i]); }

	/* atexit(stats_exit); -- does not give exit_status */

	/* install signal handlers */
	INSTALL_HANDLER(sighandlers, SIGSEGV, invocation_log_on_signal);
	INSTALL_HANDLER(sighandlers, SIGBUS, invocation_log_on_signal);
	INSTALL_HANDLER(sighandlers, SIGPIPE, invocation_log_on_signal);
	INSTALL_HANDLER(sighandlers, SIGXCPU, invocation_log_on_signal);
	INSTALL_HANDLER(sighandlers, SIGXFSZ, invocation_log_on_signal);
}

/*
** This function must be used to log dataset that vanilla thinks
** it is using to process the data. If "-files" command-line option
** is encountered, invocation_log_files must be used instead.
*/
void
invocation_log_dataset(char *dataset)
{
	loginfo.process_files = 0;
	realpath(dataset, loginfo.dataset_path[loginfo.npaths++]);
}

/*
** This function must be used instead of invocation_log_dataset
** when processing files using "-files" command-line option. It
** must be called for each of the files individually.
*/
void
invocation_log_files(char *file)
{
	loginfo.process_files = 1;
	if (loginfo.npaths < MAX_PATHS){
		realpath(file, loginfo.dataset_path[loginfo.npaths++]);
	}
	else {
		loginfo.npaths_overflow = 1;
	}
}


#endif /* LOGGING */

