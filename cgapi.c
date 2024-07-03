
#include <archive.h>
#include <archive_entry.h>
#include <curl/curl.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "cgapi.h"
#include "cgrip.h"

#include "lodepng.h"

static const char *cgapi_quality[] = {
    "1K-PNG",
    "2K-PNG",
    "4K-PNG",
    "8K-PNG",
    NULL,
};

static const char *cgapi_matmap[] = {
    "AmbientOcclusion.png",
    "Color.png",
    "Displacement.png",
    "Emission.png",
    "Metalness.png",
    "NormalDX.png",
    "NormalGL.png",
    "Opacity.png",
    "Roughness.png",
    NULL,
};

static const char *cgapi_output[] = {
    "_ambientocclusion.png",
    ".png", /* no suffix for albedo */
    "_displacement.png",
    "_emission.png",
    "_metalness.png",
    "_normal_dx.png",
    "_normal_gl.png",
    "_opacity.png",
    "_roughness.png",
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
    int enabled_matmaps[CGAPI_MAPNUM];

    a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_zip(a); /* ambientCG only serves .zip */
    err = archive_read_open_memory(a, zip_mem.res, zip_mem.sz - 1);
    if (err != ARCHIVE_OK) {
        verbose("failed to read zip for %s: %s\n", mat->id, archive_error_string(a));
        archive_read_free(a);
        return 0;
    }

    enabled_matmaps[cgapi_matmap_ambientocclusion] = arguments.save_ambientocclusion;
    enabled_matmaps[cgapi_matmap_color] = arguments.save_color;
    enabled_matmaps[cgapi_matmap_displacement] = arguments.save_displacement;
    enabled_matmaps[cgapi_matmap_emission] = arguments.save_emission;
    enabled_matmaps[cgapi_matmap_metalness] = arguments.save_metalness;
    enabled_matmaps[cgapi_matmap_normaldx] = arguments.save_normal == cgrip_normal_type_both 
                                            || arguments.save_normal == cgrip_normal_type_dx;
    enabled_matmaps[cgapi_matmap_normalgl] = arguments.save_normal == cgrip_normal_type_both 
                                            || arguments.save_normal == cgrip_normal_type_gl;
    enabled_matmaps[cgapi_matmap_opacity] = arguments.save_opacity;
    enabled_matmaps[cgapi_matmap_roughness] = arguments.save_roughness;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        int i;
        verbose("%s: %s\n", mat->id, archive_entry_pathname(entry));

        for (i = 0; i < CGAPI_MAPNUM; i++) {
            if (!enabled_matmaps[i]) continue;
            if (!endcmp(archive_entry_pathname(entry), cgapi_matmap[i])) {
                struct cgapi_map *map = &mat->maps[i];
                size_t size = archive_entry_size(entry);
                void *data = malloc(size);
                doom(data);

                err = archive_read_data(a, data, size);
                if (err == ARCHIVE_FATAL) {
                    verbose("failed to read data from zip file: %s\n", archive_error_string(a));
                    free(data);
                    break;
                }
                verbose("found %s %s\n", mat->id, cgapi_matmap[i]);
                lodepng_decode32(&map->data, &map->width, &map->height, data, size);
                free(data);
                if (map->data == NULL)
                    warn("failed to load image %s\n", archive_entry_pathname(entry));
                break;
            }
        }
    }

    err = archive_read_free(a);
    if (err != ARCHIVE_OK)
        verbose("probable memory leak: %s\n", archive_error_string(a));
    return 1;
}

