# Jewels SDL texture loading test

This code shows a simple SDL2+theora+vorbis example which runs on PSP, VITA and Linux. It uses texture rendering to draw video frames and also plays audio. This makes it easier to test if there are any issues with SDL2, Theora or Vorbis for PSP.

![screenshot](screenshot.png?raw=true)

## Authors

I mostly copied the code from these sources:

- [theoraplay and its examples by icculus](https://github.com/icculus/theoraplay/blob/main/test/simplesdl.c)
- [Glusoft: Playing a theora video with SDL](https://glusoft.com/sdl2-tutorials/play-video-sdl/)
- [Big Buck Bunny 480p -ss 103.9 -t 8.15](https://download.blender.org/peach/bigbuckbunny_movies/) (c) copyright Blender Foundation | www.bigbuckbunny.org

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
