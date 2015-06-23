package sock;

// Tested on modelsim altera starter edition for linux
//
// Example compile command:
//    vlog example.sv sock.sv sock.c
//
// Example run command (windows):
//    vsim example -ldflags -lws2_32
//
// Example run command (linux):
//    vsim example

// Init the module - this should be called once at startup
// Returns 1 for OK, 0 for error
import "DPI-C" function int sock_init();

// Shutdown the module - call once at the end following closing any sockets
import "DPI-C" function void sock_shutdown();

// Open a connection to an endpoint
//   uri - Where to connect:
//           tcp://hostname:port - TCP Socket.
//           unix://socketname   - Unix Domain Socket (linux only).
//                                 Prefix with @ for abstract namespace.
// Returns a handle to be used with other functions, or null on error
import "DPI-C" function chandle sock_open(input string uri);

// Close a connection to an endpoint
//   handle - Handle returned from sock_open
import "DPI-C" function void sock_close(input chandle handle);

// Write a line to handle - automatically adds new line
// (note does not add carridge return)
//   handle - Handle returned from sock_open
//   data - String to write (note max size is 1MB currently)
// Returns 1 on success, 0 on error
import "DPI-C" function int sock_writeln(input chandle handle, input string data);

// Read a line from handle - removes newline
// (note does not remove carridge return if present)
//   handle - Handle returned from sock_open
// Returns the line, or empty string on error / EOF
// Note: we have a 1MB buffer, lines longer than this will be split up
import "DPI-C" function string sock_readln(input chandle handle);

// Write a line to handle and then read response
function string sock_w_r(input chandle handle, input string data);
   if(sock_writeln(handle, data) == 1)
     return sock_readln(handle);
   return "";
endfunction

endpackage