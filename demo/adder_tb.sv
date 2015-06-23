import sock::*;

module adder_tb;
   timeunit 1ns;

   reg clk = 0, reset = 1;
   
   reg [9:0] a;
   reg [9:0] b;
   wire [10:0] c;
   int 	       num;
   

   // Clock generator
   always begin
      #5 clk = 1;
      #5 clk = 0;
   end

   // Reset
   initial begin
      #27 reset = 0;
   end
   
   // Test stimulus
   initial begin
      // Init
      chandle h;
      string  gumf;
      int     iter;
      
      if(sock_init() < 0) begin
	 $error("Aww shucks couldn't init the library");
	 $stop();
      end 
      //h = sock_open("unix://@adder-test");
      h = sock_open("tcp://localhost:1234");
      if(h == null) begin
	 $error("Aww shucks couldn't connect");
	 sock_shutdown();
	 $stop();
      end
            
      @(negedge reset);
      @(negedge clk);
      gumf = sock_w_r(h, "stim");
      num = $sscanf(gumf, "%d %d", a, b);

      repeat(1000) begin
	 @(negedge clk);
	 gumf = sock_w_r(h, $sformatf("check %d", c));
	 if(gumf != "ok") begin
	    sock_close(h);
	    sock_shutdown();
	    $error(gumf);
	    $stop();
	 end
	 else begin
	    gumf = sock_w_r(h, "stim");
	    num = $sscanf(gumf, "%d %d", a, b);
	 end
	 iter++;
      end
      
      sock_close(h);
      sock_shutdown();
      
   end
   
   // Instance the device-under-test
   adder #(10) dut(clk, reset, a, b, c);

endmodule
