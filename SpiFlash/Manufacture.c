/** @file

  This module implements the manufacure name routine for the SPI flash driver.

Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SpiFlash.h"

VOID
EFIAPI
FlashDisplayManufactureName (
  IN UINT8 Manufacture
  )
{
  CONST CHAR16 *ManufactureName;

  switch (Manufacture) {
  default:
    ManufactureName = L"Unknown";
    break;

  case 0x01:
    ManufactureName = L"Spansion";
    break;

  case 0x1f:
    ManufactureName = L"Atmel";
    break;

  case 0x20:
    ManufactureName = L"Micron";
    break;

  case 0xef:
    ManufactureName = L"Winbond";
    break;
  }

  DEBUG ((EFI_D_INFO, "SPI flash manufacture (0x%02x): %s\n", Manufacture,
          ManufactureName));
}
