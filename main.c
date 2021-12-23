/****************************************************************************/
/**
* PROJECT		TITLE: Pepe Maze Craze (Hardware Version)
* BY:			BAYETA IV, Reginald Geoffrey L. - 11811269
*				PANES, Michael Ryan B.			- 11813911
*				TIU, Emerson Karl M.			- 11838000
* COURSE:		LBYCPH3-EQ4
* INSTRUCTOR:	Dr. Carlo Noel E. Ochotorena

* HOW TO PLAY:
*	Controls:
*		LEFT	Move sprite in west direction
* 		RIGHT	Move sprite in east direction
* 		UP 		Move sprite in north direction
* 		DOWN 	Move sprite in south direction
* 		CENTER 	Move back to starting position of current level
*	Objectives:
*		Complete three levels by traversing through the maze.
*		The main sprite can interact with two objects, SHRINK (-), and GROW (+)
*		Shrink (-)		- makes the sprite size smaller
*		Grow   (+)		- makes the sprite size larger
*
*	Loading of tcl file: source C:/Xilinx/assets.tcl
*
* Modification History:
* Ver   Who  Date     Changes
* ----- ---- -------- -----------------------------------------------
* 1.00	emer 07/20/21 Initial project file and contiguous sprite memory
* 1.01	ding 09/10/21 Wall collision and fog of war
* 1.02  emer 09/11/21 Grow and Shrink Function
* 1.03  ryan 09/13/21 Updated contiguous background memory, Initialize Level and Event Functions, Sprite coordinates in array, New Keypress
* 1.04	emer 09/13/21 Fixed Reset Keypress bug, fixed load fog bug, added stage intiial size
* 1.05	ding 09/15/21 Cleaned comments, updated level 2
* 1.06  emer 09/15/21 Added left and right facing direction
* 1.07	ryan 09/16/21 Added interrupt functions
* 1.08  ding 09/17/21 Added converted wall collission, fog of war functions, and background patcher
* 1.09  emer 09/17/21 Added converted grow and shrink event functions, change level function, update sprite function
*
*Pending:
* 	5. update_main_sprite
*
* Note: debugMode is on in this version (fog wont show if run)
* 		see note @ load_fog()
* 		gpio_group is not defined yet
* 		There's 2 undefined function, 1 unused (See functions)
**/
/*****************************************************************************/

/***************************** Include Files ********************************/
#include "xil_io.h"
#include "xil_mmu.h"
#include "xgpio.h"
#include "xtmrctr.h"
#include "xscugic.h"
#include "xparameters.h"
#include "xil_exception.h"

/************************** Hardware Definitions ****************************/
// Define some aliases for the GPIO channels
#define	GPIO_BTN_CHANNEL			1

// Define Timer Reset Value
// Reset Timer at 0.1sec
#define RESET_VALUE 				0x000F423F

// Define some aliases for the arrow button values
#define ARROW_BTN_UP				16
#define ARROW_BTN_RIGHT				8
#define ARROW_BTN_LEFT				4
#define ARROW_BTN_DOWN				2
#define ARROW_BTN_CENTER			1

// Create a macro to define the interrupt priority
#define GIC_PRIO(prio)	(prio << 3)

/************************** Framebuffer Definitions ****************************/

// Event Block Colors
#define EXIT		 				(0x480000)
#define STAGE1COLOR	 				(0x0076ff)
#define	STAGE2COLOR	 				(0x7a009f)
#define	STAGE3COLOR	 				(0xf65058)
#define	FINISHCOLOR	 				(0xff0044)
#define WALLCOLOR	 				(0x5a6988)

// Starting Memory Address
#define FRAMEBUFFER_BASEADDR   		(0x10000000)
#define BACKGROUND	    			(0x10900000)
#define MENU        				(0x10C00000)
#define LEVEL1      				(0x10F00000)
#define LEVEL2         				(0x11200000)
#define LEVEL3      				(0x11500000)
#define CREDITS						(0x11800000)
#define PSMALL       				(0x11B00000)
#define PBIG          				(0x11B02000)
#define SHRINK             			(0x11B07000)
#define GROW           				(0x11B09000)

// Additional Constants
#define WIDTH		 				1280
#define HEIGHT						720
#define SPRITE_SIZE_S 				40
#define SPRITE_SIZE_B 				80

// BGR Structure
typedef struct {
	uint8_t BLUE;
	uint8_t GREEN;
	uint8_t RED;
} colorBGR;

