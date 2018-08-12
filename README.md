# hogtess

Experimental visualization of high order finite elements using OpenGL 4.3
compute shaders.

![hogtess](https://raw.githubusercontent.com/jakubcerveny/hogtess/compute-shaders/data/screenshot.png)

(WORK IN PROGRESS)

### Compiling

hogtess shows [MFEM](https://github.com/mfem/mfem) solutions.
It also needs CMake, Qt4, and GLM:
```
$ sudo apt install cmake libqt4-dev libglm-dev
```

Configure, compile and run:
```
$ mkdir build; cd build
$ cmake ..
$ make
$ cd ..
$ ./build/hogtess data/beam.2.mesh data/beam.2.sln
```

### Troubleshooting

On some Linux systems, OpenGL 4.3 may not be enabled by default. Try running
with the following environment variable:

```
$ MESA_GL_VERSION_OVERRIDE=4.3 ./build/hogtess
```
