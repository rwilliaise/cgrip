# cgrip

Tool for downloading material sets off of AmbientCG.

## Usage

```
Usage: cgrip [OPTIONS] ID...
cgrip 0.1.1
Downloads materials using ids ID... off of AmbientCG. If specific material maps
are not specified to be saved, cgrip downloads the albedo by default.

optional arguments
    -h, --help
        Show usage information.
    -o, --output OUTPUT
        Outputs processed image file to dir OUTPUT.
    -v, --verbose
        Turn on extra debug printing. Floods console.
    -q, --quality QUALITY
        Save materials of specific quality. options: 1K, 2K, 4K, 8K. default: 1K
    -a, --all
        Save all material maps found in the zips.
    -z, --zip [DIR]
        Save material zip file, optionally to dir DIR. default: OUTPUT
    -s, --downscale SIZE
        Downscale exported matmaps. format: WxH
    --quantize [PALETTE]
        Quantize with given palette or the default Aseprite palette.
    --macro SCALE
        When downscaling, multiply the size of non-albedo maps by this.
    --gen-godot4
        Generate Godot 4 materials alongside the textures.
    --nearest
        Generate any in-game materials to use nearest filtering instead of linear.
    --ao
        Save ambientocclusion matmap.
    -c, --color
        Save color/albedo matmap. True by default if nothing specified.
    -d, --disp
        Save displacement matmap.
    -e, --emission
        Save emission matmap
    -m, --metalness
        Save metalness matmap
    --opacity
        Save opacity matmap
    --apply-opacity
        Instead of saving opacity matmap, applies it to the colormap.
    -r, --roughness
        Save roughness matmap
    -n, --normal [TYPE]
        options: NONE, GL, DX, BOTH. unsupplied: NONE, otherwise default: GL
    --disable-color
        Force cgrip to not output terminal color. Fixes odd terminal output.
```

Download Marble001 to Marble001.png in the cwd, for example:
```
cgrip Marble001
```

Download Fabric076 and Fabric077 albedo, GL normal, and roughness maps to a
`fabric/` folder.
```
cgrip -ofabric -crngl Fabric076 Fabric077
```

Advanced usage: downscale the two from before and generate Godot 4 materials
alongside them.
```
cgrip -s256x256 -ofabric --nearest --gen-godot4 -cmrngl Fabric076 Fabric077
```

## Building

Only supports Linux as of now. Required packages are `libarchive`,
and `libcurl`.

On an Arch-based distribution (you probably already have these packages):
```
pacman -S curl libarchive
make
```
