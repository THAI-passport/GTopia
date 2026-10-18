#include "DiscordWebhook.h"
#include "../IO/Log.h"
#include "../Utils/StringUtils.h"
#include "../Utils/Timer.h"

DiscordWebhook::DiscordWebhook() : m_lastStatus(0) {}

DiscordWebhook::DiscordWebhook(const string& webhookUrl) : m_lastStatus(0)
{
    SetWebhookUrl(webhookUrl);
}

DiscordWebhook::~DiscordWebhook() {}

void DiscordWebhook::SetWebhookUrl(const string& webhookUrl)
{
    m_webhookUrl = webhookUrl;
    string temp = m_webhookUrl;

    if (temp.find("https://") == 0)
        temp = temp.substr(8);
    else if (temp.find("http://") == 0)
        temp = temp.substr(7);

    usize pathPos = temp.find("/");

    if (pathPos != string::npos)
    {
        m_host = temp.substr(0, pathPos);
        m_path = temp.substr(pathPos);
    }
    else
    {
        m_host = temp;
        m_path = "/";
    }

    m_http.Init("https://" + m_host);
}

DiscordWebhook& DiscordWebhook::SetUsername(const string& username)
{
    m_username = username;
    return *this;
}

DiscordWebhook& DiscordWebhook::SetAvatarUrl(const string& avatarUrl)
{
    m_avatarUrl = avatarUrl;
    return *this;
}

DiscordWebhook& DiscordWebhook::SetContent(const string& content)
{
    m_content = content;
    return *this;
}

DiscordWebhook& DiscordWebhook::AddEmbed(const DiscordEmbed& embed)
{
    m_embeds.push_back(embed);
    return *this;
}

DiscordWebhook& DiscordWebhook::AddAttachment(const string& fileName, const std::vector<uint8>& buffer)
{
    m_attachments.push_back({fileName, buffer});
    return *this;
}

DiscordWebhook& DiscordWebhook::AddAttachmentFromFile(const string& filePath)
{
    File file;
    if (file.Open(filePath))
    {
        uint32 fileSize = file.GetSize();
        std::vector<uint8> buffer(fileSize);

        if (file.Read(buffer.data(), fileSize) == fileSize)
        {
            string fileName = filePath;
            usize lastSlash = fileName.find_last_of("/\\");
            if (lastSlash != string::npos)
            {
                fileName = fileName.substr(lastSlash + 1);
            }

            m_attachments.push_back({fileName, buffer});
        }
        file.Close();
    }
    return *this;
}

void DiscordWebhook::Clear()
{
    m_username.clear();
    m_avatarUrl.clear();
    m_content.clear();
    m_embeds.clear();
    m_lastResponse.clear();
    m_lastStatus = 0;
    m_attachments.clear();
}

JSON DiscordWebhook::BuildEmbedJSON(const DiscordEmbed& embed)
{
    JSON j;

    if (!embed.title.empty())
        j["title"] = embed.title;
    if (!embed.description.empty())
        j["description"] = embed.description;
    if (!embed.url.empty())
        j["url"] = embed.url;
    if (embed.color != 0)
        j["color"] = embed.color;
    if (!embed.timestamp.empty())
        j["timestamp"] = embed.timestamp;

    if (!embed.author.name.empty())
    {
        JSON& author = j["author"];
        author["name"] = embed.author.name;
        if (!embed.author.url.empty())
            author["url"] = embed.author.url;
        if (!embed.author.iconUrl.empty())
            author["icon_url"] = embed.author.iconUrl;
    }

    if (!embed.footerText.empty())
    {
        JSON& footer = j["footer"];
        footer["text"] = embed.footerText;
        if (!embed.footerIconUrl.empty())
            footer["icon_url"] = embed.footerIconUrl;
    }

    if (!embed.imageUrl.empty())
        j["image"]["url"] = embed.imageUrl;
    if (!embed.thumbnailUrl.empty())
        j["thumbnail"]["url"] = embed.thumbnailUrl;

    if (!embed.fields.empty())
    {
        JSON& fields = j["fields"];
        for (auto& field : embed.fields)
        {
            JSON f;
            f["name"] = field.name;
            f["value"] = field.value;
            f["inline"] = field.isInline;
            fields.push_back(std::move(f));
        }
    }

    return j;
}

