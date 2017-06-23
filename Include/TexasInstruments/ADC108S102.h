/** @file

  TexasInstruments_ADC108S102 declares the board integration data structure

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TEXAS_INSTRUMENTS_ADC108S102_H__
#define __TEXAS_INSTRUMENTS_ADC108S102_H__

#include <Protocol/SpiConfiguration.h>

typedef struct _ADC108S102_CONFIGURATION_DATA {
  ///
  /// Reference Voltage * 1000
  ///
  UINT32 ReferenceVoltage;
} ADC108S102_CONFIGURATION_DATA;

/* SPI NOR Flash */
static CONST EFI_SPI_PART TexasInstruments_ADC108S102 = {
  L"Texas Instruments",
  L"ADC108S102",
  MHz(8),               // Page 5, fSCLK
  MHz(16),              // Page 5, fSCLK
  FALSE                 // Page 1, Diagram
};

/* ADC108S102 driver */
static CONST EFI_GUID TexasInstruments_ADC108S102_Driver = {
  0x3f720667, 0x22cc, 0x4752, { 0x9d, 0x6a, 0x15, 0xa0, 0x35, 0x2a, 0xda, 0xec }
};

#endif	// __TEXAS_INSTRUMENTS_ADC108S102_H__
