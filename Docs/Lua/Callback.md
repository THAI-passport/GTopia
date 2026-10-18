# Callback Lua API Reference

Guide for handling callbacks in Lua scripts.

---

## Overview

* **Handled Callbacks (`LUA_CALLBACK_HANDLED`)**: Expect a `boolean` return value from Lua (`true` or `false`). Returning `true` signals that the event has been handled, which **blocks** the default engine logic from executing.
* **Non-Handled Callbacks (`LUA_CALLBACK_NON_HANDLED`)**: Informational callbacks that execute Lua code without blocking engine execution.

---

## Event List

| Event Name | Type | Return Type | Description |
| :--- | :--- | :--- | :--- |
| `onTileWrenchCallback` | Handled | `boolean` | Triggered when a player uses a wrench on a tile. |
| `onConsumeConsumableCallback` | Handled | `boolean` | Triggered when a player uses a consumable item. |
| `onPlayerEnterWorldCallback` | Non-Handled | `void` | Triggered when a player enters a world. |
| `onPlayerLeaveWorld` | Non-Handled | `void` | Triggered when a player leaves a world. |

---

## Callback Signatures

### `onTileWrenchCallback`
* **Type:** Handled (`LUA_CALLBACK_HANDLED`)
* **Arguments:** `(world: World, tile: TileInfo)`
* **Return:** `boolean`

### `onConsumeConsumableCallback`
* **Type:** Handled (`LUA_CALLBACK_HANDLED`)
* **Arguments:** `(player: GamePlayer, target: GamePlayer, tile: TileInfo, item: ItemInfo)`
* **Return:** `boolean`

### `onPlayerEnterWorldCallback`
* **Type:** Non-Handled (`LUA_CALLBACK_NON_HANDLED`)
* **Arguments:** `(player: GamePlayer, world: World)`
* **Return:** `none`

### `onPlayerLeaveWorld`
* **Type:** Non-Handled (`LUA_CALLBACK_NON_HANDLED`)
* **Arguments:** `(player: GamePlayer, world: World)`
* **Return:** `none`

---