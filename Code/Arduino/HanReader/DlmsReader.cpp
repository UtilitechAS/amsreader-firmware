#include "DlmsReader.h"

DlmsReader::DlmsReader()
{
    //this->Clear();
}

void DlmsReader::Clear()
{
    this->position = 0;
    this->dataLength = 0;
    this->destinationAddressLength = 0;
    this->sourceAddressLength = 0;
    this->frameFormatType = 0;
}

bool DlmsReader::Read(byte data)
{
    if (position == 0 && data != 0x7E)
    {
        // we haven't started yet, wait for the start flag (no need to capture any data yet)
        return false;
    }
    else
    {
        // We have completed reading of one package, so clear and be ready for the next
        if (dataLength > 0 && position >= dataLength + 2)
            Clear();
        
        // Check if we're about to run into a buffer overflow
        if (position >= DLMS_READER_BUFFER_SIZE)
            Clear();

        // Check if this is a second start flag, which indicates the previous one was a stop from the last package
        if (position == 1 && data == 0x7E)
        {
            // just return, we can keep the one byte we had in the buffer
            return false;
        }

        // We have started, so capture every byte
        buffer[position++] = data;

        if (position == 1)
        {
            // This was the start flag, we're not done yet
            return false;
        }
        else if (position == 2)
        {
            // Capture the Frame Format Type
            frameFormatType = (byte)(data & 0xF0);
            if (!IsValidFrameFormat(frameFormatType))
                Clear();
            return false;
        }
        else if (position == 3)
        {
            // Capture the length of the data package
            dataLength = ((buffer[1] & 0x0F) << 8) | buffer[2];
            return false;
        }
        else if (destinationAddressLength == 0)
        {
            // Capture the destination address
            destinationAddressLength = GetAddress(3, destinationAddress, 0, DLMS_READER_MAX_ADDRESS_SIZE);
            if (destinationAddressLength > 3)
                Clear();
            return false;
        }
        else if (sourceAddressLength == 0)
        {
            // Capture the source address
            sourceAddressLength = GetAddress(3 + destinationAddressLength, sourceAddress, 0, DLMS_READER_MAX_ADDRESS_SIZE);
            if (sourceAddressLength > 3)
                Clear();
            return false;
        }
        else if (position == 4 + destinationAddressLength + sourceAddressLength + 2)
        {
            // Verify the header checksum
            ushort headerChecksum = GetChecksum(position - 3);
            if (headerChecksum != Crc16.ComputeChecksum(buffer, 1, position - 3))
                Clear();
            return false;
        }
        else if (position == dataLength + 1)
        {
            // Verify the data package checksum
            ushort checksum = this->GetChecksum(position - 3);
            if (checksum != Crc16.ComputeChecksum(buffer, 1, position - 3))
                Clear();
            return false;
        }
        else if (position == dataLength + 2)
        {
            // We're done, check the stop flag and signal we're done
            if (data == 0x7E)
                return true;
            else
            {
                Clear();
                return false;
            }
        }
    }
    return false;
}

bool DlmsReader::IsValidFrameFormat(byte frameFormatType)
{
    return frameFormatType == 0xA0;
}

int DlmsReader::GetRawData(byte *dataBuffer, int start, int length)
{
    if (dataLength > 0 && position == dataLength + 2)
    {
        int headerLength = 3 + destinationAddressLength + sourceAddressLength + 2;
        int bytesWritten = 0;
        for (int i = headerLength + 1; i < dataLength - 1; i++)
        {
          dataBuffer[i + start - headerLength - 1] = buffer[i];
          bytesWritten++;
        }
        return bytesWritten;
    }
    else
        return 0;
}

int DlmsReader::GetAddress(int addressPosition, byte* addressBuffer, int start, int length)
{
    int addressBufferPos = start;
    for (int i = addressPosition; i < position; i++)
    {
        addressBuffer[addressBufferPos++] = buffer[i];
        
        // LSB=1 means this was the last address byte
        if ((buffer[i] & 0x01) == 0x01)  
            break;

        // See if we've reached last byte, try again when we've got more data
        else if (i == position - 1)
            return 0;
    }
    return addressBufferPos - start;
}

ushort DlmsReader::GetChecksum(int checksumPosition)
{
    return (ushort)(buffer[checksumPosition + 2] << 8 |
        buffer[checksumPosition + 1]);
}
