
#include <archive.h>
#include <archive_entry.h>
#include <curl/curl.h>
#include <stddef.h>
#include <string.h>

#include "cgapi.h"
#include "cgrip.h"

static const char *cgapi_quality[] = {
    "1K-PNG",
    "2K-PNG",
    NULL,
};

static const char *cgapi_matmap[] = {
    /* id, quality */
    "Color.png",
    "Displacement.png",
    "NormalDX.png",
    "NormalGL.png",
    "Opacity.png",
    "Roughness.png",
    NULL,
};

static const char *cgapi_download_ids_url = "https://ambientcg.com/api/v2/downloads_csv?type=Material&id=";

static int strncat_s(char *dest, const char *src, int sz)
{
    if (sz - 1 <= 0) return 0;
    strncat(dest, src, sz - 1);
    return strlen(src);
}

static int endcmp(const char *str, const char *end) {
    int offset = strlen(str) - strlen(end);
    if (offset < 0) return 1;
    return strcmp(str + offset, end);
}

struct cgapi_mem {
    char *res;
    size_t sz;
};

static int cgapi_read_mem(char *data, size_t membsz, size_t nmemb, void *ud)
{
    size_t size = membsz * nmemb;
    struct cgapi_mem *mem = (struct cgapi_mem *) ud;

    char *ptr = realloc(mem->res, mem->sz + size + 1);
    doom(ptr);

    mem->res = ptr;
    memcpy(&(mem->res[mem->sz]), data, size);
    mem->sz += size;
    mem->res[mem->sz] = 0;
    return size;
}

static struct cgapi_mem cgapi_curl_chunk(const char *url)
{
    struct cgapi_mem out = { 0 };
    CURL *curl = curl_easy_init();
    CURLcode res;
    char buf[CURL_ERROR_SIZE];

    if (!curl) return out;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cgapi_read_mem);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, (long) arguments.verbose);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buf);
    res = curl_easy_perform(curl);
    out.sz++; /* null char */

    if (res != CURLE_OK)
        fatal("curl error: %s\n", buf);
    return out;
}

static int cgapi_rip_textures(struct cgapi_material *mat, struct cgapi_mem zip_mem)
{
    struct archive *a;
    struct archive_entry *entry;
    int err;

    a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_zip(a); /* ambientCG only serves .zip */
    err = archive_read_open_memory(a, zip_mem.res, zip_mem.sz - 1);
    if (err != ARCHIVE_OK) {
        verbose("failed to read zip for %s: %s\n", mat->id, archive_error_string(a));
        archive_read_free(a);
        return 1;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        int i;
        verbose("%s: %s\n", mat->id, archive_entry_pathname(entry));

        for (i = 0; i < CGAPI_MAPNUM; i++) {
            if (!endcmp(archive_entry_pathname(entry), cgapi_matmap[i])) {
                struct cgapi_map *map = &mat->maps[i];
                size_t size = archive_entry_size(entry);
                void *data = malloc(size);
                ExceptionInfo *exception;
                doom(data);

                err = archive_read_data(a, data, size);
                if (err == ARCHIVE_FATAL) {
                    verbose("failed to read data from zip file: %s\n", archive_error_string(a));
                    free(data);
                    break;
                }
                verbose("found %s\n", cgapi_matmap[i]);
                
                exception = AcquireExceptionInfo();
                map->info = CloneImageInfo(NULL);
                map->image = BlobToImage(map->info, data, size, exception);
                free(data);
                if (map->image == NULL) {
                    verbose("image read failure: %s\n", exception->description);
                    DestroyExceptionInfo(exception);
                    break;
                }
                DestroyExceptionInfo(exception);
                break;
            }
        }
    }

    err = archive_read_free(a);
    if (err != ARCHIVE_OK)
        verbose("probable memory leak: %s\n", archive_error_string(a));
    return 0;
}

