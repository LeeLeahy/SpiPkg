/** @file

  Maxim_MAX6950 declares the board integration data structures

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __MAXIM_MAX6950_H__
#define __MAXIM_MAX6950_H__

#include <Protocol/SpiConfiguration.h>

//
// MAX6950 Registers
//
#define MAX6950_NO_OP           0x00
#define MAX6950_DECODE_MODE     0x01
#define MAX6950_INTENSITY       0x02
#define MAX6950_SCAN_LIMIT      0x03
#define MAX6950_CONFIG          0x04
#define MAX6950_CONFIG_NORMAL           0x01
#define MAX6950_CONFIG_BLINK_FAST       0x04
#define MAX6950_CONFIG_BLINK_ENABLE     0x08
#define MAX6950_CONFIG_BLINK_TIMING     0x10
#define MAX6950_CONFIG_CLEAR_DISPLAY    0x20
#define MAX6950_DISPLAY_TEST    0x07
#define MAX6950_DIGIT0_P0       0x20
#define MAX6950_DIGIT1_P0       0x21
#define MAX6950_DIGIT2_P0       0x22
#define MAX6950_DIGIT3_P0       0x23
#define MAX6950_DIGIT4_P0       0x24
#define MAX6950_DIGIT5_P0       0x25
#define MAX6950_DIGIT6_P0       0x26
#define MAX6950_DIGIT7_P0       0x27
#define MAX6950_DIGIT0_P1       0x40
#define MAX6950_DIGIT1_P1       0x41
#define MAX6950_DIGIT2_P1       0x42
#define MAX6950_DIGIT3_P1       0x43
#define MAX6950_DIGIT4_P1       0x44
#define MAX6950_DIGIT5_P1       0x45
#define MAX6950_DIGIT6_P1       0x46
#define MAX6950_DIGIT7_P1       0x47
#define MAX6950_DIGIT0_PX       0x60
#define MAX6950_DIGIT1_PX       0x61
#define MAX6950_DIGIT2_PX       0x62
#define MAX6950_DIGIT3_PX       0x63
#define MAX6950_DIGIT4_PX       0x64
#define MAX6950_DIGIT5_PX       0x65
#define MAX6950_DIGIT6_PX       0x66
#define MAX6950_DIGIT7_PX       0x67

typedef struct _MAX6950_CONFIGURATION_DATA {
  ///
  /// Specify the order of the displays on the board (0 - N)
  ///
  /// The easiest way to determine this is to initially place
  /// the numbers 0 - N in the array and then display the numbers
  /// 0 - N on the display.  Copy the numbers from the display
  /// from the display back into this array to complete the
  /// set up.
  ///
  CONST UINT8 *DisplayOrder;

  ///
  /// Specify the number of elements in the DisplayOrder array.
  ///
  UINT32 DisplayOrderSize;
} MAX6950_CONFIGURATION_DATA;

/* 7-Segment Display Controller */
static CONST EFI_SPI_PART Maxim_MAX6950 = {
  L"Maxim",
  L"MAX6950",
  0,
  MHz(26),              // Page 1
  FALSE                 // Page 1, Diagram
};

/* MAX6950 driver */
static CONST EFI_GUID Maxim_MAX6950_Driver = {
  0x65d2e7ad, 0xe318, 0x4b53, { 0xb5, 0x2e, 0x65, 0x75, 0xf9, 0xed, 0x3a, 0x78 }
};

#endif	// __MAXIM_MAX6950_H__
