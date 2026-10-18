#ifdef ENABLE_LUA_SCRIPTING

#include "DialogBinding.h"
#include "../../Dialog/DialogManager.h"
#include "../LuaManager.h"

static string get(ParsedTextPacket<38>* self, const char* key)
{
    if (!self || !key)
        return "";
    auto* field = self->Find(HashString(key));
    return field ? string(field->value, field->valueSize) : "";
}

static int32 getInt(ParsedTextPacket<38>* self, const char* key)
{
    if (!self || !key)
        return 0;
    auto* field = self->Find(HashString(key));
    int32 val = 0;
    if (field && field->GetInt(val) == TO_INT_SUCCESS)
        return val;
    return 0;
}

static uint32 getUInt(ParsedTextPacket<38>* self, const char* key)
{
    if (!self || !key)
        return 0;
    auto* field = self->Find(HashString(key));
    uint32 val = 0;
    if (field && field->GetUInt(val) == TO_INT_SUCCESS)
        return val;
    return 0;
}

static bool getBool(ParsedTextPacket<38>* self, const char* key)
{
    if (!self || !key)
        return false;
    auto* field = self->Find(HashString(key));
    bool val = false;
    if (field && field->GetBool(val) == TO_INT_SUCCESS)
        return val;
    return false;
}

static int luaRegisterDialog(lua_State* L)
{
    if (lua_gettop(L) < 2)
        return 0;

    string name = luaL_checkstring(L, 1);
    bool res = GetDialogManager()->RegisterLuaDialog(L, name, 2);
    lua_pushboolean(L, res);
    return 1;
}

static int luaUnregisterDialog(lua_State* L)
{
    string name = luaL_checkstring(L, 1);
    bool res = GetDialogManager()->UnregisterLuaDialog(L, name);
    lua_pushboolean(L, res);
    return 1;
}

void RegisterDialogBinding()
{
    LuaManager* pLuaManager = GetLuaManager();

    auto packetClass = pLuaManager->BindClass<ParsedTextPacket<38>>("ParsedTextPacket");
    packetClass.DefMethod("get", &get)
        .DefMethod("getInt", &getInt)
        .DefMethod("getUInt", &getUInt)
        .DefMethod("getBool", &getBool);

    pLuaManager->RegisterFunction("registerDialog", &luaRegisterDialog);
    pLuaManager->RegisterFunction("unregisterDialog", &luaUnregisterDialog);
}

#endif