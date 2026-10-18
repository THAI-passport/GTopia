# AdminBot

A small Discord bot written in Go that lets you run admin commands from Discord.

The bot does not run the commands itself. It keeps one TCP connection open to admin server, forwards the command line, and shows the answer in Discord. Both `prefix` commands and `slash` commands are supported.

---

## Features

- Commands above a user's rank are hidden (prefix) or denied (slash)
- Per-user, per-command cooldowns
- Bot status shows the live player count (`Playing with N players`) or `server offline`
- Auto reconnect to the admin server
- Replies can be plain text or a Discord embed

---

## Discord Setup

1. Create an application at the [Discord Developer Portal](https://discord.com/developers/applications) and add a bot.
2. Under **Bot**, turn on the **Message Content Intent**. Without it, prefix commands will not work.
3. Invite the bot to your server with the `bot` and `applications.commands` scopes. It needs permission to read and send messages and to add reactions.

---

## Installation

```bash
go get github.com/bwmarrin/discordgo
go build -o adminbot
```

---

## Configuration

Copy `config.json.example` to `config.json` and fill it in:

```json
{
  "token": "YOUR_DISCORD_BOT_TOKEN",
  "prefix": "!",
  "guild_id": "123456789012345678",
  "admin_addr": "127.0.0.1:17999",
  "account": "botaccount",
  "password": "botpassword",
  "ranks": {
    "1": ["111111111111111111"],
    "2": ["222222222222222222"],
    "3": ["333333333333333333"],
    "4": ["444444444444444444"]
  }
}
```

| Key | Description |
| --- | --- |
| `token` | Discord bot token |
| `prefix` | Text command prefix, `!` if empty |
| `guild_id` | Optional. Registers slash commands for this server only (instant). Leave empty for global registration, which can take a while to show up |
| `admin_addr` | `host:port` of the admin server |
| `account` | Account the bot logs in with |
| `password` | Password for that account |
| `ranks` | Discord user IDs for each rank from 1 to 4. If an ID is listed more than once, the highest rank accepted |

To connect admin server, create a bot account in `admin_server_config.txt`

---

## Running

```bash
./adminbot
```

Stop it with Ctrl+C

---

## Ranks

Every command has a rank from 0 to 4, and a user can run it if their own rank is equal to or higher than the command's rank.

| Command rank | Who can use it |
| --- | --- |
| `0` | Everyone, no entry in `ranks` needed |
| `1` to `4` | Only users listed in `ranks` under that number or a higher one |

Users who are not listed in `ranks` have rank 0. They can only use commands with rank 0 and will not see anything else in `help`. If a user is listed under several ranks, the highest one counts.

For example, a user listed under rank `2` can use every command of rank 0, 1 and 2, but not rank 3 or 4 commands.

### Ranks for admin server commands

Commands that are forwarded to the admin server (everything that is not `Local`) should use the **same rank the admin server uses for that command**. If the server requires rank 2 for `mute`, then `mute` is rank 2 in `commands.go` too.

The bot checks the rank first, so users below it never reach the server, and the server then applies its own check on the bot account. If the two do not match, you get confusing results:

- Bot rank **lower** than the server's: users can run the command in Discord, but the server refuses it.
- Bot rank **higher** than the server's: the command works, but it is hidden from users who should be allowed to use it.

The bot account on the admin server must also be at least as high as the highest rank you hand out, or the server will refuse some commands.

Commands that the bot answers by itself (`Local`, like `help`) have no server rank, so pick whatever rank you want for them. Rank `0` makes them available to everyone.

---

## Cooldowns

Each command has its own cooldown per user (for example 2 seconds for `mute`, 3 for `pban`).

---

## Adding a command

All commands live in the `commandList` slice in `commands.go`. Add an entry:

| Field | Description |
| --- | --- |
| `Name` | Command name, lowercase |
| `Desc` | Description shown in `help` and in Discord |
| `Rank` | Minimum rank, 0 to 4 |
| `Prefix` / `Slash` | Where the command is available |
| `Cooldown` | Per-user delay, `0` for none |
| `Local` | `true` if the bot answers by itself and does not call the admin server (like `help`) |
| `Options` | Arguments, in the order the server expects them |

Options can have a `Default` (used when a slash option is left empty) and `Choices` (a dropdown for slash commands). Optional options that come before required ones need a `Default`, otherwise the argument order breaks.

By default a new command is just forwarded to the admin server as `name arg1 arg2 ...`. The server has to know the command and answer it. Restart the bot to register new slash commands.

For a command the bot should handle itself, set `Local: true` and add a case for it in `runCommand` in `main.go`.

---

## Running a command

Each command is sent as a frame with a request ID, then a `|`, then the command line:

```
<id>|<command line>
```

For example, `/pban userid:12345 minutes:60` is sent as:

```
7|pban 12345 60 soft false
```

Slash options that were left empty are filled in with their defaults, so the server always receives the full argument list. Newlines are replaced with spaces.

The server answers with one of two packets, using the same ID:

```
RESP|<id>|<plain text>
RESPE|<id>|<embed json>
```

- `RESP` is plain text. The bot shows it in a code block.
- `RESPE` is a Discord embed as JSON. It uses the Discord embed format. Fields with an empty name or value are dropped, because Discord rejects them.

If the server sends a packet with ID `0` or a non-numeric ID, the bot just logs it. The bot waits up to 10 seconds for an answer before replying "Timed out waiting for the admin server."