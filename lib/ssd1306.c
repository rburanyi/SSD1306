#define pgm_read_byte *
typedef unsigned char uint8_t;
#define PROGMEM
unsigned int _counter;

#define USE_I2C_DEVICE 0
#define USE_I2CMINI 1

/**
 * --------------------------------------------------------------------------------------+
 * @desc        SSD1306 OLED Driver
 * --------------------------------------------------------------------------------------+
 *              Copyright (C) 2020 Marian Hrinko.
 *              Written by Marian Hrinko (mato.hrinko@gmail.com)
 *
 * @author      Marian Hrinko
 * @date        06.10.2020
 * @update      06.12.2022
 * @file        ssd1306.c
 * @version     2.0.0
 * @tested      AVR Atmega328p
 *
 * @depend      ssd1306.h
 * --------------------------------------------------------------------------------------+
 * @descr       Version 1.0.0 -> applicable for 1 display
 *              Version 2.0.0 -> rebuild to 'cacheMemLcd' array
 *              Version 3.0.0 -> simplified alphanumeric version for 1 display
 * --------------------------------------------------------------------------------------+
 * @usage       Basic Setup for OLED Display
 */
 
// @includes
#include "ssd1306.h"

#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <arpa/inet.h>
#include <stdlib.h>

#if USE_I2CMINI
  #include "i2cdriver.h"
#endif

// +---------------------------+
// |      Set MUX Ratio        |
// +---------------------------+
// |        0xA8, 0x3F         |
// +---------------------------+
//              |
// +---------------------------+
// |    Set Display Offset     |
// +---------------------------+
// |        0xD3, 0x00         |
// +---------------------------+
//              |
// +---------------------------+
// |  Set Display Start Line   |
// +---------------------------+
// |          0x40             |
// +---------------------------+
//              |
// +---------------------------+
// |     Set Segment Remap     |
// +---------------------------+
// |       0xA0 / 0xA1         |
// +---------------------------+
//              |
// +---------------------------+
// |   Set COM Output Scan     |
// |        Direction          |
// +---------------------------+
// |       0xC0 / 0xC8         |
// +---------------------------+
//              |
// +---------------------------+
// |   Set COM Pins hardware   |
// |       configuration       |
// +---------------------------+
// |        0xDA, 0x02         |
// +---------------------------+
//              |
// +---------------------------+
// |   Set Contrast Control    |
// +---------------------------+
// |        0x81, 0x7F         |
// +---------------------------+
//              |
// +---------------------------+
// | Disable Entire Display On |
// +---------------------------+
// |          0xA4             |
// +---------------------------+
//              |
// +---------------------------+
// |    Set Normal Display     |
// +---------------------------+
// |          0xA6             |
// +---------------------------+
//              |
// +---------------------------+
// |    Set Osc Frequency      |
// +---------------------------+
// |       0xD5, 0x80          |
// +---------------------------+
//              |
// +---------------------------+
// |    Enable charge pump     |
// |        regulator          |
// +---------------------------+
// |       0x8D, 0x14          |
// +---------------------------+
//              |
// +---------------------------+
// |        Display On         |
// +---------------------------+
// |          0xAF             |
// +---------------------------+

// @array Init command
const uint8_t INIT_SSD1306[] PROGMEM = {
  18,                                                             // number of initializers
  0, SSD1306_DISPLAY_OFF,                                         // 0xAE = Set Display OFF
  1, SSD1306_SET_MUX_RATIO, 63,                                   // 0xA8 - 64MUX for 128 x 64 version
                                                                  //      - 32MUX for 128 x 32 version
  1, SSD1306_MEMORY_ADDR_MODE, 0x00,                              // 0x20 = Set Memory Addressing Mode
                                                                  // 0x00 - Horizontal Addressing Mode
                                                                  // 0x01 - Vertical Addressing Mode
                                                                  // 0x02 - Page Addressing Mode (RESET)
  2, SSD1306_SET_COLUMN_ADDR, START_COLUMN_ADDR, END_COLUMN_ADDR, // 0x21 = Set Column Address, 0 - 127
  2, SSD1306_SET_PAGE_ADDR, START_PAGE_ADDR, END_PAGE_ADDR,       // 0x22 = Set Page Address, 0 - 7
  0, SSD1306_SET_START_LINE,                                      // 0x40
  1, SSD1306_DISPLAY_OFFSET, 0x00,                                // 0xD3
  0, SSD1306_SEG_REMAP_OP,                                        // 0xA0 / remap 0xA1
  0, SSD1306_COM_SCAN_DIR_OP,                                     // 0xC0 / remap 0xC8
  1, SSD1306_COM_PIN_CONF, 0x12, /* 0x12 */                       // 0xDA, 0x12 - Disable COM Left/Right remap, Alternative COM pin configuration
                                                                  //       0x12 - for 128 x 64 version
                                                                  //       0x02 - for 128 x 32 version
  1, SSD1306_SET_CONTRAST, 0x7F,                                  // 0x81, 0x7F - reset value (max 0xFF)
  0, SSD1306_DIS_ENT_DISP_ON,                                     // 0xA4
  0, SSD1306_DIS_NORMAL,                                          // 0xA6
  1, SSD1306_SET_OSC_FREQ, 0x80,                                  // 0xD5, 0x80 => D=1; DCLK = Fosc / D <=> DCLK = Fosc
  1, SSD1306_SET_PRECHARGE, 0xc2,                                 // 0xD9, higher value less blinking
                                                                  // 0xC2, 1st phase = 2 DCLK,  2nd phase = 13 DCLK
  1, SSD1306_VCOM_DESELECT, 0x20,                                 // Set V COMH Deselect, reset value 0x22 = 0,77xUcc
  1, SSD1306_SET_CHAR_REG, 0x14,                                  // 0x8D, Enable charge pump during display on
  0, SSD1306_DISPLAY_ON                                           // 0xAF = Set Display ON
};

