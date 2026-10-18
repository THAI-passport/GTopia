#pragma once

#include "../IO/Log.h"
#include "../Precompiled.h"
#include "../Utils/Variant.h"
#include "TCPPacket.h"
#include "VariantPacket.h"

#define PRESENCE_MAX_ELEMENTS 4096

enum eTCPPacketType
{
    TCP_PACKET_NONE,
    TCP_PACKET_HELLO,
    TCP_PACKET_AUTH,
    TCP_PACKET_PLAYER_CHECK_SESSION,
    TCP_PACKET_PLAYER_END_SESSION,
    TCP_PACKET_WORLD_INIT,
    TCP_PACKET_RENDER_WORLD,
    TCP_PACKET_HEARTBEAT,
    TCP_PACKET_WORLD_SEND_PLAYER,
    TCP_PACKET_KILL_SERVER,
    TCP_PACKET_COMMAND,
    TCP_PACKET_WORLD_PLAYER_SESSION,

    TCP_PACKET_PLAYER_SUBSCRIBE,
    TCP_PACKET_PLAYER_UNSUBSCRIBE,
    TCP_PACKET_PLAYER_UPDATE,
    TCP_PACKET_PLAYER_SNAPSHOT,

    TCP_PACKET_WORLD_SNAPSHOT,
    TCP_PACKET_WORLD_UPDATE,
    TCP_PACKET_WORLD_REMOVE,

    TCP_PACKET_STATS_UPDATE,
    TCP_PACKET_READY,
    TCP_PACKET_HOLIDAY_SYNC,
    TCP_PACKET_RELOAD_FILE,

    TCP_RENDER_REQUEST = 1000,
    TCP_RENDER_RESULT = 1001,

    TCP_WORLD_PLAYER_LEAVE = 1010,
    TCP_WORLD_PLAYER_JOIN = 1011,

    TCP_COMMAND_GENERIC_RESPONSE = 1100,
    TCP_COMMAND_EXECUTE_TEXT = 1101,
    TCP_COMMAND_SETROLE = 1102,
    TCP_COMMAND_BROADCAST_MESSAGE = 1103,
    TCP_COMMAND_GIVE_PLAYMOD = 1104,
    TCP_COMMAND_LUA = 1105
};

enum eLuaAction
{
    LUA_ACTION_RELOAD,
    LUA_ACTION_LOAD,
    LUA_ACTION_UNLOAD,
    LUA_ACTION_PROFILE_ON,
    LUA_ACTION_PROFILE_OFF,
    LUA_ACTION_PROFILE_RESET,
    LUA_ACTION_PROFILE_DUMP
};

enum eTCPPacketResult
{
    TCP_RESULT_FAIL,
    TCP_RESULT_OK
};

enum eMessagePacketType
{
    NET_MESSAGE_UNKNOWN,
    NET_MESSAGE_SERVER_HELLO,
    NET_MESSAGE_GENERIC_TEXT,
    NET_MESSAGE_GAME_MESSAGE,
    NET_MESSAGE_GAME_PACKET,
    NET_MESSAGE_ERROR,
    NET_MESSAGE_TRACK,
    NET_MESSAGE_CLIENT_LOG_REQUEST,
    NET_MESSAGE_CLIENT_LOG_RESPONSE
};

