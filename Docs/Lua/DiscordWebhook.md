# Discord Lua API Reference

Guide for the Discord Webhook and Embed bindings.

---

## Global Functions

| Function | Signature | Return Type | Description |
| :--- | :--- | :--- | :--- |
| `RegisterDiscordWebhook` | `(customID: string, url: string)` | `boolean` | Registers a webhook. |
| `GetDiscordWebhook` | `(customID: string)` | `DiscordWebhook\|nil` | Retrieves a registered webhook by its ID. |

---

## Class: `DiscordEmbed`

### Properties

| Property | Type | Description |
| :--- | :--- | :--- |
| `title` | `string` | Embed header title. |
| `description` | `string` | Main body text. |
| `url` | `string` | Title URL. |
| `color` | `integer` | Color in hex/decimal format (e.g., `0xFF0000`). |
| `footerText` | `string` | Footer text. |
| `footerIconUrl` | `string` | Footer icon image URL. |
| `imageUrl` | `string` | Main embed body image URL. |
| `thumbnailUrl` | `string` | Thumbnail image URL. |

### Methods

| Method | Signature | Description |
| :--- | :--- | :--- |
| `addField` | `(name: string, value: string, isInline: boolean)` | Appends a field to the embed payload. |
| `setTimestampToNow` | `()` | Sets timestamp field to current UTC time. |

---

## Class: `DiscordWebhook`

### Methods

| Method | Signature | Description |
| :--- | :--- | :--- |
| `setWebhookUrl` | `(url: string)` | Overrides the target URL. |
| `setUsername` | `(name: string)` | Overrides default username. |
| `setAvatarUrl` | `(url: string)` | Overrides default avatar image URL. |
| `setContent` | `(text: string)` | Sets the plain-text message content. |
| `addEmbed` | `(embed: DiscordEmbed)` | Attaches an embed payload. |
| `addAttachmentFromFile` | `(filePath: string)` | Attaches a file. |
| `send` | `()` → `boolean` | Dispatches the request. Clears payload on success. |
| `sendContent` | `(text: string)` → `boolean` | Shortcut to set content and send immediately. |
| `clear` | `()` | Resets active content, embeds, and attachments. |
| `getLastStatus` | `()` → `integer` | Returns HTTP status code (e.g., `200`, `204`). |
| `getLastResponse` | `()` → `string` | Returns raw HTTP response string. |

---

## Code Examples

### Simple Alert
```lua
RegisterDiscordWebhook("alerts", "https://discord.com/api/webhooks/...")

local webhook = GetDiscordWebhook("alerts")
if webhook then
    webhook:sendContent("Server initialization complete.")
end
```

### Rich Embed with Attachment
```lua
local webhook = GetDiscordWebhook("alerts")

if webhook then
    webhook:setUsername("Security System")
    webhook:setAvatarUrl("https://example.com/avatar.png")

    local embed = DiscordEmbed()
    embed.title = "Player Moderation Action"
    embed.color = 0xFF0000
    embed.footerText = "Anticheat"
    
    embed:addField("Target", "Player123", true)
    embed:addField("Reason", "Fly Hack", true)
    embed:setTimestampToNow()

    webhook:addEmbed(embed)
    webhook:addAttachmentFromFile("logs/latest.log")

    if not webhook:send() then
        print(("Send failed [%d]: %s"):format(webhook:getLastStatus(), webhook:getLastResponse()))
    end
end
```