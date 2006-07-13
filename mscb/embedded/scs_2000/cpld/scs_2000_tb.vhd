
-- VHDL Test Bench Created from source file scs_2000.vhd -- 11:39:44 05/08/2006
--
-- Notes: 
-- This testbench has been automatically generated using types std_logic and
-- std_logic_vector for the ports of the unit under test.  Xilinx recommends 
-- that these types always be used for the top-level I/O of a design in order 
-- to guarantee that the testbench will bind correctly to the post-implementation 
-- simulation model.
--
LIBRARY ieee;
USE ieee.std_logic_1164.ALL;
USE ieee.numeric_std.ALL;

use work.scs_2000_pack.ALL;

ENTITY scs_2000_tb IS
END scs_2000_tb;

ARCHITECTURE behavior OF scs_2000_tb IS 

	COMPONENT scs_2000
	PORT(
		P_I_UC_CLK : IN std_logic;
		P_I_UC_ALE : IN std_logic;
		P_I_UC_STROBE : IN std_logic;
		P_I_UC_DATA_IN : IN std_logic;
		P_I_SDO : IN std_logic;
		P_I_ADDR : IN std_logic_vector(3 downto 0);    
		P_IO_PORTS : INOUT type_port_array;      
		P_O_UC_DATA_OUT : OUT std_logic;
		P_O_UC_STAT : OUT std_logic;
		P_O_UC_SPARE1 : OUT std_logic;
		P_O_UC_SPARE2 : OUT std_logic;
		P_O_SDI : OUT std_logic;
		P_O_SCLK : OUT std_logic;
		P_O_CLKSEL : OUT std_logic_vector(3 downto 0);
		P_I_CLK : IN std_logic;
		P_I_5V_OK : IN std_logic;
    P_O_BEEPER : OUT std_logic;
    P_O_24V_EN : OUT std_logic;
    P_I_24V_OC : IN std_logic
		
		);
	END COMPONENT;

	SIGNAL P_I_UC_CLK :  std_logic;
	SIGNAL P_I_UC_ALE :  std_logic;
	SIGNAL P_I_UC_STROBE :  std_logic;
	SIGNAL P_I_UC_DATA_IN :  std_logic;
	SIGNAL P_O_UC_DATA_OUT :  std_logic;
	SIGNAL P_O_UC_STAT :  std_logic;
	SIGNAL P_O_UC_SPARE1 :  std_logic;
	SIGNAL P_O_UC_SPARE2 :  std_logic;
	SIGNAL P_O_SDI :  std_logic;
	SIGNAL P_I_SDO :  std_logic;
	SIGNAL P_O_SCLK :  std_logic;
	SIGNAL P_O_CLKSEL :  std_logic_vector(3 downto 0);
	SIGNAL P_I_CLK :  std_logic;
	SIGNAL P_I_ADDR :  std_logic_vector(3 downto 0);
	SIGNAL P_IO_PORTS :  type_port_array;
	SIGNAL P_I_5V_OK : std_logic;
	SIGNAL P_O_BEEPER : std_logic;
	SIGNAL P_O_24V_EN : std_logic;
	SIGNAL P_I_24V_OC : std_logic;

BEGIN

	uut: scs_2000 PORT MAP(
		P_I_UC_CLK => P_I_UC_CLK,
		P_I_UC_ALE => P_I_UC_ALE,
		P_I_UC_STROBE => P_I_UC_STROBE,
		P_I_UC_DATA_IN => P_I_UC_DATA_IN,
		P_O_UC_DATA_OUT => P_O_UC_DATA_OUT,
		P_O_UC_STAT => P_O_UC_STAT,
		P_O_UC_SPARE1 => P_O_UC_SPARE1,
		P_O_UC_SPARE2 => P_O_UC_SPARE2,
		P_O_SDI => P_O_SDI,
		P_I_SDO => P_I_SDO,
		P_O_SCLK => P_O_SCLK,
		P_O_CLKSEL => P_O_CLKSEL,
		P_I_CLK => P_I_CLK,
		P_I_ADDR => P_I_ADDR,
		P_IO_PORTS => P_IO_PORTS,
		P_I_5V_OK => P_I_5V_OK, 
		P_O_BEEPER => P_O_BEEPER,
		P_O_24V_EN => P_O_24V_EN,
		P_I_24V_OC => P_I_24V_OC
		
	);
  
