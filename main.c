#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>
#include <poll.h>

static int handler_count = 0;
static const char **handlers = NULL;
static const char *pfile = NULL;

static void usage(int argc, char *argv[]) {
    fprintf(stderr, "usage: %s [-h] <file> <handler1> [handler2] ...\n", argv[0]);
    exit(1);
}

/* fork handler */
static void run_handler(const char *handler) {
    pid_t pid = fork();

    if (pid < 0) {
        syslog(LOG_ERR, "fork error\n");
        return;
    }

    if (pid == 0) { /* the child process, exec the handler */
        execl(handler, handler, pfile, NULL);
        syslog(LOG_ERR, "fail to execute %s", handler);
        exit(255);	/* execl() failed */        
    }
    
    pid = waitpid(pid, NULL, 0);
    if (pid == -1)
        syslog(LOG_ERR, "waitpid error\n");
}

int main(int argc, char *argv[]) 
{
    int opt, count, pret, i;
    const char *arg;
    struct pollfd pfd;

    while ((opt = getopt(argc, argv, "h")) != -1)
        switch (opt) {
        case 'h':
            usage(argc, argv);
            break;
        default:
            usage(argc, argv);
        }

    if (optind >= argc)
        usage(argc, argv);
    
    /* open the file to monitor */
    pfile = argv[optind++];
    pfd.fd = open(pfile, O_RDONLY, 0);
    if (pfd.fd < 0) {
        fprintf(stderr, "fail to open `%s', exit.\n", arg);
        exit(2);
    }
#ifdef DEBUG
    printf("debug: %s is opened\n", pfile);
#endif

    /* read and verify handlers */
    count = argc - optind;
    if (count > 0) {
        handlers = malloc(count * sizeof(char*));
        while (optind < argc) {
            arg = argv[optind++];
            if (access(arg, R_OK | X_OK)) {
                printf("warning: fail to verify handler `%s', check existence and permissions\n", arg);
                continue;
            }
            handlers[handler_count++] = arg;
        }
    }
#ifdef DEBUG
    printf("debug: totally %d handlers loaded\n", handler_count);
    openlog(argv[0], LOG_PERROR, LOG_USER);
#else
    if (handler_count == 0) {
        fprintf(stderr, "no valid handler given, exit.\n");
        exit(3);
    }
    daemon(0, 0);
    openlog(argv[0], 0, LOG_DAEMON);
#endif


    /* run handlers for the very first time */
    syslog(LOG_INFO, "run event handlers for the first time\n", pfile);
    for (i=0; i<handler_count; i++)
        run_handler(handlers[i]);
    
    /* the main loop */
    pfd.events = 0;
    pfd.revents = 0;
    while ((pret = poll(&pfd, 1, -1)) > 0) {
        if (pfd.revents & POLLERR) {
            syslog(LOG_INFO, "POLLERR detected on %s, dispatching event to handlers\n", pfile);
            for (i=0; i<handler_count; i++)
                run_handler(handlers[i]);
        }
        pfd.revents = 0;
    }
    
    if (handlers != NULL)
        free(handlers);
    close(pfd.fd);
    closelog();
}
