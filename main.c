typedef unsigned char uint8_t;
#define PROGMEM 

/** 
 * --------------------------------------------------------------------------------------+  
 * @desc        OLED SSD1306 example
 * --------------------------------------------------------------------------------------+ 
 *              Copyright (C) 2020 Marian Hrinko.
 *              Written by Marian Hrinko (mato.hrinko@gmail.com)
 *
 * @author      Marian Hrinko
 * @date        06.10.2020
 * @update      19.07.2021
 * @file        main.c
 * @version     2.0.0
 * @tested      AVR Atmega328p
 *
 * @depend      lib/ssd1306.h
 * --------------------------------------------------------------------------------------+ 
 * @descr       Version 1.0.0 -> applicable for 1 display
 *              Version 2.0.0 -> rebuild to 'cacheMemLcd' array
 *              Version 3.0.0 -> simplified alphanumeric version for 1 display
 * --------------------------------------------------------------------------------------+ 
 */

// include libraries
#include "lib/ssd1306.h"
#include <unistd.h>
#include "electrical.h"
#include "exclamation.h"
#include "network.h"

// extern const unsigned char bin2c_exclamation_bmp[510];
// extern const unsigned char bin2c_electrical_bmp[510];
/**
 * @desc    Main function
 *
 * @param   void
 *
 * @return  int
 */
int main(void)
{
  uint8_t addr = SSD1306_ADDR;

  // init ssd1306
  SSD1306_Init (addr);

  while (1) {
    SSD1306_ClearScreen ();
    //SSD1306_SetPosition (80,3);
    //SSD1306_DrawString ("P S U");
    SSD1306_InsertBitmap (0,0, bin2c_exclamation_bmp);
    SSD1306_InsertBitmap (64,0, bin2c_electrical_bmp);
    SSD1306_UpdateScreen (addr);
    usleep(1000000);

    SSD1306_ClearScreen ();
    //SSD1306_SetPosition (4,6);
    //SSD1306_DrawString ("Mit csinalunk ma?");
    SSD1306_InsertBitmap (0,0, bin2c_exclamation_bmp);
    SSD1306_InsertBitmap (64,0, bin2c_network_bmp);
    SSD1306_UpdateScreen (addr);
    usleep(1000000);
  }

  // return value
  return 0;
}
