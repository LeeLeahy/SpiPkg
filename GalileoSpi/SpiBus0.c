/** @file

  This module specifies the SPI devices connected to SPI bus 0.

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
#include <Protocol/PciRootBridgeIo.h>
#include <TexasInstruments/ADC108S102.h>

#define GPIO_SWPORTA_DR         0       // Output value
#define GPIO_SWPORTA_DDR        4       // 0 = Input, 1 = Output

#define GPIO0                   BIT0

UINT32 GpioBaseAddress;

EFI_STATUS
EFIAPI
GalileoInitialize (
  VOID
  )
{
  UINT32 Data;
  union {
    UINT32 U32;
    volatile UINT32 *Reg;
  } Gpio;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciIo;
  union {
   UINT8 U8[8];
   UINT64 U64;
  } PciRegAddress;
  EFI_STATUS Status;

  //
  // Locate the PCI_ROOT_BRIDGE_IO protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL,
                  (VOID **)&PciIo
                  );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Locate the GPIO base address
  //
  PciRegAddress.U8[0] = PCI_BASE_ADDRESSREG_OFFSET + 4;
  PciRegAddress.U8[1] = 2;
  PciRegAddress.U8[2] = 21;
  PciRegAddress.U8[3] = 0;
  PciRegAddress.U8[4] = 0;
  PciRegAddress.U8[5] = 0;
  PciRegAddress.U8[6] = 0;
  PciRegAddress.U8[7] = 0;
  Status = PciIo->Pci.Read(
                  PciIo,
                  EfiPciWidthUint32,
                  PciRegAddress.U64,
                  1,
                  &GpioBaseAddress
                  );
  
  if (EFI_ERROR(Status)) {
    return Status;
  }
  GpioBaseAddress &= ~0xf;
  if (GpioBaseAddress == 0) {
    return EFI_NOT_READY;
  }

  //
  // GPIO 0: ADC108S102 chip select
  // Set output value to 1
  //
  Gpio.U32 = GpioBaseAddress + GPIO_SWPORTA_DR;
  Data = *Gpio.Reg;
  Data |= GPIO0;
  *Gpio.Reg = Data;

  //
  // Set direction to output
  //
  Gpio.U32 = GpioBaseAddress + GPIO_SWPORTA_DDR;
  Data = *Gpio.Reg;
  Data |= GPIO0;
  *Gpio.Reg = Data;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
A2dChipSelect(
  CONST EFI_SPI_PERIPHERAL *SpiPeripheral,
  BOOLEAN PinValue
  )
{
  UINT32 Data;
  union {
    UINT32 U32;
    volatile UINT32 *Reg;
  } Gpio;
  EFI_STATUS Status;

  //
  // Verify the initialization
  //
  if (GpioBaseAddress == 0) {
    Status = GalileoInitialize ();
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  //
  // Update the chip select value
  //
  Gpio.U32 = GpioBaseAddress + GPIO_SWPORTA_DR;
  Data = *Gpio.Reg;
  Data = (Data & ~GPIO0) | (PinValue ? 1 : 0);
  *Gpio.Reg = Data;
  return EFI_SUCCESS;
}

static CONST ADC108S102_CONFIGURATION_DATA Adc108s102Config = {
  5 * 1000      // Reference voltage in millivolts
};

static CONST EFI_SPI_PERIPHERAL A2dConverter = {
  NULL,                         // NextPeripheral
  L"A/D Converter",             // FriendlyName
  &TexasInstruments_ADC108S102_Driver, // SpiPeripheralDriverGuid
  &TexasInstruments_ADC108S102, // SpiPart
  0,                            // MaxClockFrequency: Use part's max frequency
  0,                            // ClockPolarity: Page 7, Figure 2
  1,                            // ClockPase: Page 7, Figure 2
  0,                            // Attributes
  &Adc108s102Config,            // ConfigurationData
  &SpiBus0,                     // SpiBus
  A2dChipSelect,                // ChipSelect
  NULL                          // ChipSelectParameter
};

CONST PCI_SPI_DEVICE_PATH SpiController0 = {
  PNPID_DEVICE_PATH_NODE(0x0a03),
  PCI_DEVICE_PATH_NODE(21, 0),
  END_DEVICE_PATH
};

CONST EFI_SPI_BUS SpiBus0 = {
  L"SPI Bus 0 - A/D converter",         // FriendlyName
  &A2dConverter,                        // PeripheralList
  &SpiController0.PciRootBridge.Header, // ControllerPath
  NULL,                                 // Clock: Use SPI host controller
  NULL                                  // ClockParameter
};
