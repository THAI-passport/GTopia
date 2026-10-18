#pragma once

#include "../Precompiled.h"
#include "ItemUtils.h"

enum eBattlePetAbility
{
    PET_ABILITY_ATTACK,
    PET_ABILITY_HEAL_SELF,
    PET_ABILITY_REVIVE
};

inline string GetFullBattlePetName(int32 basePet, int32 pet2, int32 pet3)
{
    return "";
}

class BattlePetInfo
{
public:
    int32 itemID = 0;
    string name;
    string subName;
    string endName;
    string powerName;
    uint8 element = ITEM_ELEMENT_NONE;
    uint8 cooldownSec = 0;
    uint8 ability = 0;
    uint8 abilityVal = 0;
    uint8 superAbility = 0;
    uint8 superAbilityVal = 0;
    uint8 superAbilityDurationSec = 0;
    int32 powerParticle = 0;
    int32 hitParticle = 0;
    string powerSound;
    string hitSound;
    string description;

public:
    string GetColorCodeByElement()
    {
        switch (element)
        {
            case ITEM_ELEMENT_EARTH:
                return "`2";

            case ITEM_ELEMENT_FIRE:
                return "`4";

            case ITEM_ELEMENT_AIR:
                return "`9";

            case ITEM_ELEMENT_WATER:
                return "`1";

            default:
                return "";
        }
    }

    string GetElementName()
    {
        switch (element)
        {
            case ITEM_ELEMENT_EARTH:
                return "Earth";

            case ITEM_ELEMENT_FIRE:
                return "Fire";

            case ITEM_ELEMENT_AIR:
                return "Air";

            case ITEM_ELEMENT_WATER:
                return "Water";

            default:
                return "";
        }
    }

    string GetDescribedPower() { return ""; }

    string GetFullBattlePetName(int32 basePet, int32 pet2, int32 pet3) { return ""; }
};