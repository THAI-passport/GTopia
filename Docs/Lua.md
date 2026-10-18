# Lua Scripting

Modular scripting and event handling powered by LuaJIT.

> [!NOTE]  
> Lua scripting support is still experimental and barebones.

---

## 🛠️ Setup & Prerequisites

Before building with Lua support, ensure the LuaJIT submodule is properly initialized:

```bash
git submodule update --init --recursive ThirdParty/luajit
```

---

## ⚙️ CMake Configuration

Configure build options in [`GameServer/CMakeLists.txt`](../GameServer/CMakeLists.txt):

> **Note:** Change the option value from `OFF` to `ON` to compile with scripting enabled.

| Option | Default | Description |
| :--- | :---: | :--- |
| `ENABLE_LUA_SCRIPTING` | `OFF` | Enables the Lua scripting runtime and bindings |

---

## 📁 Runtime Directory

Specify the absolute path to your script folder by setting `lua_scripts_path` in `config.txt`:

```
lua_scripts_path|/path/to/your/lua_scripts|
```

---

## 📚 API Documentation

Select a module below to view detailed:

| Module | Documentation Link |
| :--- | :--- |
| **Callbacks** | [`Lua/Callback.md`](Lua/Callback.md) |
| **Commands** | [`Lua/Command.md`](Lua/Command.md) |
| **Dialogs** | [`Lua/Dialog.md`](Lua/Dialog.md) |
| **Discord Webhook** | [`Lua/DiscordWebhook.md`](Lua/DiscordWebhook.md) | 
| **Items** | [`Lua/Item.md`](Lua/Item.md) |  |
| **Players** | [`Lua/Player.md`](Lua/Player.md) |
| **Tile Extra** | [`Lua/TileExtra.md`](Lua/TileExtra.md) |
| **World** | [`Lua/World.md`](Lua/World.md) |