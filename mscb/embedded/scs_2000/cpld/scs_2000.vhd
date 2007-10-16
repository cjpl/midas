library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

use work.scs_2000_pack.ALL;

entity scs_2000 is
    Port ( P_I_UC_CLK      : in std_logic;
           P_I_UC_ALE      : in std_logic;
           P_I_UC_STROBE   : in std_logic;
           P_I_UC_DATA_IN  : in std_logic;
           P_O_UC_DATA_OUT : out std_logic;
           P_O_UC_STAT     : out std_logic;
           P_O_UC_SPARE1   : out std_logic;
           P_O_UC_SPARE2   : out std_logic;
           P_O_SDI         : out std_logic;
           P_I_SDO         : in std_logic;
           P_O_SCLK        : out std_logic;
           P_O_CLKSEL      : out std_logic_vector(3 downto 0);
           P_I_CLK         : in std_logic;
           P_I_ADDR        : in std_logic_vector(3 downto 0);
           P_IO_EXT        : inout std_logic_vector(7 downto 0);
           P_IO_PORTS      : inout type_port_array;
        	 P_I_5V_OK       : in std_logic;
           P_O_BEEPER      : out std_logic;
           P_O_24V_EN      : out std_logic;
           P_I_24V_OK      : in std_logic
         );
end scs_2000;

architecture Behavioral of scs_2000 is

constant firmware_version : std_logic_vector(3 downto 0) := X"1";

-- uC signals, either directly from uC or from extension bus
signal i_uc_clk       : std_logic;
signal i_uc_ale       : std_logic;
signal i_uc_strobe    : std_logic;
signal i_uc_data_in   : std_logic;

signal ser_reg_in     : std_logic_vector(9 downto 0);
signal ser_reg_out    : std_logic_vector(7 downto 0);
signal o_uc_data_out  : std_logic;

signal addr_unit    : std_logic_vector(3 downto 0);
signal addr_port    : std_logic_vector(2 downto 0);
signal my_addr      : std_logic_vector(3 downto 0);
signal master       : std_logic;

-- address modifiers:
-- 0  000  AM_READ_PORT
-- 1  001  AM_READ_REG
-- 2  010  AM_WRITE_PORT
-- 3  011  AM_READ_CSR
--   0       port_dir
--   1       pwr_status
-- 4  100  AM_WRITE_CSR
--   0       port_dir
--   1       pwr_status
-- 5  101  AM_RW_SERIAL
-- 6  110  AM_RW_EEPROM
signal addr_mod     : std_logic_vector(2 downto 0);

type type_port_reg is array (const_no_ports-1 downto 0) of std_logic_vector(7 downto 0);
signal port_reg     : type_port_reg;

-- '1' is ouput, '0' is input
signal port_dir     : std_logic_vector(7 downto 0) := "00000000";                                        

-- bit0: 5V ok       readonly
-- bit1: 24V ok      readonly
-- bit2: 24V enable  read/write
-- bit3: beeper      read/write
signal pwr_status   : std_logic_vector(3 downto 0) := "0000";

