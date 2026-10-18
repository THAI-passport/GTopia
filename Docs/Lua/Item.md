# Item Lua API Reference

Guide for ItemInfo, ItemInfoManager, helper functions, and related structures.

---

## Global Functions

| Function | Signature | Return Type | Description |
| :--- | :--- | :--- | :--- |
| `getItemInfoManager` | `()` | `ItemInfoManager` | Returns the global ItemInfoManager instance. |
| `isIllegalItem` | `(itemID: integer)` | `boolean` | Checks if the item ID is an illegal item. |
| `isWorldLock` | `(itemID: integer)` | `boolean` | Checks if the item ID is any type of World Lock. |
| `isMainDoor` | `(itemID: integer)` | `boolean` | Checks if the item ID is a Main Door. |
| `isFuelPack` | `(itemID: integer)` | `boolean` | Checks if the item ID is a fuel pack item. |
| `isJammer` | `(itemID: integer)` | `boolean` | Checks if the item ID is a jammer or generator. |
| `isGauntletOfElements` | `(itemID: integer)` | `boolean` | Checks if the item ID is a Gauntlet of Elements. |
| `isPathMarker` | `(itemID: integer)` | `boolean` | Checks if the item ID is a path marker. |
| `getMaxTilesToLock` | `(itemID: integer)` | `integer` | Returns maximum tile lock range for lock items. |
| `toItemClientId` | `(id: integer)` | `integer` | Converts item ID to client-side ID. |
| `seedToItemId` | `(seedID: integer)` | `integer` | Converts a seed ID to its item ID. |
| `itemToSeedId` | `(itemID: integer)` | `integer` | Converts a block item ID to its seed ID. |

---

## Class: `ItemInfoManager`

| Method | Signature | Return Type |
| :--- | :--- | :--- |
| `getItemById` | `(itemID: integer)` | `ItemInfo\|nil` |
| `getItemByName` | `(name: string)` | `ItemInfo\|nil` |
| `getItemCount` | `()` | `integer` |
| `getSpliceInfo` | `(seed1: integer, seed2: integer)` | `ItemInfo\|nil` |
| `getConsumableInfo` | `(itemID: integer)` | `ConsumableInfo\|nil` |
| `getBattlePetInfo` | `(itemID: integer)` | `BattlePetInfo\|nil` |
| `getSortedItemIdsByName` | `(name: string, skipSeeds: boolean)` | `array<integer>` |

---

## Class: `ItemInfo`

### Properties

| Property | Type | Property | Type |
| :--- | :--- | :--- | :--- |
| `id` | `integer` | `seedBg` | `integer` |
| `flags` | `integer` | `seedFg` | `integer` |
| `flags2` | `integer` | `treeBg` | `integer` |
| `fxFlags` | `integer` | `treeFg` | `integer` |
| `type` | `integer` | `seed1` | `integer` |
| `material` | `integer` | `seed2` | `integer` |
| `name` | `string` | `growTime` | `integer` |
| `description` | `string` | `multiAnim1` | `string` |
| `textureFile` | `string` | `multiAnim2` | `string` |
| `textureHash` | `integer` | `overlayTextureFile` | `string` |
| `textureX` | `integer` | `dualAnimLayer` | `Vector2Int` |
| `textureY` | `integer` | `tileRange` | `integer` |
| `visualEffect` | `integer` | `pileSize` | `integer` |
| `cookingTime` | `integer` | `customizedPunchParameters` | `string` |
| `storage` | `integer` | `lightSourceRange` | `integer` |
| `layer` | `integer` | `variantVersionItem` | `integer` |
| `collisionType` | `integer` | `chairInfo` | `ItemChairInfo` |
| `hp` | `integer` | `configName` | `string` |
| `restoreTime` | `integer` | `otherPlayerHitParticle` | `integer` |
| `bodyPart` | `integer` | `randomSpriteInfo` | `ItemRandomSpriteReplaceInfo` |
| `rarity` | `integer` | `canTransform` | `integer` |
| `maxCanHold` | `integer` | `slipperyType` | `integer` |
| `extraString` | `string` | `element` | `integer` |
| `extraStringHash` | `integer` | `maxFruitCount` | `integer` |
| `animMs` | `integer` | `farmability` | `integer` |
| `petName` | `string` | `petEndName` | `string` |
| `petSubName` | `string` | `petPowerName` | `string` |

### Methods

| Method | Signature | Return Type | Description |
| :--- | :--- | :--- | :--- |
| `hasFlag` | `(flag: integer)` | `boolean` | Checks if item flag is set. |
| `hasFlag2` | `(flag: integer)` | `boolean` | Checks if item flag2 is set. |
| `isUnlimited` | `()` | `boolean` | Returns `true` if item maxCanHold is 0. |
| `isBackground` | `()` | `boolean` | Checks if the item is background layer. |
| `isLosBlocking` | `()` | `boolean` | Checks if item blocks line of sight. |

---

## Struct: `ItemChairInfo`

| Property | Type |
| :--- | :--- |
| `enabled` | `integer` |
| `playerOffset` | `Vector2Int` |
| `armPos` | `Vector2Int` |
| `armOffset` | `Vector2Int` |
| `armTexture` | `string` |

---

## Struct: `ItemRandomSpriteReplaceInfo`

| Property | Type |
| :--- | :--- |
| `enabled` | `integer` |
| `offsetMod` | `integer` |
| `chance` | `number` |

---

## Basic Example

```lua
local itemMgr = getItemInfoManager()
local item = itemMgr:getItemByName("Dirt")

if item then
    print("Item ID: " .. tostring(item.id))
    print("Rarity: " .. tostring(item.rarity))
    print("Is Unlimited: " .. tostring(item:isUnlimited()))
    print("Seed ID: " .. tostring(itemToSeedId(item.id)))
end
```