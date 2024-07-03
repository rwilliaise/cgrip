
#ifndef CGAPI_H_
#define CGAPI_H_

#define CGAPI_MAPNUM 9

enum cgapi_quality {
    cgapi_quality_1k_png,
    cgapi_quality_2k_png,
    cgapi_quality_4k_png,
    cgapi_quality_8k_png
};

enum cgapi_matmap {
    cgapi_matmap_ambientocclusion,
    cgapi_matmap_color,
    cgapi_matmap_displacement,
    cgapi_matmap_emission,
    cgapi_matmap_metalness,
    cgapi_matmap_normaldx,
    cgapi_matmap_normalgl,
    cgapi_matmap_opacity,
    cgapi_matmap_roughness
};

struct cgapi_map {
    unsigned char *data;
    unsigned int width, height;
};

struct cgapi_material {
    char *id;
    enum cgapi_quality quality;
    struct cgapi_map maps[CGAPI_MAPNUM];
};

struct cgapi_materials {
    struct cgapi_material *materials;
    enum cgapi_quality quality;
    int material_count;
};

void cgapi_material_get_filename(struct cgapi_material *mat, enum cgapi_matmap map, char *buf, int bufsz);
void cgapi_material_save(struct cgapi_material *mat, const char *out);
void cgapi_materials_save(struct cgapi_materials *mats, const char *out);
struct cgapi_materials cgapi_download_ids(enum cgapi_quality quality, const char **ids, int id_count);
void cgapi_materials_free(struct cgapi_materials *mats);

#endif /* CGAPI_H_ */
