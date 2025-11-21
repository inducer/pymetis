# GKlib
A library of various helper routines and frameworks used by many of the lab's 
software

## Build requirements
 - CMake 3.10, found at http://www.cmake.org/, as well as GNU make. 

Assuming that the above are available, two commands should suffice to 
build the software:
```
make config 
make
```

## Configuring the build
It is primarily configured by passing options to make config. For example:
```
make config cc=icc
```

would configure it to be built using icc.

Configuration options are:
```
cc=[compiler]     - The C compiler to use [default: gcc]
prefix=[PATH]     - Set the installation prefix [default: ~/local]
openmp=set        - To build a version with OpenMP support
```


## Building and installing
To build and install, run the following
```
make
make install
```

By default, the library file, header file, and binaries will be installed in
```
~/local/lib
~/local/include
~/local/bin
```

## Other make commands
    make uninstall 
         Removes all files installed by 'make install'.
   
    make clean 
         Removes all object files but retains the configuration options.
   
    make distclean 
         Performs clean and completely removes the build directory.


