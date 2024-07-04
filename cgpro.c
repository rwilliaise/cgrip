
#include <math.h>
#include <stdlib.h>
#include <limits.h>

#include "cgpro.h"
#include "lodepng.h"

/* default aseprite palette */
static const unsigned char default_palette[] = {
    0x00, 0x00, 0x00, /* idx 00 */
    0x22, 0x20, 0x34, /* idx 01 */
    0x45, 0x28, 0x3c, /* idx 02 */
    0x66, 0x39, 0x31, /* idx 03 */
    0x8f, 0x56, 0x3b, /* idx 04 */
    0xdf, 0x71, 0x26, /* idx 05 */
    0xd9, 0xa0, 0x66, /* idx 06 */
    0xee, 0xc3, 0x9a, /* idx 07 */
    0xfb, 0xf2, 0x36, /* idx 08 */
    0x99, 0xe5, 0x50, /* idx 09 */
    0x6a, 0xbe, 0x30, /* idx 10 */
    0x37, 0x94, 0x6e, /* idx 11 */
    0x4b, 0x69, 0x2f, /* idx 12 */
    0x52, 0x4b, 0x24, /* idx 13 */
    0x32, 0x3c, 0x39, /* idx 14 */
    0x3f, 0x3f, 0x74, /* idx 15 */
    0x30, 0x60, 0x82, /* idx 16 */
    0x5b, 0x6e, 0xe1, /* idx 17 */
    0x63, 0x9b, 0xff, /* idx 18 */
    0x5f, 0xcd, 0xe4, /* idx 19 */
    0xcb, 0xdb, 0xfc, /* idx 20 */
    0xff, 0xff, 0xff, /* idx 21 */
    0x9b, 0xad, 0xb7, /* idx 22 */
    0x84, 0x7e, 0x87, /* idx 23 */
    0x69, 0x6a, 0x6a, /* idx 24 */
    0x59, 0x56, 0x52, /* idx 25 */
    0x76, 0x42, 0x8a, /* idx 26 */
    0xac, 0x32, 0x32, /* idx 27 */
    0xd9, 0x57, 0x63, /* idx 28 */
    0xd7, 0x7b, 0xba, /* idx 29 */
    0x8f, 0x97, 0x4a, /* idx 30 */
    0x8a, 0x6f, 0x30, /* idx 31 */
};

/* bayer 8x8 matrix */
static const int matrix8[64] = {
    0,  32, 8,  40, 2,  34, 10, 42,
    48, 16, 56, 24, 50, 18, 58, 26,
    12, 44, 4,  36, 14, 46, 6,  38,
    60, 28, 52, 20, 62, 30, 54, 22,
    3,  35, 11, 43, 1,  33, 9,  41,
    51, 19, 59, 27, 49, 17, 57, 25,
    15, 47, 7,  39, 13, 45, 5,  37,
    63, 31, 55, 23, 61, 29, 53, 21
};

static unsigned int col_diff[4 * 128] = { 0 };

static unsigned int *col_diff_g;
static unsigned int *col_diff_r;
static unsigned int *col_diff_b;
static unsigned int *col_diff_a;

void cgpro_init(void)
{
    int i = 0;
    col_diff_g = &col_diff[128 * 0];
    col_diff_r = &col_diff[128 * 1];
    col_diff_b = &col_diff[128 * 2];
    col_diff_a = &col_diff[128 * 3];

    for (i = 1; i < 64; i++) {
        int k = i * i;
        col_diff_g[i] = col_diff_g[128 - i] = k * 59 * 59;
        col_diff_r[i] = col_diff_r[128 - i] = k * 30 * 30;
        col_diff_b[i] = col_diff_b[128 - i] = k * 11 * 11;
        col_diff_a[i] = col_diff_a[128 - i] = k * 8 * 8;
    }
}

struct cgpro_palette cgpro_palette_load_default(void)
{
    /* dangerous const discard but whatever */
    struct cgpro_palette out = { (unsigned char *) default_palette, 32 };
    return out;
}

struct cgpro_palette cgpro_palette_load_from_file(const char *filename)
{
    struct cgpro_palette out = { 0 };
    unsigned int width, height, err;
    err = lodepng_decode24_file(&out.data, &width, &height, filename);
    if (err)
        return out;
    out.num = width * height;
    return out;
}

struct cgpro_color cgpro_palette_get_idx(struct cgpro_palette P, unsigned int idx)
{
    struct cgpro_color col = { 0 };
    unsigned char *p = &P.data[idx * 3];
    col.r = p[0];
    col.g = p[1];
    col.b = p[2];
    col.a = idx == 0 ? 0 : 255;
    return col;
}

