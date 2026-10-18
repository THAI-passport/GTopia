# Holiday System

> Related: [Stats System](StatsSystem.md). Holiday **rules** read their values from stats.

A **holiday** is a named, time-limited event (for example `comet`, `locke`, `winterfest`) that can be switched on and off by an admin, by a schedule, or by stat rules. Game code just asks "is this holiday active?".

---

## Enabling a Holiday in config.txt

**A holiday only works if it is listed in `enabled_holidays`.** Otherwise the server will not check it, and will even end it if it was active.

`Runtime/config.txt`:

```
# Holidays should be enabled here to check the duration of it else it wont check even if have to be active
# enabled_holidays|...|...|...
enabled_holidays|comet|wolf|
```

| Do this | Result |
|---------|--------|
| Add the key (`comet`) | Master checks its schedule, rules and duration |
| Remove the key | Within 5 s any active holiday with that key is **ended**, and nothing new starts |

- Keys are separated by `|`. Use the same lowercase key you use in `/holiday create`.
- Apply changes without a restart: run `/creload config` from admin client. It reloads `config.txt` and tells all servers to reload it.
- Starting a holiday by hand with `/holiday start` does **not** bypass this. If the key is not enabled, it is ended again at the next check.

---

## Schedule Types

| Type | Command value | `param` | Behavior |
|------|---------------|---------|----------|
| `NONE` | `none` / `0` | none | No automatic start. Only manual start or rules |
| `UNIX_TIME` | `unix` / `1` | none (uses `startTimeUnix`) | Starts once when now is inside `[start, start + duration)` |
| `MONTHLY` | `monthly` / `2` | day `1-31` | Starts on that day every month. Short months use their last day (31 becomes 28/29/30) |
| `YEARLY` | `yearly` / `3` | `M-D`, e.g. `12-25` | Starts on that date every year |
| `RANDOM` | `random` / `4` | `current` (optional) | Picks a random start day each month so the whole holiday fits inside that month |

Notes:

- **MONTHLY** and **YEARLY** also look back one month or year, so a holiday that started late in the previous period and is still running is not missed.
- **RANDOM**
  - Default: picks a day in the **next** month. With param `current`, it may pick a day in the **current** month if the holiday still fits.
  - When the picked is over, a new one is picked for the next month.
  - On server restart a still-valid random date is **kept**, not re-rolled.

### Schedule examples

```
/holiday setsched comet monthly 15          # every month on the 15th
/holiday setsched wolf yearly 12-25         # every Dec 25
/holiday setsched comet random current      # random day, this month allowed
/holiday setsched comet random              # random day, next month
/holiday setsched comet none                # no automatic schedule
```

For a one-time start:

```
/holiday setsched comet unix
/holiday settime comet 1798000000           # Unix timestamp (or +2d, +12h, 30m, now)
```

---

## Rules (stat-based triggers)

A rule links a holiday to a stat from the [Stats System](StatsSystem.md).

| Field | Meaning |
|-------|---------|
| `ruleType` | `start` (0) or `end` (1) |
| `statKey` | Stat name, **lowercase**, **int stats only** |
| `operator` | `>=`, `>`, `<=`, `<`, `==` |
| `threshold` | Integer to compare against |

### Operators

| Symbol | Alias | Number |
|--------|-------|--------|
| `>=` | `ge` | 0 |
| `>` | `gt` | 1 |
| `<=` | `le` | 2 |
| `<` | `lt` | 3 |
| `==` | `eq`, `=` | 4 |

### Logic

- Holiday **inactive**: starts if **any** `start` rule is true **and no** `end` rule is true.
- Holiday **active**: ends as soon as **any** `end` rule is true.
- A holiday with no rules is not affected by rules.

### Example

```
/holiday addrule virus_pandemic start infected_humans >= 10000
/holiday addrule comet virus_pandemic zombie_killed >= 5000
```

The game code only has to count:

```cpp
GetStatsManager()->AddInt("infected_humans", 1);
GetStatsManager()->AddInt("zombie_killed", 1);
```

> **Watch out:** a started holiday still ends when its duration runs out. If the `start` rule is still true afterwards, the holiday will **start again** at the next check. Pair it with an `end` rule or change/reset the stat so the loop stops.

---

## Admin Commands

In **AdminClient**, type `/holiday` with no arguments to show the help page:

```
/holiday
```

All commands are lowercased automatically (`Comet` and `comet` are the same).

| Command | Description |
|---------|-------------|
| `/holiday list` | All holidays with status, type, duration and rule count |
| `/holiday info <key>` | Detailed embed: start/end time, schedule, rules count |
| `/holiday start <key>` | Start now (lasts `durationDays` from now) |
| `/holiday end <key>` | End now |
| `/holiday create <key> <main\|auto> <days> [time] [sched] [param]` | Create a new holiday |
| `/holiday delete <key>` | Delete the holiday **and all its rules** (ends it first if active) |
| `/holiday setduration <key> <days>` | Change duration (must be 1 or more) |
| `/holiday settime <key> <time>` | Change the start time |
| `/holiday settype <key> <main\|auto>` | Change type |
| `/holiday setsched <key> <none\|unix\|monthly\|yearly\|random> [param]` | Change schedule |
| `/holiday addrule <key> <start\|end> <statKey> <op> <threshold>` | Add a rule |
| `/holiday delrule <key> <statKey> <start\|end>` | Remove rule(s) matching stat and type |
| `/holiday clearrules <key>` | Remove all rules |

