
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gen_godot4.h"
#include "cgapi.h"
#include "cgrip.h"

#define GEN_GODOT4_HEADER "[gd_resource type=\"StandardMaterial3D\" load_steps=%d format=3]\n\n"
#define GEN_GODOT4_EXTTXT "[ext_resource type=\"Texture2D\" path=\"%s\" id=\"%d\"]\n"
#define GEN_GODOT4_RESHEADER "[resource]\n"

char gen_godot4_root[4096] = { 0 };
const char *gen_godot4_res = "res://";
const char *gen_godot4_rootpattern = "/project.godot";

extern char *realpath(const char *path, char *resolved_path);

static const char *get_extension(const char *path)
{
    const char *p;
    for (p = path + strlen(path); p > path; p--) {
        if (*p == '.')
            return ++p;
    }
    return NULL;
}

void gen_godot4_set_root(const char *path)
{
    const char *p;
    if (!realpath(path, gen_godot4_root)) {
        warn("--gen-godot4 couldn't find %s: %s\n", path, strerror(errno));
        return;
    }
    p = get_extension(gen_godot4_root);
    if (!p || strcmp(p, "godot")) {
        *gen_godot4_root = 0;
        warn("invalid --gen-godot4 root, defaulting to none\n");
    } else
        verbose("gen_godot4_root = %s\n", gen_godot4_root);
}

static char *gen_godot4_get_root(const char *path)
{
    char buf[4096] = { 0 };
    int sz;
    if (*gen_godot4_root)
        return gen_godot4_root;
    if (!realpath(path, buf))
        return NULL;
    verbose("get_root: %s\n", buf);
    sz = strlen(buf);
    while (1) {
        char *p;
        FILE *t;
        for (p = buf + sz; p > buf; p--) {
            if (*p == '/') {
                *p = 0;
                break;
            }
        }
        sz = p - buf;
        if (sz <= 0) return NULL;
        strncat_s(buf + sz, gen_godot4_rootpattern, sizeof buf - sz);
        verbose("%s\n", buf);
        t = fopen(buf, "r");
        if (t) {
            size_t size = strlen(buf) - strlen(gen_godot4_rootpattern) + 1;
            memcpy(gen_godot4_root, buf, size - 1);
            gen_godot4_root[size - 1] = 0;
            fclose(t);
            return gen_godot4_root;
        }
    }
    return NULL;
}

static int gen_godot4_is_subpath(const char *root, const char *subpath)
{
    if (strlen(subpath) < strlen(root))
        return 0;
    return !strncmp(subpath, root, strlen(root));
}

static char *gen_godot4_resource_path(const char *path)
{
    char buf[4096] = { 0 };
    char *root, *p, *out;
    int f, s;
    if (!realpath(path, buf))
        return NULL;
    root = gen_godot4_get_root(path);
    verbose("root: %s\n", root);
    if (!root)
        return NULL;
    if (!gen_godot4_is_subpath(root, buf)) {
        verbose("buf is not subpath... %s WITH %s\n", root, buf);
        return NULL;
    }
    p = &buf[strlen(root) + 1]; /* extra slash */
    f = strlen(gen_godot4_res) + strlen(p) + 1;
    out = malloc(f);
    doom(out);

    *out = 0;
    s = 0;
    s += strncat_s(out + s, gen_godot4_res, f - s);
    s += strncat_s(out + s, p, f - s);
    return out;
}

static void gen_godot4_add_map(struct cgapi_material *mat, enum cgapi_matmap matmap, const char *folder, FILE *out)
{
    struct cgapi_map *map = &mat->maps[matmap];
    char buf[256] = { 0 };
    char *path;
    int sz = 0;
    if (map->data == NULL)
        return;
    sz += strncat_s(buf + sz, folder, sizeof buf - sz);
    sz += strncat_s(buf + sz, "/", sizeof buf - sz);
    cgapi_material_get_filename(mat, matmap, buf + sz, sizeof buf - sz);
    path = gen_godot4_resource_path(buf);
    if (!path)
        return;
    fprintf(out, GEN_GODOT4_EXTTXT, path, matmap);
    free(path);
}

int gen_godot4_generate(struct cgapi_material *mat, const char *out)
{
    char buf[256] = { 0 };
    int sz = 0, i;
    FILE *fp;
    sz += strncat_s(buf + sz, out, sizeof buf - sz);
    sz += strncat_s(buf + sz, "/", sizeof buf - sz);
    sz += strncat_s(buf + sz, mat->id, sizeof buf - sz);
    sz += strncat_s(buf + sz, ".tres", sizeof buf - sz);
    fp = fopen(buf, "w");
    if (!fp) {
        warn("cannot access %s\n", buf);
        return 0;
    }

    printf("generating %s.tres\n", mat->id);
    sz = 1;
    for (i = 0; i < CGAPI_MAPNUM; i++) {
        if (cgapi_material_has_map(mat, i))
            sz++;
    }
    fprintf(fp, GEN_GODOT4_HEADER, sz);
    for (i = 0; i < CGAPI_MAPNUM; i++) {
        gen_godot4_add_map(mat, i, out, fp);
    }
    fputc('\n', fp);
    fprintf(fp, GEN_GODOT4_RESHEADER);

    if (cgapi_material_has_map(mat, cgapi_matmap_ambientocclusion)) {
        fprintf(fp, "ao_enabled = true\n");
        fprintf(fp, "ao_texture = ExtResource(\"%d\")\n", cgapi_matmap_ambientocclusion);
    }
    if (cgapi_material_has_map(mat, cgapi_matmap_color))
        fprintf(fp, "albedo_texture = ExtResource(\"%d\")\n", cgapi_matmap_color);
    if (cgapi_material_has_map(mat, cgapi_matmap_displacement)) {
        fprintf(fp, "heightmap_enabled = true\n");
        fprintf(fp, "heightmap_texture = ExtResource(\"%d\")\n", cgapi_matmap_displacement);
    }
    if (cgapi_material_has_map(mat, cgapi_matmap_emission)) {
        fprintf(fp, "emission_enabled = true\n");
        fprintf(fp, "emission_texture = ExtResource(\"%d\")\n", cgapi_matmap_emission);
    }
    if (cgapi_material_has_map(mat, cgapi_matmap_metalness)) {
        fprintf(fp, "metallic = 1.0\n");
        fprintf(fp, "metallic_texture = ExtResource(\"%d\")\n", cgapi_matmap_metalness);
    }
    if (cgapi_material_has_map(mat, cgapi_matmap_normalgl)) {
        fprintf(fp, "normal_enabled = true\n");
        fprintf(fp, "normal_texture = ExtResource(\"%d\")\n", cgapi_matmap_normalgl);
    }
    if (cgapi_material_has_map(mat, cgapi_matmap_roughness))
        fprintf(fp, "roughness_texture = ExtResource(\"%d\")\n", cgapi_matmap_roughness);
    fprintf(fp, "texture_filter = %d\n", arguments.filter_nearest ? 2 : 3);

    fclose(fp);
    return 1;
}
