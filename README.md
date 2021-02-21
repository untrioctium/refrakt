# refrakt
Animated fractal flames with OpenGL Compute Shader backend.

## Building and Running
Building is done using `cmake` on all platforms. Windows should not require any additional dependencies, but Linux platforms need to install `xorg-dev`
or whatever package provides X11 headers. All dependencies are currently pulled from repositories, built, and statically linked, but this approach will likely
change as `ffmpeg` libraries are introduced to directly encode renders.

Building and running on Linux:
```bash
mkdir build
cd build
cmake ..
make
mv refrakt ..
cd ..
./refrakt
```

Window users can similarly run `cmake ..` in a `build` directory and then load `refract.sln` in Visual Studio for development and debugging.

Flames are loaded from the `flames/` directory. Only one is currently provided from the repo, but you can provide any others in this directory to be loaded on launch.
Not all variations and parameters are implemented yet, so not all flames will load. The current goal is structural accuracy with the flam3 renderer, 
though colors and density estimation may be a bit off.
