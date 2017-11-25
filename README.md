# hogtess

Experimental visualization of higher order finite elements using OpenGL 4.0 GPU
tesselation.

![hogtess](https://raw.githubusercontent.com/jakubcerveny/hogtess/master/data/screenshot.png)

(WORK IN PROGRESS)

### Compiling

hogtess shows [MFEM](https://github.com/mfem/mfem) solutions. It also needs CMake, Qt4, and GLM:
```
$ sudo apt-get install cmake libqt4-dev libglm-dev 
```

Configure, compile and run:
```
$ cd hogtess; mkdir build; cd build
$ cmake ..
$ cd ..
$ make -C build
$ ./build/hogtess data/mesh.4 data/sln.4
```

### Troubleshooting

On some Linux systems, OpenGL 4.0 may not be enabled by default. Try running
the program with the following environment variable:

```
$ MESA_GL_VERSION_OVERRIDE=4.0 ./build/hogtess
```
