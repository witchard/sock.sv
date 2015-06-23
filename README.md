sock.sv
=======

A simple socket library for system verilog. Using the system verilog DPI, allows the user to read / write lines from a socket connection.

See sock.sv for the function definitions and example usage.

The windows branch of this code is the last time I tested it on windows... If you intend to use windows please let me know if master works still!

Todo
----

* Add support for windows named pipes


Quick Start
-----------

To get up and running with Modelsim Altera Starter edition on CentOS 6, first install the following additional packages:

* glibc.i686
* libXft.i686
* libXext.i686
* ncurses-libs.i686
* glibc-devel.i686
* zlib.i686
* libstdc++.i686

Then start up the test server - ```python test/tcp_server.py```.

Finally, spin up vsim (/opt/altera/15.0/modelsim_ase/bin/vsim in my-case), and run the following in the vsim shell to test it out:

* ```vlib work```
* ```vlog example.sv sock.sv sock.c```
* ```vsim example```
* ```run -all```

If it all worked, it will say "Howdy Partner!"

To experiment with unix domain sockets, comment out line 58 of sock.sv and comment in line 59. Then use with ```python test/unix_server.py```.