enum eGamePacketType
{
    NET_GAME_PACKET_STATE,
    NET_GAME_PACKET_CALL_FUNCTION,
    NET_GAME_PACKET_UPDATE_STATUS,
    NET_GAME_PACKET_TILE_CHANGE_REQUEST,
    NET_GAME_PACKET_SEND_MAP_DATA,
    NET_GAME_PACKET_SEND_TILE_UPDATE_DATA,
    NET_GAME_PACKET_SEND_TILE_UPDATE_DATA_MULTIPLE,
    NET_GAME_PACKET_TILE_ACTIVATE_REQUEST,
    NET_GAME_PACKET_TILE_APPLY_DAMAGE,
    NET_GAME_PACKET_SEND_INVENTORY_STATE,
    NET_GAME_PACKET_ITEM_ACTIVATE_REQUEST,
    NET_GAME_PACKET_ITEM_ACTIVATE_OBJECT_REQUEST,
    NET_GAME_PACKET_SEND_TILE_TREE_STATE,
    NET_GAME_PACKET_MODIFY_ITEM_INVENTORY,
    NET_GAME_PACKET_ITEM_CHANGE_OBJECT,
    NET_GAME_PACKET_SEND_LOCK,
    NET_GAME_PACKET_SEND_ITEM_DATABASE_DATA,
    NET_GAME_PACKET_SEND_PARTICLE_EFFECT,
    NET_GAME_PACKET_SET_ICON_STATE,
    NET_GAME_PACKET_ITEM_EFFECT,
    NET_GAME_PACKET_SET_CHARACTER_STATE,
    NET_GAME_PACKET_PING_REPLY,
    NET_GAME_PACKET_PING_REQUEST,
    NET_GAME_PACKET_GOT_PUNCHED,
    NET_GAME_PACKET_APP_CHECK_RESPONSE,
    NET_GAME_PACKET_APP_INTEGRITY_FAIL,
    NET_GAME_PACKET_DISCONNECT,
    NET_GAME_PACKET_BATTLE_JOIN,
    NET_GAME_PACKET_BATTLE_EVENT,
    NET_GAME_PACKET_USE_DOOR,
    NET_GAME_PACKET_SEND_PARENTAL,
    NET_GAME_PACKET_GONE_FISHIN,
    NET_GAME_PACKET_STEAM,
    NET_GAME_PACKET_PET_BATTLE,
    NET_GAME_PACKET_NPC,
    NET_GAME_PACKET_SPECIAL,
    NET_GAME_PACKET_SEND_PARTICLE_EFFECT_V2,
    NET_GAME_PACKET_ACTIVE_ARROW_TO_ITEM,
    NET_GAME_PACKET_SELEC_TILE_INDEX,
    NET_GAME_PACKET_SEND_PLAYER_TRIBUTE_DATA,
    NET_GAME_PACKET_ON_STEP_ONILE_MOD
};

enum eGamePacketFlags
{
    GAME_PACKET_FLAG_BROADCAST = 1 << 0,
    GAME_PACKET_FLAG_NO_LERP = 1 << 2,
    GAME_PACKET_FLAG_EXTENDED_DATA = 1 << 3,
    GAME_PACKET_FLAG_FACING_LEFT = 1 << 4,
    GAME_PACKET_FLAG_ON_GROUND = 1 << 5,
    GAME_PACKET_FLAG_ON_BURN = 1 << 6,
    GAME_PACKET_FLAG_ON_JUMP = 1 << 7,
    GAME_PACKET_FLAG_ON_KILL = 1 << 8,
    GAME_PACKET_FLAG_ON_PUNCH = 1 << 9,
    GAME_PACKET_FLAG_ON_CREATE = 1 << 10,
    GAME_PACKET_FLAG_SEND_ASAP = 1 << 11,
    GAME_PACKET_FLAG_ON_GOT_PUNCH = 1 << 12,
    GAME_PACKET_FLAG_ON_RESPAWN = 1 << 13,
    GAME_PACKET_FLAG_ON_GOT_OBJECT = 1 << 14,
    GAME_PACKET_FLAG_ON_TRAMPOLINE = 1 << 15,
    GAME_PACKET_FLAG_ON_PRICK = 1 << 16,
    GAME_PACKET_FLAG_ON_ICE = 1 << 17,
    GAME_PACKET_FLAG_HAND_ITEM_ACTIVE = 1 << 18,
    GAME_PACKET_FLAG_BACK_ITEM_ACTIVE = 1 << 19,
    GAME_PACKET_FLAG_UNDERWATER = 1 << 20,
    GAME_PACKET_FLAG_WALLHANG = 1 << 21,
    GAME_PACKET_FLAG_CHARGING_PUNCH = 1 << 22,
    GAME_PACKET_FLAG_CHARGING_PUNCH_END = 1 << 23,
    GAME_PACKET_FLAG_CHARGING_PUNCH_LEVEL_UP = 1 << 24,
    GAME_PACKET_FLAG_PLAY_HAY_CART_VISUALS = 1 << 25,
    GAME_PACKET_FLAG_DOUBLE_JUMP_ACTIVE = 1 << 26,
    GAME_PACKET_FLAG_EXTRA_TILE_INFO = 1 << 27,
    GAME_PACKET_FLAG_ON_ACID = 1 << 28,
    GAME_PACKET_FLAG_TELEPORT = 1 << 29,
    GAME_PACKET_FLAG_ON_USE_ABILITY = 1 << 30
};

