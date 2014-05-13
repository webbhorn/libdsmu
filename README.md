libdsmu
=======

Distributed Shared Memory in Userspace Library for Linux Applications. Modeled after the [Ivy](http://css.csail.mit.edu/6.824/2014/papers/li-dsm.pdf) system. Final project for [6.824](http://css.csail.mit.edu/6.824/2014/).

Authors
-------
- Webb Horn
- Tim Donegan
- Julian Gonzalez
- Ameesh Goyal


Setup
-----

libdsmu was tested on Ubuntu 14.04 x86/64 running the 3.13.0-24-generic kernel. Older kernels on a non-x86/64 machine probably will not work because the code that detects fault types is non-portable.

Third party libraries:

    $ sudo apt-get install libb64-dev


Build
-----

    $ cd src
    $ make

Running the manager
-----------------

    $ python manager/manager.py &

Running the Matrix Multiply Benchmark
-------------------------------------
matrixmultiply uses the alternation-style sharding algorithm for nodes-to-rows
mapping, while matrixmultiply2 allocates rows to nodes in contiguous chunks to
reduce the effects of page thrashing. They are both launched the same way.

First, start the manager:

    $ python manager/manager.py &

Next, start up instances of matrixultiply in parallel. To start three
instances, launch them like this:

    $ cd src
    $ make
    $ ./matrixmultiply2 127.0.0.1 4444 1 3 && \
      ./matrixmultiply2 127.0.0.1 4444 2 3 && \
      ./matrixmultiply2 127.0.0.1 4444 3 3:

