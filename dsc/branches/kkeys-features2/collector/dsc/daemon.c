/*
 * $Id$
 *
 * Copyright (c) 2007, The Measurement Factory, Inc.  All rights
 * reserved.  See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <syslog.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "xmalloc.h"
#include "dns_message.h"
#include "ip_message.h"
#include "pcap.h"
#include "syslog_debug.h"

char *progname = NULL;
char *pid_file_name = NULL;
int promisc_flag = 1;
int debug_flag = 0;
int nodaemon_flag = 0;

extern void cip_net_indexer_init(void);
extern void ParseConfig(const char *);

void
daemonize(void)
{
    int fd;
    pid_t pid;
    if ((pid = fork()) < 0) {
	syslog(LOG_ERR, "fork failed: %s", strerror(errno));
	exit(1);
    }
    if (pid > 0)
	exit(0);
    if (setsid() < 0)
	syslog(LOG_ERR, "setsid failed: %s", strerror(errno));
    closelog();
#ifdef TIOCNOTTY
    if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
	ioctl(fd, TIOCNOTTY, NULL);
	close(fd);
    }
#endif
    fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
	syslog(LOG_ERR, "/dev/null: %s\n", strerror(errno));
    } else {
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	close(fd);
    }
    openlog(progname, LOG_PID | LOG_NDELAY, LOG_DAEMON);
}

void
write_pid_file(void)
{
    FILE *fp;
    if (NULL == pid_file_name)
	return;
    syslog(LOG_INFO, "writing PID to %s", pid_file_name);
    fp = fopen(pid_file_name, "w");
    if (NULL == fp) {
        perror(pid_file_name);
        syslog(LOG_ERR, "fopen: %s: %s", pid_file_name, strerror(errno));
	return;
    }
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
}

void
usage(void)
{
    fprintf(stderr, "usage: %s [opts] dsc.conf\n",
	progname);
    fprintf(stderr, "\t-d\tDebug mode.  Exits after first write.\n");
    fprintf(stderr, "\t-f\tForeground mode.  Don't become a daemon.\n");
    fprintf(stderr, "\t-p\tDon't put interface in promiscuous mode.\n");
    exit(1);
}

int
main(int argc, char *argv[])
{
    int x;
    extern DMC dns_message_handle;
    int result;
    struct timeval break_start = {0,0};

    progname = xstrdup(strrchr(argv[0], '/') ? strchr(argv[0], '/') + 1 : argv[0]);
    if (NULL == progname)
	return 1;
    srandom(time(NULL));
    openlog(progname, LOG_PID | LOG_NDELAY, LOG_DAEMON);

    while ((x = getopt(argc, argv, "fpd")) != -1) {
	switch (x) {
	case 'f':
	    nodaemon_flag = 1;
	    break;
	case 'p':
	    promisc_flag = 0;
	    break;
	case 'd':
	    debug_flag = 1;
	    nodaemon_flag = 1;
	    break;
	default:
	    usage();
	    break;
	}
    }
    argc -= optind;
    argv += optind;

    if (argc != 1)
	usage();
    dns_message_init();
    ParseConfig(argv[0]);
    cip_net_indexer_init();

    if (!nodaemon_flag)
    	daemonize();
    write_pid_file();

    if (!debug_flag) {
        syslog(LOG_INFO, "Sleeping for %d seconds", 60 - (int) (time(NULL) % 60));
        sleep(60 - (time(NULL) % 60));
    }
    syslog(LOG_INFO, "%s", "Running");

    do {
	useArena(); /* Initialize a memory arena for data collection. */
	if (debug_flag && break_start.tv_sec > 0) {
	    struct timeval now;
	    gettimeofday(&now, NULL);
	    syslog(LOG_INFO, "inter-run processing delay: %ld ms",
		(now.tv_usec - break_start.tv_usec) / 1000 +
		1000 * (now.tv_sec - break_start.tv_sec));
	}
	result = Pcap_run(dns_message_handle, ip_message_handle);
	if (debug_flag)
	    gettimeofday(&break_start, NULL);
	if (0 == fork()) {
	    /* Child dumps data from its copy of the arena. */
	    /*amalloc_report();*/
	    dns_message_report();
	    ip_message_report();
	    _exit(0);
	}
	/* Parent quickly frees and clears its copy of the data so it can
	   resume processing packets. */
	freeArena();
	dns_message_clear_arrays();
	ip_message_clear_arrays();

	{
	    /* Reap children. (Most recent probably has not exited yet, but
	     * older ones should have.) */
	    int cstatus = 0;
	    pid_t pid;
	    while ((pid = waitpid(0, &cstatus, WNOHANG)) > 0) {
		if (WIFSIGNALED(cstatus))
		    syslog(LOG_NOTICE, "child %d exited with signal %d",
			pid, WTERMSIG(cstatus));
		if (WIFEXITED(cstatus) && WEXITSTATUS(cstatus) != 0)
		    syslog(LOG_NOTICE, "child %d exited with status %d",
			pid, WEXITSTATUS(cstatus));
	    }
	}

    } while (result > 0);

    Pcap_close();
    return 0;
}