/*************************** Sprite Attributes ******************************/
uint16_t xc;														// xc position of main_sprite
uint16_t yc;														// yc position of main_sprite
uint16_t startX[5] = { 100, 600, 900, 70, 100 };					// main sprite starting x-coordinates per level
uint16_t startY[5] = { 100, 640, 360, 655, 100 };					// main sprite starting y-coordinates per level
uint16_t dx = 3;													// speed wrt x-axis
uint16_t dy = 3;													// speed wrt y-axis
uint16_t level = 0;													// current program level
uint16_t allowance = 150;											// radius of fog
uint16_t keyDown = 0;												// for space input
uint16_t prev = 1;													// previous size of main_sprite
uint16_t current = 0;												// 0 for small 1 for big
uint16_t size[4] = { 40,80,40,40 };									// only one side given that sprite l*w ratios are 1:1 {small sprite,big sprite,shrink sprite,grow sprite}
uint16_t shrink_count[5] = { 1, 2, 4, 1, 0 };						// contains amount of shrink blocks per level; used for loop
uint16_t grow_count[5] = { 1, 3, 6, 2, 0 };							// contains amount of grow blocks per level; used for loop
uint8_t	 debugMode = 1;												// if True, does not display fog of war
uint8_t	 facing = 0;												// 0 for right 1 for left


// x-coordinates of shrink block positions; shrink_xc[level][position]
uint16_t shrink_xc[4][4] = { {1180}, {75, 155}, {120, 720, 740, 920}, {1220} };
// y-coordinates of shrink block positions; shrink_yc[level][position]
uint16_t shrink_yc[4][4] = { {100}, {330, 135}, {150, 40, 160, 440}, {660} };
// x-coordinates of grow block positions; grow_xc[level][position]
uint16_t grow_xc[4][6] = { {200}, {1100, 90, 430},{220, 840, 1100, 600, 750, 1040},  { 805 , 1130} };
// y-coordinates of grow block positions; grow_yc[level][position]
uint16_t grow_yc[4][6] = { {100}, {105, 610, 400}, {480, 620, 630, 150, 440, 210},  { 160, 275} };
// contains initial size of main_sprite per stage |  0 for small 1 for big
uint16_t stageInitial[4] = { 0,0,0,1 };

/******************************* Functions **********************************/
void enable_mm2s();
void disable_mm2s();
void configure_mm2s(unsigned int width, unsigned int height);
void mem_init();					// NOT USED!


void initiateLevel();
void patch(uint8_t dimension);
void load_bg();
void load_grow(uint16_t sprite_xc, uint16_t sprite_yc);
void load_shrink(uint16_t sprite_xc, uint16_t sprite_yc);
void load_fog();
void update_fog_east(uint8_t debug_mode);
void update_fog_wast(uint8_t debug_mode);
void update_fog_south(uint8_t debug_mode);
void update_fog_north(uint8_t debug_mode);
uint8_t is_inbounds(uint16_t x, uint16_t y);
uint8_t can_move2posxy(uint16_t nxt_xc, uint16_t nxt_yc, uint16_t dimension);
void touch_shrink(uint16_t sprite_xc, uint16_t sprite_yc);
void touch_grow(uint16_t sprite_xc, uint16_t sprite_yc);
void isEvent(uint16_t nxt_xc, uint16_t nxt_yc, uint16_t dimension);
void update_main_sprite(uint8_t sizeIndex, uint8_t direction);
void timer_interrupt(void *callback_ref);
void btn_interrupt(void *callback_ref);

/********************************* Main *************************************/
int main()
{

/*************************** Hardware Initialization *******************************/
// Create the necessary driver instances
XGpio btn;
XScuGic intc;
XScuGic_Config *cfg;
XTmrCtr timer;
int status;

// Associate the instances to their respective devices
XGpio_Initialize(&btn, XPAR_AXI_GPIO_0_DEVICE_ID);
status = XTmrCtr_Initialize(&timer, XPAR_AXI_TIMER_0_DEVICE_ID);

// Define whether each pin is an input (?)
XGpio_SetDataDirection(&btn, GPIO_BTN_CHANNEL, 0xFF);
//XGpio_SetDataDirection(&sw,  GPIO_SW_CHANNEL,  0xFF);

//timer set options
XTmrCtr_SetOptions(&timer, 0,   XTC_DOWN_COUNT_OPTION | XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);

// Load the configuration for the Generic Interrupt Controller (GIC)
cfg = XScuGic_LookupConfig(XPAR_SCUGIC_0_DEVICE_ID);

// Initialize the interrupt controller instance
status = XScuGic_CfgInitialize(&intc, cfg, cfg->CpuBaseAddress);

// Run self-tests to verify that the interrupt controller is functional
status = XScuGic_SelfTest(&intc);

// Register the GIC interrupt handler to the exception table
// This step is vital to the operation of the interrupt system and cannot be omitted
Xil_ExceptionInit();
Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, &intc);
Xil_ExceptionEnable();

