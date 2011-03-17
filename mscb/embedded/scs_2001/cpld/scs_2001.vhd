library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

use work.scs_2001_pack.ALL;

entity scs_2001 is
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
           P_O_CLKEN       : out std_logic;
           P_I_CLK         : in std_logic;
           P_I_ADDR        : in std_logic_vector(3 downto 0);
           P_IO_EXT        : inout std_logic_vector(7 downto 0);
           P_IO_PORTS      : inout type_port_array;
           P_O_SCLK_MON    : out std_logic;
           P_O_CS_MON      : out std_logic;
           P_I_DOUT_MON    : in std_logic;
           P_O_DIN_MON     : out std_logic;
           P_I_INT_MON     : in std_logic;
           P_O_BEEPER      : out std_logic;
           P_O_24V_EN      : out std_logic
         );
end scs_2001;

architecture Behavioral of scs_2001 is

constant firmware_version : std_logic_vector(3 downto 0) := X"2";

-- uC signals, either directly from uC or from extension bus
signal i_uc_clk       : std_logic;
signal i_uc_ale       : std_logic;
signal i_uc_strobe    : std_logic;
signal i_uc_data_in   : std_logic;

-- serial signals going to slave units
signal i_sl_clk       : std_logic;
signal i_sl_ale       : std_logic;
signal i_sl_strobe    : std_logic;
signal o_sl_data_out  : std_logic;

-- serial registers
signal ser_reg_in     : std_logic_vector(10 downto 0);
signal ser_reg_out    : std_logic_vector(7 downto 0);
signal ser_reg_slave  : std_logic_vector(7 downto 0);
signal o_uc_data_out  : std_logic;

-- internal clock counter
signal counter        : std_logic_vector(31 downto 0);

-- various variables
signal addr_unit      : std_logic_vector(3 downto 0);
signal addr_port      : std_logic_vector(2 downto 0);
signal my_addr        : std_logic_vector(3 downto 0);
signal master         : std_logic;

-- address modifiers:
-- 0  0000  AM_READ_PORT
-- 1  0001  AM_READ_REG
-- 2  0010  AM_WRITE_PORT
-- 4  0100  AM_WRITE_PORTDIR
-- 5  0101  AM_RW_SERIAL
-- 6  0110  AM_RW_EEPROM
-- 7  0111  AM_RW_MONITOR
-- 8  1000  AM_READ_CSR
-- 9  1001  AM_WRITE_CSR
signal addr_mod     : std_logic_vector(3 downto 0);

type type_port_reg is array (const_no_ports-1 downto 0) of std_logic_vector(7 downto 0);
signal port_reg     : type_port_reg;

-- '1' is ouput, '0' is input
signal port_dir     : type_port_reg;                                        

