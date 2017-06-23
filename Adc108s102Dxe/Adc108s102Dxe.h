/** @file

  Adc108s102Dxe declares the items necessary to manage the analog to digital
  converter.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ADC108S102_DXE_H__
#define __ADC108S102_DXE_H__

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <TexasInstruments/ADC108S102.h>
#include <TexasInstruments/Protocol/ADC108S102.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/SpiIo.h>

extern EFI_GUID gTexasInstrumentsAdc108s102ProtocolGuid;

#define ADC108S102_SIGNATURE	SIGNATURE_32 ('A', '2', 'D', 'C')

typedef struct _ADC108S102
{
  //
  // Structure identification
  //
  UINT32 Signature;
  EFI_DRIVER_BINDING_PROTOCOL *DriverBinding;
  EFI_HANDLE ControllerHandle;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  EFI_SPI_IO_PROTOCOL *SpiIo;
  UINT8 NextChannel;
  TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL Adc108s102Protocol;
} ADC108S102;

#define ADC108S102_CONTEXT_FROM_PROTOCOL(protocol)         \
    CR (protocol, ADC108S102, Adc108s102Protocol, ADC108S102_SIGNATURE)

EFI_STATUS
EFIAPI
AdcComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
AdcShutdown (
  IN TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL *Adc108s102Protocol
  );

EFI_STATUS
EFIAPI
AdcStartup (
  IN EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN EFI_HANDLE ControllerHandle
  );

#endif	// __ADC108S102_DXE_H__
