/** @file

  TestBoard specifies the SPI devices for the SPI test board

Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "GalileoSpi.h"
#include <Library/I2cLib.h>
#include <Atmel/AT25DF321.h>
#include <Maxim/MAX3111E.h>
#include <Maxim/MAX6950.h>
#include <Micron/N25Q128A.h>
#include <Protocol/SpiNorFlash.h>
#include <Spansion/S25FL164K.h>
#include <Winbond/W25Q80DV.h>
#include <Winbond/W25Q16DV.h>
#include <Winbond/W25Q32FV.h>
#include <Winbond/W25Q64FV.h>
#include <Winbond/W25Q128FV.h>

#define DISPLAY_CHIP_SELECT_SHIFT       5
#define DISPLAY_CHIP_SELECT             (1 << DISPLAY_CHIP_SELECT_SHIFT)

#define FLASH_CHIP_SELECT_SHIFT         6
#define FLASH_CHIP_SELECT               (1 << FLASH_CHIP_SELECT_SHIFT)

#define MUX4_SEL			BIT6
#define MUX5_SEL			BIT4
#define MUX8_SEL			BIT6

#define LVL_B_OE0_N                     BIT0
#define LVL_B_OE4_N                     BIT0
#define LVL_B_OE7_N			BIT6

#define LVL_C_OE5_N                     BIT2
#define LVL_C_PU5                       BIT3

BOOLEAN TestBoardInitialized;

EFI_STATUS
EFIAPI
I2cGpioRegister(
  IN UINT8 SlaveAddress,
  IN UINT8 GpioRegister,
  IN INTN AndValue,
  IN INTN XorValue
  )
{
  UINT8 Data[2];
  EFI_I2C_DEVICE_ADDRESS I2cAddress;
  UINTN ReadLength;
  UINTN WriteLength;
  EFI_STATUS Status;

  /* Get the current state of the output ports */
  ReadLength = 1;
  WriteLength = 1;
  Data[1] = GpioRegister;
  I2cAddress.I2CDeviceAddress = SlaveAddress;
  Status = I2cReadMultipleByte (
                 I2cAddress,
                 EfiI2CSevenBitAddrMode,
                 &WriteLength,
                 &ReadLength,
                 &Data[1]
                 );
  if (!EFI_ERROR(Status)) {

    /* Update the chip select value */
    Data[1] = (UINT8)((Data[1] & AndValue) ^ XorValue);

    /* Set the chip select state */
    Data[0] = GpioRegister;
    WriteLength = sizeof(Data);
    Status = I2cWriteMultipleByte (
                   I2cAddress,
                   EfiI2CSevenBitAddrMode,
                   &WriteLength,
                   &Data[0]
                   );
  }
  return Status;
}

