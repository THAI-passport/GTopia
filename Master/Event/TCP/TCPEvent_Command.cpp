#include "TCPEvent_Command.h"
#include "../../Command/CommandManager.h"
#include "../../Server/ServerManager.h"

void TCPEvent_Command(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    int32 commandSubtype = 0;
    if (!reader.Read(commandSubtype))
        return;

    switch (commandSubtype)
    {
        case TCP_COMMAND_EXECUTE_TEXT:
        {
            uint32 senderUserID = 0;
            string commandStr;

            if (!reader.Read(senderUserID) || !reader.ReadString(commandStr))
                return;

            CommandContext ctx;
            ctx.source = CMD_SOURCE_GAMESERVER;
            ctx.originServerID = pServer->serverID;
            ctx.adminNetID = senderUserID;
            ctx.args = Split(commandStr, ' ');

            GetAdminCommandManager()->ExecuteCommand(ctx);
            break;
        }

        case TCP_COMMAND_GENERIC_RESPONSE:
        {
            uint64 requestID = 0;
            uint8 success = 0;
            string resultText;

            if (!reader.Read(requestID) || !reader.Read(success) || !reader.ReadString(resultText))
                return;

            CommandContext* pCtx = GetAdminCommandManager()->GetPendingContext(requestID);
            if (pCtx)
            {
                pCtx->SendReply(success ? resultText : "Error: " + resultText);
                GetAdminCommandManager()->RemovePendingContext(requestID);
            }
            break;
        }
    }
}