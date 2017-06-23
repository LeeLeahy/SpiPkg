/** @file
  Debug dump support

  Copyright (c) 2010 - 2016, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD
  License which accompanies this distribution.  The full text of the license
  may be found at http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/AsciiDump.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>


#define ANSI_CR                     13    ///< ANSI carriage return value
#define ANSI_LF                     10    ///< ANSI line feed value

///
///< Number of spaces between hex and ANSI displays
///
#define DUMP_ANSI_SEPARATION        2

///
/// Number of spaces between the offset and the hex display
///
#define DUMP_OFFSET_SEPARATION      2

///
/// Number of nibbles to display in the address
///
#define DUMP_NIBBLES_IN_OFFSET      ( sizeof ( UINT8 *) * 2 )

///
/// Hex display offset
///
#define DUMP_LINE_OFFSET            ( DUMP_NIBBLES_IN_OFFSET + 1 \
                                      + DUMP_OFFSET_SEPARATION )

///
/// ANSI display offset
///
#define DUMP_ANSI_OFFSET            ( DUMP_LINE_OFFSET + ( DUMP_BYTES_PER_LINE \
                                      * 3 ) + 1 + DUMP_ANSI_SEPARATION )


/**
  Convert a 4-bit value into a ANSI hex character.

  @param [in] Nibble            The value to convert into ANSI, only
                                the low 4-bits are valid.

  @returns The ANSI character value of the hex digit (0-9,A-F).
**/
UINT8
NibbleToAnsi (
  IN UINT8 Nibble
  )
{
  //
  //  Set the value.
  //
  Nibble &= 0xf;

  //
  //  Convert the nibble to ASCII.
  //
  Nibble += '0';
  if ( '9' < Nibble ) {
    //
    //  The value was above 9.
    //
    Nibble += 'a' - '0' - 10;
  }

  //
  //  Return the hex digit.
  //
  return Nibble;
};


/**
  Perform the control character detection and replacement.

  @param [in] Character         The character to be replaced if
                                it is a control character.

  @returns The replacement character value.
**/
UINT8
ReplaceControlCharacter (
  IN UINT8 Character
  )
{
  //
  //  Replace the control characters.
  //
  if (( 0x20 > Character ) || ( 0x7f <= Character )) {
    return '.';
  }

  //
  //  No replacement is necessary.
  //
  return Character;
}


/**
  Display the buffer contents

  @param [in] pDisplayOffset    Display address
  @param [in] pBuffer           Data buffer address
  @param [in] LengthInBytes     Length of data in buffer

**/
VOID
EFIAPI
AsciiDump (
  IN CONST UINT8 * pDisplayOffset,
  IN CONST UINT8 * pBuffer,
  IN INTN LengthInBytes
  )
{
  UINT8 Buffer[ DUMP_ANSI_OFFSET + DUMP_BYTES_PER_LINE + 2 + 1 ];
  UINT8 * pAnsi;
  UINT8 * pLine;
  UINT8 * pOffset;
  INTN Index;
  INTN LineIndex;
  INTN AnsiIndex;
  INTN BytesToDisplay;

  //
  //  Initialize the constant areas of the buffer
  //
  pOffset = &Buffer[ 0 ];
  SetMem ( &Buffer[ 0 ], sizeof ( Buffer ) - 1, ' ' );
  pOffset[ DUMP_NIBBLES_IN_OFFSET ] = ':';
  pLine = &pOffset[ DUMP_LINE_OFFSET ];
  pAnsi = &pOffset [ DUMP_ANSI_OFFSET ];

  //
  //  Dump the next line if more data exists
  //
  while ( 0 < LengthInBytes ) {
    //
    //  Display the offset
    //
    for ( Index = DUMP_NIBBLES_IN_OFFSET - 1; 0 <= Index; Index-- ) {
      pOffset[ DUMP_NIBBLES_IN_OFFSET - 1 - Index ] =
         NibbleToAnsi ((UINT8)RShiftU64 ((UINT64)((UINTN)pDisplayOffset),
                       Index << 2 ));
    }

    //
    //  Determine the number of characters to read
    //
    BytesToDisplay = LengthInBytes;
    if ( DUMP_BYTES_PER_LINE < BytesToDisplay ) {
      BytesToDisplay = DUMP_BYTES_PER_LINE;
    }

    //
    //  Start at the beginning of the line
    //
    LineIndex = 0;
    AnsiIndex = 0;

    //
    //  Loop through all the bytes
    //
    for ( Index = 0; BytesToDisplay > Index; Index++ ) {
      //
      //  Get the data
      //
      pAnsi[ AnsiIndex ] = *pBuffer++;

      //
      //  Place this byte in the line
      //
      pLine[ LineIndex++ ] = NibbleToAnsi ( pAnsi[ AnsiIndex ] >> 4 );
      pLine[ LineIndex++ ] = NibbleToAnsi ( pAnsi[ AnsiIndex ]);
      LineIndex++;

      //
      //  Replace the control character
      //
      pAnsi[ AnsiIndex ] = ReplaceControlCharacter ( pAnsi[ AnsiIndex ]);
      AnsiIndex++;
    }

    //
    //  Space fill the rest of the line
    //
    while ( DUMP_BYTES_PER_LINE > Index++ ) {
      pLine[ LineIndex++ ] = ' ';
      pLine[ LineIndex++ ] = ' ';
      LineIndex++;
    }

    //
    //  Terminate the line
    //
    pAnsi[ AnsiIndex++ ] = ANSI_CR;
    pAnsi[ AnsiIndex++ ] = ANSI_LF;
    pAnsi[ AnsiIndex++ ] = 0;

    //
    //  Display the line
    //
    if (gST->ConOut != NULL) {
      Print ( L"%a", &Buffer[ 0 ]);
    } else {
      DEBUG ((EFI_D_ERROR, "%a", &Buffer[0]));
    }

    //
    //  Account for these bytes
    //
    pDisplayOffset += BytesToDisplay;
    LengthInBytes -= BytesToDisplay;
  }
}
