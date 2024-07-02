
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#include "cgapi.h"

static const char *cgapi_quality[] = {
    "1K-PNG",
    "2K-PNG",
};

static const char *cgapi_matmap[] = {
    /* id, quality */
    "%s_%s_Color.png",
    "%s_%s_Displacement.png",
    "%s_%s_NormalDX.png",
    "%s_%s_NormalGL.png",
    "%s_%s_Opacity.png",
    "%s_%s_Roughness.png",
};

static const char *cgapi_download_ids_url = "https://ambientcg.com/api/v2/downloads_csv?type=Material&id=%s";

static int cgapi_read_mem(char *data, size_t size, size_t nmemb, void *ud)
{
    return 0;
}

static int sprintf_checked(char *dest, int sz, const char *fmt, ...)
{
    int out_size;
    va_list args;

    va_start(args, fmt);
    out_size = vsprintf(dest, fmt, args);
    va_end(args);
    
    return out_size;
}

struct cgapi_materials cgapi_download_ids(enum cgapi_quality quality, const char **ids, int id_count)
{
    struct cgapi_materials out = { 0 };
    char url[256];
    int sz = 0, i;
    sz += snprintf(url + sz, 256, cgapi_download_ids_url, 2);
    for (i = 0; i < id_count; i++) {
    }
    return out;
}

