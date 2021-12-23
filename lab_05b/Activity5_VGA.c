#include "xil_io.h"
#include "xil_mmu.h"
#include "xparameters.h"

#define FRAMEBUFFER_BASEADDR	(0x10000000) // The updated linker script allows us to freely use the memory space from 0x10000000 to 0x1FFFFFFF
#define WIDTH 1279



void enable_mm2s();
void disable_mm2s();
void configure_mm2s(unsigned int width, unsigned int height);


typedef struct {
	uint8_t colorBLUE;
	uint8_t colorGREEN;
	uint8_t colorRED;

}colorRGB;

colorRGB* RGB = FRAMEBUFFER_BASEADDR;

void setBlack(int x, int y){
	RGB[x + y * 1280].colorRED = 0x00;
	RGB[x + y * 1280].colorGREEN = 0x00;
	RGB[x + y * 1280].colorBLUE = 0x00;

}

void setWhite(int x, int y){
	RGB[x + y * 1280].colorRED = 0xFF;
	RGB[x + y * 1280].colorGREEN = 0xFF;
	RGB[x + y * 1280].colorBLUE = 0xFF;

}

int main()
{
	//force fpga to access RAM
	//bypass cache
	Xil_SetTlbAttributes(FRAMEBUFFER_BASEADDR, SHAREABLE | NORM_NONCACHE);
	Xil_SetTlbAttributes(FRAMEBUFFER_BASEADDR + 0x100000, SHAREABLE | NORM_NONCACHE);
	Xil_SetTlbAttributes(FRAMEBUFFER_BASEADDR + 0x200000, SHAREABLE | NORM_NONCACHE);



	memset((void *)FRAMEBUFFER_BASEADDR, 0, 1280 * 720 * 3);
	//yellow background = FF DD 3C
	//gray walls = 85 82 7B

	//fill frame with color
	for(int y =1; y<719; y++){
		for(int x=1; x<1279; x++){
			if(y<20 || y>700 || x<20 || x>1260) // gray border
			{
				RGB[x + y * 1280].colorRED = 0x85;
				RGB[x + y * 1280].colorGREEN = 0x82;
				RGB[x + y * 1280].colorBLUE = 0x7b;
			}
			else if(x>420 && x<440 && y<520 ) // first border
			{
				RGB[x + y * 1280].colorRED = 0x85;
				RGB[x + y * 1280].colorGREEN = 0x82;
				RGB[x + y * 1280].colorBLUE = 0x7b;
			}
			else if(x>840 && x<860 && y>200 ) // second border
			{
				RGB[x + y * 1280].colorRED = 0x85;
				RGB[x + y * 1280].colorGREEN = 0x82;
				RGB[x + y * 1280].colorBLUE = 0x7b;
			}
			else if((y>20 && y<=120) && (x>20 &&x<=420)) //for starting line
			{
				RGB[x + y * 1280].colorRED = 0xFF;
				RGB[x + y * 1280].colorGREEN = 0x00;
				RGB[x + y * 1280].colorBLUE = 0x00;
			}
			else //Yellow BG
			{
			    RGB[x + y * 1280].colorRED = 0xFF;
			    RGB[x + y * 1280].colorGREEN = 0xDD;
			    RGB[x + y * 1280].colorBLUE = 0x3C;
			}


		}
	}

	// Enable and configure the MM2S DMA (Memory to Video Out)
	enable_mm2s();
	configure_mm2s(1280, 720);

	// Wait for some time to ensure that the frame buffer contains the video data
	usleep(100000);


	while (1);

	return 0;
}


void enable_mm2s()
{
	// Write to the MM2S_VDMACR (MM2S control register)
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x00, 0x8B);

	// Clear all the status flags
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x04, 0xFFFFFFFF);
}



void disable_mm2s()
{
	// Write to the MM2S_VDMACR (MM2S control register)
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x00, 0x00);

	// Clear all the status flags
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x04, 0xFFFFFFFF);
}


void configure_mm2s(unsigned int width, unsigned int height)
{
	// Assign the frame buffer address
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x5C, FRAMEBUFFER_BASEADDR);

	// Define the size of the frame
	// This assumes that the data is in RGB 8-bit format
	// It also assumes that the stride is equal to the frame width
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x58, width * 3); // Stride (in bytes)
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x54, width * 3); // Width (in bytes)
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x50, height); // Height
}

