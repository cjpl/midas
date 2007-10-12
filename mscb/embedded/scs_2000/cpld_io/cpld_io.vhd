library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity cpld_io is
    Port ( P_I_CS1         : in std_logic;
           P_I_SDI         : in std_logic;
           P_I_SCLK        : in std_logic;
           P_IO_SDO        : inout std_logic;
           P_IO_INTERN     : inout std_logic_vector(7 downto 0);
           P_O_1A          : out std_logic_vector(7 downto 0);
           P_I_2A          : in std_logic_vector(7 downto 0);
           P_O_CLKSEL      : out std_logic_vector(3 downto 0);
           P_I_CLK         : in std_logic;
           P_O_DUMMY       : out std_logic
         );
end cpld_io;

architecture Behavioral of cpld_io is

begin

  -- quarz speed in MHz:
  -- 0:100, 1:33.33, 2:30, 3:120, 4:25, 5:20, 6:70, 7:80
  -- 8:75, 9:66.66, 10:60, 11:60, 12: 50, 13:45, 14:90, 15:40 
  P_O_CLKSEL  <= "0000";
  
  -- serial bus
  P_IO_SDO <= 'Z';
  P_O_DUMMY <= P_IO_SDO xor P_I_CS1 xor P_I_SDI xor P_I_SCLK;
  
  -- simply send input to central CPLD
  P_IO_INTERN(7 downto 0) <= P_I_2A(7 downto 0);
  P_O_1A <= "00000000"; 
  
end Behavioral;
