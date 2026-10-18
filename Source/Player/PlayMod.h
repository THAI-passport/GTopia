#pragma once

#include "../Math/Color.h"
#include "../Precompiled.h"

enum ePlayModType
{
    PLAYMOD_TYPE_NONE,
    PLAYMOD_TYPE_STEALTH,
    PLAYMOD_TYPE_BLUEBERRY,
    PLAYMOD_TYPE_GHOST,
    PLAYMOD_TYPE_DOUBLE_JUMP,
    PLAYMOD_TYPE_THE_ONE_RING,
    PLAYMOD_TYPE_LEGENDARY,
    PLAYMOD_TYPE_SPEEDY,
    PLAYMOD_TYPE_CARRYING_A_TORCH,
    PLAYMOD_TYPE_ENERGIZED_HORN,
    PLAYMOD_TYPE_FIRE_HOSE,
    PLAYMOD_TYPE_FOCUSED_EYES,
    PLAYMOD_TYPE_PUTT_PUTT_PUTT,
    PLAYMOD_TYPE_HUNTING_GHOSTS,
    PLAYMOD_TYPE_SLIMED,
    PLAYMOD_TYPE_MIND_CONTROL,
    PLAYMOD_TYPE_HAUNTED,
    PLAYMOD_TYPE_XENONITE,
    PLAYMOD_TYPE_EGGED,
    PLAYMOD_TYPE_BAN,
    PLAYMOD_TYPE_IN_THE_SPOTLIGHT,
    PLAYMOD_TYPE_GEIGER_COUNTER,
    PLAYMOD_TYPE_CHARGING_GEIGER_COUNTER,
    PLAYMOD_TYPE_IRRADIATED,
    PLAYMOD_TYPE_DUCT_TAPE,
    PLAYMOD_TYPE_CURSE,
    PLAYMOD_TYPE_PRECISION_TOOL,
    PLAYMOD_TYPE_BURNING_HANDS,
    PLAYMOD_TYPE_HOWLIN_MAD
};

class PlayMod
{
public:
    friend class PlayModManager;

public:
    PlayMod() {}
    ~PlayMod() {}

public:
    ePlayModType GetType() const { return m_modType; }
    uint8 GetPunchType() const { return m_punchType; }
    int32 GetDisplayItem() const { return m_displayItem; }
    uint32 GetCharStates() const { return m_charState; }
    uint32 GetChar2States() const { return m_char2State; }
    uint32 GetTime() const { return m_durationTime; }

    uint8 GetBuildRange() const { return m_buildRange; }

    float GetPunchDamage() const { return m_punchDamage; }
    float GetSpeed() const { return m_speed; }
    float GetPunchPower() const { return m_punchPower; }

    const string& GetName() const { return m_name; }
    const string& GetAddMessage() const { return m_addMessage; }
    const string& GetRemoveMessage() const { return m_removeMessage; }
    const string& GetAudioFile() const { return m_audioFile; }
    Color& GetSkinColor() { return m_skinColor; }

private:
    ePlayModType m_modType = PLAYMOD_TYPE_NONE;
    uint8 m_punchType = 0;

    int32 m_displayItem = 0;
    string m_name;
    string m_addMessage;
    string m_removeMessage;
    string m_audioFile;

    float m_punchDamage = 0.0f;
    float m_punchPower = 0.0f;
    float m_speed = 0.0f;

    uint8 m_buildRange = 0;

    uint32 m_charState = 0;
    uint32 m_char2State = 0;
    Color m_skinColor;

    uint32 m_durationTime = 0;
};