sock.sv
=======

A simple socket library for system verilog.

Using the system verilog DPI, allows the user to read / write lines over a socket. Several types are supported:

* TCP Socket
* Pipe - Execute a program and read / write lines to its standard input / output

See sock.sv for the function definitions and example usage.

Todo
----

* Test on windows (not fully tested yet!)
* Maybe add support for unix domain sockets / named pipes