// Connect the specific interrupt vector to the handler function "input_changed"
// Any data that the handler needs must be transmitted through either global variables or through the callback reference
// In this example the structure "inputs" is passed as a generic pointer
status = XScuGic_Connect(&intc, XPAR_FABRIC_GPIO_0_VEC_ID, (Xil_ExceptionHandler)btn_interrupt, (void *)&btn);
status = XScuGic_Connect(&intc, XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR,(Xil_InterruptHandler)XTmrCtr_InterruptHandler,&timer);

XTmrCtr_SetHandler(&timer, (XTmrCtr_Handler)timer_interrupt, (void *)&btn);

// Set the priority and trigger of the interrupt
// This example uses a priority of 10. Priorities should be from 0 to 31 with 0 being the highest priority interrupt
// In addition, a trigger of 3 indicates that the interrupt is triggered by a rising edge
XScuGic_SetPriorityTriggerType(&intc, XPAR_FABRIC_GPIO_0_VEC_ID, 			  GIC_PRIO(10), 0x03);
XScuGic_SetPriorityTriggerType(&intc, XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR, GIC_PRIO(20), 0x03);

// Enable the interrupt controller
XScuGic_Enable(&intc, XPAR_FABRIC_GPIO_0_VEC_ID);
XScuGic_Enable(&intc, XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR);

// Enable the interrupt on the GPIO device
XGpio_InterruptEnable(&btn, XGPIO_IR_CH1_MASK);
XGpio_InterruptGlobalEnable(&btn);


XTmrCtr_SetResetValue(&timer, 0, RESET_VALUE);
XTmrCtr_Start(&timer, 0);

/**************************** Video Initialization *********************************/

// Loads framebuffer
colorBGR* fb_BGR = FRAMEBUFFER_BASEADDR;

// initiates background memory
colorBGR* lvl1_BGR = LEVEL1;

// Force fpga to access RAM
// Bypass cache
Xil_SetTlbAttributes(FRAMEBUFFER_BASEADDR , SHAREABLE | NORM_NONCACHE);
Xil_SetTlbAttributes(FRAMEBUFFER_BASEADDR + 0x100000, SHAREABLE | NORM_NONCACHE);
Xil_SetTlbAttributes(FRAMEBUFFER_BASEADDR + 0x200000, SHAREABLE | NORM_NONCACHE);

//set framebuffer memory allocation size
memset((void *)FRAMEBUFFER_BASEADDR, 0, WIDTH * HEIGHT * 3);

// Initiate level
initiateLevel();


// Enable and configure the MM2S DMA (Memory to Video Out)
enable_mm2s();
configure_mm2s(WIDTH, HEIGHT);

// Wait for some time to ensure that the frame buffer contains the video data
usleep(100000);

/**************************** Program Loop *********************************/
while(1){

	update_main_sprite(current, facing);

	// Check if sprite touches shrink
	for (uint16_t i = 0; i < shrink_count[level]; i++)
	{
		if (current == 1)touch_shrink(shrink_xc[level][i], shrink_yc[level][i]);
	}

	// Check if sprite touches grow
	for (uint16_t i = 0; i < grow_count[level]; i++)
	{
		touch_grow(grow_xc[level][i], grow_yc[level][i]);
	}
}

return 0;
}

/************************** Function Definitions ****************************/

/****************************************************************************/
/**
* Enables M2S DMA (Memory to Video Out)
*
* @param	none
*
* @return	none
*
*****************************************************************************/
void enable_mm2s()
{
	// Write to the MM2S_VDMACR (MM2S control register)
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x00, 0x8B);

	// Clear all the status flags
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x04, 0xFFFFFFFF);
}

/****************************************************************************/
/**
* Disables M2S DMA (Memory to Video Out)
*
* @param	none
*
* @return	none
*
*****************************************************************************/
void disable_mm2s()
{
	// Write to the MM2S_VDMACR (MM2S control register)
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x00, 0x00);

	// Clear all the status flags
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x04, 0xFFFFFFFF);
}

/****************************************************************************/
/**
* Configures M2S DMA (Memory to Video Out)
*
* @param	none
*
* @return	none
*
*****************************************************************************/
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

