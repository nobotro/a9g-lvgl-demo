#include "arst7789.h"
#include "api_hal_gpio.h"
#include "api_hal_pm.h"
#include "api_hal_spi.h"
#include "api_os.h"
#include "api_debug.h"




uint8_t  _colstart, _rowstart, _xstart, _ystart,_width,_height; // some displays need this changed
GPIO_PIN  _dc, _rst;
SPI_ID_t _spi;
uint8_t* buff ;

SPI_Config_t spi_cfg =
    {
	    
        .freq = 40000000,//40M here
        
         .cs = SPI_CS_0,
       .txMode = SPI_MODE_DMA_POLLING,
       .rxMode = SPI_MODE_DMA_POLLING,
       
       
        .line = SPI_LINE_4,
        .txOnly =true,
        .cpol = 1,
        .cpha = 0,
        .csActiveLow = true,
        .dataBits = SPI_DATA_BITS_8
        
         
    };

uint8_t cmd_240x240[] = {                 		// Initialization commands for 7789 screens
    10,                       				// 9 commands in list:
    ST7789_SWRESET,   ST_CMD_DELAY,  		// 1: Software reset, no args, w/delay
      150,                     				// 150 ms delay
    ST7789_SLPOUT ,   ST_CMD_DELAY,  		// 2: Out of sleep mode, no args, w/delay
      255,                    				// 255 = 500 ms delay
    ST7789_COLMOD , 1+ST_CMD_DELAY,  		// 3: Set color mode, 1 arg + delay:
      0x55,                   				// 16-bit color
      10,                     				// 10 ms delay
    ST7789_MADCTL , 1,  					// 4: Memory access ctrl (directions), 1 arg:
      0x00,                   				// Row addr/col addr, bottom to top refresh
    ST7789_CASET  , 4,  					// 5: Column addr set, 4 args, no delay:
      0x00, ST7789_240x240_XSTART,          // XSTART = 0
	  (240+ST7789_240x240_XSTART) >> 8,
	  (240+ST7789_240x240_XSTART) & 0xFF,   // XEND = 240
    ST7789_RASET  , 4,  					// 6: Row addr set, 4 args, no delay:
      0x00, ST7789_240x240_YSTART,          // YSTART = 0
      (240+ST7789_240x240_YSTART) >> 8,
	  (240+ST7789_240x240_YSTART) & 0xFF,	// YEND = 240
    ST7789_INVON ,   ST_CMD_DELAY,  		// 7: Inversion ON
      10,
    ST7789_NORON  ,   ST_CMD_DELAY,  		// 8: Normal display on, no args, w/delay
      10,                     				// 10 ms delay
    ST7789_DISPON ,   ST_CMD_DELAY,  		// 9: Main screen turn on, no args, w/delay
    255 };                  				// 255 = 500 ms delay

void scan()
{
	 
	  
	 DC_HIGH();
	 
  /* OS_Sleep(10);
  
   int i=0;
  for(int y=240; y>0; y--) {
    for(int x=240; x>0; x--) {
		
		writedata(buff[i++]);
		writedata(buff[i++]);
		 
       
    }
  }
  
  */
   
	 //OS_Sleep(10);
  
  
	 while(!SPI_IsTxDmaDone(_spi));
 SPI_Write(_spi, buff, 115200);
 
  while(!SPI_IsTxDmaDone(_spi));
   DC_LOW();
  //OS_Sleep(20);
	
}




void ST7789_Close()
{
	 GPIO_Set(_rst,1);	  
    OS_Sleep(50);
    GPIO_Set(_rst,0);
    OS_Sleep(50);
    GPIO_Set(_rst,1);
    OS_Sleep(50);
    
	
	displayInit(cmd_240x240);
	setRotation(2);
}


void ST7789_Init(GPIO_PIN dc, GPIO_PIN rst,SPI_ID_t spi,uint8_t* buffp) 
{
 
  _dc   = dc;
  _rst  = rst;
  _spi=spi;
  buff=buffp;
 
  
  
}



void writecommand(uint8_t c) {

  DC_LOW();
  SPI_Write(_spi,&c, 1);

 
}

void writedata(uint8_t c) {
  
  DC_HIGH();
  SPI_Write(_spi, &c, 1);


  
}

