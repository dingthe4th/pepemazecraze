-- Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity zynq_base_v_tc_0_0 is
  Port ( 
    clk : in STD_LOGIC;
    clken : in STD_LOGIC;
    det_clken : in STD_LOGIC;
    gen_clken : in STD_LOGIC;
    sof_state : in STD_LOGIC;
    hsync_in : in STD_LOGIC;
    hblank_in : in STD_LOGIC;
    vsync_in : in STD_LOGIC;
    vblank_in : in STD_LOGIC;
    active_video_in : in STD_LOGIC;
    hsync_out : out STD_LOGIC;
    hblank_out : out STD_LOGIC;
    vsync_out : out STD_LOGIC;
    vblank_out : out STD_LOGIC;
    active_video_out : out STD_LOGIC;
    resetn : in STD_LOGIC;
    fsync_out : out STD_LOGIC_VECTOR ( 0 to 0 )
  );

end zynq_base_v_tc_0_0;

architecture stub of zynq_base_v_tc_0_0 is
attribute syn_black_box : boolean;
attribute black_box_pad_pin : string;
attribute syn_black_box of stub : architecture is true;
attribute x_core_info : string;
attribute x_core_info of stub : architecture is "v_tc,Vivado 2020.2";
begin
end;
