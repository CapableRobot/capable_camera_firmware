/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Gunnar Ryder for Hellbender, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * interface.cpp - Implementation for basic interface class
 */

#include "interface.hpp"

Interface::Interface(Type type, bool verbose) : 
    mVerbose(verbose),
    mType(type)
    {};
Interface::~Interface() {};

void Interface::Reconnect()
{
    if (IsOpen() == true)
    {
        Close();
    }

    Open();
}

int Interface::Read(DataArray &data, Value other)
{
    int read = -1;

    if (IsOpen() == true)
    {
        // If I2C, insert reg number into data
        if (mType == Type::I2C)
        {
            DataArray i2cData(data);
            i2cData.insert(i2cData.begin(), other);
            read = Read(data);
        }
        // Otherwise, just send it
        else
        {
            read = Read(data);
        }
    }
    
    return read;
}

int Interface::Write(DataArray &data, Value other)
{
    int wrote = -1;

    if (IsOpen() == true)
    {
        // If I2C, insert reg number into data
        if (mType == Type::I2C)
        {
            DataArray i2cData(data);
            i2cData.insert(i2cData.begin(), other);
            wrote = Write(i2cData);
        }
        // Otherwise, just send it
        else
        {
            wrote = Write(data);
        }
    }
    
    return wrote;
}

void Interface::Open()
{
    // If not open, call derived open function
    if (IsOpen() == false)
    {
        DoOpen();
    }
}

void Interface::Close()
{
    // If open, call derived close function
    if (IsOpen() == true)
    {
        DoClose();
    }
}

void Interface::PrintBuf(
    std::ostream &stream,
    DataArray data,
    unsigned int maxIndex
)
{
    // Store the stream flags so we can reset them later
    std::ios_base::fmtflags flags = stream.flags();

    // Prepare for data handling
    const unsigned char wordSize = 4;
    const unsigned char numWords = 4;
    const unsigned char rowMax = wordSize * numWords;
    unsigned int dataSize = data.size();
    unsigned int printSize = (maxIndex < dataSize) ? maxIndex : dataSize;

    // Loop through all data elements
    for (unsigned int index = 0; index < printSize; index++)
    {
        // If this element should be the start of a new row, output the
        // base address for the line
        if ((index % rowMax) == 0)
        {
            // If this index is not the first row element, add a new line
            if (index != 0)
            {
                stream << std::endl;
            }

            stream << "0x" << std::setfill('0') << std::setw(7)
                << std::hex << (index / rowMax) << "0      ";
        }
        // If the index is the start of a new word, add some separation
        else if ((index % wordSize) == 0)
        {
            stream << "    ";
        }
        // Otherwise, just add a space
        else
        {
            stream << " ";
        }

        // Add the value to the output
        stream << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)data[index];
    }

    // Reset the flags to the unmodified state
    stream.flags(flags);
};