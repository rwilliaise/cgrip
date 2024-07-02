
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdarg.h>

#include "cgrip.h"

struct {
    char *output;
    int verbose;
} arguments;

const char *usage_text =
    "Usage: cgrip [-o OUTPUT] [-v] [-h] ID...\n"
    CGRIP_VERSION
    "\n"
    "Downloads materials using ids ID... off of AmbientCG.\n"
    "\n"
    "optional arguments\n"
    "   -h, --help\n"
    "       Show usage information.\n"
    "\n"
    "   -o, --output OUTPUT\n"
    "       Outputs processed image file to OUTPUT.\n"
    "\n"
    "   -v, --verbose\n"
    "       Outputs processed image file to OUTPUT.\n";

void verbose(const char *fmt, ...)
{
    va_list args;
    if (!arguments.verbose)
        return;
    
    fputs("[VERBOSE] ", stdout);
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
}

void fatal(const char *fmt, ...)
{
    va_list args;
    fputs("[ FATAL ] ", stderr);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

static void usage(int status)
{
    puts(usage_text);
    exit(status);
}

int main(int argc, char *argv[])
{
    int opt, pargc;
    const char *short_opts = ":hv";
    struct option long_opts[] = {
        { "help", no_argument, NULL, 'h' },
        { "verbose", no_argument, NULL, 'v' },
        { 0 },
    };

    MagickWandGenesis();

    while (1) {
        opt = getopt_long(argc, argv, short_opts,long_opts, NULL);
        if (opt == -1)
            break;

        switch (opt) {
        case 'h':
            usage(EXIT_SUCCESS);
            break;
        case 'o':
        }
    }

    pargc = argc - optind;
    if (pargc < 1)
        fatal("expected one or more positional arguments, got %d\n", 1, pargc);

    MagickWandTerminus();
    return 0;
}
