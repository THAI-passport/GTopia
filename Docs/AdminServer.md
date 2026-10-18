# AdminServer Documentation

## Overview

The **AdminServer** is a TCP-based remote management interface that allows server administrators and automated tools (such as Discord bots) to execute administration and moderation commands on the game server environment. 

### Key Features
- **Remote Server Management:** Execute commands without being logged in-game.
- **Role-Based Access Control:** Configurable admin rank levels (0 to 4) restricting command usage based on privileges.
- **Discord Bot Bridge:** Dedicated payload parsing for external Discord bot connections.
- **Security & Rate Limiting:** IP whitelisting, authentication rate-limiting on failed logins, and command request throttling.

---

## Configuration Files

### 1. `admin_server_config.txt`

This file configures the server socket, IP checks, user accounts, and trusted IP whitelists.

#### Config Fields

| Field | Parameters | Description |
| :--- | :--- | :--- |
| `enable_admin_server` | `<0\|1>` | Enables (`1`) or disables (`0`) the AdminServer. |
| `admin_host` | `<IP>` | Host IP address to bind (e.g., `127.0.0.1` for local, or LAN/Public WAN IP). |
| `admin_port` | `<Port>` | Port number to listen on (e.g., `17999`). |
| `skip_ip_check` | `<0\|1>` | `0` = Strict mode (Enforces IP whitelist checking via `allow_ip`).<br>`1` = Password-only mode (Bypasses IP checking). |
| `add_account` | `<DisplayName>\|<Account>\|<Password>\|<AdminLevel>\|` | Registers a standard client/Telnet user. |
| `add_discord` | `<DisplayName>\|<Account>\|<Password>\|<AdminLevel>\|` | Registers an account flagged specifically as a Discord Bot. |
| `allow_ip` | `<IP1>\|<IP2>\|...\|` | Defines trusted IP addresses for the most recently defined account. |

#### Example `admin_server_config.txt`
```text
enable_admin_server|1
admin_host|127.0.0.1|
admin_port|17999|
skip_ip_check|1

add_account|Keichira|developer_node|mypass123|4|
allow_ip|192.168.1.1|192.168.1.2|127.0.0.1|
```

---

### Admin Ranks & Levels

Access to commands is guarded by the minimum admin level requirement:

| Rank Level | Role Name | Description |
| :---: | :--- | :--- |
| **0** | No Access | Unprivileged |
| **1** | Moderator | Basic help and utility access |
| **2** | Admin | Moderation |
| **3** | Developer | Advanced moderation |
| **4** | Root | Full permissions |

---

### Security: Trusted IPs & Rate Limiting

- **Trusted IPs:** When `skip_ip_check` is set to `0`, connection requests from IPs not listed under `allow_ip` or `m_trustedIPs` are instantly rejected.
- **Authentication Failure Rate Limiting:** Failed authentication attempts trigger a 10-second IP rate limit ban.

---

### 2. `admin_client_config.txt`

#### Purpose & Necessity
This file stores pre-configured login credentials for client connection scripts (`AdminClient.ps1` and `AdminClient.sh`) to speedup login.

- **Is it required?** **No**, it is optional. If missing, scripts can either take credentials via command-line arguments or prompt the user interactively.

#### Format
```text
<Host>|<Port>|<Account>|<Password>
```

#### Example `admin_client_config.txt`
```text
127.0.0.1|17999|developer_node|mypass123
```

---

## Client Scripts & Usage Modes

Client scripts are provided for cross-platform execution:
- **Windows:** `AdminClient.ps1` / `AdminClient.bat`
- **Linux/macOS:** `AdminClient.sh`

### Usage Styles

#### 1. Interactive Console Mode
If executed without command parameters, the script loads `admin_client_config.txt` (or prompts for user/password if missing) and enters an interactive command loop.
```bash
./AdminClient.sh
# or
./AdminClient.bat
```

#### 2. Single-Command Mode (Using `admin_client_config.txt`)
Pass a command as an argument. The script uses saved credentials from `admin_client_config.txt`, executes the command, displays the server response, and exits.
```bash
./AdminClient.sh /help
```

#### 3. Single-Command Mode (Explicit Credentials)
If `admin_client_config.txt` is not present, pass credentials and command directly:
```bash
./AdminClient.sh <account> <password> <command>
# Example:
./AdminClient.sh developer_node mypass123 /pban 1050 60 hard Spamming
```

---

## Discord Bot Integration

### How it Works
1. Account created using `add_discord` in `admin_server_config.txt`.
2. The packet payload includes a unique Request ID for async tracking:
   - **Incoming Request Format:** `<RequestID>|<Command>`
   - **Outgoing Response Format:** `RESP|<RequestID>|<Server Response>`

---

### The `/help` System
Executing `/help` dynamically filters and returns only the commands that the authenticated user's rank is allowed to execute based on their connection source (Telnet vs. Discord).