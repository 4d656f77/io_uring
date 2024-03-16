// https://man7.org/linux/man-pages/man7/io_uring.7.html

#include "IOUring.h"

IOUring g_IOUring;


int main(int argc, char* argv[]) {
    int res;
    /* Setup io_uring for use */
    if (g_IOUring.initIOUring()) {
        fprintf(stderr, "Unable to setup uring!\n");
        return 1;
    }
    /*
    * A while loop that reads from stdin and writes to stdout.
    * Breaks on EOF.
    */
    while (1) {
        /* Initiate read from stdin and wait for it to complete */
        g_IOUring.submitToSq(STDIN_FILENO, IORING_OP_READ);
        /* Read completion queue entry */
        res = g_IOUring.readFromCq();
        if (res > 0) {
            /* Read successful. Write to stdout. */
            g_IOUring.submitToSq(STDOUT_FILENO, IORING_OP_WRITE);
            g_IOUring.readFromCq();
        }
        else if (res == 0) {
            /* reached EOF */
            break;
        }
        else if (res < 0) {
            /* Error reading file */
            fprintf(stderr, "Error: %s\n", strerror(abs(res)));
            break;
        }
        g_IOUring.offset += res;
    }
    return 0;
}