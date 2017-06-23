/** @file

  SPI Configuration Protocol as defined in the PI 1.6 specification.

  The EFI SPI configuration management protocol provides platform specific
  services that allow the SPI IO protocol to reconfigure the clock frequency
  and polarity for the SPI peripheral on the SPI bus.

  Copyright (c) 2016-2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This protocol is from PI Version 1.6.

**/

#ifndef __SPI_CONFIGURATION_H__
#define __SPI_CONFIGURATION_H__

#include <Protocol/DevicePath.h>

typedef struct _EFI_SPI_CONFIGURATION_PROTOCOL EFI_SPI_CONFIGURATION_PROTOCOL;
typedef struct _EFI_SPI_PERIPHERAL EFI_SPI_PERIPHERAL;
typedef struct _EFI_SPI_BUS EFI_SPI_BUS;

///
/// Describe the properties of a SPI chip.
///
/// The EFI_SPI_PART data structure provides a description of a SPI part
/// which is independent of the use on the board.  This data is available
/// directly from the part's datasheet and may be provided by the vendor.
///
typedef struct _EFI_SPI_PART
{
  ///
  /// A Unicode string specifying the SPI chip vendor.
  ///
  CONST CHAR16 *Vendor;

  ///
  /// A Unicode string specifying the SPI chip part number.
  ///
  CONST CHAR16 *PartNumber;

  ///
  /// The minimum SPI bus clock frequency used to access this chip.  This value
  /// may be specified in the chip's datasheet.  If not, use the value of zero.
  ///
  UINT32 MinClockHz;

  ///
  /// The maximum SPI bus clock frequency used to access this chip.  This value
  /// is found in the chip's datasheet.
  ///
  UINT32 MaxClockHz;

  ///
  /// Specify the polarity of the chip select pin.  This value can be found in
  /// the SPI chip's datasheet.  Specify TRUE when a one asserts the chip select
  /// and FALSE when a zero asserts the chip select.
  ///
  BOOLEAN ChipSelectPolarity;
} EFI_SPI_PART;


