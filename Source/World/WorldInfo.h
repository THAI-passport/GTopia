#pragma once

#include "../Math/Vector2.h"
#include "../Memory/MemoryBuffer.h"
#include "../Precompiled.h"
#include "WorldObjectManager.h"
#include "WorldTileManager.h"

#define DEFAULT_WORLD_VERSION_FOR_DB 27

static const std::vector<std::pair<float, uint16>> sWorldVersionMap = {
    {4.11f, 23}, {3.68f, 20}, {3.02f, 14}, {2.995f, 13}, {2.88f, 4}, {1.30f, 3}, {0.0f, 0}};

enum eWorldGenerationType
{
    WORLD_GENERATION_DEFAULT = 0,
    WORLD_GENERATION_CLEAR = 1
};

enum eWeatherType
{
    WEATHER_TYPE_DEFAULT = 0,
    WEATHER_TYPE_SUNSET = 1,
    WEATHER_TYPE_NIGHT = 2,
    WEATHER_TYPE_DESERT = 3,
    WEATHER_TYPE_SUNNY = 4,
    WEATHER_TYPE_RAINY_CITY = 5,
    WEATHER_TYPE_HARVEST = 6,
    WEATHER_TYPE_MARS = 7,
    WEATHER_TYPE_SPOOKY = 8,
    WEATHER_TYPE_MAW = 9,
    WEATHER_TYPE_BLANK = 10,
    WEATHER_TYPE_SNOWY = 11,
    WEATHER_TYPE_GROWCH = 12,
    WEATHER_TYPE_GROWCH_HAPPY = 13,
    WEATHER_TYPE_UNDERSEA = 14,
    WEATHER_TYPE_WARP = 15,
    WEATHER_TYPE_COMET = 16,
    WEATHER_TYPE_COMET2 = 17,
    WEATHER_TYPE_PARTY = 18,
    WEATHER_TYPE_PINEAPPLE = 19,
    WEATHER_TYPE_SNOWY_NIGHT = 20,
    WEATHER_TYPE_SPRING = 21,
    WEATHER_TYPE_WOLF = 22,
    WEATHER_TYPE_NOT_INITIALIZED = 23,
    WEATHER_TYPE_PURPLE_HAZE = 24,
    WEATHER_TYPE_FIRE_HAZE = 25,
    WEATHER_TYPE_GREEN_HAZE = 26,
    WEATHER_TYPE_AQUA_HAZE = 27,
    WEATHER_TYPE_CUSTOM_HAZE = 28,
    WEATHER_TYPE_CUSTOM_ITEMS = 29,
    WEATHER_TYPE_PAGODA = 30,
    WEATHER_TYPE_APOCALYPSE = 31,
    WEATHER_TYPE_JUNGLE = 32,
    WEATHER_TYPE_BALLOON_WARZ = 33,
    WEATHER_TYPE_BACKGROUND = 34,
    WEATHER_TYPE_AUTUMN = 35,
    WEATHER_TYPE_HEART = 36,
    WEATHER_TYPE_STPATRICKS = 37,
    WEATHER_TYPE_ICE_AGE = 38,
    WEATHER_TYPE_VOLCANO = 39,
    WEATHER_TYPE_FLOATING_ISLANDS = 40,
    WEATHER_TYPE_MASCOT = 41,
    WEATHER_TYPE_DIGITAL_RAIN = 42,
    WEATHER_TYPE_MONOCHROME = 43,
    WEATHER_TYPE_TREASURE = 44,
    WEATHER_TYPE_SURGERY = 45,
    WEATHER_TYPE_BOUNTIFUL = 46,
    WEATHER_TYPE_METEOR = 47,
    WEATHER_TYPE_STARS = 48,
    WEATHER_TYPE_ASCENDED = 49,
    WEATHER_TYPE_DESTROYED = 50,
    WEATHER_TYPE_GROWTOPIA_SIGN = 51,
    WEATHER_TYPE_DUNGEON = 52,
    WEATHER_TYPE_LEGENDARY_CITY = 53,
    WEATHER_TYPE_BLOOD_DRAGON = 54,
    WEATHER_TYPE_POP_CITY = 55,
    WEATHER_TYPE_ANZU = 56,
    WEATHER_TYPE_TMNT_CITY = 57,
    WEATHER_TYPE_RAD_CITY = 58,
    WEATHER_TYPE_PLAZA = 59,
    WEATHER_TYPE_NEBULA = 60,
    WEATHER_TYPE_PROTOSTAR = 61,
    WEATHER_TYPE_DARK_MOUNTAINS = 62,
    WEATHER_TYPE_AC15 = 63,
    WEATHER_TYPE_MOUNT_GROWMORE = 64,
    WEATHER_TYPE_CRACK_IN_REALITY = 65,
    WEATHER_TYPE_LNY_NIAN = 66,
    WEATHER_TYPE_RAYMAN_LOCK = 67,
    WEATHER_TYPE_STEAMPUNK = 68,
    WEATHER_TYPE_REALM_OF_SPIRITS = 69,
    WEATHER_TYPE_BLACK_HOLE = 70,
    WEATHER_TYPE_GEMS = 71,
    WEATHER_TYPE_HOLIDAY_HAVEN = 72,
    WEATHER_TYPE_FENYX_LOCK = 73,
    WEATHER_TYPE_ENCHANTED_LOCK = 74,
    WEATHER_TYPE_ROYAL_ENCHANTED_LOCK = 75,
    WEATHER_TYPE_NEPTUNES_ATLANTIS = 76,
    WEATHER_TYPE_PINUSKI_PETAL_PURRFECT_HAVEN = 77,
    WEATHER_TYPE_CANDY_LAND = 78,
    WEATHER_TYPE_DRAGONS_KEEP = 79,
    WEATHER_TYPE_EMERALD_CITY = 80,
    WEATHER_TYPE_ANCESTRAL_PLANE = 81,
    WEATHER_TYPE_BLACK_DIGITAL_RAIN = 82,
    WEATHER_TYPE_SOUTH_POLE_WINTERFEST = 83,
    WEATHER_TYPE_LATIUM_AND_ALBION = 84,
    WEATHER_TYPE_CONCERT_LIGHTS = 85,
    WEATHER_TYPE_FRUIT_KINGDOM = 86,
    WEATHER_TYPE_BEDAWANG_NALA = 87
};

