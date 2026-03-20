#include "pch.h"
#include "SandforgePacketBuffer.h"

namespace
{
    template <typename TValue>
    void appendBytes(vector<uint8_t>& buffer, const TValue& value)
    {
        const auto* bytes = reinterpret_cast<const uint8_t*>(&value);
        buffer.insert(buffer.end(), bytes, bytes + sizeof(TValue));
    }
}

SandforgePacketWriter::SandforgePacketWriter(SandforgePacketType type, uint32_t sequence)
{
    appendBytes(_buffer, static_cast<uint16_t>(0));
    appendBytes(_buffer, static_cast<uint16_t>(type));
    appendBytes(_buffer, sequence);
}

void SandforgePacketWriter::writeUInt8(uint8_t value)
{
    appendBytes(_buffer, value);
}

void SandforgePacketWriter::writeUInt16(uint16_t value)
{
    appendBytes(_buffer, value);
}

void SandforgePacketWriter::writeUInt32(uint32_t value)
{
    appendBytes(_buffer, value);
}

void SandforgePacketWriter::writeFloat(float value)
{
    appendBytes(_buffer, value);
}

void SandforgePacketWriter::writeFixedString(const string& value, size_t fixedSize)
{
    const size_t writeSize = (std::min)(value.size(), fixedSize);
    _buffer.insert(_buffer.end(), value.data(), value.data() + writeSize);
    _buffer.insert(_buffer.end(), fixedSize - writeSize, 0);
}

const vector<uint8_t>& SandforgePacketWriter::buffer() const
{
    auto& sizeField = *reinterpret_cast<uint16_t*>(const_cast<uint8_t*>(_buffer.data()));
    sizeField = static_cast<uint16_t>(_buffer.size());
    return _buffer;
}

SandforgePacketReader::SandforgePacketReader(const uint8_t* data, size_t size)
    : _data(data), _size(size)
{
}

bool SandforgePacketReader::readUInt8(uint8_t& outValue)
{
    if (_offset + sizeof(uint8_t) > _size)
    {
        _valid = false;
        return false;
    }

    outValue = *reinterpret_cast<const uint8_t*>(_data + _offset);
    _offset += sizeof(uint8_t);
    return true;
}

bool SandforgePacketReader::readUInt16(uint16_t& outValue)
{
    if (_offset + sizeof(uint16_t) > _size)
    {
        _valid = false;
        return false;
    }

    memcpy(&outValue, _data + _offset, sizeof(uint16_t));
    _offset += sizeof(uint16_t);
    return true;
}

bool SandforgePacketReader::readUInt32(uint32_t& outValue)
{
    if (_offset + sizeof(uint32_t) > _size)
    {
        _valid = false;
        return false;
    }

    memcpy(&outValue, _data + _offset, sizeof(uint32_t));
    _offset += sizeof(uint32_t);
    return true;
}

bool SandforgePacketReader::readFloat(float& outValue)
{
    if (_offset + sizeof(float) > _size)
    {
        _valid = false;
        return false;
    }

    memcpy(&outValue, _data + _offset, sizeof(float));
    _offset += sizeof(float);
    return true;
}

bool SandforgePacketReader::readFixedString(string& outValue, size_t fixedSize)
{
    if (_offset + fixedSize > _size)
    {
        _valid = false;
        return false;
    }

    outValue.assign(reinterpret_cast<const char*>(_data + _offset), fixedSize);
    const size_t nullPos = outValue.find('\0');
    if (nullPos != string::npos)
    {
        outValue.resize(nullPos);
    }

    _offset += fixedSize;
    return true;
}

bool SandforgePacketReader::isValid() const
{
    return _valid;
}
