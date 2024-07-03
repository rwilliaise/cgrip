
#ifndef CGRIP_H_
#define CGRIP_H_

#define CGRIP_VERSION "0.1.0"

#ifdef __GNUC__
#define inline __inline
#else
/* hellish consequences but whatever */
#define inline
#endif

#include <MagickWand/MagickWand.h>
#include <curl/curl.h>

#ifdef CGRIP_TERMCOLOR
#define CGRIP_RESET "\033[0m"
#define CGRIP_RED "\033[31m"
#define CGRIP_CYAN "\033[36m"

void putcolor(const char *col);
void fputcolor(const char *col, FILE *fp);
#else
#define CGRIP_RESET
#define CGRIP_RED
#define CGRIP_CYAN
#define putcolor(x)
#define fputcolor(x,y)
#endif

#define doom(ptr) if (!ptr) fatal("out of memory\n")

extern struct arguments {
    char *output;
    unsigned verbose : 1;
#ifdef CGRIP_TERMCOLOR
    unsigned use_term_colors : 1;
#endif
} arguments;

void verbose(const char *fmt, ...);
void fatal(const char *fmt, ...);

#endif /* CGRIP_H_ */
