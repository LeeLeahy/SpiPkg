/** @file

  Winbond_W25Q128FV declares the board integration data structure

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __WINBOND_W25Q128FV_H__
#define __WINBOND_W25Q128FV_H__

#include <Protocol/SpiConfiguration.h>

/* SPI NOR Flash */
static CONST EFI_SPI_PART Winbond_W25Q128FV = {
  L"Winbond",
  L"W25Q128FV",
  0,
  MHz(80),              // Page 85
  FALSE                 // Page 6, Section 3
};

#define WINBOND_W25Q128FV_READ_03_FREQUENCY      MHz(50) // Page 85

#endif	// __WINBOND_W25Q128FV_H__
