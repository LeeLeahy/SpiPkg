/** @file

  Maxim_MAX3111E declares the board integration data structures

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __MAX3111E_CONFIGURATION_H__
#define __MAX3111E_CONFIGURATION_H__

#include <Protocol/SpiConfiguration.h>

typedef struct _MAX3111E_CONFIGURATION_DATA {
  ///
  /// Determine if the part is using an external clock or a crystal.
  ///
  BOOLEAN HasCrystal;

  ///
  /// Frequency in Hertz of the input clock or crystal.
  //
  UINT32 Frequency;
} MAX3111E_CONFIGURATION_DATA;

/* UART */
static CONST EFI_SPI_PART Maxim_MAX3111E = {
  L"Maxim",
  L"MAX3111E",
  0,
  KHz(4200),            // Page 4, SCLK Period 238 nS
  FALSE                 // Page 1, Diagram
};

/* MAX3111E driver */
static CONST EFI_GUID Maxim_MAX3111E_Driver = {
  0xe6e116b2, 0x7d82, 0x4832, { 0x8f, 0x39, 0xc, 0xd8, 0x20, 0xc8, 0xe7, 0xa0 }
};

#endif	// __MAX3111E_CONFIGURATION_H__
