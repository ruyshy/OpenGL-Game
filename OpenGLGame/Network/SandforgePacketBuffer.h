#pragma once

#ifndef SANDFORGE_PACKET_BUFFER_H_
#define SANDFORGE_PACKET_BUFFER_H_

#include "SandforgePacketTypes.h"

class SandforgePacketWriter
{
public:
    explicit SandforgePacketWriter(SandforgePacketType type, uint32_t sequence);

    void writeUInt8(uint8_t value);
    void writeUInt16(uint16_t value);
    void writeUInt32(uint32_t value);
    void writeFloat(float value);
    void writeFixedString(const string& value, size_t fixedSize);
    const vector<uint8_t>& buffer() const;

private:
    vector<uint8_t> _buffer;
};

class SandforgePacketReader
{
public:
    SandforgePacketReader(const uint8_t* data, size_t size);

    bool readUInt8(uint8_t& outValue);
    bool readUInt16(uint16_t& outValue);
    bool readUInt32(uint32_t& outValue);
    bool readFloat(float& outValue);
    bool readFixedString(string& outValue, size_t fixedSize);
    bool isValid() const;

private:
    const uint8_t* _data = nullptr;
    size_t _size = 0;
    size_t _offset = 0;
    bool _valid = true;
};

#endif // !SANDFORGE_PACKET_BUFFER_H_
