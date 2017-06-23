/** @file

  Atmel_AT25DF321 declares the board integration data structure

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ATMEL_ATAT25DF321_H__
#define __ATMEL_ATAT25DF321_H__

#include <Protocol/SpiConfiguration.h>

/* SPI NOR Flash */
static CONST EFI_SPI_PART Atmel_AT25DF321 = {
  L"Atmel",
  L"AT25DF321",
  0,
  MHz(70),              // Page 29
  FALSE                 // Page 4, Figure 2-1
};

#define ATMEL_AT25DF321_READ_03_FREQUENCY       MHz(33) // Page 29

#endif	// __ATMEL_ATAT25DF321_H__
