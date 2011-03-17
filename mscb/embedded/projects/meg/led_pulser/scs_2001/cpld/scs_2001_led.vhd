library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

use work.scs_2001_pack.ALL;

entity scs_2001_led is
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
end scs_2001_led;

architecture Behavioral of scs_2001_led is

constant firmware_version : std_logic_vector(3 downto 0) := X"2";

signal ser_reg      : std_logic_vector(10 downto 0);
signal uc_data_out  : std_logic;

signal addr_unit    : std_logic_vector(3 downto 0);
signal addr_port    : std_logic_vector(2 downto 0);

-- address modifiers:
-- 0  0000  AM_READ_PORT
-- 1  0001  AM_READ_REG
-- 2  0010  AM_WRITE_PORT
-- #4  0100  AM_WRITE_DIR
-- 5  0101  AM_RW_SERIAL
-- 6  0110  AM_RW_EEPROM
-- 7  0111  AM_RW_MONITOR
-- 8  1000  AM_READ_CSR
-- 9  1001  AM_WRITE_CSR
signal addr_mod     : std_logic_vector(3 downto 0);

type type_port_reg is array (const_no_ports-1 downto 0) of std_logic_vector(7 downto 0);
signal port_reg     : type_port_reg;

-- '1' is ouput, '0' is input
signal port_dir     : std_logic_vector(7 downto 0) := "00011111"; -- out (0-4) in (5,6,7)

-- bit0: beeper      read/write
signal csr          : std_logic_vector(3 downto 0);

signal counter      : std_logic_vector(31 downto 0);
signal maxcnt       : std_logic_vector(31 downto 0) := conv_std_logic_vector(10000000, 32); -- =1E8 -> 100Hz @ 100 MHz
signal pwidth       : std_logic_vector(5 downto 0) := "000110"; -- 60ns @ 100 MHz
signal led_pulse    : std_logic;
signal clk_pulse    : std_logic;
signal clk_pulse_out: std_logic;
signal ext_trig     : std_logic;

