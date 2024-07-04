
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>

#include "cgrip.h"
#include "cgapi.h"
#include "cgpro.h"

struct arguments arguments = { 0 };

struct usage {
    char *option;
    char *description;
} usages[] = {
    { "-h, --help", "Show usage information." },
    { "-o, --output OUTPUT", "Outputs processed image file to dir OUTPUT." },
    { "-v, --verbose", "Turn on extra debug printing. Floods console." },
    { "-q, --quality QUALITY", "Save materials of specific quality. options: 1K, 2K, 4K, 8K. default: 1K" },
    { "-a, --all", "Save all material maps found in the zips." },
    { "-z, --zip [DIR]", "Save material zip file, optionally to dir DIR. default: OUTPUT" },
    { "-s, --downscale SIZE", "Downscale exported matmaps. format: WxH" },
    { "--quantize [PALETTE]", "Quantize with given palette or the default Aseprite palette." },
    { "--macro SCALE", "When downscaling, multiply the size of non-albedo maps by this." },
    { "--ao", "Save ambientocclusion matmap." },
    { "-c, --color", "Save color/albedo matmap. True by default if nothing specified." },
    { "-d, --disp", "Save displacement matmap." },
    { "-e, --emission", "Save emission matmap" },
    { "-m, --metalness", "Save metalness matmap" },
    { "--opacity", "Save opacity matmap" },
    { "-r, --roughness", "Save roughness matmap" },
    { "-n, --normal [TYPE]", "options: NONE, GL, DX, BOTH. unsupplied: NONE, otherwise default: GL" },
    { "--disable-color", "Force cgrip to not output terminal color. Fixes odd terminal output." },
    { 0 },
};

const char *usage_text =
    "Usage: cgrip [OPTIONS] ID...\n"
    "cgrip " CGRIP_VERSION
    "\n"
    "Downloads materials using ids ID... off of AmbientCG. If specific material maps\n"
    "are not specified to be saved, cgrip downloads the albedo by default.\n"
    "\n"
    "optional arguments";

void verbose(const char *fmt, ...)
{
    va_list args;
    if (!arguments.verbose)
        return;
    
    putcolor(CGRIP_CYAN);
    fputs("[VERBOSE] ", stdout);
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    putcolor(CGRIP_RESET);
}

void warn(const char *fmt, ...)
{
    va_list args;
    putcolor(CGRIP_YELLOW);
    fputs("[WARN] ", stdout);
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    putcolor(CGRIP_RESET);
}

CGRIP_NORETURN void fatal(const char *fmt, ...)
{
    va_list args;
    fputcolor(CGRIP_RED, stderr);
    fputs("[FATAL] ", stderr);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputcolor(CGRIP_RESET, stderr);
    exit(EXIT_FAILURE);
}

#ifdef CGRIP_TERMCOLOR
void putcolor(const char *col)
{
    if (!arguments.use_term_colors) return;
    fputs(col, stdout);
}

void fputcolor(const char *col, FILE *fp)
{
    if (!arguments.use_term_colors) return;
    fputs(col, fp);
}
#endif

static void usage(int status)
{
    struct usage *u;
    puts(usage_text);
    for (u = usages; u->description; u++) {
        printf("    %s\n", u->option);
        printf("        %s\n", u->description);
    }
    exit(status);
}

/* TODO: cross-platform */
static int is_directory(const char *path)
{
    struct stat s;
    if (stat(path, &s) == 0) {
        return S_ISDIR(s.st_mode);
    }
    return 0;
}

static const char *quality_types[] = {
    "1k",
    "2k",
    "4k",
    "8k",
};

static enum cgapi_quality get_quality_type(char *arg)
{
    char *p;
    enum cgapi_quality i;
    for (p = arg; *p; p++) *p = tolower(*p);
    for (i = 0; i < 4; i++)
        if (!strcmp(quality_types[i], arg)) return i;
    warn("got unexpected --quality argument %s, defaulting to 1k\n", arg);
    return cgapi_quality_1k_png;
}

static const char *save_normal_types[] = {
    "none",
    "gl",
    "dx",
    "both",
};

static enum cgrip_normal_type get_normal_type(char *arg)
{
    char *p;
    enum cgrip_normal_type i;
    if (arg == NULL) return cgrip_normal_type_gl;
    for (p = arg; *p; p++) *p = tolower(*p);
    for (i = 0; i < 4; i++)
        if (!strcmp(save_normal_types[i], arg)) return i;
    warn("got unexpected --save-normal argument %s, defaulting to gl\n", arg);
    return cgrip_normal_type_gl;
}