/****************************************************************************/
/**
* Set all memory to zero
*
* @param	none
*
* @return	none
*
* @note		unused
*
*****************************************************************************/
void mem_init() {
    memset((void *)FRAMEBUFFER_BASEADDR, 0, WIDTH * HEIGHT * 3);
    memset((void *)BACKGROUND,  0, WIDTH * HEIGHT * 3);
    memset((void *)MENU,        0, WIDTH * HEIGHT * 3);
    memset((void *)LEVEL1,      0, WIDTH * HEIGHT * 3);
    memset((void *)LEVEL1,      0, WIDTH * HEIGHT * 3);
    memset((void *)LEVEL1,      0, WIDTH * HEIGHT * 3);
    memset((void *)CREDITS,     0, WIDTH * HEIGHT * 3);
    memset((void *)PSMALL,      0, SPRITE_SIZE_S * SPRITE_SIZE_S * 3);
    memset((void *)PBIG,        0, SPRITE_SIZE_B * SPRITE_SIZE_B * 3);
    memset((void *)SHRINK,      0, SPRITE_SIZE_S * SPRITE_SIZE_S * 3);
    memset((void *)GROW,        0, SPRITE_SIZE_S * SPRITE_SIZE_S * 3);
}


/****************************************************************************/
/**
* Loads background, fog, and initializes sprites based on level
*
* @param	none
*
* @return	none
*
* @note		sprites should be loaded first before background
*
*****************************************************************************/

void initiateLevel()
{
    colorBGR* levels_BGR[5] = {MENU,LEVEL1,LEVEL2,LEVEL3,CREDITS}; //to be changed
    colorBGR*  bgmem_BGR = BACKGROUND;

    for(int y =1; y<719; y++){
        for(int x=1; x<1279; x++){
            bgmem_BGR[WIDTH * y + x]= levels_BGR[level][WIDTH * y + x];
        }
    }

    for (uint16_t i = 0; i < shrink_count[level]; i++)
    {
        // Place shrink on x and y note one for each instance
        load_shrink(shrink_xc[level][i], shrink_yc[level][i]);
    }

    // Load grow sprites
    for (uint16_t i = 0; i < grow_count[level]; i++)
    {
        // Place grow on x and y note one for each instance
        load_grow(grow_xc[level][i], grow_yc[level][i]);
    }

    // Copy background to framebuffer
    load_bg();

    // Initiates moving sprite starting coordinate
    xc = startX[level];
    yc = startY[level];
    // Load initial fog
	if (level != 0 && level != 4 && !debugMode) load_fog();
}

/****************************************************************************/
/**
* Copy background to framebuffer
*
* @param	none
*
* @return	none
*
*****************************************************************************/
void load_bg() {
    colorBGR* bgmem_BGR = BACKGROUND;
    colorBGR* fb_BGR = FRAMEBUFFER_BASEADDR;

    for(int y = 1; y < 719; y++){
        for(int x = 1; x < 1279; x++){
             fb_BGR[WIDTH * y + x]= bgmem_BGR[WIDTH * y + x];
        }
    }
}

/****************************************************************************/
/**
* Patch the background display of previous location of main_sprite
*
* @param	dimension of main_sprite
*
* @return	none
*
*****************************************************************************/
void patch(uint8_t dimension)
{
    colorBGR* bgmem_BGR = BACKGROUND;
    colorBGR* fb_BGR = FRAMEBUFFER_BASEADDR;
	for (int y = 0; y < size[dimension]; y++)
	{
		for (int x = 0; x < size[dimension]; x++)
		{
			fb_BGR[WIDTH * (yc + y - size[dimension] / 2) + (xc + x - size[dimension] / 2)] = bgmem_BGR[WIDTH * (yc + y - size[dimension] / 2) + (xc + x - size[dimension] / 2)];
		}
	}
}


/****************************************************************************/
/**
* This overwrites the memory of the background with the grow sprite depending on the
* specified x and y coordinates
*
* @param	xc and yc of sprite on map
*
* @return	none
*
* @note		call this function for each grow sprite to be placed*
*
*****************************************************************************/
void load_grow(uint16_t sprite_xc, uint16_t sprite_yc)
{
	colorBGR* grow_BGR = GROW;
	colorBGR* bgmem_BGR = BACKGROUND;

	for (int y = 0; y < size[3]; y++)
	{
		for (int x = 0; x < size[3]; x++)
		{
			bgmem_BGR[WIDTH * (sprite_yc + y - size[3] / 2) + (sprite_xc + x - size[3] / 2)] = grow_BGR[size[3] * y + x];
		}
	}
}

