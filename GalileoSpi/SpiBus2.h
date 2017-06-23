/** @file

  This module specifies the SPI devices for SPI bus 2.

Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "GalileoSpi.h"
#include <Protocol/SpiNorFlash.h>
#include <Winbond/W25Q64FV.h>

static LEGACY_SPI_CONFIG BiosFlashChipSelect = {
  SPIADDR_CSC_SS0
};

static CONST EFI_SPI_NOR_FLASH_CONFIGURATION_DATA BiosFlashConfig = {
  NULL,                                 // SpiFlashList
  65536,                                // EraseBlockBytes
  8 * BIT20,                            // FlashSize
  TRUE,                                 // LowFrequencyReadOnly
  WINBOND_W25Q64FV_READ_03_FREQUENCY,   // Opcode 03 read frequency, use maximum
  256,                                  // WritePageBytes
  SPI_NOR_ENABLE_WRITE_OR_ERASE,        // Write status prefix opcode
  { 0xEF, 0x40, 0x17 }                  // Manufacture and device ID
};

static CONST EFI_SPI_PERIPHERAL BiosFlash = {
  NULL,                         // NextPeripheral
  L"BIOS Flash",                // FriendlyName
  &SPI_FLASH_DRIVER_GUID,       // SpiPeripheralDriverGuid
  &Winbond_W25Q64FV,            // SpiPart
  0,                            // MaxClockHz: Use part's max frequency
  0,                            // ClockPolarity: 
  0,                            // ClockPase: 
  0,                            // Attributes
  &BiosFlashConfig,             // ConfigurationData
  &SpiBus2,                     // SpiBus
  NULL,                         // ChipSelect: Use SPI host controller
  &BiosFlashChipSelect          // ChipSelectParameter
};

CONST LEGACY_SPI_DEVICE_PATH SpiController2 = {
  LEGACY_SPI_DEVICE_PATH_NODE,
  END_LEGACY_DEVICE_PATH
};

CONST EFI_SPI_BUS SpiBus2 = {
  L"SPI Bus 2 - BIOS Flash",            // FriendlyName
  &BiosFlash,                           // PeripheralList
  &SpiController2.LegacySpiHc.Header,   // ControllerPath
  NULL,                                 // Clock: Use SPI host controller
  NULL                                  // ClockParameter
};
