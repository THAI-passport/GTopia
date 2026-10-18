# audit/

- `build-fixes.patch`: proposed Linux build fixes found during the 2026-10-19 audit (GCC 14, MariaDB client). Not applied on this branch. Apply with `git apply audit/build-fixes.patch`, then configure with `-DMYSQL_INCLUDE_DIR=/usr/include/mariadb -DMYSQL_LIB_DIR=/usr/lib/x86_64-linux-gnu` and, for GameServer, `-DENABLE_LUA_SCRIPTING=OFF`.