/****************************************************************************/
/**
* This overwrites the memory of the background with the shrink sprite depending on the
* specified x and y coordinates
*
* @param	xc and yc of sprite on map
*
* @return	none
*
* @note		call this function for each grow sprite to be placed
*
*****************************************************************************/
void load_shrink(uint16_t sprite_xc, uint16_t sprite_yc)
{
	colorBGR* shrink_BGR = SHRINK;
	colorBGR* bgmem_BGR = BACKGROUND;

	for (int y = 0; y < size[3]; y++)
	{
		for (int x = 0; x < size[3]; x++)
		{
			bgmem_BGR[WIDTH * (sprite_yc + y - size[3] / 2) + (sprite_xc + x - size[3] / 2)] = shrink_BGR[size[3] * y + x];
		}
	}
}

/****************************************************************************/
/**
* Checks if x and y coordinate of element fog is within the set
* boundaries of the window (WIDTH x HEIGHT)
*
* @param	x refers to the x-coordinate of a given fog
*			y refers to the y-coordinate of a given fog
*
* @return	return if point(x,y) exceeds boundary in four directions
*
*****************************************************************************/
uint8_t is_inbounds(uint16_t x, uint16_t y) {
	uint8_t w = x >= 0;			// left-most
	uint8_t e = x < WIDTH;		// right-most
	uint8_t n = y >= 0;			// top-most
	uint8_t s = y < HEIGHT;		// bottom-most
	return n && e && w && s;
}

/****************************************************************************/
/**
* Checks if sprite can move to pos(x,y)
*
* @param	nxt_xc refers to the x-coordinate if sprite moves east/west
*			nxt_yc refers to the y-coordinate if sprite moves north/south
*			dimension is the size of the sprite
*
* @return	whether or not sprite collides with walls
*
*****************************************************************************/
uint8_t can_move2posxy(uint16_t nxt_xc, uint16_t nxt_yc, uint16_t dimension) {
	colorBGR* fb_BGR = FRAMEBUFFER_BASEADDR;
	//(0x5a6988)
	colorBGR wallColor;
	wallColor.BLUE = 0x88;
	wallColor.GREEN = 0x69;
	wallColor.RED = 0x5a;
	for (int y = 0; y < size[dimension]; y++)
	{
		for (int x = 0; x < size[dimension]; x++)
		{
			colorBGR pixel_color = fb_BGR[WIDTH * (nxt_yc + y - size[dimension] / 2) + (nxt_xc + x - size[dimension] / 2)];
			if (pixel_color.BLUE == wallColor.BLUE && pixel_color.GREEN == wallColor.GREEN && pixel_color.RED == wallColor.RED) return 0;
		}
	}
	return 1;
}


/****************************************************************************/
/**
* Checks what to do when touching event block at menu
*
* @param	nxt_xc refers to the x-coordinate if sprite moves east/west
*			nxt_yc refers to the y-coordinate if sprite moves north/south
*			dimension is the size of the sprite
*			window is the miniFB window structure
*
* @return	none
*
*
*****************************************************************************/
void isEvent(uint16_t nxt_xc, uint16_t nxt_yc, uint16_t dimension)
{
	/*
	 * #define STAGE1COLOR	 				(0x0076ff)
		#define	STAGE2COLOR	 				(0x7a009f)
		#define	STAGE3COLOR	 				(0xf65058)
		#define	FINISHCOLOR	 				(0xff0044)
		#define WALLCOLOR	 				(0x5a6988)
	 */

	colorBGR* fb_BGR = FRAMEBUFFER_BASEADDR;
	colorBGR stage1, stage2, stage3, finish;
	stage1.BLUE = 0xff;
	stage1.GREEN = 0x76;
	stage1.RED = 0x00;

	stage2.BLUE = 0x9f;
	stage2.GREEN = 0x00;
	stage2.RED = 0x7a;

	stage3.BLUE = 0x58;
	stage3.GREEN = 0x50;
	stage3.RED = 0xf6;

	finish.BLUE = 0x44;
	finish.GREEN = 0x00;
	finish.RED = 0xff;

	for (int y = 0; y < size[dimension]; y++)
	{
		for (int x = 0; x < size[dimension]; x++)
		{
			colorBGR pixel_color = fb_BGR[WIDTH * (nxt_yc + y - size[dimension] / 2) + (nxt_xc + x - size[dimension] / 2)];
			if (level == 0)
			{
				// Exit game
				/*if (pixel_color == EXIT) mfb_close(window);*/
				// 1st Level block
				/*else*/ if (pixel_color.BLUE == stage1.BLUE && pixel_color.GREEN == stage1.GREEN && pixel_color.RED == stage1.RED)
				{
					level = 1;
					current = stageInitial[level];
					initiateLevel();
				}
				// 2nd Level block
				else if (pixel_color.BLUE == stage2.BLUE && pixel_color.GREEN == stage2.GREEN && pixel_color.RED == stage2.RED)
				{
					level = 2;
					current = stageInitial[level];
					initiateLevel();
				}
				// 3rd Level block
				else if (pixel_color.BLUE == stage3.BLUE && pixel_color.GREEN == stage3.GREEN && pixel_color.RED == stage3.RED)
				{
					level = 3;
					current = stageInitial[level];
					initiateLevel();
				}
			}
			else if (level != 0)
			{
				// Return to menu
				if (pixel_color.BLUE == finish.BLUE && pixel_color.GREEN == finish.GREEN && pixel_color.RED == finish.RED)
				{
					level++;
					if (level > 4)level = 0;
					current = stageInitial[level];
					initiateLevel();
				}
			}

		}
	}
}

