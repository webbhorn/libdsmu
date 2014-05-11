libdsmu
=======

Distributed Shared Memory in Userspace Library for Linux Applications

Authors
-------
- Webb Horn
- Tim Donegan
- Julian Gonzalez
- Ameesh Goyal


Setup
-----
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

