
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>

#include "cgrip.h"
#include "cgapi.h"

struct arguments arguments = { 0 };

const char *usage_text =
    "Usage: cgrip [OPTIONS] ID...\n"
    "cgrip " CGRIP_VERSION
    "\n"
    "Downloads materials using ids ID... off of AmbientCG.\n"
    "\n"
    "optional arguments\n"
    "   -h, --help\n"
    "       Show usage information.\n"
    "   -o, --output OUTPUT\n"
    "       Outputs processed image file to OUTPUT.\n"
    "   -v, --verbose\n"
    "       Print some extra debug things.\n"
#ifdef CGRIP_TERMCOLOR
    "   --no-color\n"
    "       Forces cgrip to not print color codes.\n"
#endif
    ;

void verbose(const char *fmt, ...)
{
    va_list args;
    if (!arguments.verbose)
        return;
    
    putcolor(CGRIP_CYAN);
    fputs("[VERBOSE] ", stdout);
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    putcolor(CGRIP_RESET);
}

void fatal(const char *fmt, ...)
{
    va_list args;
    fputcolor(CGRIP_RED, stderr);
    fputs("[FATAL] ", stderr);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputcolor(CGRIP_RESET, stderr);
    exit(EXIT_FAILURE);
}

#ifdef CGRIP_TERMCOLOR
void putcolor(const char *col)
{
    if (!arguments.use_term_colors) return;
    fputs(col, stdout);
}

void fputcolor(const char *col, FILE *fp)
{
    if (!arguments.use_term_colors) return;
    fputs(col, fp);
}
#endif

static void usage(int status)
{
    puts(usage_text);
    exit(status);
}

static int is_directory(const char *path) {
    struct stat s;
    if (stat(path, &s) == 0) {
        return S_ISDIR(s.st_mode);
    }
    return 0;
}

static const char *get_extension(const char *path) {
    const char *p;
    for (p = path + strlen(path); p > path; p--) {
        if (*p == '.')
            return ++p;
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int opt, pargc;
    const char *short_opts = ":hvo:";
    struct option long_opts[] = {
        { "help", no_argument, NULL, 'h' },
        { "output", required_argument, NULL, 'o' },
        { "verbose", no_argument, NULL, 'v' },
        { "no-color", no_argument, NULL, 'n' },
        { 0 },
    };
    struct cgapi_materials mats = { 0 };

    MagickWandGenesis();

#ifdef CGRIP_TERMCOLOR
    arguments.use_term_colors = getenv("TERM") != NULL;
#endif

    while (1) {
        opt = getopt_long(argc, argv, short_opts,long_opts, NULL);
        if (opt == -1)
            break;

        switch (opt) {
        case 'h':
            usage(EXIT_SUCCESS);
            break;
        case 'o':
            if (!is_directory(optarg))
                fatal("%s is not valid output directory\n", optarg);
            arguments.output = optarg;
            break;
        case 'v':
            arguments.verbose = 1;
            break;
#ifdef CGRIP_TERMCOLOR
        case 'n':
            arguments.use_term_colors = 0;
            break;
#endif
        case ':':
            fatal("missing argument for option '%c'\n", optopt);
            break;
        case '?':
        default:
            fatal("unknown option '%c'\n", optopt);
            break;
        }
    }


    pargc = argc - optind;
    if (pargc < 1)
        usage(EXIT_FAILURE);

    mats = cgapi_download_ids(cgapi_quality_1k_png, (const char **) &argv[optind], pargc);

    MagickWandTerminus();
    return 0;
}