/****************************************************************************/
/**
* Load four (4) black rectangles around the main_sprite, creating
* a fog of war to make the maze more difficult to traverse
*
* @param	none
*
* @return	none
*
* @note		allowance can be resized above (see Sprite Attributes)
* 			y,x init value set to 1 for converted version, max set to H/W -1 (correct?)
*****************************************************************************/
void load_fog()
{
	colorBGR* fb_BGR = FRAMEBUFFER_BASEADDR;

	for (int y = 1; y < HEIGHT-1; y++)
	{
		for (int x = 1; x < WIDTH-1; x++)
		{
			uint8_t north = y >= 0 && y <= yc - allowance - 1;
			uint8_t south = y >= yc + allowance;
			uint8_t east = x >= xc + allowance;
			uint8_t west = x >= 0 && x <= xc - allowance - 1;

			if (north || south || east || west)
			{
				fb_BGR[WIDTH * y + x].BLUE = 0x00;
				fb_BGR[WIDTH * y + x].GREEN = 0x00;
				fb_BGR[WIDTH * y + x].RED = 0x00;
			}
		}
	}
}

/****************************************************************************/
/**
* update the fog display at the north of main_sprite if not on debug mode
*
* @param	debug_mode
*
* @return	none
*
* @note		issue fixed
*
*****************************************************************************/
void update_fog_north(uint8_t debug_mode) {
    colorBGR* bgmem_BGR = BACKGROUND;
    colorBGR* fb_BGR = FRAMEBUFFER_BASEADDR;

	if (!debug_mode) {
		for (int y = yc - allowance; y < yc - allowance + dy; y++)
		{
			for (int x = xc - allowance; x < xc + allowance; x++)
			{
				if (is_inbounds(x, y)) {
					fb_BGR[WIDTH * y + x] = bgmem_BGR[WIDTH * y + x];
				}
			}
		}

		// Updates the tail (the tail is south)
		for (int y = yc + allowance; y < yc + allowance + dy; y++)
		{
			for (int x = xc - allowance; x < xc + allowance; x++)
			{
				if (is_inbounds(x, y)) {
					fb_BGR[WIDTH * y + x].BLUE = 0x00;
					fb_BGR[WIDTH * y + x].GREEN = 0x00;
					fb_BGR[WIDTH * y + x].RED = 0x00;
				}
			}
		}
	}
}

/****************************************************************************/
/**
* update the fog display at the south of main_sprite if not on debug mode
*
* @param	debug_mode
*
* @return	none
*
* @note		none, issues fixed
*
*****************************************************************************/
void update_fog_south(uint8_t debug_mode) {
    colorBGR* bgmem_BGR = BACKGROUND;
    colorBGR* fb_BGR = FRAMEBUFFER_BASEADDR;
	if (!debug_mode) {
		for (int y = yc + allowance - dy; y < yc + allowance; y++)
		{
			for (int x = xc - allowance; x < xc + allowance; x++)
			{
				if (is_inbounds(x, y)) {
					fb_BGR[WIDTH * y + x] = bgmem_BGR[WIDTH * y + x];
				}
			}
		}

		// Updates the tail (the tail is north)
		for (int y = yc - allowance - dy; y < yc - allowance; y++)
		{
			for (int x = xc - allowance; x < xc + allowance; x++)
			{
				if (is_inbounds(x, y)) {
					fb_BGR[WIDTH * y + x].BLUE = 0x00;
					fb_BGR[WIDTH * y + x].GREEN = 0x00;
					fb_BGR[WIDTH * y + x].RED = 0x00;
				}
			}
		}
	}
}