// @var array Chache memory Lcd 8 * 128 = 1024
static char cacheMemLcd[CACHE_SIZE_MEM];

#if USE_I2C_DEVICE
  static int fd = -1;
#endif

#if USE_I2CMINI
  I2CDriver i2c;
#endif

/**
 * @desc    SSD1306 Init
 *
 * @param   uint8_t address
 *
 * @return  uint8_t
 */
uint8_t SSD1306_Init (uint8_t address)
{ 
  // variables
  const uint8_t *commands = INIT_SSD1306;
  // number of commands
  unsigned short int no_of_commands = pgm_read_byte(commands++);
  // argument
  uint8_t no_of_arguments;
  // command
  uint8_t command;
  // init status
  uint8_t status = INIT_STATUS;

#if USE_I2C_DEVICE 
  fd = open("/dev/i2c-1", O_RDWR);
  if (fd == -1)
  {
    perror("could not open /dev/i2c-1");
    return -1;
  }
#endif

#if USE_I2CMINI
  i2c_connect(&i2c, "/dev/ttyUSB0");
  if (!i2c.connected)
  {
    perror("could not open /dev/ttyUSB0");
    return -1;
  }
#endif

  // loop through commands
  while (no_of_commands) {

    // number of arguments
    no_of_arguments = pgm_read_byte (commands++);
    // command
    command = pgm_read_byte (commands++);

    // send command
    // -------------------------------------------------------------------------------------
    status = SSD1306_Send_Command (command);
    // request - start TWI
    if (SSD1306_SUCCESS != status) {
      // error
      return status;
    }

    // send arguments
  // -------------------------------------------------------------------------------------
    while (no_of_arguments--) {
      // send command
      status = SSD1306_Send_Command (pgm_read_byte(commands++));
      // request - start TWI
      if (SSD1306_SUCCESS != status) {
        // error
        return status;
      }
    }
    // decrement
    no_of_commands--;
  }

  // success
  return SSD1306_SUCCESS;
}

/**
 * @desc    SSD1306 Send Start and SLAW request
 *
 * @param   uint8_t
 *
 * @return  uint8_t
 */
uint8_t SSD1306_Send_StartAndSLAW (uint8_t address)
{
  // success
  return SSD1306_SUCCESS;
}

/**
 * @desc    SSD1306 Send command
 *
 * @param   uint8_t command
 *
 * @return  uint8_t
 */
uint8_t SSD1306_Send_Command (uint8_t command)
{
#if USE_I2C_DEVICE
  uint8_t cmd[] = {
    SSD1306_COMMAND, command };

  struct i2c_msg message = { SSD1306_ADDR, 0, sizeof(cmd), cmd };
  struct i2c_rdwr_ioctl_data ioctl_data = { &message, 1 };
  int result = ioctl(fd, I2C_RDWR, &ioctl_data);
  if (result != 1)
  {
    perror("failed to set target");
    return -1;
  }
#endif

#if USE_I2CMINI
  uint8_t dev = SSD1306_ADDR;
  uint8_t cmd[] = {
    SSD1306_COMMAND, command };
  i2c_start(&i2c, dev, 0);
  i2c_write(&i2c, cmd, sizeof(cmd));
  i2c_stop(&i2c);
#endif 

  // success
  return SSD1306_SUCCESS;
}

/**
 * @desc    SSD1306 Normal colors
 *
 * @param   uint8_t address
 *
 * @return  uint8_t
 */