bool IsValidWorldName(const string& worldName, bool allowColon = false);
uint16 ChooseWorldVersionForClient(float gameVersion);

class WorldInfo
{
public:
    WorldInfo();
    virtual ~WorldInfo();

public:
    virtual void OnHeartMonitorAdded(TileInfo* pTile);
    virtual void OnHeartMonitorRemoved(TileInfo* pTile);
    virtual void OnSuckerBlockAdded(TileInfo* pTile);
    virtual void OnSuckerBlockRemoved(TileInfo* pTile);

    virtual uint16 GetCurrentWeather() const;
    virtual void GenerateWorld(eWorldGenerationType type);

public:
    void Kill();
    void Clear();

    bool Serialize(MemoryBuffer& memBuffer, bool write, bool database, int16 worldVersion = -1,
                   float gameVersion = 0.0f);
    uint32 GetMemEstimate(bool database, int16 worldVersion = -1, float gameVersion = 0.0f);

    bool IsThereAnyUncoveredObject();

    void SetName(const string& worldName) { m_name = worldName; }
    const string& GetWorldName() const { return m_name; }

    void SetCurrentWeather(uint32 newWeather) { m_currentWeather = newWeather; }

    uint32 GetDefaultWeather() const { return m_defaultWeather; }

    uint16 GetWorldVersion() const { return m_version; }

    WorldTileManager* GetTileManager() { return m_pTileMgr; };
    WorldObjectManager* GetObjectManager() { return m_pObjMgr; }

private:
    uint16 m_version;
    uint32 m_flags;
    string m_name;

    WorldTileManager* m_pTileMgr;
    WorldObjectManager* m_pObjMgr;

    uint16 m_defaultWeather;
    uint16 m_currentWeather;
};