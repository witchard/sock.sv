library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity adder is
  
  generic (
    width : positive := 1);

  port (
    clk   : in  std_ulogic;
    reset : in  std_ulogic;
    a     : in  std_ulogic_vector(width-1 downto 0);
    b     : in  std_ulogic_vector(width-1 downto 0);
    c     : out std_ulogic_vector(width downto 0));

end adder;

architecture rtl of adder is
  
begin  -- rtl
  
  -- purpose: Add the values
  -- type   : sequential
  -- inputs : clk, reset, a, b
  -- outputs: c
  add: process (clk, reset)
  begin  -- process add
    if reset = '1' then                 -- asynchronous reset (active high)
      c <= (others => '0');
    elsif clk'event and clk = '1' then  -- rising clock edge
      c <= std_ulogic_vector( unsigned('0' & a) + unsigned('0' & b) );
    end if;
  end process add;
  
end rtl;