uint8_t SSD1306_NormalScreen (uint8_t address)
{
  // send command
  // -------------------------------------------------------------------------------------   
  uint8_t status = SSD1306_Send_Command (SSD1306_DIS_NORMAL);
  // request succesfull
  if (SSD1306_SUCCESS != status) {
    // error
    return status;
  }

  // success
  return SSD1306_SUCCESS;
}

/**
 * @desc    SSD1306 Inverse colors
 *
 * @param   uint8_t address
 *
 * @return  uint8_t
 */
uint8_t SSD1306_InverseScreen (uint8_t address)
{
  // send command
  // -------------------------------------------------------------------------------------   
  uint8_t status = SSD1306_Send_Command (SSD1306_DIS_INVERSE);
  // request succesfull
  if (SSD1306_SUCCESS != status) {
    // error
    return status;
  }

  // success
  return SSD1306_SUCCESS;
}

/**
 * @desc    SSD1306 Update screen
 *
 * @param   uint8_t address
 *
 * @return  uint8_t
 */
uint8_t SSD1306_UpdateScreen (uint8_t address)
{
#if USE_I2C_DEVICE
  // init status
  uint8_t status = INIT_STATUS;

  uint8_t cmd[CACHE_SIZE_MEM+1] = {SSD1306_DATA_STREAM};
  memcpy(cmd+1, cacheMemLcd, CACHE_SIZE_MEM);

  struct i2c_msg message = { SSD1306_ADDR, 0, sizeof(cmd), cmd };
  struct i2c_rdwr_ioctl_data ioctl_data = { &message, 1 };
  int result = ioctl(fd, I2C_RDWR, &ioctl_data);
  if (result != 1)
  {
    perror("failed to set bitmap");
    return -1;
  }
#endif

#if USE_I2CMINI
  uint8_t dev = SSD1306_ADDR;
  uint8_t cmd[] = {SSD1306_DATA_STREAM};
  i2c_start(&i2c, dev, 0);
  i2c_write(&i2c, cmd, sizeof(cmd));
  i2c_write(&i2c, cacheMemLcd, CACHE_SIZE_MEM);
  i2c_stop(&i2c);
#endif
  // success
  return SSD1306_SUCCESS;
}

/**
 * @desc    SSD1306 Clear screen
 *
 * @param   void
 *
 * @return  void
 */
void SSD1306_ClearScreen (void)
{
  // null cache memory lcd
  memset (cacheMemLcd, 0x00, CACHE_SIZE_MEM);
}

/**
 * @desc    SSD1306 Set position
 *
 * @param   uint8_t column -> 0 ... 127 
 * @param   uint8_t page -> 0 ... 7 or 3 
 *
 * @return  void
 */
void SSD1306_SetPosition (uint8_t x, uint8_t y) 
{
  // calculate counter
  _counter = x + (y << 7);
}

/**
 * @desc    SSD1306 Update text poisition - this ensure that character will not be divided at the end of row, 
 *          the whole character will be depicted on the new row
 *
 * @param   void
 *
 * @return  uint8_t
 */
uint8_t SSD1306_UpdatePosition (void) 
{
  // y / 8
  uint8_t y = _counter >> 7;
  // y % 8
  uint8_t x = _counter - (y << 7);
  // x + character length + 1
  uint8_t x_new = x + CHARS_COLS_LENGTH + 1;

  // check position
  if (x_new > END_COLUMN_ADDR) {
    // if more than allowable number of pages
    if (y > END_PAGE_ADDR) {
      // return out of range
      return SSD1306_ERROR;
    // if x reach the end but page in range
    } else if (y < (END_PAGE_ADDR-1)) {
      // update
      _counter = ((++y) << 7);
    }
  }
 
  // success
  return SSD1306_SUCCESS;
}

/**
 * @desc    SSD1306 Draw character
 *
 * @param   char character
 *
 * @return  uint8_t
 */
uint8_t SSD1306_DrawChar (char character)
{
  // variables
  int i=0;
  const uint8_t map[] = {0x00, // 0000 0000 
                         0x03, // 0000 0011
                         0x0c, // 0000 1100
                         0x0f, // 0000 1111
                         0x30, // 0011 0000
                         0x33, // 0011 0011
                         0x3c, // 0011 1100
                         0x3f, // 0011 1111
                         0xc0, // 1100 0000 
                         0xc3, // 1100 0011
                         0xcc, // 1100 1100
                         0xcf, // 1100 1111
                         0xf0, // 1111 0000
                         0xf3, // 1111 0011
                         0xfc, // 1111 1100
                         0xff, // 1111 1111
  };

  // update text position
  // this ensure that character will not be divided at the end of row, the whole character will be depicted on the new row
  if (SSD1306_UpdatePosition () == SSD1306_ERROR) {
    // error
    return SSD1306_ERROR;
  }

  // loop through 5 bits
  while (i < CHARS_COLS_LENGTH) {
    // read byte 
    uint8_t data = pgm_read_byte(&FONTS[character-32][i++]);
#if 1
    uint8_t upper = map[data & 0x0f];
    uint8_t lower = map[data >> 4];
    cacheMemLcd[_counter] = upper;
    cacheMemLcd[_counter+END_COLUMN_ADDR+1] = lower;
#else
    cacheMemLcd[_counter] = data;
#endif
    _counter++;
  }

  // update position
  _counter++;

  // success
  return SSD1306_SUCCESS;
}

