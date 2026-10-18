#pragma once

#include "Event/EventDispatcher.h"
#include "Packet/PacketUtils.h"
#include "Precompiled.h"

#ifdef ENABLE_LUA_SCRIPTING
#include <lua.hpp>
#endif

class GamePlayer;

#ifdef ENABLE_LUA_SCRIPTING
struct LuaDialogInfo
{
    string name;
    int luaRef = LUA_NOREF;
};
#endif

class DialogManager
{
public:
    DialogManager() = default;
    ~DialogManager();

public:
    static DialogManager* GetInstance()
    {
        static DialogManager instance;
        return &instance;
    }

public:
    void Execute(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    void RegisterAllDialogs();

#ifdef ENABLE_LUA_SCRIPTING
    bool RegisterLuaDialog(lua_State* L, const string& name, int funcStackIdx);
    bool UnregisterLuaDialog(lua_State* L, const string& name);
    void ClearLuaDialogs(lua_State* L);
#endif

private:
#ifdef ENABLE_LUA_SCRIPTING
    void ExecuteLuaDialog(const LuaDialogInfo& luaDialog, GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
#endif

    template <void (*DialogFunction)(GamePlayer*, ParsedTextPacket<38>&)> void RegisterDialog(uint32 eventHash)
    {
        m_dispatcher.Register(eventHash, Delegate<GamePlayer*, ParsedTextPacket<38>&>::Create<DialogFunction>());
    }

    EventDispatcher<uint32, GamePlayer*, ParsedTextPacket<38>&> m_dispatcher;

#ifdef ENABLE_LUA_SCRIPTING
    std::unordered_map<uint32, LuaDialogInfo> m_luaDialogs;
#endif
};

DialogManager* GetDialogManager();