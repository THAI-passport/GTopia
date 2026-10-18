# Command Lua API Reference

Guide for registering Lua commands, unregistering commands.

---

## Global Functions

| Function | Signature | Return Type | Description |
| :--- | :--- | :--- | :--- |
| `registerCommand` | `(name: string, usage: string, desc: string, perm: string, callback: function)` | `boolean` | Registers a single command to a Lua function. |
| `registerCommandWithAliases` | `(aliases: table<string>, usage: string, desc: string, perm: integer, callback: function)` | `boolean` | Registers multiple command aliases to a single Lua function. |
| `unregisterCommand` | `(name: string)` | `boolean` | Unregisters a registered command by name or alias. |

---

## Code Examples

### Register Single Command

```lua
registerCommand("ping", "/ping", "Check response", "", function(player, args)
    player:sendOnConsoleMessage("`2Pong!``")
end)
```

### Register Command with Aliases

```lua
local aliases = {"spawn", "hub", "home"}

registerCommandWithAliases(aliases, "/spawn", "Teleport to home", "command.spawn", function(player, args)
    player:setTargetJoinWorld("START")
    player:sendOnConsoleMessage("`oTeleporting to home...``")
end)
```

### Example Command with Args Usage

```lua
registerCommand("giveitem", "/giveitem <item_id> [amount]", "Gives an item to your inventory", "command.giveitem", function(player, args)
    if #args < 1 then
        player:sendOnConsoleMessage("`4Usage: /giveitem <item_id> [amount]``")
        return
    end

    local itemId = tonumber(args[1])
    if not itemId then
        player:sendOnConsoleMessage("`4Invalid item ID provided.``")
        return
    end

    local inventory = player:getInventory()
    if not inventory then
        player:sendOnConsoleMessage("`4Failed to retrieve player inventory.``")
        return
    end

    local requestedAmount = nil
    if #args >= 2 then
        requestedAmount = tonumber(args[2])
    end

    local fitCount = inventory:getFitItemCount(itemId, requestedAmount or 200)
    local amountToGive = requestedAmount or fitCount

    if amountToGive <= 0 or fitCount <= 0 then
        player:sendOnConsoleMessage("`4Your inventory is full or cannot fit this item!``")
        return
    end

    if amountToGive > fitCount then
        player:sendOnConsoleMessage(("`wInventory can only fit `2%d`w items. Adjusting amount...``"):format(fitCount))
        amountToGive = fitCount
    end

    player:modifyInventoryItem(itemId, amountToGive)  
    player:sendOnConsoleMessage(("`2Added `w%d`2 x (ID: %d) to your inventory.``"):format(amountToGive, itemId))
end)
```

### Unregister Command

```lua
unregisterCommand("ping")
```