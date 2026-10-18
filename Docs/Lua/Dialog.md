# Dialog Lua API Reference

Guide for registering dialog handlers, unregistering dialogs, and reading incoming dialog field data.

---

## Global Functions

| Function | Signature | Return Type | Description |
| :--- | :--- | :--- | :--- |
| `registerDialog` | `(name: string, callback: function)` | `boolean` | Registers a handler function for a dialog. |
| `unregisterDialog` | `(name: string)` | `boolean` | Unregisters a dialog handler by name. |

---

## Class: `ParsedTextPacket`

| Method | Signature | Return Type | Description |
| :--- | :--- | :--- | :--- |
| `get` | `(key: string)` | `string` | Returns the raw string value of a key, or `""` if missing. |
| `getInt` | `(key: string)` | `integer` | Returns the parsed integer value of a key, or `0` if invalid/missing. |
| `getUInt` | `(key: string)` | `integer` | Returns the parsed unsigned integer value of a key, or `0` if invalid/missing. |
| `getBool` | `(key: string)` | `boolean` | Returns the parsed boolean value of a key, or `false` if invalid/missing. |

---

## Code Examples

### Registering a Custom Dialog Handler

```lua
registerDialog("my_custom_dialog", function(player, packet)
    local button = packet:get("buttonClicked")
    local amount = packet:getInt("item_amount")
    local confirm = packet:getBool("confirm_check")

    if button == "submit" then
        player:sendOnConsoleMessage("Amount: " .. tostring(amount) .. " | IsConfirmed: " .. tostring(confirm))
    end
end)
```

### Unregistering a Dialog

```lua
unregisterDialog("my_custom_dialog")
```