/****************************************************************************/
/**
* update the fog display at the east of main_sprite if not on debug mode
*
* @param	debug_mode
*
* @return	none
*
*****************************************************************************/
void update_fog_east(uint8_t debug_mode) {
    colorBGR* bgmem_BGR = BACKGROUND;
    colorBGR* fb_BGR = FRAMEBUFFER_BASEADDR;
	if (!debug_mode) {
		for (int y = yc - allowance; y < yc + allowance; y++)
		{
			for (int x = xc + allowance - dx; x < xc + allowance; x++)
			{
				if (is_inbounds(x, y)) {
					fb_BGR[WIDTH * y + x] = bgmem_BGR[WIDTH * y + x];
				}
			}
		}

		// Updates the tail (left side is tail)
		for (int y = yc - allowance; y < yc + allowance; y++)
		{
			for (int x = xc - allowance - dy; x < xc - allowance; x++)
			{
				if (is_inbounds(x, y)) {
					fb_BGR[WIDTH * y + x].BLUE = 0x00;
					fb_BGR[WIDTH * y + x].GREEN = 0x00;
					fb_BGR[WIDTH * y + x].RED = 0x00;
				}
			}
		}
	}
}

/****************************************************************************/
/**
* update the fog display at the west of main_sprite if not on debug mode
*
* @param	debug_mode
*
* @return	none
*
*****************************************************************************/
void update_fog_west(uint8_t debug_mode) {
    colorBGR* bgmem_BGR = BACKGROUND;
    colorBGR* fb_BGR = FRAMEBUFFER_BASEADDR;
	if (!debug_mode) {
		for (int y = yc - allowance; y < yc + allowance; y++)
		{
			for (int x = xc - allowance; x < xc - allowance + dx; x++)
			{
				if (is_inbounds(x, y)) {
					fb_BGR[WIDTH * y + x] = bgmem_BGR[WIDTH * y + x];
				}
			}
		}

		// Updates the tail (the right is now the tail)
		for (int y = yc - allowance; y < yc + allowance; y++)
		{
			for (int x = xc + allowance; x < xc + allowance + dx; x++)
			{
				if (is_inbounds(x, y)) {
					fb_BGR[WIDTH * y + x].BLUE = 0x00;
					fb_BGR[WIDTH * y + x].GREEN = 0x00;
					fb_BGR[WIDTH * y + x].RED = 0x00;
				}
			}
		}
	}
}

/****************************************************************************/
/**
* This detects if the current sprite size and location overlaps the grow sprite
* based on the specified x and y coordinates.
*
* @param	location of grow sprite
*
* @return	none
*
* @note
*
*****************************************************************************/
void touch_grow(uint16_t sprite_xc, uint16_t sprite_yc)
{
	if ((xc + size[current] / 2 > sprite_xc - size[3] / 2 && xc + size[current] / 2 < sprite_xc + size[3] / 2) || (xc + size[current] / 2 > sprite_xc - size[3] / 2 && xc - size[current] / 2 < sprite_xc + size[3] / 2) && !current)
	{
		if ((yc + size[current] / 2 > sprite_yc - size[3] / 2 && yc + size[current] / 2 < sprite_yc + size[3] / 2) || (yc + size[current] / 2 > sprite_yc - size[3] / 2 && yc - size[current] / 2 < sprite_yc + size[3] / 2))
		{
			current = 1;
			prev = 0;
		}
	}
}

/****************************************************************************/
/**
* This detects if the current sprite size and location overlaps the shrink sprite
* based on the specified x and y coordinates.
*
* @param	location of shrink sprite
*
* @return	none
*
* @note
*
*****************************************************************************/
void touch_shrink(uint16_t sprite_xc, uint16_t sprite_yc)
{
	if ((xc + size[current] / 2 > sprite_xc - size[2] / 2 && xc + size[current] / 2 < sprite_xc + size[2] / 2) || (xc + size[current] / 2 > sprite_xc - size[2] / 2 && xc - size[current] / 2 < sprite_xc + size[2] / 2) && current)
	{
		if ((yc + size[current] / 2 > sprite_yc - size[2] / 2 && yc + size[current] / 2 < sprite_yc + size[2] / 2) || (yc + size[current] / 2 > sprite_yc - size[2] / 2 && yc - size[current] / 2 < sprite_yc + size[2] / 2))
		{
			patch(current);
			current = 0;
			prev = 1;

		}
	}
}

