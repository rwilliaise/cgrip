
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

void verbose(const char *fmt, ...);
void fatal(const char *fmt, ...);

#endif // CGRIP_H_
