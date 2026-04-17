library ieee;
use ieee.std_logic_1164.all;

entity my_processor is
  port (
    clk      : in  std_logic;
    reset    : in  std_logic;
    data_in  : in  std_logic_vector(7 downto 0);
    addr     : in  std_logic_vector(3 downto 0);
    data_out : out std_logic_vector(7 downto 0);
    ready    : out std_logic;
    valid    : out std_logic
  );
end entity;

architecture behavioral of my_processor is
begin
  process(clk, reset)
  begin
    if reset = '1' then
      data_out <= (others => '0');
      ready <= '0';
      valid <= '0';
    elsif rising_edge(clk) then
      -- do something
      valid <= '1';
    end if;
  end process;
end behavioral;