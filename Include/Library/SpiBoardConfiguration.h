/** @file

  This file declares the routines associated with the SPI Board Configuration
  library.

  Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SPI_BOARD_CONFIGURATION_H__
#define __SPI_BOARD_CONFIGURATION_H__


#include <Protocol/SpiConfiguration.h>

/**
  Initialize the SPI board configuration protocol for the DXE layer.

  @param[in] SpiConfiguration   Address of the board's SPI configuration

  @retval EFI_SUCCESS           Successfully installed SPI configuration
                                protocol
  @retval other                 Some error occurs when executing this routine

**/
EFI_STATUS
EFIAPI
SbcInitialize(
  IN CONST EFI_SPI_CONFIGURATION_PROTOCOL *SpiConfiguration
  );

/**
  Initialize the SPI board configuration protocol for the SMM layer.

  @param[in] SpiConfiguration   Address of the board's SPI configuration

  @retval EFI_SUCCESS           Successfully installed SPI configuration
                                protocol
  @retval other                 Some error occurs when executing this routine

**/
EFI_STATUS
EFIAPI
SbcSmmInitialize(
  IN CONST EFI_SPI_CONFIGURATION_PROTOCOL *SpiConfiguration
  );

#endif	/* __SPI_BOARD_CONFIGURATION_H__ */