-- *** Test Bench - User Defined Section ***
   tb : PROCESS

      procedure proc_addr_cycle
        (
          constant I_ADDR_UNIT: in std_logic_vector(3 downto 0);
          constant I_ADDR_PORT: in std_logic_vector(2 downto 0);
          constant I_ADDR_AM: in std_logic_vector(2 downto 0)
        ) is
      begin

        P_I_UC_ALE <= '1';
        
        loop_addr_unit: for i in 3 downto 0 loop      
          P_I_UC_DATA_IN <= I_ADDR_UNIT(i);
          wait for 100 ns;
          P_I_UC_CLK <= '1';
          wait for 100 ns;
          P_I_UC_CLK <= '0';
        end loop;  
        
        loop_addr_port: for i in 2 downto 0 loop      
          P_I_UC_DATA_IN <= I_ADDR_PORT(i);
          wait for 100 ns;
          P_I_UC_CLK <= '1';
          wait for 100 ns;
          P_I_UC_CLK <= '0';
        end loop;  

        loop_addr_am: for i in 2 downto 0 loop      
          P_I_UC_DATA_IN <= I_ADDR_AM(i);
          wait for 100 ns;
          P_I_UC_CLK <= '1';
          wait for 100 ns;
          P_I_UC_CLK <= '0';
        end loop;  

        -- strobe output
        P_I_UC_STROBE <= '1';
        wait for 100 ns;
        P_I_UC_CLK    <= '1';
        wait for 100 ns;
        P_I_UC_STROBE <= '0';
        P_I_UC_CLK    <= '0';
        wait for 100 ns;

        P_I_UC_ALE <= '0';

      end proc_addr_cycle;  
   
      procedure proc_write_cycle
        (
          constant I_DATA: in std_logic_vector(7 downto 0)
        ) is
      begin

        loop_data: for i in 7 downto 0 loop      
          P_I_UC_DATA_IN <= I_DATA(i);
          wait for 100 ns;
          P_I_UC_CLK <= '1';
          wait for 100 ns;
          P_I_UC_CLK <= '0';
        end loop;  
        
        -- strobe output
        P_I_UC_STROBE <= '1';
        wait for 100 ns;
        P_I_UC_CLK    <= '1';
        wait for 100 ns;
        P_I_UC_STROBE <= '0';
        P_I_UC_CLK    <= '0';
        wait for 100 ns;

      end proc_write_cycle;  


      procedure proc_read_cycle is
      begin

        -- strobe input
        P_I_UC_STROBE <= '1';
        wait for 100 ns;
        P_I_UC_CLK    <= '1';
        wait for 100 ns;
        P_I_UC_STROBE <= '0';
        P_I_UC_CLK    <= '0';

        loop_data: for i in 7 downto 0 loop      
          wait for 100 ns;
          P_I_UC_CLK <= '1';
          wait for 100 ns;
          P_I_UC_CLK <= '0';
        end loop;  

      end proc_read_cycle;  

   BEGIN
      P_I_24V_OC <= '1';
      P_I_ADDR <= "1111";
      P_I_UC_CLK <= '0';
      P_I_UC_STROBE <= '0';
      
      wait for 100 ns;
      P_I_24V_OC <= '0';
      wait for 10 ns;
      P_I_24V_OC <= '1';
      wait for 100 ns;
      
      proc_addr_cycle("0000", "010", "100"); -- AM_WRITE_CSR, pwr_status
      proc_write_cycle("00000100");          -- 24V reset
      proc_write_cycle("00000000");

      --proc_addr_cycle("0000", "000", '1', '0', '0');
      --proc_write_cycle("01010101");
      --proc_write_cycle("10101010");

      --P_IO_PORTS(0).port_pin <= "11111011";      
      --proc_addr_cycle("0000", "000", '0', '0', '0');
      --proc_read_cycle;

      wait for 100 ns;      
      assert false
        report "Simulation Complete (this is not a failure)"
        severity failure;
      
   END PROCESS;
-- *** End Test Bench - User Defined Section ***

END;
