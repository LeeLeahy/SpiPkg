/** @file

  Winbond_W25Q64FV declares the board integration data structure

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __WINBOND_W25Q64FV_H__
#define __WINBOND_W25Q64FV_H__

#include <Protocol/SpiConfiguration.h>

/* SPI NOR Flash */
static CONST EFI_SPI_PART Winbond_W25Q64FV = {
  L"Winbond",
  L"W25Q64FV",
  0,
  MHz(104),             // Page 75, 3.0V - 3.6V
  FALSE                 // Page 6, Section 3.1
};

#define WINBOND_W25Q64FV_READ_03_FREQUENCY      MHz(33) // Page 75, 2.7V - 3.0V

#endif	// __WINBOND_W25Q64FV_H__
