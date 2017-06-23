/** @file

  Micron_N25Q128A declares the board integration data structure

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __MICRON_N25Q128A_H__
#define __MICRON_N25Q128A_H__

#include <Protocol/SpiConfiguration.h>

/* SPI NOR Flash */
static CONST EFI_SPI_PART Micron_N25Q128A = {
  L"Micron",
  L"N25Q128A",
  0,
  MHz(108),             // Page 72
  FALSE                 // Page 7, Figure 1
};

#define MICRON_N25Q128A_READ_03_FREQUENCY       MHz(54) // Page 72

#endif	// __MICRON_N25Q128A_H__
