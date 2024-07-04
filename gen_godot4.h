
#ifndef GEN_GODOT4_H_
#define GEN_GODOT4_H_

#include "cgapi.h"

void gen_godot4_set_root(const char *path);
int gen_godot4_generate(struct cgapi_material *mat, const char *out);

#endif /* GEN_GODOT4_H_ */
