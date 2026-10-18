#include "PlayerExtraData.h"
#include "GamePlayer.h"
#include "IO/Log.h"

PlayerExtraData::PlayerExtraData(GamePlayer* pPlayer) : m_pPlayer(pPlayer)
{
    m_data.resize(PLAYER_EXTRA_COUNT);
}

PlayerExtraData::~PlayerExtraData() {}

bool PlayerExtraData::Set(ePlayerExtraKey key, const Variant& data)
{
    if (key >= PLAYER_EXTRA_COUNT)
    {
        LOGGER_LOG_ERROR("[PlayerExtraData] Out of bounds key: %d for player: %d", key,
                         (m_pPlayer ? m_pPlayer->GetUserID() : -1));
        return false;
    }

    m_data[key] = data;
    return true;
}

const Variant& PlayerExtraData::Get(ePlayerExtraKey key) const
{
    if (key >= PLAYER_EXTRA_COUNT)
    {
        static const Variant emptyVariant;
        return emptyVariant;
    }

    return m_data[key];
}

bool PlayerExtraData::Has(ePlayerExtraKey key) const
{
    if (key >= PLAYER_EXTRA_COUNT)
        return false;

    return m_data[key].GetType() != VARIANT_TYPE_NONE;
}

void PlayerExtraData::Clear(ePlayerExtraKey key)
{
    if (key < PLAYER_EXTRA_COUNT)
        m_data[key] = Variant();
}

void PlayerExtraData::ClearAll()
{
    for (auto& var : m_data)
        var = Variant();
}

void PlayerExtraData::Serialize(MemoryBuffer& memBuffer, bool write)
{
    if (write)
    {
        uint16 activeCount = 0;
        for (auto& var : m_data)
        {
            if (var.GetType() != VARIANT_TYPE_NONE)
                activeCount++;
        }

        memBuffer.Write(activeCount);

        for (uint16 key = 0; key < PLAYER_EXTRA_COUNT; ++key)
        {
            auto& var = m_data[key];
            if (var.GetType() == VARIANT_TYPE_NONE)
                continue;

            memBuffer.Write(key);
            memBuffer.Write((uint8)(var.GetType()));

            switch (var.GetType())
            {
                case VARIANT_TYPE_STRING:
                    memBuffer.WriteStringRaw(var.GetString());
                    break;
                case VARIANT_TYPE_FLOAT:
                    memBuffer.Write(var.GetFloat());
                    break;
                case VARIANT_TYPE_UINT:
                    memBuffer.Write(var.GetUINT());
                    break;
                case VARIANT_TYPE_BOOL:
                    memBuffer.Write(var.GetBool());
                    break;
                case VARIANT_TYPE_INT:
                    memBuffer.Write(var.GetINT());
                    break;
                case VARIANT_TYPE_VECTOR2INT:
                    memBuffer.Write(var.GetVector2Int());
                    break;
                case VARIANT_TYPE_VECTOR2FLOAT:
                    memBuffer.Write(var.GetVector2Float());
                    break;
                case VARIANT_TYPE_VECTOR3INT:
                    memBuffer.Write(var.GetVector3Int());
                    break;
                case VARIANT_TYPE_VECTOR3FLOAT:
                    memBuffer.Write(var.GetVector3Float());
                    break;
                default:
                    break;
            }
        }
    }
    else
    {
        ClearAll();

        uint16 dataCount = 0;
        memBuffer.Read(dataCount);

        for (uint16 i = 0; i < dataCount; ++i)
        {
            uint16 keyInt = 0;
            memBuffer.Read(keyInt);

            uint8 typeInt = 0;
            memBuffer.Read(typeInt);

            ePlayerExtraKey key = (ePlayerExtraKey)(keyInt);
            eVariantTypes type = (eVariantTypes)(typeInt);

            Variant var;
            switch (type)
            {
                case VARIANT_TYPE_STRING:
                {
                    std::string str;
                    memBuffer.ReadStringRaw(str);
                    var = str;
                    break;
                }
                case VARIANT_TYPE_FLOAT:
                {
                    float val = 0.0f;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
                case VARIANT_TYPE_UINT:
                {
                    uint32 val = 0;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
                case VARIANT_TYPE_BOOL:
                {
                    bool val = false;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
                case VARIANT_TYPE_INT:
                {
                    int32 val = 0;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
                case VARIANT_TYPE_VECTOR2INT:
                {
                    Vector2Int val;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
                case VARIANT_TYPE_VECTOR2FLOAT:
                {
                    Vector2Float val;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
                case VARIANT_TYPE_VECTOR3INT:
                {
                    Vector3Int val;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
                case VARIANT_TYPE_VECTOR3FLOAT:
                {
                    Vector3Float val;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
                default:
                    break;
            }

            if (key < PLAYER_EXTRA_COUNT)
            {
                m_data[key] = var;
            }
        }
    }
}

uint32 PlayerExtraData::GetMemEstimate() const
{
    uint32 memEstimate = sizeof(uint16);
    for (auto& data : m_data)
    {
        if (data.GetType() == VARIANT_TYPE_NONE)
            continue;

        memEstimate += sizeof(uint16) + sizeof(uint8) + data.GetSize();
        if (data.GetType() == VARIANT_TYPE_STRING)
            memEstimate += sizeof(uint16);
    }

    return memEstimate;
}