### Time format (`create` and `settime`)

| Input | Meaning |
|-------|---------|
| `0` | Not set |
| `now` | Current time |
| `1798000000` | Unix timestamp (anything above 100000000) |
| `+30m`, `+12h`, `+1d`, `+1w` | Relative to now (`m` minutes, `h` hours, `d` days, `w` weeks) |
| `45` | Plain number is read as seconds from now |

### Examples

```
# Simple holiday, manual control only
/holiday create comet auto 3

# Starts in 1 day, one-time unix schedule
/holiday create comet auto 3 +1d unix

# Yearly on Dec 25, no fixed time (use 0 for the time slot)
/holiday create winterfest main 7 0 yearly 12-25

# Random monthly holiday, this month allowed
/holiday create comet auto 2 0 random current

# Stat driven
/holiday create virus_pandemic auto 3
/holiday addrule virus_pandemic start infected_humans >= 10000
/holiday addrule virus_pandemic end zombie_killed >= 5000
```

Remember to add the key to `enabled_holidays`

## Using Holidays in Code

```cpp
#include "../Server/HolidayManager.h"
```

### Check if active

```cpp
if (GetHolidayManager()->IsHolidayActive("anniversary_party"))
{
    // double drops, special spawns, etc.
}
```

### Access the holiday object

```cpp
BaseHoliday* pHoliday = GetHolidayManager()->GetHoliday("comet");
if (pHoliday && pHoliday->IsActive())
{
    HolidayData& data = pHoliday->GetData();
    // data.startTimeUnix, data.durationDays, ...
}
```

### Custom holiday logic

Create a class that extends `BaseHoliday`:

```cpp
class AutoHoliday_Comet : public BaseHoliday
{
public:
    AutoHoliday_Comet(const HolidayData& data) : BaseHoliday(data) {}

    void OnStarted() override { /* called on every server when it becomes active */ }
    void OnEnded() override   { /* called when it ends */ }
    void OnUpdate() override  { /* every tick, active holidays on game servers only */ }

    string GetWelcomeMessage() const override { return "It's the `2Night of the Comet!``"; }
};
```

Register it in `GameServer/Server/GameServer.cpp``

```cpp
GetHolidayManager()->RegisterCustomHoliday<AutoHoliday_Comet>("comet");
```

If a key has no registered class, a plain `BaseHoliday` is created. That is fine for holidays that only need an `IsHolidayActive()` check.

| Callback | Runs on |
|----------|---------|
| `OnStarted` / `OnEnded` | Every server (master and game servers) |
| `OnUpdate` | Active holidays on game servers only |

---

## Adding a New Holiday (step by step)

1. **(Optional) Write the class.** Extend `BaseHoliday` and register it with `RegisterCustomHoliday<T>("key")`.
2. **Use it in game code.** Check `IsHolidayActive("key")`.
3. **Enable it.** Add the key to `enabled_holidays` in `Runtime/config.txt` and run `/creload config`.
4. **Create it.** From AdminClient: `/holiday create <key> <main|auto> <days> ...`
5. **Choose how it starts.** Manual only (`/holiday start`), a schedule (`setsched`), or stat rules (`addrule`, plus `AddInt` calls in code).
6. **Verify.** `/holiday info <key>` shows status, times, schedule and rule count.

### Typical setups

| Goal | Setup |
|------|-------|
| Admin starts and stops it | `create`, then `start` / `end` |
| Every month on the 1st | `setsched <key> monthly 1` |
| Every Christmas | `setsched <key> yearly 12-25` |
| Random once a month | `setsched <key> random` |
| After the community reaches a goal | `addrule <key> start <stat> >= N` (+ an `end` rule) |

---

## Changing / Managing Holidays

| Want to change | Command |
|----------------|---------|
| Length | `/holiday setduration <key> <days>` |
| Start time | `/holiday settime <key> <time>` |
| Type | `/holiday settype <key> <main\|auto>` |
| Schedule | `/holiday setsched <key> <type> [param]` |
| A rule | `delrule` then `addrule` |
| Turn the whole system off for one holiday | Remove its key from `enabled_holidays`, then `/creload config` |
| Remove completely | `/holiday delete <key>` |

Changes are saved to the database right away and sent to all servers.

---

## Network Sync

Packet id: `TCP_PACKET_HOLIDAY_SYNC`. The master sends it whenever a holiday starts, ends or is edited.

- Game servers create a missing holiday automatically and call `SetActive()`, which fires `OnStarted` / `OnEnded`.
- **Schedules and rules are not synced.** Only the master handles them.