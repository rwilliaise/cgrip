# cgrip

Tool for downloading material sets off of AmbientCG.

## Usage

```
Usage: cgrip [OPTIONS] ID...
cgrip 0.1.0
Downloads materials using ids ID... off of AmbientCG.

optional arguments
   -h, --help
       Show usage information.
   -o, --output OUTPUT
       Outputs processed image file to OUTPUT.
   -v, --verbose
       Print some extra debug things.
   --no-color
       Forces cgrip to not print color codes.
```

Download Marble001 to Marble001.png, for example:
```
cgrip Marble001
```

## Building

Only supports Linux as of now. Required packages are `libarchive`,
and `libcurl`.

```
make
```
