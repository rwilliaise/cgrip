
#ifndef CGPRO_H_
#define CGPRO_H_

#include "cgapi.h"

struct cgpro_color {
    unsigned char r, g, b, a;
};

struct cgpro_palette {
    unsigned char *data;
    unsigned int num;
};

void cgpro_init(void);

struct cgpro_palette cgpro_palette_load_default(void);
struct cgpro_palette cgpro_palette_load_from_file(const char *filename);
struct cgpro_color cgpro_palette_get_idx(struct cgpro_palette P, unsigned int idx);
unsigned int cgpro_palette_nearest(struct cgpro_palette P, struct cgpro_color c);
unsigned int cgpro_palette_bayer8x8(struct cgpro_palette P, struct cgpro_color c, unsigned int x, unsigned int y);
void cgpro_palette_free(struct cgpro_palette palette);

int cgpro_quantize_to(struct cgapi_map *target, struct cgpro_palette palette);
int cgpro_scale_nearest(struct cgapi_map *target, unsigned int new_width, unsigned int new_height);

#endif /* CGPRO_H_ */