begin
    
  -----------------------------------------
  -- handle serial communication with uC --
  -----------------------------------------

  P_O_UC_DATA_OUT <= uc_data_out when
                     addr_mod /= AM_RW_SERIAL and addr_mod /= AM_RW_EEPROM
                     else P_I_SDO;
  P_O_UC_STAT     <= P_IO_PORTS(CONV_INTEGER(addr_port)).port_pin(0);
  P_O_UC_SPARE1   <= '1';
  P_O_UC_SPARE2   <= '1';

  process(P_I_UC_CLK)
  begin
    if rising_edge(P_I_UC_CLK) then

      if P_I_UC_ALE = '1' then
        
        if (P_I_UC_STROBE = '1') then
          -- upon strobe, copy serial register
          addr_unit   <= ser_reg(10 downto 7);
          addr_port   <= ser_reg(6 downto 4);
          addr_mod    <= ser_reg(3 downto 0);
        else
          -- address cycle
          ser_reg(10 downto 0) <= ser_reg(9 downto 0) & P_I_UC_DATA_IN;
        end if;
        
      else -- data cycle if ALE = '0'
      
        if (P_I_UC_STROBE = '1') then
          if (addr_mod = AM_READ_PORT) then
            -- read from port
            ser_reg <= P_IO_PORTS(CONV_INTEGER(addr_port)).port_pin;
          elsif (addr_mod = AM_READ_REG) then
            -- read from register
            ser_reg(7 downto 0) <= port_reg(CONV_INTEGER(addr_port));
          elsif (addr_mod = AM_WRITE_PORT) then
            -- write to port
            port_reg(CONV_INTEGER(addr_port)) <= ser_reg(7 downto 0);
          elsif (addr_mod = AM_READ_CSR) then
            -- read CSR
            ser_reg(7)             <= '0'; -- indicate master mode
            ser_reg(6 downto 4)    <= firmware_version(2 downto 0);
            ser_reg(3 downto 0)    <= csr;
          elsif (addr_mod = AM_WRITE_CSR) then
            if (addr_port = "001") then    -- 1: write CSR
              csr(2 downto 0)      <= ser_reg(2 downto 0);
            elsif (addr_port = "010") then  -- 2: pulse width
              pwidth <= ser_reg(5 downto 0);
            elsif (addr_port = "011") then  -- 3: divider 0
              maxcnt(7 downto 0)   <= ser_reg(7 downto 0);
            elsif (addr_port = "100") then  -- 4: divider 1
              maxcnt(15 downto 8)  <= ser_reg(7 downto 0);
            elsif (addr_port = "101") then  -- 5: divider 2
              maxcnt(23 downto 16) <= ser_reg(7 downto 0);
            elsif (addr_port = "110") then  -- 6: divider 3
              maxcnt(31 downto 24) <= ser_reg(7 downto 0);
            end if;   
          end if;
     
        elsif (P_I_UC_STROBE = '0') then

          -- send data to uC
          uc_data_out <= ser_reg(7);
          
          -- read data from uC
          ser_reg <= ser_reg(9 downto 0) & P_I_UC_DATA_IN;
          
        end if;  
      end if;
    end if;
  end process;

  ---------------------------------
  -- definitions for all 8 ports --
  ---------------------------------

  port_gen: for port_no in 0 to 7 generate
    
    -- connect port pin to pulser or tristate it
    bit_gen: for bit_no in 0 to 7 generate
      P_IO_PORTS(port_no).port_pin(bit_no) <=
        led_pulse when port_dir(port_no) = '1' and port_reg(port_no)(bit_no) = '1' else
        '0' when port_dir(port_no) = '1' and port_reg(port_no)(bit_no) = '0' else
        'Z';
    end generate;    
       
    -- activate CS if addressed                                     
    P_IO_PORTS(port_no).cs <= '0' when -- inverse logic 
      P_I_UC_ALE = '0' and
      P_I_UC_STROBE = '0' and
      addr_port = port_no and 
      addr_mod = AM_RW_SERIAL
      else '1';

    -- activate EEPROM CS if addressed                                     
    P_IO_PORTS(port_no).cs_eeprom <= '1' when 
      P_I_UC_ALE = '0' and
      P_I_UC_STROBE = '0' and
      addr_port = port_no and 
      addr_mod = AM_RW_EEPROM
      else '0';
  end generate;
  
  --------------------------------
  -- handle serial bus to ports --
  --------------------------------

  P_O_SDI         <= P_I_UC_DATA_IN when 
                     P_I_UC_ALE = '0'
                     else '0';
  P_O_SCLK        <= P_I_UC_CLK when
                     P_I_UC_ALE = '0'
                     else '0';

  ------------------------------------------------
  -- handle serial bus to MAX1253 power monitor --
  ------------------------------------------------
  
  P_O_SCLK_MON    <= P_I_UC_CLK when
                     P_I_UC_ALE = '0'
                     else '0';
  P_O_DIN_MON     <= P_I_UC_DATA_IN when
                     P_I_UC_ALE = '0'
                     else '0';
  P_O_CS_MON      <= '0' when
                     P_I_UC_ALE = '0' and
                     P_I_UC_STROBE = '0' and
                     addr_mod = AM_RW_MONITOR
                     else '1';

  -------------------------
  -- enable quartz clock --
  -------------------------

  P_O_CLKEN       <= '1';

  -----------------------------
  -- handle power management --
  -----------------------------
  
  -- 24V enable
  P_O_24V_EN <= P_I_INT_MON;
  
  -- Beeper (active low)
  P_O_BEEPER <= not (((not P_I_INT_MON) and counter(24)) or 
                       csr(0));

  -------------------------
  -- LED pulse generator --
  -------------------------
  
  process(P_I_CLK)
  begin
    if rising_edge(P_I_CLK) then
      counter <= counter+1;
      if (counter >= maxcnt) then
         counter <= (others => '0');
      end if;
      if (counter = 0) then
         clk_pulse <= '1';
      end if;   
      if (counter = pwidth) then
         clk_pulse <= '0';
      end if;
    end if;
  end process;
  
  clk_pulse_out <= '0' when maxcnt = 0 else clk_pulse;
  ext_trig <= P_IO_PORTS(5).port_pin(1) and not P_IO_PORTS(5).port_pin(0);
  led_pulse <= clk_pulse_out or ext_trig; 
  
  P_IO_EXT(0) <= led_pulse;
 
end Behavioral;