-- bit0: beeper      read/write
signal csr          : std_logic_vector(3 downto 0);

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
  
  P_O_UC_DATA_OUT <= o_sl_data_out when master = '0'
                     else P_IO_EXT(4) when addr_unit /= my_addr
                     else P_I_DOUT_MON when addr_mod = AM_RW_MONITOR
                     else o_uc_data_out when addr_mod /= AM_RW_SERIAL and addr_mod /= AM_RW_EEPROM
                     else P_I_SDO;
  P_O_UC_STAT     <= P_IO_PORTS(CONV_INTEGER(addr_port)).port_pin(0) when addr_unit = my_addr
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
  P_IO_EXT(4)     <= 'Z' when master = '1' or addr_unit /= my_addr
                     else P_I_SDO when addr_mod = AM_RW_SERIAL or addr_mod = AM_RW_EEPROM
                     else P_I_DOUT_MON when addr_mod = AM_RW_MONITOR
                     else o_uc_data_out;
                     
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
          addr_unit   <= ser_reg_in(10 downto 7);
          addr_port   <= ser_reg_in(6 downto 4);
          addr_mod    <= ser_reg_in(3 downto 0);
        else
          -- address cycle
          ser_reg_in(10 downto 0) <= ser_reg_in(9 downto 0) & i_uc_data_in;
        end if;
      
      else -- data cycle if ALE = '0'
      
        if (i_uc_strobe = '1' and addr_unit = my_addr) then
          if (addr_mod = AM_READ_PORT) then
            -- read from port
            ser_reg_out <= P_IO_PORTS(CONV_INTEGER(addr_port)).port_pin;
          elsif (addr_mod = AM_READ_REG) then
            -- read from register
            ser_reg_out <= port_reg(CONV_INTEGER(addr_port));
          elsif (addr_mod = AM_WRITE_PORT) then
            -- write to port
            port_reg(CONV_INTEGER(addr_port)) <= ser_reg_in(7 downto 0);
          elsif (addr_mod = AM_WRITE_DIR) then
            -- write port dir
            port_dir(CONV_INTEGER(addr_port)) <= ser_reg_in(7 downto 0);
          elsif (addr_mod = AM_READ_CSR) then
            -- read CSR
            ser_reg_out(7)          <= '0'; -- indicate master mode
            ser_reg_out(6 downto 4) <= firmware_version(2 downto 0);
            ser_reg_out(3 downto 0) <= csr;
          elsif (addr_mod = AM_WRITE_CSR) then
            -- write CSR
            csr(2 downto 0) <= ser_reg_in(2 downto 0);
          end if;
        
        elsif (i_uc_strobe = '0') then
        
          -- send data to uC
          o_uc_data_out <= ser_reg_out(7);
          
          -- rotate serial register
          ser_reg_out <= ser_reg_out(6 downto 0) & '0';
          
          -- read data from uC
          ser_reg_in <= ser_reg_in(9 downto 0) & i_uc_data_in;
        
        end if;  
      end if;
    end if;
  end process;

  ------------------------------------------------
  -- serial communication to read slave address --
  ------------------------------------------------

  i_sl_clk    <= P_I_UC_CLK     when master = '0' else '0';
  i_sl_strobe <= P_I_UC_STROBE  when master = '0' else '0';
  i_sl_ale    <= P_I_UC_ALE     when master = '0' else '0';

  process(i_sl_clk)
  begin
    if rising_edge(i_sl_clk) then
      if i_sl_ale = '0' then
        if (i_sl_strobe = '1') then
            ser_reg_slave(7)          <= '1'; -- indicate slave mode
            ser_reg_slave(6 downto 4) <= firmware_version(2 downto 0);
            ser_reg_slave(3 downto 0) <= my_addr;
        else
          -- send data to uC
          o_sl_data_out <= ser_reg_slave(7);
          
          -- rotate serial register
          ser_reg_slave <= ser_reg_slave(6 downto 0) & '0';
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
        port_reg(port_no)(bit_no) when port_dir(port_no)(bit_no) = '1' else
        'Z';
    end generate;    
       
    -- activate CS if addressed                                     
    P_IO_PORTS(port_no).cs <= '0' when -- inverse logic 
      i_uc_ale = '0' and
      i_uc_strobe = '0' and
      addr_unit = my_addr and 
      addr_port = port_no and 
      addr_mod = AM_RW_SERIAL
      else '1';

    -- activate EEPROM CS if addressed                                     
    P_IO_PORTS(port_no).cs_eeprom <= '1' when 
      i_uc_ale = '0' and
      i_uc_strobe = '0' and
      addr_unit = my_addr and 
      addr_port = port_no and 
      addr_mod = AM_RW_EEPROM
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

  ------------------------------------------------
  -- handle serial bus to MAX1253 power monitor --
  ------------------------------------------------
  
  P_O_SCLK_MON    <= i_uc_clk when
                     i_uc_ale = '0'
                     else '0';
  P_O_DIN_MON     <= i_uc_data_in when
                     i_uc_ale = '0'
                     else '0';
  P_O_CS_MON      <= '0' when
                     i_uc_ale = '0' and
                     i_uc_strobe = '0' and
                     addr_unit = my_addr and
                     addr_mod = AM_RW_MONITOR
                     else '1';

  ------------------
  -- quartz clock --
  ------------------

  -- enable quartz
  P_O_CLKEN  <= '1';
  
  -- LED pulse generator
  process(P_I_CLK)
  begin
    if rising_edge(P_I_CLK) then
      counter <= counter+1;
    end if;
  end process;
  
  -----------------------------
  -- handle power management --
  -----------------------------
  
  -- 24V enable
  P_O_24V_EN <= P_I_INT_MON;
  
  -- Beeper (active low)
  P_O_BEEPER <= not (((not P_I_INT_MON) and counter(24)) or 
                       csr(0));

end Behavioral;