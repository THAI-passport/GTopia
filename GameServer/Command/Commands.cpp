#include "../Context.h"
#include "../Dialog/GameDialogs.h"
#include "../Player/PlayerManager.h"
#include "../Server/MasterBroadway.h"
#include "../World/WorldManager.h"
#include "CommandManager.h"
#include "Item/ItemInfoManager.h"
#include "Math/Math.h"
#include "Math/Random.h"
#include "Player/RoleManager.h"
#include "Utils/StringUtils.h"

#ifdef ENABLE_LUA_SCRIPTING
#include "../Lua/LuaManager.h"
#endif

CommandManager::~CommandManager()
{
#ifdef ENABLE_LUA_SCRIPTING
    lua_State* L = GetLuaManager()->GetState();
    ClearLuaCommands(L);
#endif
}

CommandManager* GetCommandManager()
{
    return CommandManager::GetInstance();
}

MAKE_COMMAND(AgeWorld, "/ageworld <ageMin>", "Age current map", "command.ageworld"_hash, "ageworld"_hash)
{
    if (args.size() < 2)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    uint32 ageMin = 0;
    if (ToUInt(args[1], ageMin) != TO_INT_SUCCESS)
    {
        pPlayer->SendOnConsoleMessage("`oAgeMin must be positive number.");
        return;
    }

    if (ageMin > 100000)
    {
        pPlayer->SendOnConsoleMessage("`oUmm, the target age seems like so old... please age it a bit lesser");
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    pWorld->GetTileManager()->AgeTiles(ageMin * 60 * 1000);
    pPlayer->SendOnConsoleMessage("World \"`#" + pWorld->GetWorldName() + "``\" aged " + ToString(ageMin) +
                                  " minutes.");
    pWorld->ReconnectPlayers();
}

MAKE_COMMAND(Emotes, "", "", 0, "dance"_hash, "dance2"_hash, "facepalm"_hash, "mad"_hash, "shrug"_hash, "foldarms"_hash,
             "stubborn"_hash, "fold"_hash, "rolleyes"_hash, "eyeroll"_hash, "sad"_hash, "fa"_hash, "idk"_hash,
             "no"_hash, "omg"_hash, "yes"_hash, "sleep"_hash, "cheer"_hash, "troll"_hash, "love"_hash, "kiss"_hash,
             "wave"_hash, "fp"_hash)
{
    if (pPlayer->GetLastActionTime().GetElapsedTime() <= 200)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    pPlayer->GetLastActionTime().Reset();
    pWorld->SendOnActionToAll(pPlayer, args[0]);
    pWorld->CheckOuijaBoardCommand(pPlayer, args[0]);
}

MAKE_COMMAND(FindItem, "/finditem <item_name>", "Find item from database", "command.finditem"_hash, "finditem"_hash)
{
    if (args.size() < 2)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    string itemName = JoinString(args, " ", 1);
    StripWhiteSpace(itemName);

    if (itemName.size() < 3)
    {
        pPlayer->SendOnConsoleMessage("`4Error:`` Item name must be at least `w3`` characters long.");
        return;
    }

    auto items = GetItemInfoManager()->GetSortedItemIdsByName(itemName, true);
    if (items.empty())
    {
        pPlayer->SendOnConsoleMessage("`4Error:`` No items matched your search criteria.");
        return;
    }

    if (items.size() > 5000)
    {
        pPlayer->SendOnConsoleMessage("`4Error:`` Too many results found. Please provide a more specific item name.");
        return;
    }

    pPlayer->OpenPaginatedDialog(std::make_unique<ItemFinderDialog>(std::move(items)));
}

MAKE_COMMAND(Ghost, "/ghost", "Walk throught blocks", "command.ghost"_hash, "ghost"_hash)
{
    PlayerPlayModController& modController = pPlayer->GetModController();

    if (modController.HasPlayMod(PLAYMOD_TYPE_GHOST))
    {
        modController.RemovePlayMod(PLAYMOD_TYPE_GHOST);
    }
    else
    {
        modController.AddPlayMod(PLAYMOD_TYPE_GHOST);
    }
}

MAKE_COMMAND(GiveItem, "/giveitem <userID> <amount> <item name>", "Give item to player", "command.giveitem"_hash,
             "giveitem"_hash)
{
    if (args.size() < 4)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    uint32 userID = 0;
    if (ToUInt(args[1], userID) != TO_INT_SUCCESS)
    {
        pPlayer->SendOnConsoleMessage("`oUserID must be number!");
        return;
    }

    uint32 amount = 0;
    if (ToUInt(args[2], amount) != TO_INT_SUCCESS)
    {
        pPlayer->SendOnConsoleMessage("`oItem amount must be number!");
        return;
    }

    string itemName = JoinString(args, " ", 3);
    ItemInfo* pItem = GetItemInfoManager()->GetItemByName(itemName);
    if (!pItem)
    {
        pPlayer->SendOnConsoleMessage("`oFailed to find the item " + itemName);
        return;
    }

    GamePlayer* pTarget = GetPlayerManager()->GetPlayerByUserID(userID);
    if (!pTarget)
    {
        pPlayer->SendOnConsoleMessage("`oFailed to find user with given id");
        return;
    }

    uint8 givenCount = pTarget->GetInventory().AddItem(pItem->id, amount, pTarget);
    if (givenCount == 0)
    {
        /**
         * handle
         */
    }

    pPlayer->SendOnConsoleMessage("`oGiven " + ToString(givenCount) + " " + pItem->name + " to " +
                                  pTarget->GetRawName() + " (ID: " + ToString(pTarget->GetUserID()) + ")");
    pTarget->SendOnConsoleMessage("`oGiven: " + ToString(givenCount) + " " + pItem->name);
}

MAKE_COMMAND(Magic, "/magic", "Bass da da da", "command.magic"_hash, "magic"_hash)
{
    if (pPlayer->GetCurrentWorld() == 0)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    pWorld->PlaySFXForEveryone("magic.wav");
    Vector2Float playerPos = pPlayer->GetWorldPos();

    for (uint8 i = 0; i < 20; ++i)
    {
        float offsetX = RandomRangeFloat(-80.0f, 100.0f);
        float offsetY = RandomRangeFloat(-80.0f, 100.0f);

        int32 particleType = RandomRangeInt(0, 3);
        pWorld->SendParticleEffectToAll(playerPos.x + offsetX, playerPos.y + offsetY, particleType, 4, 150 * i);
    }
}

MAKE_COMMAND(RenderWorld, "/renderworld", "View world as image", "command.renderworld"_hash, "renderworld"_hash)
{
    if (pPlayer->GetCurrentWorld() == 0)
        return;

    if (pPlayer->HasState(PLAYER_STATE_RENDERING_WORLD))
    {
        pPlayer->SendOnConsoleMessage("`4OOPS! `oYou already requested for world rendering, you should wait!");
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    TileInfo* pLockTile = pWorld->GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK);
    if (!pLockTile)
    {
        pPlayer->SendOnTalkBubble("``Sorry, only `5World Locked`` worlds that you own can be rendered.", false);
        return;
    }

    TileExtra_Lock* pTileExtra = pLockTile->GetExtra<TileExtra_Lock>();
    if (pTileExtra->ownerID != pPlayer->GetUserID())
    {
        pPlayer->SendOnTalkBubble("`oSorry, only the world owner can render it.", false);
        return;
    }

    RenderWorldDialog::Request(pPlayer);
}

MAKE_COMMAND(TogglePlayMod, "/toggleplaymod <playmodID>", "Toggle playmods", "command.toggleplaymod"_hash,
             "toggleplaymod"_hash)
{
    if (args.size() < 2)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    uint32 playModID = 0;
    if (ToUInt(args[1], playModID) != TO_INT_SUCCESS)
    {
        pPlayer->SendOnConsoleMessage("`PlayModID must be number!");
        return;
    }

    PlayerPlayModController& modController = pPlayer->GetModController();
    if (modController.HasPlayMod((ePlayModType)playModID))
    {
        modController.RemovePlayMod((ePlayModType)playModID);
    }
    else
    {
        modController.AddPlayMod((ePlayModType)playModID);
    }
}

MAKE_COMMAND(Trade, "/trade <playerName>", "Start trade with someone", 0, "trade"_hash)
{
    if (args.size() < 2)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (pWorld->GetPlayerCount() <= 1)
    {
        pPlayer->SendOnConsoleMessage("There are no people to trade with.");
        return;
    }

    if (pPlayer->GetTradeManager().IsTrading())
    {
        pPlayer->SendOnConsoleMessage("You'd better cancel your current trade first.");
        return;
    }

    string error;
    GamePlayer* pTarget = pWorld->GetPlayerByNameStartsWith(args[1], error);
    if (!pTarget)
    {
        pPlayer->SendOnConsoleMessage(error);
        return;
    }

    if (pTarget == pPlayer)
    {
        pPlayer->SendOnConsoleMessage("You trade all your stuff to yourself in exchange for all your stuff.");
        return;
    }

    GamePlayer* pTargetPartner = pTarget->GetTradeManager().GetPartner();
    if (pTargetPartner && pTargetPartner != pPlayer)
    {
        pPlayer->SendOnTalkBubble("That person is busy.", false);
        return;
    }

    if (pTarget->GetTradeManager().IsTrading() && pTarget->GetTradeManager().GetPartner() != pPlayer)
    {
        pPlayer->SendOnTalkBubble("That person is busy", false);
        return;
    }

    pPlayer->SendOnConsoleMessage("Trading with " + pTarget->GetDisplayName(true) + "...");
    pPlayer->SendOnStartTrade(pTarget->GetDisplayName(true), pTarget->GetNetID());
}

MAKE_COMMAND(Help, "/help", "Shows available commands", 0, "help"_hash)
{
    if (!pPlayer)
        return;

    Role* pRole = pPlayer->GetRole();
    uint32 roleID = pRole ? pRole->GetID() : GetRoleManager()->GetDefaultRoleID();

    const string& helpMsg = GetCommandManager()->GetHelpMessageForRole(roleID);
    pPlayer->SendOnConsoleMessage(helpMsg);
}

MAKE_COMMAND(PBan, "/pban <userID> <durationMin> [ip/hard/soft/device] [isSilent (true/false)] [reason]",
             "Ban a player", "command.pban"_hash, "pban"_hash)
{
    if (!pPlayer)
        return;

    if (args.size() < 3)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    GetMasterBroadway()->SendCommandExecuteText(pPlayer->GetUserID(), JoinString(args, " "));
}

MAKE_COMMAND(Mute, "/mute <userID> <durationMin> [ip/hard/soft/device] [isSilent (true/false)] [reason]",
             "Duct-tape a player's mouth", "command.mute"_hash, "mute"_hash)
{
    if (!pPlayer)
        return;

    if (args.size() < 3)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    GetMasterBroadway()->SendCommandExecuteText(pPlayer->GetUserID(), JoinString(args, " "));
}

MAKE_COMMAND(Curse, "/curse <userID> <durationMin> [ip/hard/soft/device] [isSilent (true/false)] [reason]",
             "Curse a player to netherworld", "command.curse"_hash, "curse"_hash)
{
    if (!pPlayer)
        return;

    if (args.size() < 3)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    GetMasterBroadway()->SendCommandExecuteText(pPlayer->GetUserID(), JoinString(args, " "));
}

MAKE_COMMAND(News, "/news", "Shows news gazzete", 0, "news"_hash)
{
    if (!pPlayer)
        return;

    GameConfig* pGameConfig = GetContext()->GetGameConfig();
    if (!pGameConfig->gameNews.empty())
    {
        pPlayer->SendOnDialogRequest(pGameConfig->gameNews);
    }
}

MAKE_COMMAND(WImport, "/wimport <filename>", "Import the exported world to current world", "command.wimport"_hash,
             "wimport"_hash)
{
    if (!pPlayer)
        return;

    if (args.size() < 2)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (args[1].find('.') != string::npos || args[1].find('/') != string::npos || args[1].find('\\') != string::npos)
    {
        pPlayer->SendOnConsoleMessage(
            "Invalid file name. Only enter the file name without path characters or extensions.");
        return;
    }

    if (pWorld->ImportWorld(args[1]))
    {
        pPlayer->SendOnConsoleMessage("Successfully imported '" + args[1] + "' to world " + pWorld->GetWorldName());
        LOGGER_LOG_INFO("Playerd %d (%s) imported %s to world %s", pPlayer->GetUserID(), pPlayer->GetRawName().c_str(),
                        args[1].c_str(), pWorld->GetWorldName().c_str());
    }
    else
        pPlayer->SendOnConsoleMessage("Failed to import '" + args[1] +
                                      "', the file doesn't exists or forgot to set up import/export path in config.");
}

void CommandManager::RegisterAllCommands()
{
    Register<Command_AgeWorld>();
    Register<Command_Emotes>();
    Register<Command_FindItem>();
    Register<Command_Ghost>();
    Register<Command_GiveItem>();
    Register<Command_Magic>();
    Register<Command_RenderWorld>();
    Register<Command_TogglePlayMod>();
    Register<Command_Trade>();
    Register<Command_PBan>();
    Register<Command_Mute>();
    Register<Command_Curse>();
    Register<Command_Help>();
    Register<Command_News>();
    Register<Command_WImport>();
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
///////////////// ADD COMMANDS ON TOP ONLY ///////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

void CommandManager::BuildHelpCache()
{
    m_roleHelpCache.clear();

    RoleManager* pRoleManager = GetRoleManager();
    auto& roles = pRoleManager->GetRoles();

    for (auto& [_, pRole] : roles)
    {
        if (!pRole)
            continue;

        std::vector<std::string_view> cmds;
        cmds.reserve(m_registeredCommandsForHelp.size()
#ifdef ENABLE_LUA_SCRIPTING
                     + m_luaCommands.size()
#endif
        );

        for (auto& cmd : m_registeredCommandsForHelp)
        {
            if (pRole->HasPerm(cmd.perm))
            {
                cmds.push_back(cmd.name);
            }
        }

#ifdef ENABLE_LUA_SCRIPTING
        for (auto& [_, cmd] : m_luaCommands)
        {
            if (pRole->HasPerm(cmd.perm))
            {
                std::string_view usageView = cmd.usage;
                if (!usageView.empty() && usageView[0] == '/')
                {
                    usize spacePos = usageView.find(' ');
                    if (spacePos != std::string_view::npos)
                    {
                        usageView = usageView.substr(0, spacePos);
                    }

                    cmds.push_back(usageView);
                }
                else if (!cmd.name.empty())
                {
                    cmds.push_back(cmd.name);
                }
            }
        }
#endif

        if (cmds.empty())
        {
            m_roleHelpCache[pRole->GetID()] = ">> Commands: None";
        }

        string helpMessage = ">> Commands: ";
        for (int32 i = 0; i < cmds.size(); ++i)
        {
            helpMessage += cmds[i];
            if ((i + 1) < cmds.size())
                helpMessage += ", ";
        }

        m_roleHelpCache[pRole->GetID()] = helpMessage;
    }
}

const string& CommandManager::GetHelpMessageForRole(uint32 roleID)
{
    auto it = m_roleHelpCache.find(roleID);
    if (it != m_roleHelpCache.end())
        return it->second;

    static const string emptyHelp = "`4No commands available for your role.``";
    return emptyHelp;
}

#ifdef ENABLE_LUA_SCRIPTING

bool CommandManager::RegisterLuaCommand(lua_State* L, const string& name, const string& usage, const string& desc,
                                        uint32 perm, int funcStackIdx)
{
    if (name.empty() || !lua_isfunction(L, funcStackIdx))
        return false;

    uint32 hashCmd = HashString(name);

    auto it = m_luaCommands.find(hashCmd);
    if (it != m_luaCommands.end())
    {
        luaL_unref(L, LUA_REGISTRYINDEX, it->second.luaRef);
    }

    lua_pushvalue(L, funcStackIdx);
    int luaRef = luaL_ref(L, LUA_REGISTRYINDEX);

    CommandInfo luaCmd;
    luaCmd.name = name;
    luaCmd.usage = usage;
    luaCmd.desc = desc;
    luaCmd.perm = perm;
    luaCmd.luaRef = luaRef;
    luaCmd.isLuaCommand = true;
    luaCmd.enabled = true;

    m_luaCommands[hashCmd] = std::move(luaCmd);
    return true;
}

bool CommandManager::RegisterLuaCommand(lua_State* L, const string& name, const string& usage, const string& desc,
                                        const string& permStr, int funcStackIdx)
{
    uint32 perm = permStr.empty() ? 0 : HashString(permStr);
    return RegisterLuaCommand(L, name, usage, desc, perm, funcStackIdx);
}

bool CommandManager::RegisterLuaCommandWithAliases(lua_State* L, const std::vector<string>& aliases,
                                                   const string& usage, const string& desc, uint32 perm,
                                                   int funcStackIdx)
{
    if (aliases.empty() || !lua_isfunction(L, funcStackIdx))
        return false;

    bool success = false;
    for (auto& alias : aliases)
    {
        if (alias.empty())
            continue;

        if (RegisterLuaCommand(L, alias, usage, desc, perm, funcStackIdx))
            success = true;
    }
    return success;
}

bool CommandManager::RegisterLuaCommandWithAliases(lua_State* L, const std::vector<string>& aliases,
                                                   const string& usage, const string& desc, const string& permStr,
                                                   int funcStackIdx)
{
    uint32 perm = permStr.empty() ? 0 : HashString(permStr);
    return RegisterLuaCommandWithAliases(L, aliases, usage, desc, perm, funcStackIdx);
}

bool CommandManager::UnregisterLuaCommand(lua_State* L, const string& name)
{
    auto it = m_luaCommands.find(HashString(name));
    if (it != m_luaCommands.end())
    {
        if (L && it->second.luaRef != LUA_NOREF)
        {
            luaL_unref(L, LUA_REGISTRYINDEX, it->second.luaRef);
        }
        m_luaCommands.erase(it);
        return true;
    }
    return false;
}

void CommandManager::ClearLuaCommands(lua_State* L)
{
    if (L)
    {
        for (auto& [_, cmd] : m_luaCommands)
        {
            if (cmd.luaRef != LUA_NOREF)
            {
                luaL_unref(L, LUA_REGISTRYINDEX, cmd.luaRef);
            }
        }
    }
    m_luaCommands.clear();
}

void CommandManager::ExecuteLuaCommand(const CommandInfo& luaCmd, GamePlayer* pPlayer, std::vector<string>& args)
{
    LuaManager* pLuaManager = GetLuaManager();
    lua_State* L = pLuaManager->GetState();

    if (!L || luaCmd.luaRef == LUA_NOREF)
        return;

    lua_rawgeti(L, LUA_REGISTRYINDEX, luaCmd.luaRef);

    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 1);
        return;
    }

    PushArg(L, (GamePlayer*)pPlayer);

    lua_newtable(L);
    for (int32 i = 0; i < args.size(); ++i)
    {
        lua_pushstring(L, args[i].c_str());
        lua_rawseti(L, -2, i + 1);
    }

    if (lua_pcall(L, 2, 0, 0) != LUA_OK)
    {
        LOGGER_LOG_ERROR("[Lua Command Error]: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

#endif