/* based on old Aseprite bestfit func */
unsigned int cgpro_palette_nearest(struct cgpro_palette P, struct cgpro_color c)
{
    int r = c.r >> 3;
    int g = c.g >> 3;
    int b = c.b >> 3;
    int a = c.a >> 3;
    int i = 0, best_fit = 0, lowest = INT_MAX;

    if (c.a == 0) 
        return 0;

    for (i = 1; i < P.num; i++) {
        struct cgpro_color col = cgpro_palette_get_idx(P, i);
        int col_diff = col_diff_g[((col.g >> 3) - g) & 0x7F];
        if (col_diff < lowest) {
            col_diff += col_diff_r[((col.r >> 3) - r) & 0x7F];
            if (col_diff < lowest) {
                col_diff += col_diff_b[((col.b >> 3) - b) & 0x7F];
                if (col_diff < lowest) {
                    col_diff += col_diff_a[((col.a >> 3) - a) & 0x7F];
                    if (col_diff < lowest) {
                        if (col_diff == 0)
                            return i;
                        best_fit = i;
                        lowest = col_diff;
                    }
                }
            }
        }
    }
    return best_fit;
}

static int cgpro_color_distance(struct cgpro_color a, struct cgpro_color b)
{
    int result = 0;

    if (a.a && b.a) {
        result += abs(a.r - b.r) * 2126;
        result += abs(a.g - b.g) * 7152;
        result += abs(a.b - b.b) * 722;
    }
    result += (abs(a.a - b.a) * 20000);
    return result;
}

static unsigned char cgpro_fit(int x)
{
    int out = abs(x);
    return out > 255 ? 255 : out;
}

static int cgpro_bayer_value(unsigned int x, unsigned int y)
{
    return matrix8[(y & 0xF) * 8 + (x & 0xF)];
}

unsigned int cgpro_palette_bayer8x8(struct cgpro_palette P, struct cgpro_color c, unsigned int x, unsigned int y)
{
    int d, D, th;
    unsigned int idx1, idx2;
    struct cgpro_color c1, c2;

    if (c.a == 0)
        return 0;
    idx1 = cgpro_palette_nearest(P, c);
    c1 = cgpro_palette_get_idx(P, idx1);

    c2.r = cgpro_fit(c.r - (c1.r - c.r));
    c2.g = cgpro_fit(c.g - (c1.g - c.g));
    c2.b = cgpro_fit(c.b - (c1.b - c.b));
    c2.a = cgpro_fit(c.a - (c1.a - c.a));
    idx2 = cgpro_palette_nearest(P, c2);

    if (idx1 == idx2)
        return idx1;
    c2 = cgpro_palette_get_idx(P, idx2);
    d = cgpro_color_distance(c1, c);
    D = cgpro_color_distance(c1, c2);
    if (D == 0)
        return idx1;
    d = ((int) (64.0f * ((float) d / D)));
    th = cgpro_bayer_value(x, y);
    return d > th ? idx2 : idx1;
}

void cgpro_palette_free(struct cgpro_palette palette)
{
    if (palette.data == default_palette) return;
    free(palette.data);
}

/* TODO: more quantization methods */
int cgpro_quantize_to(struct cgapi_map *target, struct cgpro_palette palette)
{
    unsigned int i, j = 0;
    for (i = 0; i < target->width; i++) {
        for (j = 0; j < target->height; j++) {
            unsigned char *p = &target->data[(j * target->width + i) * 4];
            unsigned int idx;
            struct cgpro_color oldcol;
            struct cgpro_color newcol;
            oldcol.r = p[0];
            oldcol.g = p[1];
            oldcol.b = p[2];
            oldcol.a = p[3];
            idx = cgpro_palette_bayer8x8(palette, oldcol, i, j);
            newcol = cgpro_palette_get_idx(palette, idx);
            p[0] = newcol.r;
            p[1] = newcol.g;
            p[2] = newcol.b;
        }
    }
    return 1;
}

int cgpro_scale_nearest(struct cgapi_map *target, unsigned int new_width, unsigned int new_height)
{
    unsigned int old_width = target->width;
    unsigned int old_height = target->height;
    float width_ratio = old_width / (float) new_width;
    float height_ratio = old_height / (float) new_height;
    unsigned int i, j;
    unsigned char *new_data;
    unsigned char *old_data = target->data;

    if (!old_data)
        return 0;
    new_data = malloc(4 * new_width * new_height * sizeof(unsigned char));
    if (!new_data)
        return 0;

    for (i = 0; i < new_width; i++) {
        for (j = 0; j < new_height; j++) {
            unsigned int nearest_i = i * width_ratio;
            unsigned int nearest_j = j * height_ratio;
            unsigned char *new = &new_data[(j * new_width + i) * 4];
            unsigned char *old = &old_data[(nearest_j * old_width + nearest_i) * 4];
            new[0] = old[0]; /* r */
            new[1] = old[1]; /* g */
            new[2] = old[2]; /* b */
            new[3] = old[3]; /* a */
        }
    }

    free(old_data);
    target->data = new_data;
    target->width = new_width;
    target->height = new_height;
    return 4 * new_width * new_height * sizeof(unsigned char);
}