begin
    
  ------------------------------------------------------  
  -- read address switch and handle master/slave mode --
  ------------------------------------------------------

  -- address switch
  my_addr    <= not P_I_ADDR; -- inverse logic  

  -- address zero means master mode
  master     <= '1' when my_addr = "0000" else '0';
  
  -- get CPU signals from uC port (master) or from extension port (slave)
  i_uc_clk     <= P_I_UC_CLK     when master = '1' else P_IO_EXT(0);
  i_uc_ale     <= P_I_UC_ALE     when master = '1' else P_IO_EXT(1);
  i_uc_strobe  <= P_I_UC_STROBE  when master = '1' else P_IO_EXT(2);
  i_uc_data_in <= P_I_UC_DATA_IN when master = '1' else P_IO_EXT(3); 
  
  P_O_UC_DATA_OUT <= '1' when master = '0'
                     else P_IO_EXT(4) when 
                     addr_unit /= my_addr
                     else o_uc_data_out when
                     addr_mod /= "101" and addr_mod /= "110"
                     else P_I_SDO;
  P_O_UC_STAT     <= P_IO_PORTS(CONV_INTEGER(addr_port)).port_pin(0) when
                     addr_unit = my_addr
                     else P_IO_EXT(5); -- get status from slave bus                   
  P_O_UC_SPARE1   <= '1';
  P_O_UC_SPARE2   <= '1';
    
  -------------------  
  -- extension bus --
  -------------------
  
  -- copy all CPU outputs to bus in master mode, tristate them in slave mode
  P_IO_EXT(0)     <= P_I_UC_CLK     when master = '1' else 'Z';     
  P_IO_EXT(1)     <= P_I_UC_ALE     when master = '1' else 'Z';
  P_IO_EXT(2)     <= P_I_UC_STROBE  when master = '1' else 'Z';
  P_IO_EXT(3)     <= P_I_UC_DATA_IN when master = '1' else 'Z';

  -- output o_uc_data_out and STAT in slave mode when addressed
  P_IO_EXT(4)     <= o_uc_data_out when
                     master = '0' and addr_unit = my_addr and addr_mod /= "101" and addr_mod /= "110" else P_I_SDO when
                     master = '0' and addr_unit = my_addr else 'Z';
  P_IO_EXT(5)     <= P_IO_PORTS(CONV_INTEGER(addr_port)).port_pin(0) when
                     master = '0' and addr_unit = my_addr else 'Z';
  
  P_IO_EXT(6)     <= '0' when master = '1' else 'Z';
  P_IO_EXT(7)     <= '0' when master = '1' else 'Z';
  
  
  -----------------------------------------
  -- handle serial communication with uC --
  -----------------------------------------
  
  process(i_uc_clk)
  begin
    if rising_edge(i_uc_clk) then
      
      if i_uc_ale = '1' then
      
        if (i_uc_strobe = '1') then
          -- upon strobe, copy serial register
          addr_unit   <= ser_reg_in(9 downto 6);
          addr_port   <= ser_reg_in(5 downto 3);
          addr_mod    <= ser_reg_in(2 downto 0);
        else
          -- address cycle
          ser_reg_in(9 downto 0) <= ser_reg_in(8 downto 0) & i_uc_data_in;
        end if;
      
      else -- data cycle if ALE = '0'
      
        if (i_uc_strobe = '1' and addr_unit = my_addr) then
          if (addr_mod = "000") then
            -- read from port
            ser_reg_out <= P_IO_PORTS(CONV_INTEGER(addr_port)).port_pin;
          elsif (addr_mod = "001") then
            -- read from register
            ser_reg_out <= port_reg(CONV_INTEGER(addr_port));
          elsif (addr_mod = "010") then
            -- write to port
            port_reg(CONV_INTEGER(addr_port)) <= ser_reg_in(7 downto 0);
          elsif (addr_mod = "011") then
            -- read CSR
            if (addr_port = "000") then     -- 0
               ser_reg_out <= port_dir;
            elsif (addr_port = "001") then  -- 1
               ser_reg_out(7 downto 4) <= firmware_version;
               ser_reg_out(3 downto 0) <= pwr_status;    
            end if;   
          elsif (addr_mod = "100") then
            -- write CSR
            if (addr_port = "000") then     -- 0
               port_dir <= ser_reg_in(7 downto 0);
            elsif (addr_port = "001") then  -- 1
               pwr_status(2) <= ser_reg_in(2); -- 24V enable
               pwr_status(3) <= ser_reg_in(3); -- beeper
            end if;   
          end if;
        
        elsif (i_uc_strobe = '0') then
        
          -- send data to uC
          o_uc_data_out <= ser_reg_out(7);
          
          -- rotate serial register
          ser_reg_out <= ser_reg_out(6 downto 0) & '0';
          
          -- read data from uC
          ser_reg_in <= ser_reg_in(8 downto 0) & i_uc_data_in;
        
        end if;  
      end if;
    end if;
  end process;

  ---------------------------------
  -- definitions for all 8 ports --
  ---------------------------------

  port_gen: for port_no in 0 to 7 generate
    
    -- connect port pin to output register or tristate it
    bit_gen: for bit_no in 0 to 7 generate
      P_IO_PORTS(port_no).port_pin(bit_no) <= 
        port_reg(port_no)(bit_no) when port_dir(port_no) = '1' else
        'Z';
    end generate;    
       
    -- activate CS if addressed                                     
    P_IO_PORTS(port_no).cs <= '0' when -- inverse logic 
      i_uc_ale = '0' and
      i_uc_strobe = '0' and
      addr_unit = my_addr and 
      addr_port = port_no and 
      addr_mod = "101"
      else '1';

    -- activate EEPROM CS if addressed                                     
    P_IO_PORTS(port_no).cs_eeprom <= '1' when 
      i_uc_ale = '0' and
      i_uc_strobe = '0' and
      addr_unit = my_addr and 
      addr_port = port_no and 
      addr_mod = "110"
      else '0';
      
  end generate;
  
  --------------------------------
  -- handle serial bus to ports --
  --------------------------------
  
  P_O_SDI         <= i_uc_data_in when 
                     i_uc_ale = '0'
                     else '0';
  P_O_SCLK        <= i_uc_clk when
                     i_uc_ale = '0'
                     else '0';

  -----------------------------
  -- handle power management --
  -----------------------------
  
  pwr_status(0) <= P_I_5V_OK;    -- put signals into status register
  pwr_status(1) <= P_I_24V_OK;

  -- 24V enable
  P_O_24V_EN <= pwr_status(2);
  
  -- Beeper (on if low)
  P_O_BEEPER <= not pwr_status(3);

  -- quarz speed in MHz:
  -- 0:100, 1:33.33, 2:30, 3:120, 4:25, 5:20, 6:70, 7:80
  -- 8:75, 9:66.66, 10:60, 11:60, 12: 50, 13:45, 14:90, 15:40 
  P_O_CLKSEL  <= "0000";

end Behavioral;
