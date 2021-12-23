-- Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
-- --------------------------------------------------------------------------------
-- Tool Version: Vivado v.2020.2 (win64) Build 3064766 Wed Nov 18 09:12:45 MST 2020
-- Date        : Thu Aug 26 15:23:38 2021
-- Host        : Reginald-Bayeta running 64-bit major release  (build 9200)
-- Command     : write_vhdl -force -mode synth_stub {d:/000 Term 9
--               000/LBYCPH3/lab_05/lab_05.gen/sources_1/bd/zynq_base/ip/zynq_base_clk_wiz_0_0/zynq_base_clk_wiz_0_0_stub.vhdl}
-- Design      : zynq_base_clk_wiz_0_0
-- Purpose     : Stub declaration of top-level module interface
-- Device      : xc7z020clg484-1
-- --------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity zynq_base_clk_wiz_0_0 is
  Port ( 
    clk_out1 : out STD_LOGIC;
    clk_in1 : in STD_LOGIC
  );

end zynq_base_clk_wiz_0_0;

architecture stub of zynq_base_clk_wiz_0_0 is
attribute syn_black_box : boolean;
attribute black_box_pad_pin : string;
attribute syn_black_box of stub : architecture is true;
attribute black_box_pad_pin of stub : architecture is "clk_out1,clk_in1";
begin
end;
