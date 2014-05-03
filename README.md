libdsmu
=======

Distributed Shared Memory in Userspace Library for Linux Applications


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

Starting up a program instance
------------------------------

    $ cd src
    $ ./main
