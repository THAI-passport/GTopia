#pragma once

#include "../Player/GamePlayer.h"
#include "Event/EventDispatcher.h"
#include "Player/Role.h"
#include "Precompiled.h"

#ifdef ENABLE_LUA_SCRIPTING
#include <lua.hpp>
#endif

struct CommandInfo
{
    string usage = "";
    string desc = "";
    uint32 perm = 0;
    std::vector<uint32> aliases;
    bool enabled = true;

#ifdef ENABLE_LUA_SCRIPTING
    string name = "";
    int luaRef = LUA_NOREF;
    bool isLuaCommand = false;
#endif
};

struct HelpCommandEntry
{
    string name;
    uint32 perm = 0;
};

template <typename T> class CommandBase
{
public:
    static const CommandInfo& GetInfo() { return T::GetInfo(); }

    static void Execute(GamePlayer* pPlayer, std::vector<string>& args)
    {
        if (!CheckPerm(pPlayer))
            return;

        T::Execute(pPlayer, args);
    }

    static bool CheckPerm(GamePlayer* pPlayer)
    {
        if (!pPlayer)
            return false;

        Role* pRole = pPlayer->GetRole();
        if (!pRole || !pRole->HasPerm(GetInfo().perm) || !GetInfo().enabled)
        {
            pPlayer->SendOnConsoleMessage("`4Unknown command. ``Enter `$/help`` for a list of valid commands.");
            return false;
        }

        return true;
    }
};

class CommandManager
{
public:
    CommandManager() = default;
    ~CommandManager();

public:
    static CommandManager* GetInstance()
    {
        static CommandManager instance;
        return &instance;
    }

public:
    void RegisterAllCommands();
    void BuildHelpCache();
    const string& GetHelpMessageForRole(uint32 roleID);

#ifdef ENABLE_LUA_SCRIPTING
    bool RegisterLuaCommand(lua_State* L, const string& name, const string& usage, const string& desc,
                            const string& permStr, int funcStackIdx);
    bool RegisterLuaCommandWithAliases(lua_State* L, const std::vector<string>& aliases, const string& usage,
                                       const string& desc, const string& permStr, int funcStackIdx);

    bool UnregisterLuaCommand(lua_State* L, const string& name);
    void ClearLuaCommands(lua_State* L);
#endif

    void ExecuteCommand(GamePlayer* pPlayer, std::vector<string>& args)
    {
        if (!pPlayer || args.empty() || args[0].empty())
            return;

        const char* cmdNamePtr = (args[0][0] == '/') ? args[0].c_str() + 1 : args[0].c_str();
        uint32 hashCmd = HashString(cmdNamePtr);

#ifdef ENABLE_LUA_SCRIPTING
        auto it = m_luaCommands.find(hashCmd);
        if (it != m_luaCommands.end())
        {
            auto& luaCmd = it->second;

            if (luaCmd.perm != 0)
            {
                Role* pRole = pPlayer->GetRole();
                if (!pRole || !pRole->HasPerm(luaCmd.perm))
                {
                    pPlayer->SendOnConsoleMessage("`4Unknown command. ``Enter `$/help`` for a list of valid commands.");
                    return;
                }
            }

            ExecuteLuaCommand(luaCmd, pPlayer, args);
            return;
        }
#endif

        if (!m_commands.HasHandler(hashCmd))
        {
            pPlayer->SendOnConsoleMessage("`4Unknown command. ``Enter `$/help`` for a list of valid commands.");
            return;
        }

        m_commands.Dispatch(hashCmd, pPlayer, args);
    }

private:
#ifdef ENABLE_LUA_SCRIPTING
    void ExecuteLuaCommand(const CommandInfo& luaCmd, GamePlayer* pPlayer, std::vector<string>& args);
    bool RegisterLuaCommand(lua_State* L, const string& name, const string& usage, const string& desc, uint32 perm,
                            int funcStackIdx);
    bool RegisterLuaCommandWithAliases(lua_State* L, const std::vector<string>& aliases, const string& usage,
                                       const string& desc, uint32 perm, int funcStackIdx);
#endif

    template <class T> void Register()
    {
        const CommandInfo& info = T::GetInfo();

        for (auto& alias : info.aliases)
        {
            m_commands.Register(alias, Delegate<GamePlayer*, std::vector<string>&>::Create<&T::Execute>());
        }

        if (info.enabled && !info.usage.empty() && info.usage[0] == '/')
        {
            usize spacePos = info.usage.find(' ');
            string cmdName = (spacePos != string::npos) ? info.usage.substr(0, spacePos) : info.usage;

            m_registeredCommandsForHelp.push_back({std::move(cmdName), info.perm});
        }
    }

private:
    EventDispatcher<uint32, GamePlayer*, std::vector<string>&> m_commands;
    std::vector<HelpCommandEntry> m_registeredCommandsForHelp;
    std::unordered_map<uint32, string> m_roleHelpCache;

#ifdef ENABLE_LUA_SCRIPTING
    std::unordered_map<uint32, CommandInfo> m_luaCommands;
#endif
};

CommandManager* GetCommandManager();

#define MAKE_COMMAND(Name, Usage, Desc, Perm, ...)                                                                     \
    class Command_##Name : public CommandBase<Command_##Name>                                                          \
    {                                                                                                                  \
    public:                                                                                                            \
        static const CommandInfo& GetInfo()                                                                            \
        {                                                                                                              \
            static CommandInfo info = {Usage, Desc, Perm, {__VA_ARGS__}, true};                                        \
            return info;                                                                                               \
        }                                                                                                              \
        static void Execute(GamePlayer* pPlayer, std::vector<string>& args);                                           \
    };                                                                                                                 \
    void Command_##Name::Execute(GamePlayer* pPlayer, std::vector<string>& args)
