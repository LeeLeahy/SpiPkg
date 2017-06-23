/** @file

  Maxim MAX6950 protocol for displaying data on seven-segment displays.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __MAXIM_MAX6950_PROTOCOL_H__
#define __MAXIM_MAX6950_PROTOCOL_H__

#define MAXIM_MAX6950_PROTOCOL_GUID \
{ 0x554701f8, 0x3a7, 0x4001, { 0x8d, 0xc1, 0x30, 0x63, 0x7d, 0x4b, 0x13, 0x7e }}

typedef struct _MAXIM_MAX6950_PROTOCOL MAXIM_MAX6950_PROTOCOL;

/**
  Display the contents of a string on the seven segment displays.

  This routine must be called at or below TPL_CALLBACK.

  This routine takes the input data and converts it from ASCII into something
  that can be displayed on the seven segment display.  Only certain characters
  of the ASCII set (0-127) are displayable, all others display as blanks.

  A period only consumes the a display if another character is not preceeding
  it.  When a character preceeds the period, both the character and the period
  are displayed on the same seven segment display.

  @param[in]  This              Pointer to a MAXIM_MAX6950_PROTOCOL structure.
  @param[in]  Data              ASCII character data to display.
  @param[in]  LengthInBytes     The length of the string in bytes.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The display was successful.
  @retval EFI_INVALID_PARAMETER Data was NULL
  @retval EFI_INVALID_PARAMETER Length too long for string being displayed
  @retval EFI_UNSUPPORTED       The board does not support seven-segment
                                displays
**/
typedef
EFI_STATUS
(EFIAPI *MAXIM_MAX6950_PROTOCOL_DISPLAY_STRING) (
  IN CONST MAXIM_MAX6950_PROTOCOL *This,
  IN UINT8 *Data,
  IN UINTN LengthInBytes
  );

///
/// Display data on seven segment displays
///
struct _MAXIM_MAX6950_PROTOCOL {
  ///
  /// The number of digits in the display
  ///
  UINT32 DigitsInDisplay;
  MAXIM_MAX6950_PROTOCOL_DISPLAY_STRING DisplayString;
};

#endif  //  __MAXIM_MAX6950_PROTOCOL_H__