/****************************************************************************/
/**
* Update main sprite display
*
* @param	none
*
* @return	none
*
*****************************************************************************/
void update_main_sprite(uint8_t sizeIndex, uint8_t direction)
{

	colorBGR* main_BGR[2] = {PSMALL,PBIG};
    colorBGR* levels_BGR[5] = {MENU,LEVEL1,LEVEL2,LEVEL3,CREDITS};
    colorBGR* fb_BGR = FRAMEBUFFER_BASEADDR;

	colorBGR pixel_bg;
	colorBGR pixel_sprite;

	// Copy the sprite to the window
	for (int y = 0; y < size[sizeIndex]; y++)
	{
		for (int x = 0; x < size[sizeIndex]; x++)
		{
			// Get the RGB data from the background
			pixel_bg.BLUE= levels_BGR[level][WIDTH * (yc + y - size[sizeIndex] / 2) + (xc + x - size[sizeIndex] / 2)].BLUE;
			pixel_bg.GREEN= levels_BGR[level][WIDTH * (yc + y - size[sizeIndex] / 2) + (xc + x - size[sizeIndex] / 2)].GREEN;
			pixel_bg.RED= levels_BGR[level][WIDTH * (yc + y - size[sizeIndex] / 2) + (xc + x - size[sizeIndex] / 2)].RED;

			//pixel_sprite.BLUE= levels_BGR[level][WIDTH * (yc + y - size[sizeIndex] / 2) + (xc + x - size[sizeIndex] / 2)].BLUE;
			//pixel_sprite.GREEN= levels_BGR[level][WIDTH * (yc + y - size[sizeIndex] / 2) + (xc + x - size[sizeIndex] / 2)].GREEN;
			//pixel_sprite.RED= levels_BGR[level][WIDTH * (yc + y - size[sizeIndex] / 2) + (xc + x - size[sizeIndex] / 2)].RED;



			if (direction == 0)
			{
				pixel_sprite.BLUE = main_BGR[sizeIndex][size[sizeIndex] * y + x].BLUE;
				pixel_sprite.GREEN = main_BGR[sizeIndex][size[sizeIndex] * y + x].GREEN;
				pixel_sprite.RED = main_BGR[sizeIndex][size[sizeIndex] * y + x].RED;
			}
			else
			{
				pixel_sprite.BLUE = main_BGR[sizeIndex][size[sizeIndex] * y + (size[sizeIndex] - 1) - x].BLUE;
				pixel_sprite.GREEN = main_BGR[sizeIndex][size[sizeIndex] * y + (size[sizeIndex] - 1) - x].GREEN;
				pixel_sprite.RED = main_BGR[sizeIndex][size[sizeIndex] * y + (size[sizeIndex] - 1) - x].RED;
			}


			if (pixel_sprite.BLUE == 0 && pixel_sprite.GREEN == 0 && pixel_sprite.RED == 0)
				fb_BGR[WIDTH * (yc + y - size[sizeIndex] / 2) + (xc + x - size[sizeIndex] / 2)] = pixel_bg;

			else
				fb_BGR[WIDTH * (yc + y - size[sizeIndex] / 2) + (xc + x - size[sizeIndex] / 2)] = pixel_sprite;
		}
	}
}


/*************************** Handles keypress *******************************/
void btn_interrupt(void *callback_ref)
{
	XGpio *btn = callback_ref;
	uint8_t key =  XGpio_DiscreteRead(btn, 1);


	if (key == ARROW_BTN_CENTER)
	{
		current = stageInitial[level];
		initiateLevel();
	}
	XGpio_InterruptClear(btn, XGPIO_IR_MASK);
}

void timer_interrupt(void *callback_ref)
{
	XGpio *btn = callback_ref;
	uint8_t key =  XGpio_DiscreteRead(btn, 1);

	if (key == ARROW_BTN_LEFT)
	{
		patch(current);
		if (can_move2posxy(xc - dx, yc, current)==1) {

			facing = 1;
			xc -= dx;
			if (level != 0 && level != 4)update_fog_west(debugMode);

		}
	}
	else if (key == ARROW_BTN_RIGHT)
	{
		facing = 0;
		patch(current);
		if (can_move2posxy(xc + dx, yc, current)==1) {
			xc += dx;
			if (level != 0 && level != 4)update_fog_east(debugMode);
		}
	}
	else if (key == ARROW_BTN_UP)
	{
		patch(current);
		if (can_move2posxy(xc, yc - dy, current)==1) {

			yc -= dy;
			if (level != 0 && level != 4)update_fog_north(debugMode);
		}
	}
	else if (key == ARROW_BTN_DOWN)
	{
		patch(current);
		if (can_move2posxy(xc, yc + dy, current)==1) {

			yc += dy;
			if (level != 0 && level != 4)update_fog_south(debugMode);
		}

	}
	isEvent(xc, yc, current);
}