int main(int argc, char *argv[])
{
    int opt, pargc, i;
    const char *short_opts = ":ho:vq:as:z::demrcn::";
    struct option long_opts[] = {
        { "help", no_argument, NULL, 'h' },
        { "output", required_argument, NULL, 'o' },
        { "zip", optional_argument, NULL, 'z' },
        { "verbose", no_argument, NULL, 'v' },
        { "disable-color", no_argument, NULL, 'D' },
        { "quality", required_argument, NULL, 'q' },
        { "downscale", required_argument, NULL, 's' },
        { "macro", required_argument, NULL, 'M' },
        { "quantize", optional_argument, NULL, 'Q' },
        
        { "all", no_argument, NULL, 'a' },
        { "ao", no_argument, NULL, 'A' },
        { "disp", no_argument, NULL, 'd' },
        { "emission", no_argument, NULL, 'e' },
        { "metalness", no_argument, NULL, 'm' },
        { "opacity", no_argument, NULL, 'O' },
        { "roughness", no_argument, NULL, 'r' },
        { "color", no_argument, NULL, 'c' },
        { "normal", required_argument, NULL, 'n' },
        { 0 },
    };
    struct cgapi_materials mats = { 0 };
    struct cgpro_palette palette;
    enum cgapi_quality quality = cgapi_quality_1k_png;
    char *endptr;

    cgpro_init();

    arguments.macro_scale = 0;
#ifdef CGRIP_TERMCOLOR
    arguments.use_term_colors = getenv("TERM") != NULL;
#endif

    while (1) {
        opt = getopt_long(argc, argv, short_opts,long_opts, NULL);
        if (opt == -1)
            break;

        switch (opt) {
        case 'h': /* --help */
            usage(EXIT_SUCCESS);
            break;
        case 'o': /* --output */
            if (!is_directory(optarg))
                fatal("%s is not valid output directory\n", optarg);
            arguments.output = optarg;
            break;
        case 'v': /* --verbose */
            arguments.verbose = 1;
            break;
        case 'q': /* --quality */
            quality = get_quality_type(optarg);
            break;
        case 'z': /* --zip */
            arguments.save_zip = 1;
            arguments.output_zip = optarg;
            break;
        case 's': /* --downscale */
            arguments.downscale_width = strtol(optarg, &endptr, 10);
            if (*endptr != 'x')
                fatal("incorrect --downscale SIZE format, expected WxH\n");
            arguments.downscale_height = strtol(++endptr, &endptr, 10);
            verbose("target downscale: %dx%d\n", arguments.downscale_width, arguments.downscale_height);
            if (arguments.downscale_width * arguments.downscale_height == 0)
                warn("invalid downscale SIZE, ignoring\n");
            else
                arguments.downscale = 1;
            break;
        case 'M': /* --macro */
            arguments.macro_scale = strtol(optarg, &endptr, 10);
            verbose("using macro_scale %u\n", arguments.macro_scale);
            if (*endptr && *endptr != 'x')
                warn("ignored garbage characters for --macro: %s\n", endptr);
            break;
        case 'Q': /* --quantize */
            arguments.quantize = 1;
            if (!optarg)
                palette = cgpro_palette_load_default();
            else
                palette = cgpro_palette_load_from_file(optarg);
            break;

        case 'a': /* --all */
            arguments.save_ambientocclusion = 1;
            arguments.save_color = 1;
            arguments.save_displacement = 1;
            arguments.save_emission = 1;
            arguments.save_metalness = 1;
            arguments.save_opacity = 1;
            arguments.save_roughness = 1;
            if (arguments.save_normal == cgrip_normal_type_none)
                arguments.save_normal = cgrip_normal_type_both;
            break;
        case 'A': /* --ao */
            arguments.save_ambientocclusion = 1;
            break;
        case 'c': /* --color */
            arguments.save_color = 1;
            break;
        case 'd': /* --disp */
            arguments.save_displacement = 1;
            break;
        case 'e': /* --emission */
            arguments.save_emission = 1;
            break;
        case 'm': /* --metalness */
            arguments.save_metalness = 1;
            break;
        case 'O': /* --opacity */
            arguments.save_opacity = 1;
            break;
        case 'r': /* --roughness */
            arguments.save_roughness = 1;
            break;
        case 'n': /* --normal */
            arguments.save_normal = get_normal_type(optarg);
            break;
#ifdef CGRIP_TERMCOLOR
        case 'D':
            arguments.use_term_colors = 0;
            break;
#endif
        case ':':
            fatal("missing argument for option '%c'\n", optopt);
            break;
        case '?':
        default:
            fatal("unknown option '%c'\n", optopt);
            break;
        }
    }

    if (
        !arguments.save_ambientocclusion
        && !arguments.save_color
        && !arguments.save_displacement
        && !arguments.save_emission
        && !arguments.save_metalness
        && !arguments.save_normal 
        && !arguments.save_opacity 
        && !arguments.save_roughness
    ) {
        arguments.save_color = 1;
        verbose("no matmaps specified, saving albedo\n");
    }

    pargc = argc - optind;
    if (pargc < 1)
        usage(EXIT_FAILURE);

    mats = cgapi_download_ids(quality, (const char **) &argv[optind], pargc);

    if (arguments.quantize && palette.data)
        for (i = 0; i < mats.material_count; i++) {
            struct cgapi_material *mat = &mats.materials[i];
            struct cgapi_map *color = &mat->maps[cgapi_matmap_color];
            if (color->data) {
                verbose("quantizing %s\n", mat->id);
                cgpro_quantize_to(color, palette);
            }
        }

    if (arguments.downscale)
        for (i = 0; i < mats.material_count; i++) {
            struct cgapi_material *mat = &mats.materials[i];
            int j;
            for (j = 0; j < CGAPI_MAPNUM; j++) {
                struct cgapi_map *map = &mat->maps[j];
                unsigned int width = arguments.downscale_width, height = arguments.downscale_height;
                if (j != cgapi_matmap_color && arguments.macro_scale > 0) {
                    width *= arguments.macro_scale;
                    height *= arguments.macro_scale;
                }
                if (map->data && !cgpro_scale_nearest(map, width, height))
                    warn("failed to scale %d for matmap %s\n", j, mat->id);
            }
        }

    cgapi_materials_save(&mats, arguments.output);
    cgapi_materials_free(&mats);

    return 0;
}
