puts "uploading menu.bin"
mwr -bin -file C:/Xilinx/assets/menu.bin 0x10C00000 691200
puts "uploading level1.bin"
mwr -bin -file C:/Xilinx/assets/level1.bin 0x10F00000 691200
puts "uploading level2.bin"
mwr -bin -file C:/Xilinx/assets/level2.bin 0x11200000 691200
puts "uploading level3.bin"
mwr -bin -file C:/Xilinx/assets/level3.bin 0x11500000 691200
puts "uploading credits.bin"
mwr -bin -file C:/Xilinx/assets/credits.bin 0x11800000 691200
puts "uploading pepeSmall.bin"
mwr -bin -file C:/Xilinx/assets/pepeSmall.bin 0x11B00000 1200
puts "uploading pepeBig.bin"
mwr -bin -file C:/Xilinx/assets/pepeBig.bin 0x11B02000 4800
puts "uploading shrink.bin"
mwr -bin -file C:/Xilinx/assets/shrink.bin 0x11B07000 1200
puts "uploading grow.bin"
mwr -bin -file C:/Xilinx/assets/grow.bin 0x11B09000 1200
puts "uploading pepeSmall2.bin"
mwr -bin -file C:/Xilinx/assets/pepeSmall2.bin 0x11B0B000 1200
puts "uploading pepeBig2.bin"
mwr -bin -file C:/Xilinx/assets/pepeBig2.bin 0x11B0D000 4800
puts "done"