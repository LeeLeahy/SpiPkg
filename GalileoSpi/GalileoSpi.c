/** @file

  This module specifies the SPI busses on the Galileo board used in DXE
  mode.

Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "GalileoSpi.h"
#include <IndustryStandard/Pci.h>
#include <Library/UefiBootServicesTableLib.h>

#define SPI_FLASH_DRIVER_GUID           gEfiSpiNorFlashDriverGuid

#include "SpiBus2.h"

volatile BOOLEAN KeepLooping = TRUE;

/* List each SPI bus on the board */
static CONST EFI_SPI_BUS *CONST SpiBusses[] = {
  &SpiBus0,
  &SpiBus1,
  &SpiBus2
};

static CONST EFI_SPI_CONFIGURATION_PROTOCOL SpiConfiguration = {
  sizeof(SpiBusses)/sizeof(SpiBusses[1]),
  &SpiBusses[0]
};

/**
  The entry point for the Galileo SPI module.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
GalileoSpiEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  //
  // Make the SPI bus configuration available to the SPI driver stack
  //
  return SbcInitialize(&SpiConfiguration);
}
