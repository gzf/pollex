#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <poll.h>

static int handler_count = 0;
static char** handlers = NULL;

static void usage(int argc, char* argv[]) {
    fprintf(stderr, "usage: %s [-h] <file> [handler1] [handler2] ...\n", argv[0]);
    exit(1);
}

int main(int argc, char* argv[]) 
{
    int opt, count, pret;
    char* arg;
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
    
    // open the file to monitor
    arg = argv[optind++];
    pfd.fd = open(arg, O_RDONLY, 0);
    if (pfd.fd < 0) {
        fprintf(stderr, "fail to open `%s', exit.\n", arg);
        exit(2);
    }
#ifdef DEBUG
    printf("debug: %s is opened\n", arg);
#endif

    // read and verify handlers
    count = argc - optind;
    if (count > 0) {
        handlers = malloc(count * sizeof(char*));
        while (optind < argc) {
            arg = argv[optind++];
            if (access(arg, R_OK | X_OK)) {
                printf("warning: verify handler `%s' failed, check existence and permissions\n", arg);
                continue;
            }
            handlers[handler_count++] = arg;
        }
    }
#ifdef DEBUG
    printf("debug: totally %d handlers loaded\n", handler_count);
#endif

    // the main loop
    pfd.events = 0;
    pfd.revents = 0;
    while ((pret = poll(&pfd, 1, -1)) > 0) {
        if (pfd.revents & POLLERR) {
#ifdef DEBUG
            printf("debug: POLLERR detected\n");
#endif
        }
        pfd.revents = 0;
    }
    
    if (handlers != NULL)
        free(handlers);
    close(pfd.fd);
}
