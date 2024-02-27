# Jewels SDL texture loading test

This code shows a simple SDL2+FreeType+HarfBuzz example which runs on PSP, VITA and Linux. It uses texture rendering to draw text using TTF fonts. This makes it easier to test if there are any issues with SDL2, FreeType or HarfBuzz for PSP.

![screenshot](screenshot.png?raw=true)

## Authors

I mostly copied the code from these repositories:

- [anoek/ex-sdl-cairo-freetype-harfbuzz](https://github.com/anoek/ex-sdl-cairo-freetype-harfbuzz)
- [lxnt/ex-sdl-freetype-harfbuzz](https://github.com/lxnt/ex-sdl-freetype-harfbuzz)
- [GerHobbelt/ex-leptonica-freetype-harfbuzz](https://github.com/GerHobbelt/ex-leptonica-freetype-harfbuzz)

## Building

### PSP

This one requires upstream SDL2.

```
mkdir psp && cd psp
psp-cmake ..
make
```

### VITA

```
mkdir vita && cd vita
cmake -DCMAKE_TOOLCHAIN_FILE="${VITASDK}/share/vita.toolchain.cmake" ..
make
```

### PC

```
mkdir build && cd build
cmake ..
make
```
