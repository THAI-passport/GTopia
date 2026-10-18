#pragma once

// lol cant push this now
// #include "AvatarRenderer.h"

#include "Item/ItemInfoManager.h"
#include "Item/ItemUtils.h"
#include "Math/Vector2.h"
#include "Precompiled.h"
#include "Renderer2D.h"
#include "Texture.h"
#include "World/WorldInfo.h"
#include <blend2d/blend2d.h>

// todo add some option to scale to bigger
// small things looks like broken, thats all i can do i guess
// since we are not using opengl lol

#define WORLD_UNITS_PER_TILE 16.0f

struct WorldRenderConfig
{
    int32 targetLongEdgePx = 1600;
    int32 minTilePx = 4;
    int32 maxTilePx = 32;
    int64 maxPixels = 4096 * 4096;

    // leave it 255 for auto select thread count
    uint8 threadCount = 255;

    bool enableShadows = true;
    uint8 shadowAlpha = 128;
    float tileShadowOffsetX = -0.125f;
    float tileShadowOffsetY = 0.125f;
    float objectShadowOffsetX = -0.125f;
    float objectShadowOffsetY = 0.12f;
};

class WorldRenderer
{
public:
    WorldRenderer(Renderer2D* pRenderer);
    ~WorldRenderer();

public:
    bool LoadWorld(uint32 worldID, WorldInfo* pOutWorld);
    bool Draw(WorldInfo* pWorld);
    void ReleaseMemory();
    void LoadResources();

private:
    struct TextureSheets
    {
        Texture* pPickupBoxTex = nullptr;
        Texture* pLockOutlineTex = nullptr;
        Texture* pAmorkolgTex = nullptr;
        Texture* pSeedTex = nullptr;
        Texture* pRtsTex = nullptr;
    };

    struct Layout
    {
        int32 tilePx = 16;
        float viewScale = 1.0f;
        int32 surfaceWidth = 0;
        int32 surfaceHeight = 0;
    };

    struct TileLayer
    {
        BLRectI src;
        BLRgba32 color = BLRgba32(255, 255, 255, 255);
    };

    struct TileDraw
    {
        BLImage* pImage = nullptr;
        TileLayer layers[2];
        uint8 layerCount = 0;
        BLRect dst;
        bool flipX = false;
        bool castsShadow = true;
        TileInfo* pTree = nullptr;
    };

    struct ObjectDrawParams
    {
        int32 itemID = 0;
        Vector2Float pos;
        Vector2Float offset;
        bool drawAlternate = false;
        float scale = 1.0f;
        bool hideAmount = false;
        WorldObject* pObject = nullptr;
        uint8 objectFlags = 0;
        uint32 colorOverride = 0;
        bool ghost = false;
        bool drawBox = true;
        bool shadow = false;
    };

    struct ObjectDrawState
    {
        Vector2Float pos;
        Vector2Float offset;
        float rotation = 0.0f;
        float itemPx = 0.0f;
        float boxPx = 0.0f;
        bool shadow = false;
    };

private:
    Layout ComputeLayout(int32 tilesX, int32 tilesY) const;
    uint8 ResolveThreadCount() const;

    void BuildTileLists(WorldInfo* pWorld);
    bool ResolveTile(WorldInfo* pWorld, TileInfo* pTile, int16 itemID, bool isBackground, TileDraw& out);

    void DrawWeather(WorldInfo* pWorld);
    void DrawTileLayer(WorldInfo* pWorld, const std::vector<TileDraw>& layer);
    void DrawTileShadows(WorldInfo* pWorld);
    void DrawLockAreas(WorldInfo* pWorld);
    void DrawObjects(WorldInfo* pWorld, bool shadow);
    void DrawBedrockShade(WorldInfo* pWorld);
    void DrawWorldBanner(WorldInfo* pWorld);

    void DrawTree(WorldInfo* pWorld, TileInfo* pTile, bool shadow);
    void DrawObject(WorldInfo* pWorld, const ObjectDrawParams& params);
    bool DrawSeedObject(ObjectDrawState& state, ItemInfo* pItem, const ObjectDrawParams& params);
    void DrawItemObject(ObjectDrawState& state, ItemInfo* pItem, const ObjectDrawParams& params);
    void DrawObjectCount(const ObjectDrawState& state, ItemInfo* pItem, const ObjectDrawParams& params);
    void DrawObjectFrame(const ObjectDrawState& state, Texture* pTex, int32 frameX, int32 frameY, uint32 color,
                         bool isPickupBox);
    bool IsObjectHiddenByTile(WorldInfo* pWorld, WorldObject* pObject);

    Vector2Int GetSpriteCoords(WorldInfo* pWorld, TileInfo* pTile, ItemInfo* pItem);
    int32 GemsToIconFrame(int32 gemValue);
    int32 GetPickupBoxFrameFromType(int32 itemType);

private:
    Renderer2D* m_pRenderer;
    WorldRenderConfig m_config;
    Layout m_layout;
    TextureSheets m_sheets;

    std::vector<TileDraw> m_bgDraws;
    std::vector<TileDraw> m_fgDraws;
};