/**
 * @desc    SSD1306 Draw String
 *
 * @param   char * string
 *
 * @return  void
 */
void SSD1306_DrawString (char *str)
{
  // init
  int i = 0;
  // loop through character of string
  while (str[i] != '\0') {
    // draw string
    SSD1306_DrawChar (str[i++]);
  }
}

/**
 * @desc    Draw pixel
 *
 * @param   uint8_t x -> 0 ... MAX_X
 * @param   uint8_t y -> 0 ... MAX_Y
 *
 * @return  uint8_t
 */
uint8_t SSD1306_DrawPixel (uint8_t x, uint8_t y)
{
  uint8_t page = 0;
  uint8_t pixel = 0;

  // if out of range
  if ((x > MAX_X) && (y > MAX_Y)) {
    // out of range
    return SSD1306_ERROR;
  }
  // find page (y / 8)
  page = y >> 3;
  // which pixel (y % 8)
  pixel = 1 << (y - (page << 3));
  // update counter
  _counter = x + (page << 7);
  // save pixel
  cacheMemLcd[_counter++] |= pixel;

  // success
  return SSD1306_SUCCESS;
}

/**
 * @desc    Draw line by Bresenham algoritm
 *  
 * @param   uint8_t x start position / 0 <= cols <= MAX_X-1
 * @param   uint8_t x end position   / 0 <= cols <= MAX_X-1
 * @param   uint8_t y start position / 0 <= rows <= MAX_Y-1 
 * @param   uint8_t y end position   / 0 <= rows <= MAX_Y-1
 *
 * @return  uint8_t
 */
uint8_t SSD1306_DrawLine (uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2)
{
  // determinant
  int16_t D;
  // deltas
  int16_t delta_x, delta_y;
  // steps
  int16_t trace_x = 1, trace_y = 1;

  // delta x
  delta_x = x2 - x1;
  // delta y
  delta_y = y2 - y1;

  // check if x2 > x1
  if (delta_x < 0) {
    // negate delta x
    delta_x = -delta_x;
    // negate step x
    trace_x = -trace_x;
  }

  // check if y2 > y1
  if (delta_y < 0) {
    // negate detla y
    delta_y = -delta_y;
    // negate step y
    trace_y = -trace_y;
  }

  // Bresenham condition for m < 1 (dy < dx)
  if (delta_y < delta_x) {
    // calculate determinant
    D = (delta_y << 1) - delta_x;
    // draw first pixel
    SSD1306_DrawPixel (x1, y1);
    // check if x1 equal x2
    while (x1 != x2) {
      // update x1
      x1 += trace_x;
      // check if determinant is positive
      if (D >= 0) {
        // update y1
        y1 += trace_y;
        // update determinant
        D -= 2*delta_x;    
      }
      // update deteminant
      D += 2*delta_y;
      // draw next pixel
      SSD1306_DrawPixel (x1, y1);
    }
  // for m > 1 (dy > dx)    
  } else {
    // calculate determinant
    D = delta_y - (delta_x << 1);
    // draw first pixel
    SSD1306_DrawPixel (x1, y1);
    // check if y2 equal y1
    while (y1 != y2) {
      // update y1
      y1 += trace_y;
      // check if determinant is positive
      if (D <= 0) {
        // update y1
        x1 += trace_x;
        // update determinant
        D += 2*delta_y;    
      }
      // update deteminant
      D -= 2*delta_x;
      // draw next pixel
      SSD1306_DrawPixel (x1, y1);
    }
  }
  // success return
  return SSD1306_SUCCESS;
}

void SSD1306_InsertBitmap(int offsetx, int offsety, const char* bitmap)
{
  // insert a bitmap
  int x,y;
  uint32_t rows,cols;
  memcpy(&cols, bitmap+18, 4);
  memcpy(&rows, bitmap+22, 4);
  // bitmap rows are 32-bit aligned.
  int ccols = ((cols+31)/32)*32;
  bitmap += 62;

  for (y=0; y<rows; y++)
    for (x=0; x<cols; x++) {
      uint bit = (bitmap[(y*ccols+x)/8] >> (7 - (x%8))) & 1;
      if (bit) SSD1306_DrawPixel(offsetx+x,offsety+rows - y);
    }
}

