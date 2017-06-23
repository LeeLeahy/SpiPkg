/** @file

  This module declares the Texas Instruments ADC108S102 protocol for the
  analog to digital converter.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL_H__
#define __TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL_H__

#include <TexasInstruments/ADC108S102.h>

#define TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL_GUID \
{ 0xb290feca, 0x8466, 0x4cdd, { 0x8d, 0xc3, 0xfe, 0xe0, 0x9f, 0xc8, 0xd8, 0x8b}}

typedef struct _TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL
               TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL;

/**
  Read the 10-bit analog to digital converter channel value.

  This routine must be called at or below TPL_CALLBACK.

  @param[in]  This              Pointer to a
                                TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL structure.
  @param[in]  Channel           The channel to read
  @param[out] AdcValue          Pointer to a buffer to receive the value.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The conversion was done successfully.
  @retval EFI_INVALID_PARAMETER AdcValue was NULL
  @retval EFI_INVALID_PARAMETER Channel > 7
**/
typedef
EFI_STATUS
(EFIAPI *TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL_READ_CHANNEL) (
  IN CONST TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL *This,
  IN UINT8 Channel,
  IN UINT16 *AdcValue
  );

///
/// Perform analog to digital conversions
///
struct _TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL {
  ///
  /// Pointer to the SPI peripheral
  ///
  CONST EFI_SPI_PERIPHERAL *SpiPeripheral;
  TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL_READ_CHANNEL ReadChannel;
};

#endif  //  __TEXAS_INSTRUMENTS_ADC108S102_PROTOCOL_H__
