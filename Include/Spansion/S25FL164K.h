/** @file

  Spansion_S25FL164K declares the board integration data structure

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SPANSION_S25FL164K_H__
#define __SPANSION_S25FL164K_H__

#include <Protocol/SpiConfiguration.h>

/* SPI NOR Flash */
static CONST EFI_SPI_PART Spansion_S25FL164K = {
  L"Spansion",
  L"S25FL164K",
  0,
  MHz(80),              // Page 85
  FALSE                 // Page 6, Section 3
};

#define SPANSION_S25FL164K_READ_03_FREQUENCY    MHz(50) // Page 85

#endif	// __SPANSION_W25Q128FV_H__
