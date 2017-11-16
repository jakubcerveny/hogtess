# hogtess

Experimental visualization of higher order finite elements using OpenGL 4.0 GPU
tesselation.

![hogtess](https://raw.githubusercontent.com/jakubcerveny/hogtess/master/data/screenshot.png)

(WORK IN PROGRESS)

### Compiling

hogtess needs CMake, Qt4, and GLM:
```
$ sudo apt-get install cmake libqt4-dev libglm-dev 
```

### Troubleshooting

On some Linux systems, OpenGL 4.0 may not be enabled by default. Try running
the program with the following environment variable:

```
MESA_GL_VERSION_OVERRIDE=4.0 build/hogtess
```
