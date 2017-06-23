## @file
# Include file containing Quark specific extensions to the Coreboot Payload
# Package
#
# Provides drivers and definitions to create a UEFI payload for coreboot.
#
# Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials are licensed and made available
# under the terms and conditions of the BSD License that accompanies this
# distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform.
#
################################################################################

[Components]

  #------------------------------
  #  SPI
  #------------------------------

[PcdsFeatureFlag.X64]
  gEfiSpiPkgTokenSpaceGuid.PcdDisplaySpiHcDevicePath|TRUE

[LibraryClasses]
  AsciiDump|SpiPkg/Library/AsciiDump/AsciiDump.inf
  I2cLib|QuarkSocPkg/QuarkSouthCluster/Library/I2cLib/I2cLib.inf
  IohLib|QuarkSocPkg/QuarkSouthCluster/Library/IohLib/IohLib.inf
  SpiBoardConfigurationLib|SpiPkg/Library/SpiBoardConfiguration/SpiBoardConfiguration.inf

[Components.X64]
  SpiPkg/VoltMeterDxe/VoltMeterDxe.inf
  SpiPkg/Adc108s102Dxe/Adc108s102Dxe.inf
  SpiPkg/SpiFlashDxe/SpiFlashDxe.inf
  SpiPkg/ClockDxe/ClockDxe.inf
  SpiPkg/Max6950Dxe/Max6950Dxe.inf
  SpiPkg/SpiBusDxe/SpiBusDxe.inf
  SpiPkg/GalileoSpi/GalileoSpi.inf {
    <LibraryClasses>
      MtrrLib|QuarkSocPkg/QuarkNorthCluster/Library/MtrrLib/MtrrLib.inf
      QNCAccessLib|QuarkSocPkg/QuarkNorthCluster/Library/QNCAccessLib/QNCAccessLib.inf
  }
  SpiPkg/QuarkSpiDxe/QuarkSpiDxe.inf
  SpiPkg/LegacySpiDxe/LegacySpiDxe.inf