#pragma pack(push, 1)
struct GameUpdatePacket
{
    GameUpdatePacket() { Clear(); }
    void Clear() { memset(this, 0, sizeof(GameUpdatePacket)); }

    uint8 type = NET_GAME_PACKET_STATE;

    uint8 field_1 = 0;
    uint8 field_2 = 0;
    uint8 field_3 = 0;
    int32 field_4 = 0;
    int32 field_5 = 0;
    uint32 flags = 0;
    float field_6 = 0.0f;
    int32 field_7 = 0;
    Vector2Float field_8;
    Vector2Float field_9;
    float field_10 = 0.0f;
    int32 field_11 = 0;
    int32 field_12 = 0;

    uint32 extraDataSize = 0;

    bool HasFlag(eGamePacketFlags flag) { return flags & flag; }
    void SetFlag(eGamePacketFlags flag) { flags |= flag; }
};
#pragma pack(pop)

struct PlayerPresencePacketElement
{
    uint32 userID = 0;
    uint8 status = 0;
    string name;

    void Write(TCPPacketWriter& w) const
    {
        w.Write<uint32>(userID);
        w.Write<uint8>(status);
        w.WriteString(name);
    }

    bool Read(TCPPacketReader& r) { return r.Read<uint32>(userID) && r.Read<uint8>(status) && r.ReadString(name); }
};

struct WorldPresenceSnapshotElement
{
    uint32 worldID = 0;
    string name;
    uint16 playerCount = 0;
    uint8 worldType = 0;
    bool isSignalJammed = false;

    void Write(TCPPacketWriter& w) const
    {
        w.Write<uint32>(worldID);
        w.WriteString(name);
        w.Write<uint16>(playerCount);
        w.Write<uint8>(worldType);
        w.Write<uint8>(isSignalJammed ? 1 : 0);
    }

    bool Read(TCPPacketReader& r)
    {
        uint8 jammed = 0;
        if (!r.Read(worldID) || !r.ReadString(name) || !r.Read(playerCount) || !r.Read(worldType) ||
            !r.Read<uint8>(jammed))
            return false;

        isSignalJammed = (jammed != 0);
        return true;
    }
};

struct WorldPresenceUpdateElement
{
    uint32 worldID = 0;
    uint16 playerCount = 0;
    bool isSignalJammed = false;

    void Write(TCPPacketWriter& w) const
    {
        w.Write(worldID);
        w.Write(playerCount);
        w.Write<uint8>(isSignalJammed ? 1 : 0);
    }

    bool Read(TCPPacketReader& r)
    {
        uint8 jammed = 0;
        if (!r.Read(worldID) || !r.Read(playerCount) || !r.Read(jammed))
            return false;

        isSignalJammed = (jammed != 0);
        return true;
    }
};

struct WorldPresenceRemoveElement
{
    uint32 worldID = 0;

    void Write(TCPPacketWriter& w) const { w.Write(worldID); }
    bool Read(TCPPacketReader& r) { return r.Read(worldID); }
};

template <typename T> void WritePresenceElements(TCPPacketWriter& w, const std::vector<T>& elems)
{
    w.Write<uint32>(elems.size());
    for (const T& e : elems)
        e.Write(w);
}

template <typename T> bool ReadPresenceElements(TCPPacketReader& r, std::vector<T>& out)
{
    out.clear();

    uint32 count = 0;
    if (!r.Read(count) || count > PRESENCE_MAX_ELEMENTS)
        return false;

    out.reserve(count);
    for (uint32 i = 0; i < count; ++i)
    {
        T e;
        if (!e.Read(r))
        {
            out.clear();
            return false;
        }
        out.push_back(std::move(e));
    }
    return true;
}