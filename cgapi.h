
#ifndef CGAPI_H_
#define CGAPI_H_

enum cgapi_quality {
    cgapi_quality_1k_png,
    cgapi_quality_2k_png
};

enum cgapi_matmap {
    cgapi_matmap_color,
    cgapi_matmap_displacement,
    cgapi_matmap_normaldx,
    cgapi_matmap_normalgl,
    cgapi_matmap_opacity,
    cgapi_matmap_roughness
};

struct cgapi_map {
    unsigned char *data;
    int width, height;
};

struct cgapi_material {
    char *id;
    enum cgapi_quality quality;
    struct cgapi_map maps[6];
};

struct cgapi_materials {
    struct cgapi_material *materials;
    int material_count;
};

struct cgapi_materials cgapi_download_ids(enum cgapi_quality quality, const char **ids, int id_count);

#endif // CGAPI_H_
