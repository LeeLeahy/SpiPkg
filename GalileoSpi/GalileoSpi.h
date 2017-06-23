/** @file

  Galileo SPI configuration declarations and support

Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __GALILEO_SPI_H__
#define __GALILEO_SPI_H__

#include <Uefi.h>
#include <Intel/LegacySpiConfig.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/SpiBoardConfiguration.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SpiConfiguration.h>

//
// NXP PCAL9535A I2C GPIO Expanders
//
#define I2C_GPIO_EXP0           0x25
#define I2C_GPIO_EXP1           0x26
#define I2C_GPIO_EXP2           0x27
#define I2C_LED                 0x47

//
// NXP PCAL9535A I2C GPIO Expanders Registers
//
#define I2C_GPIO_OUTPUT0        2
#define I2C_GPIO_OUTPUT1        3
#define I2C_GPIO_CONFIG0        6
#define I2C_GPIO_CONFIG1        7
#define I2C_GPIO_PUD_ENABLE0    0x46
#define I2C_GPIO_PUD_ENABLE1    0x47
#define I2C_GPIO_PULL_UP_DOWN0  0x48
#define I2C_GPIO_PULL_UP_DOWN1  0x49

//
// NXP PCA9685 I2C LED Driver Registers
//
#define LED_8_ON                0x27
#define LED_8_OFF               0x29

//
// LEDx_Ox_H Bits
#define LED_ON_OFF              BIT4

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           PciSpi;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PCI_SPI_DEVICE_PATH;

#define PNPID_DEVICE_PATH_NODE(PnpId) \
  { \
    { \
      ACPI_DEVICE_PATH, \
      ACPI_DP, \
      { \
        (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), \
        (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8) \
      } \
    }, \
    EISA_PNP_ID((PnpId)), \
    0 \
  }

#define PCI_DEVICE_PATH_NODE(Dev, Func) \
  { \
    { \
      HARDWARE_DEVICE_PATH, \
      HW_PCI_DP, \
      { \
        (UINT8) (sizeof (PCI_DEVICE_PATH)), \
        (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8) \
      }, \
    }, \
    (Func), \
    (Dev) \
  }

#define END_DEVICE_PATH \
  { \
    END_DEVICE_PATH_TYPE, \
    END_ENTIRE_DEVICE_PATH_SUBTYPE, \
    { \
      END_DEVICE_PATH_LENGTH, \
      0 \
    } \
  }

extern CONST EFI_SPI_BUS SpiBus0;
extern CONST EFI_SPI_BUS SpiBus1;
extern CONST EFI_SPI_BUS SpiBus2;

extern CONST PCI_SPI_DEVICE_PATH SpiController1;

extern volatile BOOLEAN KeepLooping;

#endif	// __GALILEO_SPI_H__