EFI_STATUS
EFIAPI
TestBoardInitialize (
  VOID
  )
{
  EFI_STATUS Status;
#define TIMEOUT_COUNT   (60 /* * 60 */)
STATIC UINTN TimeoutCount = TIMEOUT_COUNT;

  //
  // Test board GPIO usage:
  //
  //   DIGITAL  2 --> 13: MAX6950 chip select
  //   DIGITAL  3 --> 17: Flash chip select
  //   DIGITAL 11 -->  7: MOSI
  //   DIGITAL 12 <-- 11: MISO
  //   DIGITAL 13 -->  9: SCLK
  //   Ground     --> 12: Ground
  //              -->  1: +3.3V
  //              -->  2: Ground
  //

  //
  // Route the MAX6950 chip select to pin DIGITAL 2
  // I2C 0x27 P1.5 (output value)
  //
TimeoutCount++;
if (TimeoutCount >= TIMEOUT_COUNT) {
TimeoutCount = 0;
}
if (TimeoutCount == 0) {
DEBUG ((EFI_D_ERROR, "TestBoardInitialize entered\n"));
}
  Status = I2cGpioRegister(
              I2C_GPIO_EXP2,
              I2C_GPIO_OUTPUT1,
              ~DISPLAY_CHIP_SELECT,
              DISPLAY_CHIP_SELECT
              );
  if (EFI_ERROR(Status)) {
if (TimeoutCount == 0) {
DEBUG ((EFI_D_ERROR, "ERROR - TestBoard failed to initialize, Status: %r\n", Status));
}
    return Status;
  }
  TestBoardInitialized = TRUE;

  I2cGpioRegister(
              I2C_GPIO_EXP2,
              I2C_GPIO_CONFIG1,
              ~DISPLAY_CHIP_SELECT,
              0
              );

  //
  // Route the flash chip select to pin DIGITAL 3
  // I2C 0x25 P0.0 LVL_B_OE0_N = 1
  // I2C 0x27 P1.6 (output value)
  //
  I2cGpioRegister(
              I2C_GPIO_EXP0,
              I2C_GPIO_OUTPUT0,
              ~LVL_B_OE0_N,
              LVL_B_OE0_N
              );
  I2cGpioRegister(
              I2C_GPIO_EXP0,
              I2C_GPIO_CONFIG0,
              ~LVL_B_OE0_N,
              0
              );

  I2cGpioRegister(
              I2C_GPIO_EXP2,
              I2C_GPIO_OUTPUT1,
              ~FLASH_CHIP_SELECT,
              FLASH_CHIP_SELECT
              );
  I2cGpioRegister(
              I2C_GPIO_EXP2,
              I2C_GPIO_CONFIG1,
              ~FLASH_CHIP_SELECT,
              0
              );

  //
  // Route the MOSI signal to the connector pin DIGITAL 11
  // I2C 0x26 P1.4 MUX5_SEL = 1
  // I2C 0x47 LED8 MUX4_SEL = 0
  // I2C 0x25 P0.6 LVL_B_OE4_N = 0
  // DIGITAL 11
  //
  I2cGpioRegister(
              I2C_GPIO_EXP1,
              I2C_GPIO_OUTPUT1,
              ~MUX5_SEL,
              MUX5_SEL
              );
  I2cGpioRegister(
              I2C_GPIO_EXP1,
              I2C_GPIO_CONFIG1,
              ~MUX5_SEL,
              0
              );
  I2cGpioRegister(
              I2C_LED,
              LED_8_ON,
              ~LED_ON_OFF,
              LED_ON_OFF
              );
  I2cGpioRegister(
              I2C_LED,
              LED_8_OFF,
              ~LED_ON_OFF,
              0
              );
  I2cGpioRegister(
              I2C_GPIO_EXP0,
              I2C_GPIO_OUTPUT1,
              ~LVL_B_OE4_N,
              0
              );
  I2cGpioRegister(
              I2C_GPIO_EXP0,
              I2C_GPIO_CONFIG1,
              ~LVL_B_OE4_N,
              0
              );

  //
  // Route the MISO signal to the connector pin DIGITAL 12
  // I2C 0x26 P1.2 LVL_C_OE5_N = 1
  // I2C 0x26 P1.3 LVL_C_PU5 = input (tri-state) with a 100K pull up resistor
  I2cGpioRegister(
              I2C_GPIO_EXP1,
              I2C_GPIO_OUTPUT1,
              ~LVL_C_OE5_N,
              LVL_C_OE5_N
              );
  I2cGpioRegister(
              I2C_GPIO_EXP1,
              I2C_GPIO_CONFIG1,
              ~LVL_C_OE5_N,
              0
              );
  I2cGpioRegister(
              I2C_GPIO_EXP1,
              I2C_GPIO_PULL_UP_DOWN1,
              ~LVL_C_PU5,
              LVL_C_PU5
              );
  I2cGpioRegister(
              I2C_GPIO_EXP1,
              I2C_GPIO_PUD_ENABLE1,
              ~LVL_C_PU5,
              LVL_C_PU5
              );
  I2cGpioRegister(
              I2C_GPIO_EXP1,
              I2C_GPIO_CONFIG1,
              ~LVL_C_PU5,
              LVL_C_PU5
              );

  //
  // Route the SCLK signal to the connector pin DIGITAL 13
  // I2C 0x26 P1.6 MUX8_SEL = 1
  // I2C 0X25 P1.6 LVL_B_OE7N = 0
  // DIGITAL 13
  //
  I2cGpioRegister(
              I2C_GPIO_EXP1,
              I2C_GPIO_OUTPUT1,
              ~MUX8_SEL,
              MUX8_SEL
              );
  I2cGpioRegister(
              I2C_GPIO_EXP1,
              I2C_GPIO_CONFIG1,
              ~MUX8_SEL,
              0
              );
  I2cGpioRegister(
              I2C_GPIO_EXP0,
              I2C_GPIO_OUTPUT1,
              ~LVL_B_OE7_N,
              0
              );
  I2cGpioRegister(
              I2C_GPIO_EXP0,
              I2C_GPIO_CONFIG1,
              ~LVL_B_OE7_N,
              0
              );
if (TimeoutCount == 0) {
DEBUG ((EFI_D_ERROR, "TestBoardInitialize exiting, Status: EFI_SUCCESS\n"));
}
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
NorFlashChipSelect(
  CONST EFI_SPI_PERIPHERAL *SpiPeripheral,
  BOOLEAN PinValue
  )
{
  EFI_STATUS Status;

  //
  // Verify that the GPIOs are initialized properly
  //
  if (!TestBoardInitialized) {
    Status = TestBoardInitialize ();
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  //
  // Update the chip select value
  //
  I2cGpioRegister (
            I2C_GPIO_EXP2,
            I2C_GPIO_OUTPUT1,
            ~FLASH_CHIP_SELECT,
            (PinValue & 1) << FLASH_CHIP_SELECT_SHIFT
            );
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SevenSegmentChipSelect(
  CONST EFI_SPI_PERIPHERAL *SpiPeripheral,
  BOOLEAN PinValue
  )
{
  EFI_STATUS Status;

  //
  // Verify that the GPIOs are initialized properly
  //
  if (!TestBoardInitialized) {
    Status = TestBoardInitialize ();
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  //
  // Update the chip select value
  //
  I2cGpioRegister (
            I2C_GPIO_EXP2,
            I2C_GPIO_OUTPUT1,
            ~DISPLAY_CHIP_SELECT,
            (PinValue & 1) << DISPLAY_CHIP_SELECT_SHIFT
            );
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UartChipSelect(
  CONST EFI_SPI_PERIPHERAL *SpiPeripheral,
  BOOLEAN PinValue
  )
{
  EFI_STATUS Status;

  //
  // Verify that the GPIOs are initialized properly
  //
  if (!TestBoardInitialized) {
    Status = TestBoardInitialize ();
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  DEBUG ((EFI_D_ERROR, "ERROR: Hung in UartChipSelect.\n"));
  while (KeepLooping);
  return EFI_SUCCESS;
}

static CONST EFI_SPI_NOR_FLASH_CONFIGURATION_DATA W25Q80DV_Config = {
  NULL,                                 // SpiFlashList
  32768,                                // EraseBlockBytes
  1 * BIT20,                            // FlashSize
  FALSE,                                // LowFrequencyReadOnly
  WINBOND_W25Q32FV_READ_03_FREQUENCY,   // Opcode 03 read frequency
  256,                                  // WritePageBytes
  SPI_NOR_ENABLE_WRITE_OR_ERASE,        // Write status prefix opcode
  { 0xEF, 0x40, 0x14 }                  // Manufacture and device ID
};

static CONST EFI_SPI_PERIPHERAL W25Q80DV = {
  NULL,                         // NextPeripheral
  L"SPI NOR Flash",             // FriendlyName
  &gEfiSpiNorFlashDriverGuid,   // SpiPeripheralDriverGuid
  &Winbond_W25Q80DV,            // SpiPart
  MHz(50),                      // Reduce MaxClockHz due to board layout
  1,                            // ClockPolarity
  0,                            // ClockPase
  0,                            // Attributes
  &W25Q80DV_Config,             // ConfigurationData
  &SpiBus1,                     // SpiBus
  NorFlashChipSelect,           // ChipSelect
  NULL                          // ChipSelectParameter
};

static CONST EFI_SPI_NOR_FLASH_CONFIGURATION_DATA W25Q16DV_Config = {
  NULL,                                 // SpiFlashList
  32768,                                // EraseBlockBytes
  2 * BIT20,                            // FlashSize
  FALSE,                                // LowFrequencyReadOnly
  WINBOND_W25Q16DV_READ_03_FREQUENCY,   // Opcode 03 read frequency
  256,                                  // WritePageBytes
  SPI_NOR_ENABLE_WRITE_OR_ERASE,        // Write status prefix opcode
  { 0xEF, 0x40, 0x15 }                  // Manufacture and device ID
};

static CONST EFI_SPI_PERIPHERAL W25Q16DV = {
  &W25Q80DV,                    // NextPeripheral
  L"SPI NOR Flash",             // FriendlyName
  &gEfiSpiNorFlashDriverGuid,   // SpiPeripheralDriverGuid
  &Winbond_W25Q16DV,            // SpiPart
  MHz(50),                      // Reduce MaxClockHz due to board layout
  1,                            // ClockPolarity
  0,                            // ClockPase
  0,                            // Attributes
  &W25Q16DV_Config,             // ConfigurationData
  &SpiBus1,                     // SpiBus
  NorFlashChipSelect,           // ChipSelect
  NULL                          // ChipSelectParameter
};

static CONST EFI_SPI_NOR_FLASH_CONFIGURATION_DATA At25df321Config = {
  NULL,                                 // SpiFlashList
  32768,                                // EraseBlockBytes
  4 * BIT20,                            // FlashSize
  FALSE,                                // LowFrequencyReadOnly
  ATMEL_AT25DF321_READ_03_FREQUENCY,    // Opcode 03 read frequency
  256,                                  // WritePageBytes
  SPI_NOR_ENABLE_WRITE_OR_ERASE,        // Write status prefix opcode
  { 0x1F, 0x47, 0x06 }                  // Manufacture and device ID
};

static CONST EFI_SPI_PERIPHERAL AT25DF321 = {
  &W25Q16DV,                         // NextPeripheral
  L"SPI NOR Flash",             // FriendlyName
  &gEfiSpiNorFlashDriverGuid,   // SpiPeripheralDriverGuid
  &Atmel_AT25DF321,             // SpiPart
  0,                            // MaxClockHz: Use part's max frequency
  0,                            // ClockPolarity: Page 6, Figure 5-1
  1,                            // ClockPase: Page 6, Figure 5-1
  0,                            // Attributes
  &At25df321Config,             // ConfigurationData
  &SpiBus1,                     // SpiBus
  NorFlashChipSelect,           // ChipSelect
  NULL                          // ChipSelectParameter
};

static CONST EFI_SPI_NOR_FLASH_CONFIGURATION_DATA W25Q32FV_Config = {
  NULL,                                 // SpiFlashList
  32768,                                // EraseBlockBytes
  4 * BIT20,                            // FlashSize
  FALSE,                                // LowFrequencyReadOnly
  WINBOND_W25Q32FV_READ_03_FREQUENCY,   // Opcode 03 read frequency
  256,                                  // WritePageBytes
  SPI_NOR_ENABLE_WRITE_OR_ERASE,        // Write status prefix opcode
  { 0xEF, 0x40, 0x16 }                  // Manufacture and device ID
};

static CONST EFI_SPI_PERIPHERAL W25Q32FV = {
  &AT25DF321,                   // NextPeripheral
  L"SPI NOR Flash",             // FriendlyName
  &gEfiSpiNorFlashDriverGuid,   // SpiPeripheralDriverGuid
  &Winbond_W25Q32FV,            // SpiPart
  MHz(50),                      // Reduce MaxClockHz due to board layout
  1,                            // ClockPolarity
  0,                            // ClockPase
  0,                            // Attributes
  &W25Q32FV_Config,             // ConfigurationData
  &SpiBus1,                     // SpiBus
  NorFlashChipSelect,           // ChipSelect
  NULL                          // ChipSelectParameter
};

static CONST EFI_SPI_NOR_FLASH_CONFIGURATION_DATA W25Q64FV_Config = {
  NULL,                                 // SpiFlashList
  32768,                                // EraseBlockBytes
  8 * BIT20,                            // FlashSize
  FALSE,                                // LowFrequencyReadOnly
  WINBOND_W25Q64FV_READ_03_FREQUENCY,   // Opcode 03 read frequency
  256,                                  // WritePageBytes
  SPI_NOR_ENABLE_WRITE_OR_ERASE,        // Write status prefix opcode
  { 0xEF, 0x40, 0x17 }                  // Manufacture and device ID
};

static CONST EFI_SPI_PERIPHERAL W25Q64FV = {
  &W25Q32FV,                    // NextPeripheral
  L"SPI NOR Flash",             // FriendlyName
  &gEfiSpiNorFlashDriverGuid,   // SpiPeripheralDriverGuid
  &Winbond_W25Q64FV,            // SpiPart
  MHz(50),                      // Reduce MaxClockHz due to board layout
  1,                            // ClockPolarity
  0,                            // ClockPase
  0,                            // Attributes
  &W25Q64FV_Config,             // ConfigurationData
  &SpiBus1,                     // SpiBus
  NorFlashChipSelect,           // ChipSelect
  NULL                          // ChipSelectParameter
};

static CONST EFI_SPI_NOR_FLASH_CONFIGURATION_DATA S25FL164K_Config = {
  NULL,                                 // SpiFlashList
  32768,                                // EraseBlockBytes
  8 * BIT20,                            // FlashSize
  FALSE,                                // LowFrequencyReadOnly
  SPANSION_S25FL164K_READ_03_FREQUENCY, // Opcode 03 read frequency
  256,                                  // WritePageBytes
  SPI_NOR_ENABLE_WRITE_OR_ERASE,        // Write status prefix opcode
  { 0x01, 0x40, 0x17 }                  // Manufacture and device ID
};

static CONST EFI_SPI_PERIPHERAL S25FL164K = {
  &W25Q64FV,                    // NextPeripheral
  L"SPI NOR Flash",             // FriendlyName
  &gEfiSpiNorFlashDriverGuid,   // SpiPeripheralDriverGuid
  &Spansion_S25FL164K,          // SpiPart
  MHz(50),                      // Reduce MaxClockHz due to board layout
  1,                            // ClockPolarity
  0,                            // ClockPase
  0,                            // Attributes
  &S25FL164K_Config,            // ConfigurationData
  &SpiBus1,                     // SpiBus
  NorFlashChipSelect,           // ChipSelect
  NULL                          // ChipSelectParameter
};

static CONST EFI_SPI_NOR_FLASH_CONFIGURATION_DATA W25Q128FV_Config = {
  NULL,                                 // SpiFlashList
  32768,                                // EraseBlockBytes
  16 * BIT20,                           // FlashSize
  FALSE,                                // LowFrequencyReadOnly
  WINBOND_W25Q128FV_READ_03_FREQUENCY,  // Opcode 03 read frequency
  256,                                  // WritePageBytes
  SPI_NOR_ENABLE_WRITE_OR_ERASE,        // Write status prefix opcode
  { 0xEF, 0x40, 0x18 }                  // Manufacture and device ID
};

static CONST EFI_SPI_PERIPHERAL W25Q128FV = {
  &S25FL164K,                   // NextPeripheral
  L"SPI NOR Flash",             // FriendlyName
  &gEfiSpiNorFlashDriverGuid,   // SpiPeripheralDriverGuid
  &Winbond_W25Q128FV,           // SpiPart
  MHz(50),                      // Reduce MaxClockHz due to board layout
  1,                            // ClockPolarity
  0,                            // ClockPase
  0,                            // Attributes
  &W25Q128FV_Config,            // ConfigurationData
  &SpiBus1,                     // SpiBus
  NorFlashChipSelect,           // ChipSelect
  NULL                          // ChipSelectParameter
};

static CONST EFI_SPI_NOR_FLASH_CONFIGURATION_DATA N25Q128A_Config = {
  NULL,                                 // SpiFlashList
  32768,                                // EraseBlockBytes
  16 * BIT20,                           // FlashSize
  FALSE,                                // LowFrequencyReadOnly
  MICRON_N25Q128A_READ_03_FREQUENCY,    // Opcode 03 read frequency
  256,                                  // WritePageBytes
  SPI_NOR_ENABLE_WRITE_OR_ERASE,        // Write status prefix opcode
  { 0x20, 0xba, 0x18 }                  // Manufacture and device ID
};

static CONST EFI_SPI_PERIPHERAL N25Q128A = {
  &W25Q128FV,                   // NextPeripheral
  L"SPI NOR Flash",             // FriendlyName
  &gEfiSpiNorFlashDriverGuid,   // SpiPeripheralDriverGuid
  &Micron_N25Q128A,             // SpiPart
  MHz(50),                      // Reduce MaxClockHz due to board layout
  1,                            // ClockPolarity
  0,                            // ClockPase
  0,                            // Attributes
  &N25Q128A_Config,             // ConfigurationData
  &SpiBus1,                     // SpiBus
  NorFlashChipSelect,           // ChipSelect
  NULL                          // ChipSelectParameter
};

/* SPI NOR Flash */
static CONST EFI_SPI_PART GenericSpiNorFlash = {
  L"Generic",                   // Vendor
  L"SPI NOR Flash",             // PartNumber
  0,                            // MinClockHz - None
  MHz(30),                      // MaxClockHz
  FALSE                         // ChipSelectPolarity
};

static CONST EFI_SPI_NOR_FLASH_CONFIGURATION_DATA GenericSpiNorFlashConfig = {
  &N25Q128A,                            // SpiFlashList
  32768,                                // EraseBlockBytes
  4 * BIT20,                            // FlashSize
  FALSE,                                // LowFrequencyReadOnly
  MHz(30),                              // Opcode 03 read frequency
  256,                                  // WritePageBytes
  SPI_NOR_ENABLE_WRITE_OR_ERASE,        // Write status prefix opcode
  { 0x00, 0x00, 0x00 }                  // Manufacture and device ID - Generic
};

static CONST EFI_SPI_PERIPHERAL NorFlash = {
  NULL,                         // NextPeripheral
  L"Unknown Size",              // FriendlyName
  &gEfiSpiNorFlashDriverGuid,   // SpiPeripheralDriverGuid
  &GenericSpiNorFlash,          // SpiPart
  MHz(50),                      // Reduce MaxClockHz due to board layout
  1,                            // ClockPolarity
  0,                            // ClockPase
  0,                            // Attributes
  &GenericSpiNorFlashConfig,    // ConfigurationData
  &SpiBus1,                     // SpiBus
  NorFlashChipSelect,           // ChipSelect
  NULL                          // ChipSelectParameter
};

static CONST UINT8 DisplayOrder[] = {
  3, 2, 1, 0                    // Order of the displays on the board
};

static CONST MAX6950_CONFIGURATION_DATA Max6950Config = {
  &DisplayOrder[0],             // DisplayOrder array address
  sizeof(DisplayOrder)          // Number of seven-segment displays
};

static CONST EFI_SPI_PERIPHERAL SevenSegment = {
  &NorFlash,                    // NextPeripheral
  L"7 Segment Display",         // FriendlyName
  &Maxim_MAX6950_Driver,        // SpiPeripheralDriverGuid
  &Maxim_MAX6950,               // SpiPart
  0,                            // MaxClockHz: Use part's max frequency
  0,                            // ClockPolarity: Page 7, Figure 1
  0,                            // ClockPase: Page 7, Figure 1
  0,                            // Attributes
  &Max6950Config,               // ConfigurationData
  &SpiBus1,                     // SpiBus
  SevenSegmentChipSelect,       // ChipSelect
  NULL                          // ChipSelectParameter
};

static CONST MAX3111E_CONFIGURATION_DATA UartConfig = {
  TRUE,                         // HasCrystal
  KHz(3680)                     // Frequency
};

static CONST EFI_SPI_PERIPHERAL Uart = {
  &SevenSegment,                // NextPeripheral
  L"UART",                      // FriendlyName
  &Maxim_MAX3111E_Driver,       // SpiPeripheralDriverGuid
  &Maxim_MAX3111E,              // SpiPart
  0,                            // MaxClockHz: Use part's max frequency
  0,                            // ClockPolarity: Page 12, Figure 4
  0,                            // ClockPase: Page 12, Figure 4
  0,                            // Attributes
  &UartConfig,                  // ConfigurationData
  &SpiBus1,                     // SpiBus
  UartChipSelect,               // ChipSelect
  NULL                          // ChipSelectParameter
};

CONST PCI_SPI_DEVICE_PATH SpiController1 = {
  PNPID_DEVICE_PATH_NODE(0x0a03),
  PCI_DEVICE_PATH_NODE(21, 1),
  END_DEVICE_PATH
};

CONST EFI_SPI_BUS SpiBus1 = {
  L"SPI Bus 1 - Test Board",            // FriendlyName
  &Uart,                                // PeripheralList
  &SpiController1.PciRootBridge.Header, // ControllerPath
  NULL,                                 // Clock: Use SPI host controller
  NULL                                  // ClockParameter
};
