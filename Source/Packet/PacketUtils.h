#pragma once

#include "../Precompiled.h"
#include "../Utils/StringUtils.h"

struct TextPacketField
{
    uint32 hash = 0;
    const char* key = nullptr;
    uint16 keySize = 0;
    const char* value = nullptr;
    uint16 valueSize = 0;
    const char* extraValues = nullptr;
    uint16 extraSize = 0;

    std::string_view GetKeyStringView() const { return std::string_view(key, keySize); }
    std::string_view GetStringView() const { return std::string_view(value, valueSize); }
    string GetString() const { return string(value, valueSize); }

    eToIntResult GetUInt(uint32& out) const { return ToUInt(value, valueSize, out); }
    eToIntResult GetInt(int32& out) const { return ToInt(value, valueSize, out); }
    eToIntResult GetBool(bool& out) const
    {
        int32 tempVal = 0;

        eToIntResult res = ToInt(value, valueSize, tempVal);
        if (res != TO_INT_SUCCESS)
            return res;

        out = (tempVal != 0);
        return TO_INT_SUCCESS;
    }

    // multiple value support (trade system)
    std::string_view GetValueAt(uint8 index) const
    {
        if (index == 0)
            return GetStringView();

        if (!extraValues || extraSize == 0)
            return {};

        uint8 currentIndex = 1;
        const char* p = extraValues;
        const char* end = extraValues + extraSize;

        while (p < end)
        {
            const char* nextPipe = (const char*)memchr(p, '|', end - p);
            const char* valEnd = nextPipe ? nextPipe : end;

            if (currentIndex == index)
                return std::string_view(p, (usize)(valEnd - p));

            if (!nextPipe)
                break;

            p = nextPipe + 1;
            currentIndex++;
        }

        return {};
    }

    eToIntResult GetIntAt(uint8 index, int32& out) const
    {
        if (index == 0)
            return GetInt(out);

        std::string_view sv = GetValueAt(index);
        if (sv.empty())
            return TO_INT_FAIL;

        return ToInt(sv.data(), (uint32)(sv.size()), out);
    }

    eToIntResult GetUIntAt(uint8 index, uint32& out) const
    {
        if (index == 0)
            return GetUInt(out);

        std::string_view sv = GetValueAt(index);
        if (sv.empty())
            return TO_INT_FAIL;

        return ToUInt(sv.data(), (uint32)(sv.size()), out);
    }
};

template <uint8 N> struct ParsedTextPacket
{
    TextPacketField fields[N];
    uint8 count;

    TextPacketField* Find(uint32 hash)
    {
        for (uint8 i = 0; i < count; ++i)
        {
            if (fields[i].hash == hash)
                return &fields[i];
        }
        return nullptr;
    }

    // thanks that i worked with Orbion lol
    // multiple key support (trade system)
    struct KeyIterator
    {
        // lol forgot const
        const ParsedTextPacket* packet;
        uint32 hash;
        uint8 index;

        bool operator!=(const KeyIterator& other) const { return index != other.index; }

        void operator++()
        {
            for (uint8 i = index + 1; i < packet->count; ++i)
            {
                if (packet->fields[i].hash == hash)
                {
                    index = i;
                    return;
                }
            }
            index = packet->count;
        }

        const TextPacketField& operator*() const { return packet->fields[index]; }
    };

    struct KeyRange
    {
        const ParsedTextPacket* packet;
        uint32 hash;

        KeyIterator begin() const
        {
            for (uint8 i = 0; i < packet->count; ++i)
            {
                if (packet->fields[i].hash == hash)
                    return KeyIterator{packet, hash, i};
            }
            return end();
        }

        KeyIterator end() const { return KeyIterator{packet, hash, packet->count}; }
    };

    KeyRange FindAll(uint32 hash) const { return KeyRange{this, hash}; }
};

template <uint8 N> inline void ParseTextPacket(const char* data, uint32 size, ParsedTextPacket<N>& out)
{
    out.count = 0;
    if (!data || size == 0)
        return;

    const char* p = data;
    const char* end = data + size;

    while (p < end && out.count < N)
    {
        while (p < end && (*p == '\n' || *p == '\r'))
            ++p;

        if (p >= end)
            break;

        const char* nextNL = (const char*)(memchr(p, '\n', end - p));
        const char* lineEnd = nextNL ? nextNL : end;
        const char* lineStart = p;
        p = nextNL ? nextNL + 1 : end;

        if (lineEnd > lineStart && *(lineEnd - 1) == '\r')
            --lineEnd;

        // |itemID|5 -> itemID|5
        while (lineStart < lineEnd && *lineStart == '|')
            ++lineStart;

        // itemID|5| -> itemID|5
        while (lineEnd > lineStart && *(lineEnd - 1) == '|')
            --lineEnd;

        if (lineStart >= lineEnd)
            continue;

        const char* pipe = (const char*)(memchr(lineStart, '|', lineEnd - lineStart));
        if (!pipe)
            continue;

        const char* keyStart = lineStart;
        const char* keyEnd = pipe;
        const char* valStart = pipe + 1;
        const char* valEnd = lineEnd;
        const char* extraStart = nullptr;
        const char* extraEnd = nullptr;

        const char* secondPipe = (const char*)(memchr(pipe + 1, '|', lineEnd - (pipe + 1)));
        if (secondPipe)
        {
            const char* secondKeyStart = secondPipe;
            while (secondKeyStart > valStart)
            {
                char c = *(secondKeyStart - 1);
                if (IsAlpha(c))
                {
                    --secondKeyStart;
                }
                else
                    break;
            }

            // key1|val1key2|val2 - somehow lmode comes like this
            if (secondKeyStart > valStart && secondKeyStart < secondPipe)
            {
                valEnd = secondKeyStart;

                if (keyStart < keyEnd && valStart <= valEnd && out.count < N)
                {
                    uint32 hash = HashString(keyStart, (uint32)(keyEnd - keyStart));
                    out.fields[out.count++] = {
                        hash, keyStart, (uint16)(keyEnd - keyStart), valStart, (uint16)(valEnd - valStart), nullptr, 0};
                }

                keyStart = secondKeyStart;
                keyEnd = secondPipe;
                valStart = secondPipe + 1;
                valEnd = lineEnd;
            }
            else
            {
                valEnd = secondPipe;
                extraStart = secondPipe + 1;
                extraEnd = lineEnd;
            }
        }

        if (valEnd > valStart && *(valEnd - 1) == '\0')
            --valEnd;

        if (extraEnd > extraStart && *(extraEnd - 1) == '\0')
            --extraEnd;

        if (keyStart >= keyEnd || valStart > valEnd)
            continue;

        uint32 hash = HashString(keyStart, (uint32)(keyEnd - keyStart));
        uint16 extraLen = (extraStart && extraEnd >= extraStart) ? (uint16)(extraEnd - extraStart) : 0;

        out.fields[out.count++] = {
            hash, keyStart, (uint16)(keyEnd - keyStart), valStart, (uint16)(valEnd - valStart), extraStart, extraLen};
    }
}