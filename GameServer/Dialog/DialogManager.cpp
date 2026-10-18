
#include "DialogManager.h"
#include "../Player/GamePlayer.h"
#include "GameDialogs.h"

#ifdef ENABLE_LUA_SCRIPTING
#include "../Lua/LuaManager.h"
#endif

void DialogManager::RegisterAllDialogs()
{
    RegisterDialog<SignDialog::Handle>("sign_edit"_hash);
    RegisterDialog<TrashDialog::Handle>("trash_item"_hash);
    RegisterDialog<TrashDialog::HandleUntradeable>("trash_item2"_hash);
    RegisterDialog<LockDialog::Handle>("lock_edit"_hash);
    RegisterDialog<RenderWorldDialog::Handle>("render_reply"_hash);
    RegisterDialog<RegisterDialog::Handle>("growid_apply"_hash);
    RegisterDialog<DropItemDialog::Handle>("drop_item"_hash);
    RegisterDialog<AchievementBlockDialog::Handle>("achieve_reply"_hash);
    RegisterDialog<OuijaBoardDialog::Handle>("ouijaboard"_hash);
    RegisterDialog<BattleCageDialog::Handle>("battlecage"_hash);
    RegisterDialog<XenoniteDialog::Handle>("xenonite_edit"_hash);
    RegisterDialog<MailboxBlockDialog::Handle>("mailbox_edit"_hash);
    RegisterDialog<PopupDialog::Handle>("plyr_wrench"_hash);
    RegisterDialog<PopupDialog::HandleAcceptAccess>("acceptaccess"_hash);
    RegisterDialog<BulletinBlockDialog::Handle>("bulletin_edit"_hash);
    RegisterDialog<BulletinBlockDialog::HandleDeleteEntry>("remove_bulletin"_hash);
    RegisterDialog<PopupDialog::HandleTitleEdit>("title_edit"_hash);
    RegisterDialog<DoorDialog::Handle>("door_edit"_hash);
    RegisterDialog<DoorDialog::HandlePasswordReply>("password_reply"_hash);
    RegisterDialog<TradeDialog::Handle>("trade_item"_hash);
    RegisterDialog<DonationBoxDialog::Handle>("donation_box_edit"_hash);
    RegisterDialog<DonationBoxDialog::HandleGiveItem>("give_item"_hash);
    RegisterDialog<PopupDialog::HandleBillboardEdit>("billboard_edit"_hash);
    RegisterDialog<WeatherSpecialDialog::Handle>("weatherspcl"_hash);
    RegisterDialog<MannequinDialog::Handle>("mannequin_edit"_hash);
    RegisterDialog<DressupDialog::Handle>("dressup_edit"_hash);
    RegisterDialog<DressupDialog::HandleAsk>("dressup_ask"_hash);
    RegisterDialog<SpotlightDialog::Handle>("spotlight"_hash);
    RegisterDialog<DisplayBlockDialog::Handle>("displayblock"_hash);
    RegisterDialog<SuckerBlockDialog::Handle>("itemsucker"_hash);
    RegisterDialog<SuckerBlockDialog::HandleAddItem>("itemaddedtosucker"_hash);
    RegisterDialog<SuckerBlockDialog::HandleRetrieveItem>("itemremovedfromsucker"_hash);
    RegisterDialog<GatewayDialog::Handle>("gateway_edit"_hash);
    RegisterDialog<OverflowBackpackDialog::HandleSwapReturn>("backpack_overflow_swap"_hash);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

DialogManager::~DialogManager()
{
#ifdef ENABLE_LUA_SCRIPTING
    lua_State* L = GetLuaManager()->GetState();
    ClearLuaDialogs(L);
#endif
}

void DialogManager::Execute(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pDialogName = packet.Find("dialog_name"_hash);
    if (!pDialogName || pDialogName->valueSize > 50)
        return;

    std::string_view dialogNameStr = pDialogName->GetStringView();

    if (DialogPagination* pActivePagination = pPlayer->GetActivePaginatedDialog())
    {
        if (pActivePagination->GetDialogName() == dialogNameStr)
        {
            auto pButtonClicked = packet.Find("buttonClicked"_hash);
            if (pButtonClicked)
            {
                if (pActivePagination->ProcessButton(pPlayer, pButtonClicked))
                    return;
            }
        }
        else
            pPlayer->ClosePaginatedDialog();
    }

    uint32 hashedDialogName = HashString(pDialogName->value, pDialogName->valueSize);

#ifdef ENABLE_LUA_SCRIPTING
    auto it = m_luaDialogs.find(hashedDialogName);
    if (it != m_luaDialogs.end())
    {
        ExecuteLuaDialog(it->second, pPlayer, packet);
        return;
    }
#endif

    m_dispatcher.Dispatch(hashedDialogName, pPlayer, packet);
}

#ifdef ENABLE_LUA_SCRIPTING

bool DialogManager::RegisterLuaDialog(lua_State* L, const string& name, int funcStackIdx)
{
    if (name.empty() || !lua_isfunction(L, funcStackIdx))
        return false;

    uint32 hashCmd = HashString(name);

    auto it = m_luaDialogs.find(hashCmd);
    if (it != m_luaDialogs.end())
    {
        luaL_unref(L, LUA_REGISTRYINDEX, it->second.luaRef);
    }

    lua_pushvalue(L, funcStackIdx);
    int luaRef = luaL_ref(L, LUA_REGISTRYINDEX);

    m_luaDialogs[hashCmd] = LuaDialogInfo{name, luaRef};
    return true;
}

bool DialogManager::UnregisterLuaDialog(lua_State* L, const string& name)
{
    auto it = m_luaDialogs.find(HashString(name));
    if (it != m_luaDialogs.end())
    {
        if (L && it->second.luaRef != LUA_NOREF)
        {
            luaL_unref(L, LUA_REGISTRYINDEX, it->second.luaRef);
        }
        m_luaDialogs.erase(it);
        return true;
    }
    return false;
}

void DialogManager::ClearLuaDialogs(lua_State* L)
{
    if (L)
    {
        for (auto& [_, dialog] : m_luaDialogs)
        {
            if (dialog.luaRef != LUA_NOREF)
            {
                luaL_unref(L, LUA_REGISTRYINDEX, dialog.luaRef);
            }
        }
    }
    m_luaDialogs.clear();
}

void DialogManager::ExecuteLuaDialog(const LuaDialogInfo& luaDialog, GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    LuaManager* pLuaManager = GetLuaManager();
    lua_State* L = pLuaManager->GetState();

    if (!L || luaDialog.luaRef == LUA_NOREF)
        return;

    lua_rawgeti(L, LUA_REGISTRYINDEX, luaDialog.luaRef);
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 1);
        return;
    }

    PushArg(L, pPlayer);

    lua_pushlightuserdata(L, &packet);
    luaL_getmetatable(L, "ParsedTextPacket");
    lua_setmetatable(L, -2);

    if (lua_pcall(L, 2, 0, 0) != LUA_OK)
    {
        string errorMsg = lua_tostring(L, -1);
        pPlayer->SendOnConsoleMessage("[Lua Dialog Error]: " + errorMsg);
        lua_pop(L, 1);
    }
}

#endif

DialogManager* GetDialogManager()
{
    return DialogManager::GetInstance();
}