string DiscordWebhook::BuildJSONPayload()
{
    JSON j;

    if (!m_content.empty())
        j["content"] = m_content;
    if (!m_username.empty())
        j["username"] = m_username;
    if (!m_avatarUrl.empty())
        j["avatar_url"] = m_avatarUrl;

    if (!m_embeds.empty())
    {
        JSON& embeds = j["embeds"];
        for (auto& embed : m_embeds)
            embeds.push_back(BuildEmbedJSON(embed));
    }

    return j.Get();
}

string DiscordWebhook::BuildMultipartPayload(const string& boundary)
{
    string body;

    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"payload_json\"\r\n";
    body += "Content-Type: application/json\r\n\r\n";
    body += BuildJSONPayload() + "\r\n";

    for (int32 i = 0; i < m_attachments.size(); ++i)
    {
        DiscordAttachment& file = m_attachments[i];

        body += "--" + boundary + "\r\n";
        body +=
            "Content-Disposition: form-data; name=\"file" + ToString(i) + "\"; filename=\"" + file.fileName + "\"\r\n";
        body += "Content-Type: application/octet-stream\r\n\r\n";

        body.append((const char*)(file.buffer.data()), file.buffer.size());
        body += "\r\n";
    }

    body += "--" + boundary + "--\r\n";
    return body;
}

bool DiscordWebhook::Send()
{
    if (m_webhookUrl.empty())
    {
        LOGGER_LOG_ERROR("DiscordWebhook: Webhook URL is empty!");
        return false;
    }

    if (m_attachments.empty())
    {
        m_http.SetHeader("Content-Type", "application/json");
        m_http.SetBody(BuildJSONPayload());
    }
    else
    {
        string boundary = "----Boundary" + ToString(Time::GetSystemTime());
        m_http.SetHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
        m_http.SetBody(BuildMultipartPayload(boundary));
    }

    bool success = m_http.Post(m_path);

    m_lastStatus = m_http.GetStatus();
    m_lastResponse = m_http.GetBody();

    if (m_lastStatus != 200 && m_lastStatus != 204)
    {
        LOGGER_LOG_ERROR("DiscordWebhook: Failed. Status: %d, Resp: %s", m_lastStatus, m_lastResponse.c_str());
        return false;
    }

    Clear();
    return success;
}

bool DiscordWebhook::SendContent(const string& content)
{
    SetContent(content);
    return Send();
}

DiscordWebhookManager::DiscordWebhookManager() : m_pGameConfig(nullptr) {}

DiscordWebhookManager::~DiscordWebhookManager()
{
    Kill();
}

void DiscordWebhookManager::Kill()
{
    for (auto& [_, pWebhook] : m_webhooks)
    {
        SAFE_DELETE(pWebhook);
    }

    m_webhooks.clear();
}

void DiscordWebhookManager::RegisterFromConfig(eConfigServerType serverType)
{
    if (!m_pGameConfig)
        return;

    for (auto& webhook : m_pGameConfig->discordWebhooks)
    {
        if (webhook.serverType != serverType)
            continue;

        Register(webhook.customID, webhook.webhookURL);
    }
}

bool DiscordWebhookManager::Register(const string& customID, const string& webhookUrl)
{
    auto it = m_webhooks.find(customID);
    if (it != m_webhooks.end())
        return false;

    if (customID.empty())
    {
        LOGGER_LOG_ERROR("Discord Webhook: Failed to register webhook, customID is empty.");
        return false;
    }

    if (webhookUrl.empty())
    {
        LOGGER_LOG_ERROR("Discord Webhook: Failed to register webhook, webhookURL is empty.");
        return false;
    }

    m_webhooks.insert_or_assign(customID, new DiscordWebhook{webhookUrl});
    return true;
}

DiscordWebhook* DiscordWebhookManager::GetWebhook(const string& customID)
{
    auto it = m_webhooks.find(customID);
    if (it == m_webhooks.end())
        return nullptr;

    return it->second;
}

DiscordWebhookManager* GetDiscordWebhookManager()
{
    return DiscordWebhookManager::GetInstance();
}