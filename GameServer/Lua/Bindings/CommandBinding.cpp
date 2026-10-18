#ifdef ENABLE_LUA_SCRIPTING

#include "CommandBinding.h"
#include "../../Command/CommandManager.h"
#include "../LuaManager.h"

static int luaRegisterCommand(lua_State* L)
{
    if (lua_gettop(L) < 5)
        return 0;

    string name = luaL_checkstring(L, 1);
    string usage = luaL_checkstring(L, 2);
    string desc = luaL_checkstring(L, 3);
    string perm = luaL_checkstring(L, 4);

    bool res = GetCommandManager()->RegisterLuaCommand(L, name, usage, desc, perm, 5);
    lua_pushboolean(L, res);
    return 1;
}

static int luaRegisterCommandWithAliases(lua_State* L)
{
    if (lua_gettop(L) < 5 || !lua_istable(L, 1))
        return 0;

    std::vector<string> aliases;
    usize len = lua_objlen(L, 1);

    for (int32 i = 1; i <= len; ++i)
    {
        lua_rawgeti(L, 1, (int)i);
        if (lua_isstring(L, -1))
        {
            aliases.push_back(lua_tostring(L, -1));
        }

        lua_pop(L, 1);
    }

    string usage = luaL_checkstring(L, 2);
    string desc = luaL_checkstring(L, 3);
    string perm = luaL_checkstring(L, 4);

    bool res = GetCommandManager()->RegisterLuaCommandWithAliases(L, aliases, usage, desc, perm, 5);
    lua_pushboolean(L, res);
    return 1;
}

static int luaUnregisterCommand(lua_State* L)
{
    string name = luaL_checkstring(L, 1);
    bool res = GetCommandManager()->UnregisterLuaCommand(L, name);
    lua_pushboolean(L, res);
    return 1;
}

void RegisterCommandBinding()
{
    LuaManager* pLuaManager = GetLuaManager();

    pLuaManager->RegisterFunction("registerCommand", &luaRegisterCommand);
    pLuaManager->RegisterFunction("registerCommandWithAliases", &luaRegisterCommandWithAliases);
    pLuaManager->RegisterFunction("unregisterCommand", &luaUnregisterCommand);
}

#endif