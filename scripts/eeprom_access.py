#!/usr/bin/python3
#
# eeprom_access.py
#
# Updated EEPROM read/write utility
# Based off Gunnar's earlier work
#
# Copyright 2022 Hellbender Inc.
# All Rights Reserved
#
# Changelog:
# Author Email, Date,     , Comment
# niessl      , 2022-11-12, Created File
#

#import smbus
import argparse

class EEPROM_OOB_Exception:
  pass
  
class EEPROM_membank:

  def __init__(self, devAddr, memSize):
    self.devAddr = devAddr
    self.memSize = memSize
    
class EEPROM_endpoint:

  def __init__(self, busAddr, bankList, debug):
    self.busAddr  = busAddr
    self.bankList = bankList
    self.debug    = debug

  def connect(self):
    #self.bus = smbus.SMBus(self.busAddr)
    self.bus.write_quick(addressList[0])
    if self.debug:
      print("Connecting to I2C dev {}".format(self.busAddr))

  def readByte(self, bank, memAddr):
    devAddr = self.bankList[bank].devAddr
    if memAddr < 0 or memAddr >= self.bankList[bank].memSize:
      raise EEPROM_OOB_Exception
    if self.debug:
      print("reading bank {} addr {}".format(bank, memAddr))
    return bus.read_byte_data(devAddr, memAddr)

  def writeByte(self, val, bank, memAddr):
    devAddr = self.bankList[bank].devAddr
    if memAddr < 0 or memAddr >= self.bankList[bank].memSize:
      raise EEPROM_OOB_Exception
    if self.debug:
      print("writing {} to bank {} addr {}".format(val, bank, memAddr))
    return bus.read_byte_data(devAddr, memAddr)  

  def readBytes(self, bank, startAddr, length):
    if startAddr < 0 or length < 1:
      raise EEPROM_OOB_Exception
    if startAddr + length > self.bankList[bank].memSize:
      raise EEPROM_OOB_Exception
    retVal = bytearray(b'')
    for ii in count(0, length):
      addr = startAddr + ii
      val = self.readByte(bank, addr)
      retVal.append(val)
    return retVal

  def writeBytes(self, bank, startAddr, writeBuff, length):
    if startAddr < 0 or length < 1:
      raise EEPROM_OOB_Exception
    if startAddr + length > self.bankList[bank].memSize:
      raise EEPROM_OOB_Exception
    for ii in count(0, length):
      addr = startAddr + ii
      val = writeBuff[ii]
      self.writeByte(val, bank, addr)

def writeFileContentToEEPROM(file, endpoint, bank, addr, length):
  with open(file, 'r') as fileCont:

def handleArgs():
  parser = argparse.ArgumentParser()
  RWgroup = parser.add_mutually_exclusive_group()
  RWgroup.add_argument("-r", "--read", action="store_true", help="Read data from EEPROM")
  RWgroup.add_argument("-w", "--write", action="store_true", help="Wriate data to EEPROM")
  FTgroup = parser.add_mutually_exclusive_group()
  FTgroup.add_argument("-t", "--terminal", type=str, help="Read/Write to/from standard out/in. (Data should be hex formatted)")
  FTgroup.add_argument("-f", "--file", type=str, help="Read/Write to/from file source. (Data read as is/UTF-8)")
  parser.add_argument("-b", "--bank", type=int, help="EEPROM bank (0 or 1). Bank 0 is for reserved data.")
  parser.add_argument("-o", "--offset", type=int, nargs='?', const=0, help="Address offset of bank (default 0)")
  parser.add_argument("-l", "--length", type=int, nargs='?', const=256, help="Length of data to read or write (default to whole bank/source)")
  parser.add_argument("-v", "--verbose", help="Verbose output")
  return parser.parse_args()
  
if __name__ == "__main__":
  args = handleArgs()
