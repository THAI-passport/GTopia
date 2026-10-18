# Security audit of GTopia (fork THAI-passport/GTopia, branch grok/gtopia-ripoff)

Audited commit: upstream keichira/GTopia main 9b821f63bc514624dcf44d98d9f91a832eca9a32 (2026-10-18). Audit date 2026-10-19. This file and `audit/` are documentation only: no source file was changed on this branch. The full write-up with every finding, bug list and evidence lives in the owner's notes repo (reGrowtopia, `targets/growtopia/gtopia-findings.md`).

## Verdict
No malicious code was found: no credential theft, no hidden network calls, no run-time downloads or process spawning, no obfuscated blobs, no binaries in the tree or in the 78-commit history (gitleaks: 0 findings). Vendored zlib, concurrentqueue and ENet match their upstreams apart from the documented Growtopia ENet patch.

One item is rated suspicious: `HTTPServer/main.go:19` sets `LOGIN_URL = "g-topia-login.vercel.app"`, a third-party site that receives players' GrowID and password; the server's ltoken format carries the password in clear (`Source/Player/PlayerLoginDetail.cpp:57-115`). Replace it before real players log in.

## Risky defaults (not hostile; change before going public)
- `Util/setup.py`: unpinned mkcert download, `mkcert -install` (adds a root CA), wildcard certs for growtopia1/2.com, DB password on the command line, root with empty password default, PowerShell `-ExecutionPolicy Bypass` firewall rule.
- `Runtime/run_servers.sh` runs everything as root.
- Admin server (TCP 17999): plaintext auth, sample account with a published password, non-constant-time compare, `skip_ip_check`.
- `Configs/config.txt`: DB root with an empty password.
- Lua (on by default): full io/os/ffi, bytecode accepted, admin load-path check missing a `return`, bindings expose the player password and file upload to any webhook.
- Login text with the ltoken (password) goes to the crash context and debug log.
- HTTP client disables TLS verification (`Source/Network/NetHTTP_Curl.cpp:75-76`).
- Server bus auth is a nonce echo; the XOR secret is commented out.
- Session hop token is a 7-digit PCG32 value seeded from the clock; no IP binding.
- Passwords stored as unsalted MD5; guest accounts keyed only by client-sent device ids.
- zlib 1.2.3 (old CVEs); DiscordBot uses gorilla/websocket 1.4.2 (GO-2026-6278).

## Most serious bugs (details and file:line in the notes repo)
1. Pre-login remote crash: `std::stof` on the client's `game_version` throws and nothing catches it (`PlayerLoginDetail.cpp:209`). Confirmed with a Serialize/ToFloat harness: `abc` throws [V].
2. Packet pool double release on every in-world game packet (`GameServer/Server/GameServer.cpp:151,156`). Pattern confirmed with a pool harness [V].
3. Trade use-after-free when the partner disconnects (`GameServer/Player/PlayerTrade.cpp`, no cleanup in `LogOff`).
4. Master null dereference when a player leaves during a DB query (`Master/Player/GamePlayer.cpp:141-143,225-227`).
5. Any clothing can be worn without owning it (`GameServer/Player/GamePlayer.cpp:799`).
6. World save timer never reset: saves on every tick after 40 minutes (`GameServer/World/World.cpp:258`); no save on unload; non-atomic world file writes.
7. No chat: non-command input is dropped.
8. Lock-access offer null-deref guard (`GamePlayer.cpp:1391`, `!a && !b && a->...`).
9. Battle-cage error path uses `pPetInfo->name` after a null check (`GameDialogs.cpp:499-501`).

## Follow-up scanners (2026-10-19)
cppcheck finished on Source/Master/GameServer/WorldRenderer: real issues folded into the list above and into notes B31-B33 (achievement etch guard always false; world-balancer comma operator; `std::string` to `%s` logger). Go: staticcheck clean; HTTPServer govulncheck clean; DiscordBot still GO-2026-6278 and gosec G304/G104 (config path from argv / unchecked keepAlive send).

## Build
At this commit the Linux build fails with GCC 14 and the MariaDB client library. `audit/build-fixes.patch` is the minimal set of changes that made Master, GameServer (with `-DENABLE_LUA_SCRIPTING=OFF`) and WorldRenderer build in a sandbox; it is a proposal and is not applied. The Lua binder does not compile with GCC (`GameServer/Lua/LuaBinder.h:108,264`). Configure with `-DMYSQL_INCLUDE_DIR=/usr/include/mariadb` because `CMake/FindMySQL.cmake` expects one include path.
