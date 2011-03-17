--	Package File Template
--
--	Purpose: This package defines supplemental types, subtypes, 
--		 constants, and functions 


library IEEE;
use IEEE.STD_LOGIC_1164.all;

package scs_2001_pack is

-- Declare constants

  constant const_no_ports   : integer := 8;
  
  constant AM_READ_PORT     : integer := 0;
  constant AM_READ_REG      : integer := 1;
  constant AM_WRITE_PORT    : integer := 2;
  constant AM_WRITE_DIR     : integer := 3;
  constant AM_READ_CSR      : integer := 4;
  constant AM_WRITE_CSR     : integer := 5;
  constant AM_RW_SERIAL     : integer := 6;
  constant AM_RW_EEPROM     : integer := 7;
  constant AM_RW_MONITOR    : integer := 8;
  
-- Declare types

  type type_port_rec is 
    record
        port_pin  : std_logic_vector(7 downto 0);
        cs        : std_logic;
        cs_eeprom : std_logic;
    end record;

  type type_port_array is array (const_no_ports-1 downto 0) of type_port_rec;

end scs_2001_pack;