static const char *get_extension(const char *path)
{
    const char *p;
    for (p = path + strlen(path); p > path; p--) {
        if (*p == '.')
            return ++p;
    }
    return NULL;
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
        mat->maps[i].data = NULL;
    }

    printf("downloading %s.zip\n", id);
    verbose("downloading material %s\n", id);
    zip_mem = cgapi_curl_chunk(url);
    verbose("downloaded %s -> %lu B\n", id, zip_mem.sz);

    if (arguments.save_zip) {
        char buf[256];
        int sz = 0;

        verbose("saving zip (%s.zip)...\n", id);

        *buf = 0;
        if (arguments.output_zip) {
            sz += strncat_s(buf + sz, arguments.output_zip, sizeof buf - sz);
            sz += strncat_s(buf + sz, "/", sizeof buf - sz);
        } else if (arguments.output) {
            sz += strncat_s(buf + sz, arguments.output, sizeof buf - sz);
            sz += strncat_s(buf + sz, "/", sizeof buf - sz);
        }
        sz += strncat_s(buf + sz, mat->id, sizeof buf - sz);
        sz += strncat_s(buf + sz, ".zip", sizeof buf - sz);
        if (get_extension(buf) && !strcmp(get_extension(buf), "zip")) {
            FILE *out = fopen(buf, "wb");
            fwrite(zip_mem.res, sizeof(char), zip_mem.sz - 1, out);
            fclose(out);
            verbose("saved zip (%s.zip)\n", id);
        } else {
            warn("path too long, failed to save zip to %s\n", buf);
        }
    }

    if (!cgapi_rip_textures(mat, zip_mem)) {
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

static void cgapi_map_save(struct cgapi_material *mat, enum cgapi_matmap matmap, const char *out)
{
    struct cgapi_map *map = &mat->maps[matmap];
    char buf[256];
    int sz = 0;

    if (map->data == NULL) return;
    *buf = 0;
    if (out) {
        sz += strncat_s(buf + sz, out, sizeof buf - sz);
        sz += strncat_s(buf + sz, "/", sizeof buf - sz);
    }
    sz += strncat_s(buf + sz, mat->id, sizeof buf - sz);
    sz += strncat_s(buf + sz, cgapi_output[matmap], sizeof buf - sz);
    verbose("found extension: %s\n", get_extension(buf));
    printf("extracting %s%s\n", mat->id, cgapi_output[matmap]);
    if (get_extension(buf) && !strcmp(get_extension(buf), "png")) { /* TODO: other formats? */
        unsigned error = lodepng_encode32_file(buf, map->data, map->width, map->height);
        if (error)
            warn("png write error: %s\n", lodepng_error_text(error));
    } else {
        warn("path too long, failed to save material map for %s to %s\n", mat->id, buf);
    }
}

void cgapi_material_save(struct cgapi_material *mat, const char *out)
{
    if (arguments.save_ambientocclusion)
        cgapi_map_save(mat, cgapi_matmap_ambientocclusion, out);
    if (arguments.save_color)
        cgapi_map_save(mat, cgapi_matmap_color, out);
    if (arguments.save_displacement)
        cgapi_map_save(mat, cgapi_matmap_displacement, out);
    if (arguments.save_emission)
        cgapi_map_save(mat, cgapi_matmap_emission, out);
    if (arguments.save_metalness)
        cgapi_map_save(mat, cgapi_matmap_metalness, out);
    if (arguments.save_opacity)
        cgapi_map_save(mat, cgapi_matmap_opacity, out);
    if (arguments.save_roughness)
        cgapi_map_save(mat, cgapi_matmap_roughness, out);

    switch (arguments.save_normal) {
    case cgrip_normal_type_dx:
        cgapi_map_save(mat, cgapi_matmap_normaldx, out);
        break;
    case cgrip_normal_type_both:
        cgapi_map_save(mat, cgapi_matmap_normaldx, out);
    case cgrip_normal_type_gl:
        cgapi_map_save(mat, cgapi_matmap_normalgl, out);
        break;
    default:
        break;
    }
        
}

void cgapi_materials_save(struct cgapi_materials *mats, const char *out)
{
    int i = 0;
    for (i = 0; i < mats->material_count; i++) {
        cgapi_material_save(&mats->materials[i], out);
    }
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

    if (out.material_count == 0)
        warn("found no materials matching given ids.\n");
    return out;
}

void cgapi_materials_free(struct cgapi_materials *mats)
{
    int i, j;
    for (i = 0; i < mats->material_count; i++) {
        free(mats->materials[i].id);
        for (j = 0; j < CGAPI_MAPNUM; j++) {
            if (mats->materials[i].maps[j].data != NULL)
                free(mats->materials[i].maps[j].data);
        }
    }
    free(mats->materials);
}