// Companion code to the above tables.  Reads and issues
// a series of LCD commands stored in PROGMEM byte array.
void displayInit(uint8_t *addr) {

  uint8_t  numCommands, numArgs;
  uint16_t ms;

  
  numCommands = *(addr++);   // Number of commands to follow
  while(numCommands--) {                 // For each command...
    writecommand(*(addr++)); //   Read, issue command
    numArgs  = *(addr++);    //   Number of args to follow
    ms       = numArgs & ST_CMD_DELAY;   //   If hibit set, delay follows args
    numArgs &= ~ST_CMD_DELAY;            //   Mask out delay bit
    while(numArgs--) {                   //   For each argument...
      writedata(*(addr++));  //     Read, issue argument
    }

    if(ms) {
      ms = *(addr++); // Read post-command delay time (ms)
      if(ms == 255) ms = 500;     // If 255, delay for 500 ms
      OS_Sleep(ms);
    }
  }
}


// Initialization code common to all ST7789 displays
void commonInit() {
  _ystart = _xstart = 0;
  _colstart  = _rowstart = 0; // May be overridden in init func

  
  
//PM_PowerEnable(POWER_TYPE_VPAD,true);



   GPIO_config_t pin = {
        .mode         = GPIO_MODE_OUTPUT,
        .pin          = _dc,
        .defaultLevel = GPIO_LEVEL_LOW
    };
	 GPIO_Init(pin);
	 
	
	  SPI_Init(_spi, spi_cfg);
	 
	 
    pin.pin = _rst;
	GPIO_Init(pin);
 
   
     GPIO_Set(_rst,1);	  
    OS_Sleep(50);
    GPIO_Set(_rst,0);
    OS_Sleep(50);
    GPIO_Set(_rst,1);
    OS_Sleep(50);
    

	DC_LOW();
	
	
 
  
}

void setRotation(uint8_t m) {

  writecommand(ST7789_MADCTL);
  int rotation = m % 4; // can't be higher than 3
  switch (rotation) {
   case 0:
     writedata(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);

     _xstart = _colstart;
     _ystart = _rowstart;
     break;
   case 1:
     writedata(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);

     _ystart = _colstart;
     _xstart = _rowstart;
     break;
  case 2:
     writedata(ST7789_MADCTL_RGB);
 
     _xstart = _colstart;
     _ystart = _rowstart;
     break;

   case 3:
     writedata(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);

     _ystart = _colstart;
     _xstart = _rowstart;
     break;
  }
}

void setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1,uint8_t y1) {

  uint16_t x_start = x0 + _xstart, x_end = x1 + _xstart;
  uint16_t y_start = y0 + _ystart, y_end = y1 + _ystart;
  

  writecommand(ST7789_CASET); // Column addr set
  writedata(x_start >> 8);
  writedata(x_start & 0xFF);     // XSTART 
  writedata(x_end >> 8);
  writedata(x_end & 0xFF);     // XEND

  writecommand(ST7789_RASET); // Row addr set
  writedata(y_start >> 8);
  writedata(y_start & 0xFF);     // YSTART
  writedata(y_end >> 8);
  writedata(y_end & 0xFF);     // YEND

  writecommand(ST7789_RAMWR); // write to RAM
}


// fill a rectangle
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h,uint16_t color) {

  // rudimentary clipping (drawChar w/big text requires this)
  if((x >= _width) || (y >= _height)) return;
  if((x + w - 1) >= _width)  w = _width  - x;
  if((y + h - 1) >= _height) h = _height - y;

  setAddrWindow(x, y, x+w-1, y+h-1);

  uint8_t hi = color >> 8, lo = color;
    

  
 int i=0;
  for(y=h; y>0; y--) {
    for(x=w; x>0; x--) {
		
		buff[i++]=hi;
		buff[i++]=lo;
       
    }
  }
 
}


void DC_HIGH(void) {

 GPIO_Set(_dc,1);	  

}

void DC_LOW(void) {

 GPIO_Set(_dc,0);	  

}

void fillScreen(uint16_t color) {
  fillRect(0, 0,  _width, _height, color);
}

void init(uint16_t width, uint16_t height) {
  commonInit();

  _colstart = ST7789_240x240_XSTART;
  _rowstart = ST7789_240x240_YSTART;
  _height = 240;
  _width = 240;
 

 displayInit(cmd_240x240);

  setRotation(2);
  


 
}