/**

  Manipulate the chip select for a SPI device.

  This routine must be called at or below TPL_NOTIFY.

  Update the value of the chip select line for a SPI peripheral.  The SPI bus
  layer calls this routine either in the board layer or in the SPI controller
  to manipulate the chip select pin at the start and end of a SPI transaction.

  @param[in]  SpiPeripheral     The address of an EFI_SPI_PERIPHERAL data
                                structure describing the SPI peripheral whose
                                chip select pin is to be manipulated.  The
                                routine may access the ChipSelectParameter field
                                to gain sufficient context to complete the
                                operation.
  @param[in]  PinValue          The value to be applied to the chip select line
                                of the SPI peripheral.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The chip select was set successfully
  @retval EFI_NOT_READY         Support for the chip select is not properly
                                initialized
  @retval EFI_UNSUPPORTED       The SpiPeripheral->ChipSelectParameter value is
                                invalid

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_CHIP_SELECT) (
  CONST EFI_SPI_PERIPHERAL *SpiPeripheral,
  BOOLEAN PinValue
  );


///
/// Peripheral Attributes
///

///
/// The SPI peripheral is wired to support a 2-bit data bus
///
#define SPI_PART_SUPPORTS_2_BIT_DATA_BUS_WIDTH  0x00000001

///
/// The SPI peripheral is wired to support a 4-bit data bus
///
#define SPI_PART_SUPPORTS_4_BIT_DATA_BUS_WIDTH  0x00000002

///
/// Describe the board specific properties associated with a specific SPI chip.
///
/// The EFI_SPI_PERIPHERAL data structure describes how a specific block of
/// logic which is connected to the SPI bus.  This data structure also selects
/// which upper level driver is used to manipulate this SPI device.  The
/// SpiPeripheralDriverGuid is available from the vendor of the SPI
/// peripheral driver.
///
typedef struct _EFI_SPI_PERIPHERAL
{
  ///
  /// Address of the next EFI_SPI_PERIPHERAL data structure.  Specify NULL
  /// if the current data structure is the last one on the SPI bus.
  ///
  CONST EFI_SPI_PERIPHERAL *NextSpiPeripheral;

  ///
  /// A unicode string describing the function of the SPI part.
  ///
  CONST CHAR16 *FriendlyName;

  ///
  /// Address of a GUID provided by the vendor of the SPI peripheral driver.
  /// Instead of using a EFI_SPI_IO_PROTOCOL GUID, the SPI bus driver uses this
  /// GUID to identify an EFI_SPI_IO_PROTOCOL data structure and to provide the
  /// connection points for the SPI peripheral drivers.  This reduces the
  /// comparison logic in the SPI peripheral driver DriverSupported routine.
  ///
  CONST EFI_GUID *SpiPeripheralDriverGuid;

  ///
  /// The address of an EFI_SPI_PART data structure which describes this chip.
  ///
  CONST EFI_SPI_PART *SpiPart;

  ///
  /// The maximum clock frequency is specified in the EFI_SPI_PART.  When this
  /// this value is non-zero and less than the value in the EFI_SPI_PART then
  /// this value is used for the maximum clock frequency for the SPI part.
  ///
  UINT32 MaxClockHz;

  ///
  /// Specify the idle value of the clock as found in the datasheet.  Use zero
  /// (0) if the clock'S idle value is low or one (1) if the the clock's idle
  /// value is high.
  ///
  BOOLEAN ClockPolarity;

  ///
  /// Specify the clock delay after chip select.  Specify zero (0) to delay an
  /// entire clock cycle or one (1) to delay only half a clock cycle.
  ///
  BOOLEAN ClockPhase;

  ///
  /// SPI peripheral attributes
  ///
  UINT32 Attributes;

  ///
  /// Address of a vendor specific data structure containing additional board
  /// configuration details related to the SPI chip.  The SPI peripheral layer
  /// uses this data structure when configuring the chip.
  ///
  CONST VOID *ConfigurationData;

  ///
  /// The address of an EFI_SPI_BUS data structure which describes the SPI
  /// bus to which this chip is connected.
  ///
  CONST EFI_SPI_BUS *SpiBus;

  ///
  /// Address of the routine which controls the chip select pin for this SPI
  /// peripheral.  Call the SPI host controller's chip select routine when this
  /// value is set to NULL.
  ///
  EFI_SPI_CHIP_SELECT ChipSelect;

  ///
  /// Address of a data structure containing the additional values which
  /// describe the necessary control for the chip select.  When ChipSelect is
  /// NULL, the declaration for this data structure is provided by the vendor of
  /// the host's SPI controller driver.  The vendor's documentation specifies
  /// the necessary values to use for the chip select pin selection and control.
  ///
  /// When Chipselect is not NULL, the declaration for this data structure is
  /// provided by the board layer.
  ///
  VOID  *ChipSelectParameter;
} EFI_SPI_PERIPHERAL;


/**
  Set up the clock generator to produce the correct clock frequency, phase and
  polarity for a SPI chip.

  This routine must be called at or below TPL_NOTIFY.

  This routine updates the clock generator to generate the correct frequency and
  polarity for the SPI clock.

  @param[in]  SpiPeripheral     Pointer to a EFI_SPI_PERIPHERAL data structure
                                from which the routine can access the
                                ClockParameter, ClockPhase and ClockPolarity
                                fields.  The routine also has access to the
                                names for the SPI bus and chip which can be used
                                during debugging.
  @param[in,out] ClockHz        Pointer to the requested clock frequency.  The
                                clock generator will choose a supported clock
                                frequency which is less than or equal to this
                                value.  Specify zero to turn the clock generator
                                off.  The actual clock frequency supported by
                                the clock controller will be returned.

  @return  This routine returns one of the following status values:

  @retval EFI_SUCCESS           The clock was set up successfully.
  @retval EFI_UNSUPPORTED       The SPI controller was not able to support the
                                frequency requested by ClockHz
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_CLOCK) (
  CONST EFI_SPI_PERIPHERAL *SpiPeripheral,
  UINT32 *ClockHz
  );


///
/// Describe the board specific details associated with a SPI bus.
///
/// The EFI_SPI_BUS data structure provides the connection details between the
/// physical SPI bus and the EFI_SPI_HC_PROTOCOL instance which controls that
/// SPI bus.  This data structure also describes the details of how the clock is
/// generated for that SPI bus.  Finally this data structure provides the list
/// of physical SPI devices which are attached to the SPI bus.
///
typedef struct _EFI_SPI_BUS {
  ///
  /// A Unicode string describing the SPI bus
  ///
  CONST CHAR16 *FriendlyName;

  ///
  /// Address of the first EFI_SPI_PERIPHERAL data structure connected to
  /// this bus.  Specify NULL if there are no SPI peripherals connected to this
  /// bus.
  ///
  CONST EFI_SPI_PERIPHERAL *PeripheralList;

  ///
  /// Address of an EFI_DEVICE_PATH_PROTOCOL data structure which uniquely
  /// describes the SPI controller.
  ///
  CONST EFI_DEVICE_PATH_PROTOCOL *ControllerPath;

  ///
  /// Address of the routine which controls the clock used by the SPI bus for
  /// this SPI peripheral.  The SPI host controller's clock routine is called
  /// when this value is set to NULL.
  ///
  EFI_SPI_CLOCK Clock;

  ///
  /// Address of a data structure containing the additional values which
  /// describe the necessary control for the clock.  When Clock is NULL, the
  /// declaration for this data structure is provided by the vendor of the
  /// host's SPI controller driver.
  ///
  /// When Clock is not NULL, the declaration for this data structure is
  /// provided by the board layer.
  ///
  VOID *ClockParameter;
} EFI_SPI_BUS;


///
/// SPI configuration protocol
///
/// Describe the details of the board's SPI busses to the SPI driver stack.
///
/// The board layer uses the EFI_SPI_CONFIGURATION_PROTOCOL to expose the data
/// tables which describe the board's SPI busses,  The SPI bus layer uses these
/// tables to configure the clock, chip select and manage the SPI transactions
/// on the SPI controllers.
///
/// The configuration tables describe:
///   *  The number of SPI busses on the board
///   *  Which SPI chips are connected to each SPI bus
///
/// For each SPI chip the configuration describes:
///   *  The maximum clock frequency for the SPI part
///   *  The clock polarity needed for the SPI part
///   *  Whether the SPI controller is a separate clock generator needs to be
///      set up
///   *  The chip select polarity
///   *  Whether the SPI controller or a GPIO pin is used for the chip select
///   *  The data sampling edge for the SPI part
///
typedef struct _EFI_SPI_CONFIGURATION_PROTOCOL {
  ///
  /// The number of SPI busses on the board.
  ///
  UINT32 BusCount;

  ///
  /// The address of an array of EFI_SPI_BUS_CONNECTION data structure
  /// addresses.
  ///
  CONST EFI_SPI_BUS *CONST *CONST BusList;
} EFI_SPI_CONFIGURATION_PROTOCOL;


///
/// Reference to variable defined in the .DEC file
///
extern EFI_GUID gEfiSpiConfigurationProtocolGuid;
extern EFI_GUID gEfiSpiSmmConfigurationProtocolGuid;

///
/// Macros to easily specify frequencies in hertz, kilohertz and megahertz.
///
#define Hz(Frequency)		(Frequency)
#define KHz(Frequency)		(1000 * Hz(Frequency))
#define MHz(Frequency)		(1000 * KHz(Frequency))

#endif  //  __SPI_CONFIGURATION_H__
