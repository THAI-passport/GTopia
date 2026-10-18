#pragma once

#include "../Server/AdminServer.h"
#include "../Server/ServerManager.h"
#include "Event/EventDispatcher.h"
#include "IO/Log.h"
#include "Network/DiscordWebhook.h"
#include "Precompiled.h"
#include "Utils/StringUtils.h"

enum eCommandSourceFlags
{
    CMD_SOURCE_NONE = 0,
    CMD_SOURCE_TELNET = 1 << 0,
    CMD_SOURCE_DISCORD = 1 << 1,
    CMD_SOURCE_GAMESERVER = 1 << 2,
    CMD_SOURCE_ALL = CMD_SOURCE_TELNET | CMD_SOURCE_DISCORD | CMD_SOURCE_GAMESERVER
};

// lol, might be a good idea
inline string EmbedToText(const DiscordEmbed& embed)
{
    string t = "=== " + embed.title + " ===\n";
    if (!embed.description.empty())
        t += embed.description + "\n";

    for (const auto& f : embed.fields)
        t += f.name + ": " + f.value + "\n";

    if (!embed.footerText.empty())
        t += embed.footerText + "\n";
    return t;
}

struct CommandContext
{
    uint64 requestID = 0;
    uint64 clientReqID = 0;
    eCommandSourceFlags source = CMD_SOURCE_NONE;

    uint32 adminNetID = 0;
    uint16 originServerID = 0;

    Timer creationTime;
    std::vector<string> args;

    void SendReply(const string& message) const
    {
        switch (source)
        {
            case CMD_SOURCE_DISCORD:
            {
                AdminClient* pAdmin = GetAdminServer()->GetClientByNetID(adminNetID);
                if (pAdmin)
                    pAdmin->SendPacket("RESP|" + ToString(clientReqID) + "|" + message);
                break;
            }
            case CMD_SOURCE_TELNET:
            {
                AdminClient* pAdmin = GetAdminServer()->GetClientByNetID(adminNetID);
                if (pAdmin)
                    pAdmin->SendPacket(message);
                break;
            }
            case CMD_SOURCE_GAMESERVER:
            {
                ServerInfo* pServer = GetServerManager()->GetServerByID(originServerID);
                if (pServer && pServer->pClient)
                {
                    TCPPacketWriter writer(TCP_PACKET_COMMAND);
                    writer.Write(TCP_COMMAND_GENERIC_RESPONSE);
                    writer.Write(adminNetID);
                    writer.Write((uint8)1);
                    writer.WriteString(message);

                    pServer->pClient->Send(writer);
                }
                break;
            }
            default:
                break;
        }
    }

    void SendEmbed(const DiscordEmbed& embed) const
    {
        if (source == CMD_SOURCE_DISCORD)
        {
            AdminClient* pAdmin = GetAdminServer()->GetClientByNetID(adminNetID);
            if (pAdmin)
                pAdmin->SendPacket("RESPE|" + ToString(clientReqID) + "|" +
                                   DiscordWebhook::BuildEmbedJSON(embed).Get());
            return;
        }

        SendReply(EmbedToText(embed));
    }
};

struct AdminCommandInfo
{
    string usage = "";
    string desc = "";
    int32 minAdminLevel = 999;
    uint32 sourceFlags = CMD_SOURCE_ALL;
    std::vector<uint32> aliases;
};

template <typename T> class AdminCommandBase
{
public:
    static const AdminCommandInfo& GetInfo() { return T::GetInfo(); }

    static void Execute(CommandContext& ctx)
    {
        if (!CheckPerm(ctx))
            return;

        T::Execute(ctx);
    }

    static bool CheckPerm(const CommandContext& ctx)
    {
        if (ctx.source == CMD_SOURCE_GAMESERVER)
            return true;

        AdminClient* pAdmin = GetAdminServer()->GetClientByNetID(ctx.adminNetID);
        if (!pAdmin || pAdmin->GetAdminLevel() < GetInfo().minAdminLevel)
        {
            ctx.SendReply("Error: Permission denied. Required admin level: " + ToString(GetInfo().minAdminLevel));
            return false;
        }

        return true;
    }

    static void SendUsage(const CommandContext& ctx)
    {
        if (ctx.source == CMD_SOURCE_GAMESERVER)
            return;
        ctx.SendReply("Usage: " + GetInfo().usage);
    }
};

class AdminCommandManager
{
public:
    AdminCommandManager();
    ~AdminCommandManager();

public:
    static AdminCommandManager* GetInstance()
    {
        static AdminCommandManager instance;
        return &instance;
    }

public:
    void RegisterAllCommands();
    void ExecuteCommand(CommandContext& ctx);

    template <typename CmdClass> void Register()
    {
        const AdminCommandInfo& info = CmdClass::GetInfo();
        auto handler = Delegate<CommandContext&>::template Create<&CmdClass::Execute>();

        for (uint32 aliasHash : info.aliases)
        {
            m_dispatcher.Register(aliasHash, handler);
            m_commandInfos[aliasHash] = &info;
        }
    }

    const std::unordered_map<uint32, const AdminCommandInfo*>& GetCommandInfos() const { return m_commandInfos; }

    uint64 RegisterPendingContext(CommandContext& ctx);
    CommandContext* GetPendingContext(uint64 requestID);
    void RemovePendingContext(uint64 requestID);
    void ProcessTimeouts();

private:
    Timer m_lastTimeoutCheckTimer;
    uint64 m_requestCounter;
    EventDispatcher<uint32, CommandContext&> m_dispatcher;
    std::unordered_map<uint32, const AdminCommandInfo*> m_commandInfos;
    std::unordered_map<uint64, CommandContext> m_pendingCommands;
};

AdminCommandManager* GetAdminCommandManager();

#define MAKE_COMMAND(Name, Usage, Desc, Perm, Flags, ...)                                                              \
    class Command_##Name : public AdminCommandBase<Command_##Name>                                                     \
    {                                                                                                                  \
    public:                                                                                                            \
        static const AdminCommandInfo& GetInfo()                                                                       \
        {                                                                                                              \
            static AdminCommandInfo info = {Usage, Desc, Perm, Flags, {__VA_ARGS__}};                                  \
            return info;                                                                                               \
        }                                                                                                              \
        static void Execute(CommandContext& ctx);                                                                      \
    };                                                                                                                 \
    void Command_##Name::Execute(CommandContext& ctx)