static void cgapi_process_material(struct cgapi_materials *out, const char *id, const char *quality, const char *url)
{
    const char *expected_quality = cgapi_quality[out->quality];
    struct cgapi_mem zip_mem = { 0 };
    struct cgapi_material *mat;
    int i;

    if (strcmp(quality, expected_quality) != 0) return;
    out->materials = realloc(out->materials, ++out->material_count * sizeof(struct cgapi_material));
    mat = &out->materials[out->material_count - 1];
    doom(out->materials);

    mat->id = malloc(strlen(id) + 1);
    mat->quality = out->quality;
    doom(mat->id);

    memcpy(mat->id, id, strlen(id) + 1);
    for (i = 0; i < CGAPI_MAPNUM; i++) {
        mat->maps[i].image = NULL;
        mat->maps[i].info = NULL;
    }

    verbose("downloading material %s\n", id);
    zip_mem = cgapi_curl_chunk(url);
    verbose("downloaded %s -> %lu B\n", id, zip_mem.sz);

    if (cgapi_rip_textures(mat, zip_mem)) {
        out->material_count--;
        free(mat->id);
    }
    free(zip_mem.res);
    return;
}

static struct cgapi_materials cgapi_process_downloads_csv(enum cgapi_quality quality, char *csv)
{
    struct cgapi_materials out = { 0 };
    int line = 0, col = 0;
    char *p;
    char *asset_id = 0, *download_attribute = 0, *raw_link = 0;
    out.quality = quality;

    for (p = csv; *p; p++) {
        if (col == 0 && !asset_id)
            asset_id = p;
        if (col == 1 && !download_attribute)
            download_attribute = p;
        if (col == 5 && !raw_link)
            raw_link = p;

        switch(*p){
        case ',':
            col++;
            *p = 0;
            break;
        case '\r': /* handle crlf */
            *p = 0;
            p++;
        case '\n':
            *p = 0;
            if (!asset_id || !download_attribute || !raw_link)
                fatal("unexpected downloads.csv eol, try re-running\n");
            /* check columns are actually what we are processing */
            if (line == 0 && (strcmp(asset_id, "assetId") != 0
                        || strcmp(download_attribute, "downloadAttribute") != 0
                        || strcmp(raw_link, "rawLink") != 0))
                fatal("unexpected downloads.csv format, have you updated cgrip?\n");
            cgapi_process_material(&out, asset_id, download_attribute, raw_link);
            line++;
            col = 0;
            asset_id = NULL;
            download_attribute = NULL;
            raw_link = NULL;
            break;
        default:
            break;
        }
    }
    
    verbose("found %d materials\n", out.material_count);
    return out;
}

struct cgapi_materials cgapi_download_ids(enum cgapi_quality quality, const char **ids, int id_count)
{
    struct cgapi_materials out;
    struct cgapi_mem mem;
    char url[256];
    int sz = 0, i;

    *url = 0;
    sz += strncat_s(url + sz, cgapi_download_ids_url, sizeof url - sz);
    for (i = 0; i < id_count; i++) {
        if (i > 0)
            sz += strncat_s(url + sz, ",", sizeof url - sz);
        sz += strncat_s(url + sz, ids[i], sizeof url - sz);
    }

    verbose("downloading %s\n", url);
    mem = cgapi_curl_chunk(url);
    verbose("downloaded successfully -> %lu B\n", mem.sz);
    out = cgapi_process_downloads_csv(quality, mem.res);
    free(mem.res);
    return out;
}

void cgapi_materials_free(struct cgapi_materials *mats) {
    int i, j;
    for (i = 0; i < mats->material_count; i++) {
        free(mats->materials[i].id);
        for (j = 0; j < CGAPI_MAPNUM; j++) {
            if (mats->materials[i].maps[j].image != NULL)
                DestroyImage(mats->materials[i].maps[j].image);
            if (mats->materials[i].maps[j].info != NULL)
                DestroyImageInfo(mats->materials[i].maps[j].info);
        }
    }
    free(mats->materials);
}

