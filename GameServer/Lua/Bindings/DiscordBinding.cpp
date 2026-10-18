#ifdef ENABLE_LUA_SCRIPTING

#include "DiscordBinding.h"
#include "../LuaManager.h"
#include "Network/DiscordWebhook.h"

void RegisterDiscordBinding()
{
    LuaManager* pLuaManager = LuaManager::GetInstance();

    pLuaManager->BindClass<DiscordEmbed>("DiscordEmbed")
        .DefMember("title", &DiscordEmbed::title)
        .DefMember("description", &DiscordEmbed::description)
        .DefMember("url", &DiscordEmbed::url)
        .DefMember("color", &DiscordEmbed::color)
        .DefMember("footerText", &DiscordEmbed::footerText)
        .DefMember("footerIconUrl", &DiscordEmbed::footerIconUrl)
        .DefMember("imageUrl", &DiscordEmbed::imageUrl)
        .DefMember("thumbnailUrl", &DiscordEmbed::thumbnailUrl)
        .DefMethod(
            "addField",
            +[](DiscordEmbed* self, const string& name, const string& value, bool isInline)
            {
                if (self)
                    self->AddField({name, value, isInline});
            })
        .DefMethod(
            "setTimestampToNow",
            +[](DiscordEmbed* self)
            {
                if (self)
                    self->SetTimestampToNow();
            });

    pLuaManager->BindClass<DiscordWebhook>("DiscordWebhook")
        .DefMethod("setWebhookUrl", &DiscordWebhook::SetWebhookUrl)
        .DefMethod(
            "setUsername",
            +[](DiscordWebhook* self, const string& username)
            {
                if (self)
                    self->SetUsername(username);
            })
        .DefMethod(
            "setAvatarUrl",
            +[](DiscordWebhook* self, const string& avatarUrl)
            {
                if (self)
                    self->SetAvatarUrl(avatarUrl);
            })
        .DefMethod(
            "setContent",
            +[](DiscordWebhook* self, const string& content)
            {
                if (self)
                    self->SetContent(content);
            })
        .DefMethod(
            "addEmbed",
            +[](DiscordWebhook* self, DiscordEmbed* embed)
            {
                if (self && embed)
                    self->AddEmbed(*embed);
            })
        .DefMethod(
            "addAttachmentFromFile",
            +[](DiscordWebhook* self, const string& filePath)
            {
                if (self)
                    self->AddAttachmentFromFile(filePath);
            })
        .DefMethod("send", &DiscordWebhook::Send)
        .DefMethod("sendContent", &DiscordWebhook::SendContent)
        .DefMethod("clear", &DiscordWebhook::Clear)
        .DefMethod("getLastResponse", &DiscordWebhook::GetLastResponse)
        .DefMethod("getLastStatus", &DiscordWebhook::GetLastStatus);

    pLuaManager->DefFunction(
        "GetDiscordWebhook",
        +[](const string& customID) -> DiscordWebhook* { return GetDiscordWebhookManager()->GetWebhook(customID); });

    pLuaManager->DefFunction(
        "RegisterDiscordWebhook", +[](const string& customID, const string& url) -> bool
        { return GetDiscordWebhookManager()->Register(customID, url); });
}

#endif