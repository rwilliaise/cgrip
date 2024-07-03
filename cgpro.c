
#include <stdlib.h>

#include "cgpro.h"

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
