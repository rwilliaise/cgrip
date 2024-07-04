
#ifndef CGRIP_H_
#define CGRIP_H_

#define CGRIP_VERSION "0.1.0"

#ifdef __GNUC__
#define inline __inline
#define CGRIP_NORETURN __attribute((noreturn))
#else
/* hellish consequences but whatever */
#define inline
#define noreturn
#endif

#include <curl/curl.h>

#ifdef CGRIP_TERMCOLOR
#define CGRIP_RESET "\033[0m"
#define CGRIP_RED "\033[31m"
#define CGRIP_YELLOW "\033[33m"
#define CGRIP_CYAN "\033[36m"

void putcolor(const char *col);
void fputcolor(const char *col, FILE *fp);
#else
#define CGRIP_RESET
#define CGRIP_RED
#define CGRIP_YELLOW
#define CGRIP_CYAN
#define putcolor(x)
#define fputcolor(x,y)
#endif

#define doom(ptr) if (!ptr) fatal("out of memory\n")

enum cgrip_normal_type {
    cgrip_normal_type_none,
    cgrip_normal_type_gl,
    cgrip_normal_type_dx,
    cgrip_normal_type_both
};

extern struct arguments {
    char *output;
    char *output_zip;
    enum cgrip_normal_type save_normal;
    unsigned int downscale_width, downscale_height;
    unsigned int macro_scale;
    unsigned verbose : 1;
    unsigned downscale : 1;
    unsigned quantize : 1;
    unsigned save_zip : 1;
    unsigned save_ambientocclusion: 1;
    unsigned save_color : 1;
    unsigned save_displacement : 1;
    unsigned save_emission : 1;
    unsigned save_metalness : 1;
    unsigned save_opacity : 1;
    unsigned save_roughness : 1;
#ifdef CGRIP_TERMCOLOR
    unsigned use_term_colors : 1;
#endif
} arguments;

void verbose(const char *fmt, ...);
void warn(const char *fmt, ...);
CGRIP_NORETURN void fatal(const char *fmt, ...);

#endif /* CGRIP_H_ */
