library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

use work.scs_2000_pack.ALL;

entity scs_2000_led is
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
           P_I_24V_OC      : in std_logic
         );
end scs_2000_led;

architecture Behavioral of scs_2000_led is

signal ser_reg      : std_logic_vector(9 downto 0);
signal uc_data_out  : std_logic;

signal addr_unit    : std_logic_vector(3 downto 0);
signal addr_port    : std_logic_vector(2 downto 0);
signal my_addr      : std_logic_vector(3 downto 0);

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
--   2       pwidth
--   3       freq0
--   4       freq1
--   5       freq2
--   6       freq3
-- 5  101  AM_RW_SERIAL
-- 6  110  AM_RW_EEPROM
signal addr_mod     : std_logic_vector(2 downto 0);

type type_port_reg is array (const_no_ports-1 downto 0) of std_logic_vector(7 downto 0);
signal port_reg     : type_port_reg;

-- '1' is ouput, '0' is input
signal port_dir     : std_logic_vector(7 downto 0) := "00000000";                                        

-- bit0: 5V ok       readonly
-- bit1: 24V ok      readonly
-- bit2: 24V reset   writeonly
-- bit3: beeper      read/write
signal pwr_status   : std_logic_vector(3 downto 0) := "0000";

signal disable_24V  : std_logic := '0';
signal reset_24V    : std_logic := '0';
signal pwr_trip     : std_logic;

signal counter      : std_logic_vector(31 downto 0);
signal maxcnt       : std_logic_vector(31 downto 0) := conv_std_logic_vector(5000000, 32); -- =5E7 -> 100Hz @ 50 MHz
signal pwidth       : std_logic_vector(5 downto 0) := "000001"; -- 20ns @ 50 MHz
signal led_pulse    : std_logic;
signal clk_pulse    : std_logic;
signal clk_pulse_out: std_logic;
signal ext_trig     : std_logic;

begin
    
  process(P_I_UC_CLK)
  begin
    if rising_edge(P_I_UC_CLK) then
      reset_24V <= '0';
      if P_I_UC_ALE = '1' then
        if (P_I_UC_STROBE = '1') then
          addr_unit   <= ser_reg(9 downto 6);
          addr_port   <= ser_reg(5 downto 3);
          addr_mod    <= ser_reg(2 downto 0);
        else
          -- address cycle
          ser_reg(9 downto 0) <= ser_reg(8 downto 0) & P_I_UC_DATA_IN;
        end if;
      else
        if (P_I_UC_STROBE = '1' and addr_unit = my_addr) then
          if (addr_mod = "000") then
            -- read from port
            ser_reg(7 downto 0) <= P_IO_PORTS(CONV_INTEGER(addr_port)).port_pin;
          elsif (addr_mod = "001") then
            -- read from register
            ser_reg(7 downto 0) <= port_reg(CONV_INTEGER(addr_port));
          elsif (addr_mod = "010") then
            -- write to port
            port_reg(CONV_INTEGER(addr_port)) <= ser_reg(7 downto 0);
          elsif (addr_mod = "011") then
            -- read CSR
            if (addr_port = "000") then     -- 0
               ser_reg(7 downto 0) <= port_dir;
            elsif (addr_port = "001") then  -- 1
               ser_reg(7 downto 4) <= (others => '0');
               ser_reg(3 downto 0) <= pwr_status;    
            end if;   
          elsif (addr_mod = "100") then
            -- write CSR
            if (addr_port = "000") then     -- 0
               port_dir <= ser_reg(7 downto 0);
            elsif (addr_port = "001") then  -- 1
               pwr_status(3) <= ser_reg(3); -- beeper
               if (ser_reg(2) = '1') then
                  reset_24V <= '1';         -- 24V reset
               end if;
            elsif (addr_port = "010") then  -- 2
               pwidth <= ser_reg(5 downto 0);
            elsif (addr_port = "011") then  -- 3
               maxcnt(7 downto 0) <= ser_reg(7 downto 0);
            elsif (addr_port = "100") then  -- 4
               maxcnt(15 downto 8) <= ser_reg(7 downto 0);
            elsif (addr_port = "101") then  -- 5
               maxcnt(23 downto 16) <= ser_reg(7 downto 0);
            elsif (addr_port = "110") then  -- 6
               maxcnt(31 downto 24) <= ser_reg(7 downto 0);
            end if;   
          end if;
        elsif (P_I_UC_STROBE = '0') then
          -- data to uC
          uc_data_out <= ser_reg(7);
          ser_reg <= ser_reg(8 downto 0) & '0';
          -- data from uC
          ser_reg <= ser_reg(8 downto 0) & P_I_UC_DATA_IN;
        end if;  
      end if;
    end if;
  end process;

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
      addr_unit = my_addr and 
      addr_port = port_no and 
      addr_mod = "101"
      else '1';

    -- activate EEPROM CS if addressed                                     
    P_IO_PORTS(port_no).cs_eeprom <= '1' when 
      P_I_UC_ALE = '0' and
      P_I_UC_STROBE = '0' and
      addr_unit = my_addr and 
      addr_port = port_no and 
      addr_mod = "110"
      else '0';
  end generate;
  
  -- serial slave bus
  P_O_SDI         <= P_I_UC_DATA_IN when 
                     P_I_UC_ALE = '0'
                     else '0';
  P_O_SCLK        <= P_I_UC_CLK when
                     P_I_UC_ALE = '0'
                     else '0';
  P_O_UC_DATA_OUT <= uc_data_out when
                     addr_mod /= "101" and addr_mod /= "110"
                     else P_I_SDO;
  P_O_UC_STAT     <= P_IO_PORTS(CONV_INTEGER(addr_port)).port_pin(0) when
                     addr_unit = my_addr
                     else '0'; -- change later to slave bus!                   
  P_O_UC_SPARE1   <= '1';
  P_O_UC_SPARE2   <= '1';
    
  -- address switch
  my_addr <= not P_I_ADDR; -- inverse logic  
      
  -- quarz speed in MHz:
  -- 0:100, 1:33.33, 2:30, 3:120, 4:25, 5:20, 6:70, 7:80
  -- 8:75, 9:66.66, 10:60, 11:60, 12: 50, 13:45, 14:90, 15:40 
  P_O_CLKSEL  <= "1100";

  -- power management
  pwr_trip   <= not P_I_24V_OC;  -- goes zero in case of current trip
  P_O_24V_EN <= not disable_24V;

  process(pwr_trip, reset_24V)   -- power trip flip-flop  
  begin
    if pwr_trip = '1' then
      disable_24V <= '1';
    elsif reset_24V = '1' then
      disable_24V <= '0';   
    end if;
  end process;

  pwr_status(0) <= P_I_5V_OK;    -- put signals into status register
  pwr_status(1) <= not disable_24V;
  pwr_status(2) <= '0';
  
  -- Beeper (on if low)
  P_O_BEEPER <= not (pwr_status(3) or not pwr_status(0) or disable_24V);
 
  -- LED pulse generator
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
  led_pulse <= (clk_pulse_out or ext_trig) and P_IO_PORTS(6).port_pin(0); 
  
  P_IO_EXT(0) <= led_pulse;
 
end Behavioral;
