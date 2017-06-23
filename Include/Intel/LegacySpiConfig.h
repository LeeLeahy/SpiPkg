/** @file
  Declare the Legacy SPI controller configuration

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License that accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __LEGACY_SPI_CONFIG_H__
#define __LEGACY_SPI_CONFIG_H__

#include <Protocol/DevicePath.h>

///
/// Define the chip select values
///
#define SPIADDR_CSC_SS0         0           // SS0

///
/// Specify the legacy SPI controller chip select
///
typedef struct _LEGACY_SPI_CONFIG {
  UINT32 ChipSelect;
} LEGACY_SPI_CONFIG;

typedef struct {
  VENDOR_DEVICE_PATH        LegacySpiHc;
  EFI_DEVICE_PATH_PROTOCOL  End;
} LEGACY_SPI_DEVICE_PATH;

#define LEGACY_SPI_DEVICE_PATH_NODE \
  { \
    { \
      HARDWARE_DEVICE_PATH, \
      HW_VENDOR_DP, \
      { \
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)), \
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8) \
      } \
    }, \
    { 0x4eabc74e, 0x9d7f, 0x47c0, \
      { 0xbc, 0xc1, 0x5f, 0x46, 0xa5, 0x60, 0x95, 0x3c } \
    } \
  }

#define END_LEGACY_DEVICE_PATH \
  { \
    END_DEVICE_PATH_TYPE, \
    END_ENTIRE_DEVICE_PATH_SUBTYPE, \
    { \
      (UINT8) (sizeof (EFI_DEVICE_PATH_PROTOCOL)), \
      0 \
    } \
  }

#endif  //  __LEGACY_SPI_CONFIG_H__
