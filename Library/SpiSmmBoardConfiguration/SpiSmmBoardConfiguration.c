/** @file

  This file implements SPI Configuration Protocol which enables the OEM to
  describe the board specific SPI busses to the SPI driver stack.

  Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/SpiBoardConfiguration.h>

/**
  Initialize the SPI board configuration SMM protocol.

  @param[in] SpiConfiguration   Address of the board's SPI configuration

  @retval EFI_SUCCESS           Successfully installed SPI configuration
                                protocol
  @retval other                 Some error occurs when executing this routine

**/
EFI_STATUS
EFIAPI
SbcSmmInitialize(
  IN CONST EFI_SPI_CONFIGURATION_PROTOCOL *SpiConfiguration
  )
{
  EFI_HANDLE Handle;
  EFI_STATUS Status;

  Handle = NULL;

  //
  // Provide the board's SPI bus descriptions to the SPI bus layer
  //
DEBUG((EFI_D_ERROR, "SMM: Loading SPI Board Configuration\n"));
  Status = gSmst->SmmInstallProtocolInterface(&Handle,
                                              &gEfiSpiSmmConfigurationProtocolGuid,
                                              EFI_NATIVE_INTERFACE,
                                              (VOID *)SpiConfiguration);
  ASSERT_EFI_ERROR (Status);
  return Status;
}
