#include "ProtonUtils.h"
#include "../Math/Color.h"
#include "../Math/Math.h"

/**
 *
 * TAKEN FROM Seth A. Robinson's ProtonSDK
 * https://github.com/SethRobinson/proton
 *
 */

string XorCipherString(const string& str, const char* secret, int32 id)
{
    uint32 secretLen = strlen(secret);
    id %= secretLen;

    string out = str;

    for (uint32 i = 0; i < str.size(); ++i)
    {
        out[i] = (str[i] ^ secret[id++]);
        if (id >= secretLen)
        {
            id = 0;
        }
    }

    return out;
}

uint32 Proton::ColorCombine(uint32 c1, uint32 c2, float alphaMod)
{
    int r, g, b, a;

    Color c1_(c1);
    Color c2_(c2);

    if (c2_.GetAsUINT() != 0xFFFFFFFF)
    {
        r = c1_.r - (255 - c2_.r);
        g = c1_.g - (255 - c2_.g);
        b = c1_.b - (255 - c2_.b);
        a = c1_.a - (255 - c2_.a);

        r = Max(0, r);
        r = Min(255, r);
        g = Max(0, g);
        g = Min(255, g);
        b = Max(0, b);
        b = Min(255, b);
        a = Max(0, a);
        a = Min(255, a);
    }
    else
    {
        if (alphaMod == 1)
            return c1;

        r = c1_.r;
        g = c1_.g;
        b = c1_.b;
        a = c1_.a;
    }

    return Color(r, g, b, uint8(float(a) * alphaMod)).GetAsUINT();
}

uint32 Proton::HashString(const char* str, int32 len)
{
    if (!str)
        return 0;

    unsigned char* n = (unsigned char*)str;
    uint32 acc = 0x55555555;

    if (len == 0)
    {
        while (*n)
            acc = (acc >> 27) + (acc << 5) + *n++;
    }
    else
    {
        for (int32 i = 0; i < len; i++)
        {
            acc = (acc >> 27) + (acc << 5) + *n++;
        }
    }
    return acc;
}

uint8 Proton::ConvertVariantType(eVariantTypes type)
{
    switch (type)
    {
        case VARIANT_TYPE_UINT:
            return 5;
        case VARIANT_TYPE_INT:
            return 9;
        case VARIANT_TYPE_FLOAT:
            return 1;
        case VARIANT_TYPE_STRING:
            return 2;
        case VARIANT_TYPE_VECTOR2INT:
        case VARIANT_TYPE_VECTOR2FLOAT:
            return 3;
        case VARIANT_TYPE_VECTOR3INT:
        case VARIANT_TYPE_VECTOR3FLOAT:
            return 4;

        default:
            return 0;
    }

    return 0;
}

uint8* Proton::SerializeToMem(const VariantVector& varVector, uint32* pSizeOut, uint8* pDest)
{
    int varsUsed = 0;
    int memNeeded = 0;

    int tempSize;

    for (int i = 0; i < varVector.size(); ++i)
    {
        if (varVector[i].GetType() == VARIANT_TYPE_STRING)
        {
            tempSize = (int)varVector[i].GetString().size() +
                       4; // the 4 is for an int showing how long the string will be when writing
        }
        else
        {
            tempSize = varVector[i].GetSize();
        }

        if (tempSize > 0)
        {
            varsUsed++;
            memNeeded += tempSize;
        }
    }

    int totalSize = memNeeded + 1 + (varsUsed * 2);

    if (!pDest)
    {
        pDest = new uint8[totalSize]; // 1 is to write how many are coming
    }

    // write it

    uint8* pCur = pDest;

    pCur[0] = uint8(varsUsed);
    pCur++;

    uint8 type;

    for (int i = 0; i < varVector.size(); ++i)
    {
        if (varVector[i].GetType() == VARIANT_TYPE_STRING)
        {
            type = i;
            memcpy(pCur, &type, 1);
            pCur += 1; // index

            type = ConvertVariantType(VARIANT_TYPE_STRING);
            memcpy(pCur, &type, 1);
            pCur += 1; // type

            uint32 s = (int)varVector[i].GetString().size();
            memcpy(pCur, &s, 4);
            pCur += 4; // length of string
            memcpy(pCur, varVector[i].GetString().c_str(), s);
            pCur += s; // actual string data
        }
        else
        {
            tempSize = varVector[i].GetSize();

            if (tempSize > 0)
            {
                type = i;
                memcpy(pCur, &type, 1);
                pCur += 1; // index

                type = ConvertVariantType(varVector[i].GetType());
                memcpy(pCur, &type, 1);
                pCur += 1; // type

                auto varValue = varVector[i].GetValue();
                memcpy(pCur, &varValue, tempSize);
                pCur += tempSize;
            }
        }
    }

    *pSizeOut = totalSize;
    return pDest;
}

uint32 Proton::GetMemEstiamte(const VariantVector& varVector)
{
    int varsUsed = 0;
    int memNeeded = 0;

    int tempSize;

    for (int i = 0; i < varVector.size(); ++i)
    {
        if (varVector[i].GetType() == VARIANT_TYPE_STRING)
        {
            tempSize = (int)varVector[i].GetString().size() +
                       4; // the 4 is for an int showing how long the string will be when writing
        }
        else
        {
            tempSize = varVector[i].GetSize();
        }

        if (tempSize > 0)
        {
            varsUsed++;
            memNeeded += tempSize;
        }
    }

    return memNeeded + 1 + (varsUsed * 2);
}
