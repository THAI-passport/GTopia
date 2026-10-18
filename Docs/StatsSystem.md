# Server Stats System

`StatsManager` keeps named counters (kills, events, ...) that are shared across all servers and saved to the database. Holidays use them to start or end automatically.

---

## How It Works

### Data per stat (`ServerStatItem`)

| Field | Meaning |
|-------|---------|
| `key` | Unique name, e.g. `"comet_kills"` |
| `category` | `GLOBAL` or `DAILY` |
| `totalVal` | Current total value |
| `deltaVal` | Change since the last TCP send (game server only) |

### Data flow

```
 Game server 1 ──(delta)──┐
 Game server 2 ──(delta)──┼──► MASTER  adds deltas to total
 Game server 3 ──(delta)──┘       │
                                  ├──► saves to DB      (every 60 s, dirty only)
                                  └──► broadcasts TOTALS (every 10 s, dirty only)
                                            │
                       all game servers ◄───┘  overwrite their local total
```

### Master vs. Game server behavior

| Action | Master | Game server |
|--------|--------------------------|-------------|
| `AddInt` / `AddFloat` | total += amount | total += amount **and** delta += amount |
| `SetString` | total = value | total = value, delta = value |
| Receive stats packet | `AddInt` / `AddFloat` / `SetString` (adds incoming delta) | `SetTotalDirect` (overwrites total, no re-send) |
| Send stats packet | Totals | Deltas (then delta resets to 0) |

---

## Using Stats in Code

```cpp
#include "Server/StatsManager.h"
```

### Write

```cpp
GetStatsManager()->AddInt("comet_kills", 1);                        // +1
GetStatsManager()->AddInt("comet_kills", 5, SERVER_STAT_CAT_DAILY); // with category
GetStatsManager()->AddFloat("total_gold_spent", 12.5f);
GetStatsManager()->SetString("last_winner", "PlayerName");
```

### Read

```cpp
int32 kills = GetStatsManager()->GetINT("zombie_killed");
float gold = GetStatsManager()->GetFloat("total_gold_spent");
string winner = GetStatsManager()->GetString("last_winner");
```

---

## Network Packet Format

Packet id: `TCP_PACKET_STATS_UPDATE`

Game servers sends **deltas** for int/float. Master sends **totals**. Strings are always the full value.

---

## Rules

| # | Rule |
|---|------|
| 1 | **Use lowercase stat keys.** `/holiday addrule` lowercases the key you type, so `"Zombie_Killed"` in code will never match a rule written for `zombie_killed`. |
| 2 | **Holiday rules only read int stats.** They call `GetINT()`. Float or string stats cannot be used for rules. |
| 3 | **Keep one type per key.** Don't mix `AddInt` and `AddFloat` on the same key. |
| 7 | **Rule checks use the master's total.** Holiday rules are only handled on the master. |