# Setup & Running Guide

Everything you need to install, build and run GTopia on **Windows**, **Linux** and **macOS**.

**The short version:**
1. Install the requirements.
2. Run the **setup script** (once).
3. Run the **compile script**.
4. Run the **launcher** in `Runtime/`.

---

## 1. Requirements

| Tool | Used for |
| :--- | :------- |
| [CMake](https://cmake.org/download/) | Building the servers |
| C/C++ compiler | Building the servers |
| [MySQL](https://dev.mysql.com/downloads/) | Game data. **Must be running.** |
| [Python 3](https://www.python.org/downloads/) | Runs the setup script |
| [Go](https://go.dev/dl/) | The HTTP/S server and Discord bot |

### 🐧 Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install -y build-essential cmake python3 libmysqlclient-dev mysql-server go
```

### 🪟 Windows
Install each tool from the links above.
- **CMake:** tick **"Add CMake to the system PATH"** in the installer.
- **MySQL:** use the [MySQL Installer](https://dev.mysql.com/downloads/installer/). The setup script looks for `mysql.exe` in `C:\Program Files\MySQL\MySQL Server *\bin\`.
- **Compiler:** a C++ compiler such as Visual Studio (Desktop development with C++) or MinGW.

### 🍎 macOS
With [Homebrew](https://brew.sh/):
```bash
xcode-select --install
brew install cmake python go mysql
brew services start mysql
```

---

## 2. Get the code

```bash
git clone https://github.com/keichira/GTopia.git
cd GTopia
```

---

## 3. Run the setup script

All scripts live in `Build/`.

| Windows | Linux / macOS |
| :------ | :------------ |
| `Build/setup_win.bat` | `Build/setup_linux.sh` |

Both just start `Util/setup.py`, which opens a menu:

```text
[1] Full setup (start here, first time)
[2] Show latest Ubisoft CDN
[3] Generate items.txt
[4] Generate wiki_data.txt
[5] Generate filehashes.txt
[6] Update database (apply new SQL updates)
[7] Help
[0] Exit
```

### First time: choose `[1]`

Press **Enter** to accept the default (shown in brackets) at each question.

| Step | What it does | Notes |
| :--- | :----------- | :---- |
| **Checks** | Looks for CMake and MySQL | Stops if either is missing |
| **1. Database** | Creates the database (default `gtopia`) and imports `Configs/gtopia.sql` | Asks for MySQL user and password. Default user is `root` |
| **2. SSL certificates** | Downloads [mkcert](https://github.com/FiloSottile/mkcert) and creates `cert.pem` / `key.pem` in `HTTPServer/` | Needed for game version **3.90+**. You may see an admin/password prompt |
| **3. `items.txt`** | Converts your `items.dat` to `Runtime/items.txt` | **Required.** See [Items](Items.md) |
| **4. `filehashes.txt`** | Hashes your static files for a custom CDN | Optional, default **No**. See [HTTP Server & CDN](HTTP.md) |
| **5. `wiki_data.txt`** | Downloads item wiki info | Optional, default **No**. Can take a while. See [Items](Items.md) |
| **Config files** | Moves the files from `Configs/` into `Runtime/` | Existing files are **not** overwritten |
| **Auto-configuration** | Writes your database login, CDN, IPs and paths into the config files | See below |

You'll need your Growtopia `items.dat` for step 3. It's in your game folder at `Growtopia/cache/items.dat`.

### Automatic configuration

When asked *"Write your settings into the config files automatically?"*, choose where the server will run:

| Choice | Use it when | IP used |
| :----- | :---------- | :------ |
| **1) Local** | Playing on the same PC | `127.0.0.1` |
| **2) VPS/VDS** | A public server | The public IP you type in |

It then updates:

| File | What changes |
| :--- | :----------- |
| `Runtime/config.txt` | `database_info`, `cdn_server`, `world_save_path` |
| `Runtime/servers.txt` | `set_master`, `add_server` |
| `Runtime/admin_server_config.txt` | `admin_host` |
| `HTTPServer/main.go` | `SERVER_IP` |

If you say **No**, edit those files in `Runtime/` yourself.

On **Windows with a VPS** the script can also create firewall rules (run it as Administrator). Open the same ports yourself on other systems:

| Protocol | Ports |
| :------- | :---- |
| UDP (game) | `18000` to `18000 + number of game servers` |
| TCP (sockets) | `18500` to `18500 + number of game servers` |

### Other menu options

| Option | When to use it |
| :----- | :------------- |
| `[2]` Show latest CDN | See the current Growtopia CDN address |
| `[3]` Generate `items.txt` | |
| `[4]` Generate `wiki_data.txt` | |
| `[5]` Generate `filehashes.txt` | After a CDN static file update, if you use a custom CDN |
| `[6]` Update database | **After pulling a new version** of GTopia. Applies new `.sql` files from `Configs/Updates/`. Already-applied ones are skipped |

---

## 4. Compile

| Windows | Linux / macOS |
| :------ | :------------ |
| `Build/compile_win.bat` | `Build/compile_linux.sh` |

The script builds the project you select. For Lua support, see [Lua Scripting](Lua.md) before compiling.

---

## 5. Run

Make sure **MySQL is running**, then open the `Runtime/` folder.

> ⚠️ The HTTP server uses ports **80** and **443**, so run the launcher as **administrator** (Windows) or **root** (Linux/macOS).

### 🪟 Windows

| Action | How |
| :----- | :-- |
| Start | Run `run_servers.bat` as administrator |
| Stop | Run `stop_servers.bat` as administrator |

Each server opens in its own window. If one crashes, it restarts by itself after 5 seconds. Closing a window does **not** stop it for good, so use `stop_servers.bat`.

### 🐧 Linux / 🍎 macOS

```bash
sudo ./run_servers.sh
sudo ./run_servers.sh status
sudo ./run_servers.sh restart
sudo ./run_servers.sh stop
```

- Servers run in the **background**, so you can close your terminal and they keep running.
- If a server crashes, it restarts by itself after 5 seconds.
- The HTTP server is compiled once into `HTTPServer/httpserver` and rebuilt automatically when `main.go` changes.

> **If a server keeps restarting every 5 seconds**, it is failing to start. See `Runtime/logs/` and read the last error (usually a missing `items.txt` or a wrong database credentials in `config.txt`).

---

## 6. Updating

After pulling a new version:

1. Run the setup script and choose **`[6] Update database`**.
2. Compile again.
3. Start the servers.

---

## Troubleshooting

| Problem | Fix |
| :------ | :-- |
| *"CMake was not found"* | Install CMake and make sure it's on your `PATH`, then restart the script |
| *"Cannot connect to MySQL"* | Start the MySQL service and check the username and password |
| *"Required helper script missing"* | Your clone is incomplete. Re-clone the repository |
| Servers won't *start* | Run the launcher as administrator/root (ports 80 and 443), then check the logs |
| Servers won't *stop* | Run the stopper as administrator/root |

Still stuck? Ask on the [Discord](https://discord.gg/5XjTQm3kRh).