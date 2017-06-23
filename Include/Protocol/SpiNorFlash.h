/** @file

  This module declares the SPI NOR flash protocol interface.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SPI_NOR_FLASH_H__
#define __SPI_NOR_FLASH_H__

#include <Uefi.h>
#include <Protocol/SpiConfiguration.h>

typedef struct _EFI_SPI_NOR_FLASH_PROTOCOL EFI_SPI_NOR_FLASH_PROTOCOL;

/**
  Read the 3 byte manufacture and device ID from the SPI flash.

  This routine must be called at or below TPL_NOTIFY.

  This routine reads the 3 byte manufacture and device ID from the flash part
  filling the buffer provided.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[out] Buffer            Pointer to a 3 byte buffer to receive the
                                manufacture and device ID.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The manufacture and device ID was read
                                successfully.
  @retval EFI_INVALID_PARAMETER Buffer is NULL
  @retval EFI_DEVICE_ERROR      Invalid data received from SPI flash part.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_NOR_FLASH_PROTOCOL_GET_FLASH_ID) (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  OUT UINT8 *Buffer
  );

/**
  Read data from the SPI flash.

  This routine must be called at or below TPL_NOTIFY.

  This routine reads data from the SPI part in the buffer provided.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[in]  FlashAddress      Address in the flash to start reading
  @param[in]  LengthInBytes     Read length in bytes
  @param[out] Buffer            Address of a buffer to receive the data

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The data was read successfully.
  @retval EFI_INVALID_PARAMETER Buffer is NULL
  @retval EFI_INVALID_PARAMETER FlashAddress >= This->FlashSize
  @retval EFI_INVALID_PARAMETER LengthInBytes > This->FlashSize- FlashAddress
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_NOR_FLASH_PROTOCOL_READ_DATA) (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  IN UINT32 FlashAddress,
  IN UINT32 LengthInBytes,
  OUT UINT8 *Buffer
  );

/**
  Read the flash status register.

  This routine must be called at or below TPL_NOTIFY.

  This routine reads the flash part status register.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[in]  LengthInBytes     Number of status bytes to read.
  @param[out] FlashStatus       Pointer to a buffer to receive the flash status.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The status register was read successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_NOR_FLASH_PROTOCOL_READ_STATUS)  (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  IN UINT32 LengthInBytes,
  OUT UINT8 *FlashStatus
  );

/**
  Write the flash status register.

  This routine must be called at or below TPL_NOTIFY.

  This routine writes the flash part status register.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[in]  LengthInBytes     Number of status bytes to write.
  @param[in]  FlashStatus       Pointer to a buffer containing the new status.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The status write was successful.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the write buffer.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_NOR_FLASH_PROTOCOL_WRITE_STATUS) (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  IN UINT32 LengthInBytes,
  IN UINT8 *FlashStatus
  );

/**
  Write data to the SPI flash.

  This routine must be called at or below TPL_NOTIFY.

  This routine breaks up the write operation as necessary to write the data to
  the SPI part.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[in]  FlashAddress      Address in the flash to start writing
  @param[in]  LengthInBytes     Write length in bytes
  @param[in]  Buffer            Address of a buffer containing the data

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The data was written successfully.
  @retval EFI_INVALID_PARAMETER The Buffer is NULL.
  @retval EFI_INVALID_PARAMETER The FlashAddress >= This->FlashSize
  @retval EFI_INVALID_PARAMETER The LengthInBytes > This->FlashSize
                                                    - FlashAddress
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory to copy buffer.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_NOR_FLASH_PROTOCOL_WRITE_DATA) (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  IN UINT32 FlashAddress,
  IN UINT32 LengthInBytes,
  IN UINT8 *Buffer
  );

/**
  Efficiently erases one or more 4KiB regions in the SPI flash.

  This routine must be called at or below TPL_NOTIFY.

  This routine uses a combination of 4 KiB and larger blocks to erase the
  specified area.

  @param[in]  This              Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                                structure.
  @param[in]  FlashAddress      Address within a 4 KiB block to start erasing
  @param[in]  BlockCount        Number of 4 KiB blocks to erase

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The erase was completed successfully.
  @retval EFI_INVALID_PARAMETER FlashAddress >= This->FlashSize
  @retval EFI_INVALID_PARAMETER BlockCount * 4 KiB > This->FlashSize
                                                     - FlashAddress
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_NOR_FLASH_PROTOCOL_ERASE) (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL *This,
  IN UINT32 FlashAddress,
  IN UINT32 BlockCount
  );

///
/// The EFI_SPI_NOR_FLASH_PROTOCOL exists in the SPI peripheral layer.  This
/// protocol manipulates the SPI NOR flash parts using a common set of
/// commands.  The board layer provides the interconnection and configuration
/// details for the SPI NOR flash part.  The SPI NOR flash driver uses this
/// configuration data to expose a generic interface which provides the
/// following APIs:
/// * Read manufacture and device ID
/// * Read data
/// * Read data using low frequency
/// * Read status
/// * Write data
/// * Erase 4 KiB blocks
/// * Erase 32 or 64 KiB blocks
/// * Write status
///
struct _EFI_SPI_NOR_FLASH_PROTOCOL {
  ///
  /// Pointer to an EFI_SPI_PERIPHERAL data structure
  ///
  CONST EFI_SPI_PERIPHERAL *SpiPeripheral;

  ///
  /// Flash size in bytes
  ///
  UINT32 FlashSize;

  ///
  /// Manufacture and Device ID
  ///
  UINT8 DeviceId [3];

  ///
  /// Erase block size in bytes
  ///
  UINT32 EraseBlockBytes;

  EFI_SPI_NOR_FLASH_PROTOCOL_GET_FLASH_ID GetFlashId;
  EFI_SPI_NOR_FLASH_PROTOCOL_READ_DATA ReadData;
  EFI_SPI_NOR_FLASH_PROTOCOL_READ_DATA LfReadData;
  EFI_SPI_NOR_FLASH_PROTOCOL_READ_STATUS ReadStatus;
  EFI_SPI_NOR_FLASH_PROTOCOL_WRITE_STATUS WriteStatus;
  EFI_SPI_NOR_FLASH_PROTOCOL_WRITE_DATA WriteData;
  EFI_SPI_NOR_FLASH_PROTOCOL_ERASE Erase;
};

typedef struct _EFI_SPI_NOR_FLASH_CONFIGURATION_DATA {
  ///
  /// Allow multiple parts to be specified to support socket flash parts.
  ///
  CONST EFI_SPI_PERIPHERAL *SpiFlashList;

  ///
  /// Erase block size in bytes
  ///
  UINT32 EraseBlockBytes;

  ///
  /// Flash size in bytes
  ///
  UINT32 FlashSize;

  ///
  /// Force the use of low frequency reads
  ///
  BOOLEAN LowFrequencyReadOnly;

  ///
  /// Specify the read frequency for opcode 0x03
  ///
  UINT32 ReadFrequency;

  ///
  /// Write page size in bytes
  ///
  UINT32 WritePageBytes;

  ///
  /// Write status prefix opcode
  ///
  UINT8 WriteStatusPrefixOpcode;

  ///
  /// Manufacture and device ID, specify all zeros for generic flash part.
  ///
  UINT8 DeviceId [3];
} EFI_SPI_NOR_FLASH_CONFIGURATION_DATA;

///
/// Write status
/// One prefix byte, one command byte and one or two bytes of data to send
///
#define SPI_NOR_WRITE_STATUS            0x01

///
/// Page program: Write data to the SPI NOR flash part
/// One prefix byte, one command byte and 3 address bytes to send followed by
/// up to 256 bytes of data
///
#define SPI_NOR_PAGE_PROGRAM            0x02

///
/// Read data from the SPI NOR flash part
/// One command byte and 3 address bytes to send followed by one or more
/// bytes of data to receive
///
#define SPI_NOR_LOW_FREQUENCY_READ_DATA 0x03

///
/// Read status register
/// One command byte followed by one or two bytes to receive
///
#define SPI_NOR_READ_STATUS             0x05

///
/// Prefix byte to enable write or erase operations
///
#define SPI_NOR_ENABLE_WRITE_OR_ERASE   0x06

///
/// Read data from the SPI NOR flash part
/// One command byte, 3 address bytes and one dummy byte to send followed by
/// one or more bytes of data to receive
///
#define SPI_NOR_READ_DATA               0x0b

///
/// Erase 4 KBytes
/// One prefix byte, one command byte and 3 address bytes to send
///
#define SPI_NOR_ERASE_4KB               0x20

///
/// Erase 32 KBytes
/// One prefix byte, one command byte and 3 address bytes to send
///
#define SPI_NOR_ERASE_32KB              0x52

///
/// Chip erase
/// One prefix byte and one command byte to send
///
#define SPI_NOR_CHIP_ERASE              0x60

///
/// Read the three bytes of manufacture and device ID
/// One command byte to send followed by 3 bytes of data to receive
///
#define SPI_NOR_READ_MANUFACTURE_ID     0x9f

///
/// Erase 64 KBytes
/// One prefix byte, one command byte and 3 address bytes to send
///
#define SPI_NOR_ERASE_64KB              0xd8

///
/// SPI Status Register Bits
///
#define SPI_STATUS1_SRP0        0x80 // Status register protect 0
#define SPI_STATUS1_SEC         0x40 // Sector protect
#define SPI_STATUS1_TB          0x20 // Top/Bottom protect
#define SPI_STATUS1_BP2         0x10 // Block protect bit2
#define SPI_STATUS1_BP1         0x08 // Block protect bit1
#define SPI_STATUS1_BP0         0x04 // Block protect bit0
#define SPI_STATUS1_WEL         0x02 // Write enable latch
#define SPI_STATUS1_BUSY        0x01 // Busy doing write or erase

#endif	// __SPI_NOR_FLASH_H__
