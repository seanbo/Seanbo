/*
 ---- General Information ----
  Program  : slrdaemon.c
  Date     : 7/11/2011
  Author   : seanbo

 ----  Specific Information ----
  Compile     : gcc -o daemon daemon.c
  Run         : ./daemon
  Test        : ps -ef|grep daemon (or ps -aux on BSD systems)
  Test log    : tail -f /tmp/daemon.log
  Test signal : kill -HUP `cat /tmp/daemon.lock`
  Terminate   : kill `cat /tmp/daemon.lock`
*/

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#define RUNNING_DIR	"/tmp"
#define TMPDIR          "/tmp"
#define LOCK_FILE	"daemon.lock"
#define LOG_FILE	"daemon.log"

#define USAGESTRING	"[-t][-v][-c seconds][-f logfile]"
#define MAXLINE		(80 + 1)

/* Default cycle time value 5 minutes */
#define CYCLE		(5)
#define OPTSTRING	"tvc:f:"

static char _sccsid[] = { "daemon 1.0 7/10/2011 " };

char progname[ MAXLINE ];
char logfile[ MAXLINE ];
char msgline[ MAXLINE + 1 ];
int test_mode;

void signal_handler(int sig) {

    switch(sig){
    case SIGHUP:
        /* rehash the server */
        break;
    case SIGTERM:
        /* finalize the server */
        exit(EXIT_SUCCESS);
        break;
    }

}   /*  End signal_handler(int)  */

void log_message(char *filename, char *message) {

   time_t now=time(NULL);
   char *time_string=ctime(&now);
   time_string[strlen(time_string)-1]=0;        // remove \n

    FILE *logfile;
        logfile=fopen(filename,"a");
        if(!logfile) return;
        fprintf(logfile,"%s: %s\n", time_string, message);
        fclose(logfile);

}  /*  End log_message(char *, char *)  */

void daemonize() {

    int lfp;
    char str[10];

    /* Our process ID and Session ID */
    pid_t pid, sid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(027);

    /* Change the current working directory */
    if ((chdir(TMPDIR)) < 0) {
        /* Log the failure */
        syslog(LOG_ERR, "Could not change working directory to /\n");
      exit(EXIT_FAILURE);
    }

    lfp=open(LOCK_FILE, O_RDWR|O_CREAT, 0640);
    if (lfp<0) exit(EXIT_FAILURE); /* can not open */
    if (lockf(lfp,F_TLOCK,0)<0) exit(EXIT_SUCCESS); /* can not lock */
    /* only first instance continues */

    sprintf(str,"%d\n",getpid());
    write(lfp,str,strlen(str)); /* record pid to lockfile */

    /* Setup Signal handlers */
    signal(SIGHUP,signal_handler); /* hangup signal */
    signal(SIGCHLD,SIG_IGN); /* ignore child */
    signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGTERM,signal_handler); /* software termination signal from kill */

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        /* Log the failure */
        syslog(LOG_ERR, "Could not create process group\n");
        exit(EXIT_FAILURE);
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

}  /*   End daemonize()  */

int process(void) {

  syslog(LOG_NOTICE, "Running \n");

}  /*  End process()  */

int main(int argc, char *argv[]) {

    /* Begin Declarations */

        extern int getopt();
        extern int optind;
        extern char *optarg;	/* Captures arguments to options */

        int c_opt;

        long elapsed;
        int cycle_time;

    /* End Declrations */

    /* Begin Initialize Variables */

        strcpy( progname, argv[0] );
        strcpy( logfile, LOG_FILE );
        test_mode = 0;
        cycle_time = CYCLE;

    /* End Initialize Variables */

    /* Begin Parse Options */

        while( ( c_opt = getopt( argc, argv, OPTSTRING ) ) != EOF )    {
            switch( c_opt )    {
                case 'v':
                    /* -v (Just print version & exit) */
                    printf( "%s\n", _sccsid );
                    exit(EXIT_SUCCESS);
                case 't':
                    /* -t (Set Test Mode) */
                    test_mode = 1;
                    break;
                case 'f':
                    /* -f arg */
                    if( sscanf( optarg, "%s", logfile ) != 1 )    {
                        fprintf( stderr, "%s: unreadable log file argument\n", progname );
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 'c':
                    /* -c arg */
                    if( sscanf( optarg, "%d", &cycle_time ) != 1 ){
                        fprintf( stderr, "%s: unreadable cycle time argument\n", progname );
                        exit(EXIT_FAILURE);
                    }
                    break;
                default:
                    fprintf( stderr, "%s: Bad Option -%c\n", argv[ 0 ], c_opt );
                    fprintf( stderr, "Usage: %s %s\n", argv[ 0 ], USAGESTRING );
                    exit(EXIT_FAILURE);
            }
        }

	/* On exiting loop of option parsing optind indexes the next
	 * argv[] argument to the function.
	 */
	if( signal( SIGINT, SIG_IGN ) != SIG_IGN )
		signal( SIGINT, SIG_IGN );
	if( signal( SIGKILL, SIG_IGN ) != SIG_IGN )
		signal( SIGKILL, SIG_IGN );

        /* Open a connection to the syslog server */
        openlog(argv[0],LOG_NOWAIT|LOG_PID,LOG_USER);

        /* Sends a message to the syslog daemon */
        syslog(LOG_NOTICE, "Successfully started %s", progname);

        /* Daemon-specific initialization goes here */

        daemonize();

        /* The Big Loop */
        while (1) {
           /* Do some task here ... */
           process();

           /* Wake up and look every cycle_time */
          sleep( cycle_time );

        }

        /* this is optional and only needs to be done when your daemon exits */
        closelog();

   exit(EXIT_SUCCESS);

}   /*  End main(int, char *)   */
