/*  Arduino UNO R4 Minima demo code for RA4M1 timers in quadrature-encoder-count mode:
 *
 *  Base code to interface to two encoders, I used:
 *  https://www.bourns.com/pdfs/enc1j.pdf
 *
 *  Susan Parker - 5th May 2024.
 *
 * This code is "AS IS" without warranty or liability. 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

// ARM-developer - Accessing memory-mapped peripherals
// https://developer.arm.com/documentation/102618/0100

#include "susan_ra4m1_minima_register_defines.h"

#define AGT0_RELOAD   2999      // AGT0 value to give 1mS / 1kHz - with 48.0MHz/2 PCLKB; refine as needed
// #define AGT0_RELOAD   5999     // AGT0 value to give  2mS / 500Hz 
// #define AGT0_RELOAD  29999     // AGT0 value to give 10mS / 100Hz 

#define POS_X_INDEX_START  0x80000000
#define POS_Y_INDEX_START  0x80000000

static uint32_t agt_count;

void setup()
  {
  Serial.begin(115200);
  while (!Serial){};
  Serial.println("Encoder X-Y Test 1"); 

  *ICU_IELSR04 = 0x014;                                 // Reasign mS timer Slot 04 IELSR04 to non-active IRQ e.g DMAC3

  attachInterrupt(15, agt0UnderflowInterrupt, FALLING); // This IRQ will be asigned to Slot 05 IELSR05 as 0x001 PORT_IRQ0 - Table 13.4
  *ICU_IELSR05 = 0x01E;                                 // HiJack Slot 05 IELSR05 for AGT0_AGTI
  *PFS_P000PFS = 0x00000000;                            // Clear A1/D15 ISEL pin assigned Interrupt Enable
	asm volatile("dsb");                                  // Data bus Synchronization instruction
  *NVIC_IPR05_BY = 0x40;                                // Bounce the priority up from 0x80 to 0x60
// Warning: Cannot use   delay(); etc mS functions.

  *AGT0_AGT = AGT0_RELOAD;           //  Set value for mS counter - varies with different clock frequencies

  setup_timer_0();
  setup_timer_1();
  }

void loop()
  {
  static uint32_t agt_count_last;     // Note use static variable, otherwise reset to 0 each loop()
  static bool     mS_flag = false;

  static uint32_t position_x_index = POS_X_INDEX_START; 
  static uint32_t position_y_index = POS_Y_INDEX_START; 
  static uint32_t position_x_index_old = POS_X_INDEX_START; 
  static uint32_t position_y_index_old = POS_Y_INDEX_START; 

  static bool     run_x_flag = false;
  static uint32_t agt_count_x;
  static uint32_t agt_count_x_old;
  static uint16_t velocity_x_move; 
  static uint32_t mS_delay_count_x; 

  static bool     run_y_flag = false;
  static uint32_t agt_count_y;
  static uint32_t agt_count_y_old;
  static uint16_t velocity_y_move; 
  static uint32_t mS_delay_count_y; 

  if(agt_count_last != agt_count)
    {
    agt_count_last = agt_count;
    mS_flag = true;
    } 

  if(mS_flag == true)
    {
    position_x_index = *GPT320_GTCNT;
    position_y_index = *GPT321_GTCNT;

    if(position_x_index_old != position_x_index)
      {
      mS_delay_count_x = 0;
      position_x_index_old = position_x_index;
      Serial.print("X = ");
      Serial.println((int)(position_x_index - 0x80000000)); 
      }

    if(position_y_index_old != position_y_index)
      {
      mS_delay_count_y = 0;
      position_y_index_old = position_y_index;
      Serial.print("Y = ");
      Serial.println((int)(position_y_index - 0x80000000)); 
      }

    mS_flag = false;
    }

  }


void agt0UnderflowInterrupt(void)    // Note: AGT0 counter updated from reload-reg on underflow i.e. it is a DOWN counter
  {
  *PFS_P111PFS_BY = 0x05;            // D13 - Should be glowing dimly 
  agt_count++;                       // 

  *PFS_P111PFS_BY = 0x04;            //  
  }


void setup_timer_0(void)
  {
  *MSTP_MSTPCRD &= (0xFFFFFFFF - (0x01 << MSTPD5));  // Enable GTP32 timer module

// Timer 0 - BiPhase Input - Set up-counting/down-counting in phase counting mode 4

  *GPT320_GTUPSR = 0x00006000;
  *GPT320_GTDNSR = 0x00009000;
  *GPT320_GTCNT  = POS_X_INDEX_START;  // Start at UINT32 counter mid-point

  *PFS_P106PFS  |= (0x1 << 16);      // Port Mode Control - Used as an I/O port for peripheral function
  *PFS_P107PFS  |= (0x1 << 16);      // Port Mode Control - Used as an I/O port for peripheral function
  *PFS_P106PFS  |= (0b00011 << 24);  // Select PSEL[4:0] for GTIOC0B - See Table 19.6
  *PFS_P107PFS  |= (0b00011 << 24);  // Select PSEL[4:0] for GTIOC0A - See Table 19.6

  *GPT320_GTCR  |= 0x00000001;      // Enable Timer 0 Counter
  }

void setup_timer_1(void)
  {
// Timer 1 - BiPhase Input - Set up-counting/down-counting in phase counting mode 4

  *GPT321_GTUPSR = 0x00006000;
  *GPT321_GTDNSR = 0x00009000;
  *GPT321_GTCNT  = POS_Y_INDEX_START;  // Start at UINT32 counter mid-point

  *PFS_P104PFS  |= (0x1 << 16);      // Port Mode Control - Used as an I/O port for peripheral function
  *PFS_P105PFS  |= (0x1 << 16);      // Port Mode Control - Used as an I/O port for peripheral function
  *PFS_P104PFS  |= (0b00011 << 24);  // Select PSEL[4:0] for GTIOC1B - See Table 19.6
  *PFS_P105PFS  |= (0b00011 << 24);  // Select PSEL[4:0] for GTIOC1A - See Table 19.6

  *GPT321_GTCR  |= 0x00000001;      // Enable Timer 1 Counter
  }
