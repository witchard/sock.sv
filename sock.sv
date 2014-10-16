module sock();

// Tested on modelsim altera 14.0 for windows and linux
//
// Example compile command:
//    vlog sock.sv sock.c
//
// Example run command (windows):
//    vsim sock -ldflags -lws2_32
//
// Example run command (linux):
//    vsim sock

// Init the module - this should be called once at startup
// Returns 1 for OK, 0 for error
import "DPI-C" function int sock_init();

// Shutdown the module - call once at the end following closing any sockets
import "DPI-C" function void sock_shutdown();

// Open a connection to an endpoint
//   uri - Where to connect:
//           tcp://hostname:port - TCP Socket
// Returns a handle to be used with other functions, or null on error
import "DPI-C" function chandle sock_open(input string uri);

// Close a connection to an endpoint
//   handle - Handle returned from sock_open
import "DPI-C" function void sock_close(input chandle handle);

// Write a line to handle - automatically adds new line
// (note does not add carridge return)
//   handle - Handle returned from sock_open
//   data - String to write (note max size is 1k currently)
// Returns 1 on success, 0 on error
import "DPI-C" function int sock_writeln(input chandle handle, input string data);

// Read a line from handle - does not remove new line
//   handle - Handle returned from sock_open
// Returns the line, or empty string on error / EOF
// Note: we have a 1k buffer, lines longer than this will be split up
import "DPI-C" function string sock_readln(input chandle handle);



// Example usage - sends / receives some data to/from localhost port 1234
initial begin
	chandle h;
	
	// Init
	if(sock_init() < 0) begin
		$error("Aww shucks couldn't init the library");
		$stop();
	end 

	// Connect
	h = sock_open("tcp://localhost:1234");
	if(h == null) begin
		$error("Aww shucks couldn't connect");
		sock_shutdown();
		$stop();
	end 

	// Send / receive
	/*
	if(!sock_writeln(h, "Howdy partner!"))
		$error("Darn it the write failed");
	$write(sock_readln(h));*/

	// Done
	sock_close(h);
	sock_shutdown();
end

endmodule
