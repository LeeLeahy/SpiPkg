/** @file

  This module implements the SPI host controller protocol.

Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Support a SPI data transaction between the SPI controller and a SPI chip.

  @par Revision Reference:
  This protocol is from PI Version 1.6.

**/

#include "QuarkLegacySpi.h"

CONST LEGACY_SPI_DEVICE_PATH gSpiHcDevicePath = {
  LEGACY_SPI_DEVICE_PATH_NODE,
  END_LEGACY_DEVICE_PATH
};

/**
  The entry point for the legacy SPI controller driver.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
LegacySpiEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  SPI_HC *SpiHc;
  EFI_STATUS Status;

  //
  // Start the legacy SPI flash host controller
  //
  Status = SpiHcInitialize (&SpiHc, &gEfiSpiHcProtocolGuid);
  if (!EFI_ERROR(Status)) {
    //
    // Install the SPI host controller protocols
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                     &SpiHc->ControllerHandle,
                     &gEfiDevicePathProtocolGuid,
                     &gSpiHcDevicePath,
                     &gEfiLegacySpiControllerProtocolGuid,
                     &SpiHc->LegacySpiProtocol,
                     SpiHc->SpiHcGuid,
                     &SpiHc->SpiHcProtocol,
                     NULL
                     );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "ERROR - SpiHc failed to install SPI HC protocol!\n"));
    }
  }
  ASSERT_EFI_ERROR (Status);
  return Status;
}
