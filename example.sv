import sock::*;
    
module example();
   
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
	//h = sock_open("unix://@foobar");
	if(h == null) begin
		$error("Aww shucks couldn't connect");
		sock_shutdown();
		$stop();
	end 

	// Send / receive
	if(!sock_writeln(h, "Howdy partner!")) begin
		$error("Darn it the write failed");
		sock_shutdown();
		$stop();
	end
	$display(sock_readln(h));

	// Done
	sock_close(h);
	sock_shutdown();
end

endmodule