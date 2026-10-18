#include "WorldRenderer.h"
#include "Context.h"
#include "IO/File.h"
#include "IO/Log.h"
#include "Utils/ResourceManager.h"
#include "Utils/StringUtils.h"
#include "WeatherRenderer.h"
#include "WorldTileLut.h"
#include <thread>

static BLRgba32 HsvToRgba(float h, float s, float v)
{
    h = Mod(h, 360.0f);
    float c = v * s;
    float hp = h / 60.0f;
    float x = c * (1.0f - Abs(Mod(hp, 2.0f) - 1.0f));

    float r = 0.0f, g = 0.0f, b = 0.0f;
    if (hp < 1.0f)
    {
        r = c;
        g = x;
    }
    else if (hp < 2.0f)
    {
        r = x;
        g = c;
    }
    else if (hp < 3.0f)
    {
        g = c;
        b = x;
    }
    else if (hp < 4.0f)
    {
        g = x;
        b = c;
    }
    else if (hp < 5.0f)
    {
        r = x;
        b = c;
    }
    else
    {
        r = c;
        b = x;
    }

    float m = v - c;
    return BLRgba32((uint32)((r + m) * 255.0f + 0.5f), (uint32)((g + m) * 255.0f + 0.5f),
                    (uint32)((b + m) * 255.0f + 0.5f), 255);
}

WorldRenderer::WorldRenderer(Renderer2D* pRenderer) : m_pRenderer(pRenderer)
{
    if (m_pRenderer)
    {
        LoadResources();
    }
}

WorldRenderer::~WorldRenderer() {}

bool WorldRenderer::LoadWorld(uint32 worldID, WorldInfo* pOutWorld)
{
    if (!pOutWorld)
        return false;

    File file;
    if (!file.Open(GetContext()->GetGameConfig()->worldSavePath + "/world_" + ToString(worldID) + ".bin"))
        return false;

    uint32 fileSize = file.GetSize();
    std::vector<uint8> data(fileSize);

    if (file.Read(data.data(), fileSize) != fileSize)
    {
        file.Close();
        return false;
    }

    file.Close();
    MemoryBuffer memBuffer(data.data(), fileSize);
    pOutWorld->Serialize(memBuffer, false, true);
    return true;
}

void WorldRenderer::ReleaseMemory()
{
    m_pRenderer->ReleaseResources();
    m_sheets = TextureSheets();

    m_bgDraws.clear();
    m_fgDraws.clear();
}

void WorldRenderer::LoadResources()
{
    m_sheets.pPickupBoxTex = GetResourceManager()->GetTexture("game/pickup_box.rttex", 20, 20);
    m_sheets.pAmorkolgTex = GetResourceManager()->GetTexture("game/amorkolg.rttex", 32, 32);
    m_sheets.pLockOutlineTex = GetResourceManager()->GetTexture("game/lock_outline.rttex", 4, 4);
    m_sheets.pSeedTex = GetResourceManager()->GetTexture("game/seed.rttex", 16, 16);
    m_sheets.pRtsTex = GetResourceManager()->GetTexture("game/rts.rttex", 16, 16);
}

WorldRenderer::Layout WorldRenderer::ComputeLayout(int32 tilesX, int32 tilesY) const
{
    int32 longest = Max(1, Max(tilesX, tilesY));

    int32 tilePx = m_config.targetLongEdgePx / longest;
    tilePx = Max(m_config.minTilePx, Min(m_config.maxTilePx, tilePx));
    tilePx = Max(1, tilePx);

    while (tilePx > 1 && tilesX * tilePx * tilesY * tilePx > m_config.maxPixels)
        --tilePx;

    Layout layout;
    layout.tilePx = tilePx;
    layout.viewScale = (float)tilePx / WORLD_UNITS_PER_TILE;
    layout.surfaceWidth = tilesX * tilePx;
    layout.surfaceHeight = tilesY * tilePx;
    return layout;
}

uint8 WorldRenderer::ResolveThreadCount() const
{
    if (m_config.threadCount != 255)
        return m_config.threadCount;

    return std::thread::hardware_concurrency() >= 4 ? 2 : 0;
}

bool WorldRenderer::Draw(WorldInfo* pWorld)
{
    if (!m_pRenderer)
        return false;

    if (!pWorld)
        return false;

    WorldTileManager* pTileManager = pWorld->GetTileManager();
    if (!pTileManager)
        return false;

    Vector2Int& vWorldSize = pTileManager->GetSize();
    m_layout = ComputeLayout(vWorldSize.x, vWorldSize.y);

    m_pRenderer->SetThreadCount(ResolveThreadCount());
    m_pRenderer->Init(m_layout.surfaceWidth, m_layout.surfaceHeight);
    if (m_pRenderer->GetSurfaceWidth() != m_layout.surfaceWidth ||
        m_pRenderer->GetSurfaceHeight() != m_layout.surfaceHeight)
    {
        LOGGER_LOG_ERROR("Could not allocate a %dx%d surface for world %s", m_layout.surfaceWidth,
                         m_layout.surfaceHeight, pWorld->GetWorldName().c_str());
        return false;
    }

    LoadResources();
    m_pRenderer->Begin();

    Timer timer;
    LOGGER_LOG_INFO("Starting to draw %s", pWorld->GetWorldName().c_str());

    m_pRenderer->SetViewScale(1.0f);
    DrawWeather(pWorld);
    m_pRenderer->SetViewScale(m_layout.viewScale);

    BuildTileLists(pWorld);

    DrawTileLayer(pWorld, m_bgDraws);
    if (m_config.enableShadows)
    {
        DrawTileShadows(pWorld);
    }
    DrawTileLayer(pWorld, m_fgDraws);

    DrawLockAreas(pWorld);

    if (m_config.enableShadows)
    {
        DrawObjects(pWorld, true);
    }
    DrawObjects(pWorld, false);

    DrawBedrockShade(pWorld);
    DrawWorldBanner(pWorld);

    LOGGER_LOG_INFO("Drew world %s in %dms", pWorld->GetWorldName().c_str(), timer.GetElapsedTime())
    timer.Reset();

    string outPath = GetContext()->GetGameConfig()->rendererSavePath + "/" + pWorld->GetWorldName() + ".png";
    bool written = m_pRenderer->WriteToFile(outPath);
    if (!written)
        LOGGER_LOG_ERROR("Failed to write %s", outPath.c_str())
    else
        LOGGER_LOG_INFO("Written world %s in %dms to %s", pWorld->GetWorldName().c_str(), timer.GetElapsedTime(),
                        outPath.c_str());

    m_pRenderer->End();
    m_bgDraws.clear();
    m_fgDraws.clear();
    return written;
}

void WorldRenderer::DrawWeather(WorldInfo* pWorld)
{
    if (!pWorld)
        return;

    WeatherRenderer weatherRenderer(m_pRenderer);
    switch (pWorld->GetCurrentWeather())
    {
        case WEATHER_TYPE_HARVEST:
            weatherRenderer.DrawHarvest();
            break;
        case WEATHER_TYPE_SPRING:
            weatherRenderer.DrawDefault(true);
            break;
        default:
            weatherRenderer.DrawDefault();
            break;
    }
}

void WorldRenderer::DrawBedrockShade(WorldInfo* pWorld)
{
    if (!pWorld)
        return;

    Vector2Int& vWorldSize = pWorld->GetTileManager()->GetSize();
    float rows = Min(6.0f, vWorldSize.y); // probably there not gonna be but yeah

    BLRect rect(0.0f, (vWorldSize.y - rows) * WORLD_UNITS_PER_TILE, vWorldSize.x * WORLD_UNITS_PER_TILE,
                rows * WORLD_UNITS_PER_TILE);
    m_pRenderer->DrawRectFilled(rect, BLRgba32(0, 0, 0, 150));
}

void WorldRenderer::DrawWorldBanner(WorldInfo* pWorld)
{
    if (!pWorld || !m_pRenderer)
        return;

    BLFont* pFont = GetResourceManager()->GetFont(FONT_TYPE_CENTURY_GOTHIC_BOLD);
    if (!pFont)
        return;

    string text = "`#Visit `w\"" + pWorld->GetWorldName() + "\" `#in";

    float fontSize = m_layout.tilePx * 2.0f;
    float width = m_pRenderer->GetTextWidth(pFont, text, fontSize);
    float height = m_pRenderer->GetTextHeight(pFont, fontSize);

    float x = (float)m_pRenderer->GetSurfaceWidth() - width - m_layout.tilePx * 2.0f;
    float y = (float)m_pRenderer->GetSurfaceHeight() - height - m_layout.tilePx * 3.0f;

    m_pRenderer->DrawGTText(pFont, BLPoint(Max(0.0f, x), Max(0.0f, y)), text, fontSize);
}

void WorldRenderer::BuildTileLists(WorldInfo* pWorld)
{
    WorldTileManager* pTileManager = pWorld->GetTileManager();
    Vector2Int& vWorldSize = pTileManager->GetSize();

    m_bgDraws.clear();
    m_fgDraws.clear();
    m_bgDraws.reserve(vWorldSize.x * vWorldSize.y);
    m_fgDraws.reserve(vWorldSize.x * vWorldSize.y);

    for (int32 y = 0; y < vWorldSize.y; ++y)
    {
        for (int32 x = 0; x < vWorldSize.x; ++x)
        {
            TileInfo* pTile = pTileManager->GetTile(x, y);
            if (!pTile)
                continue;

            int16 bgID = pTile->GetBG();
            if (bgID != ITEM_ID_BLANK)
            {
                TileDraw draw;
                if (ResolveTile(pWorld, pTile, bgID, true, draw))
                {
                    m_bgDraws.push_back(draw);
                }
            }

            int16 fgID = pTile->GetFG();
            if (fgID == ITEM_ID_BLANK)
                continue;

            TileDraw draw;
            if (pTile->IsTree())
            {
                draw.pTree = pTile;
                m_fgDraws.push_back(draw);
            }
            else if (ResolveTile(pWorld, pTile, fgID, false, draw))
            {
                m_fgDraws.push_back(draw);
            }
        }
    }
}

bool WorldRenderer::ResolveTile(WorldInfo* pWorld, TileInfo* pTile, int16 itemID, bool isBackground, TileDraw& out)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return false;

    if (isBackground && itemID == ITEM_ID_DIMENSION_BLOCK && !pTile->HasFlag(TILE_FLAG_IS_ON))
        return false;

    if (!isBackground && itemID == ITEM_ID_TOMB_ROBBER && pTile->HasFlag(TILE_FLAG_IS_ON))
        return false;

    Texture* pTex = GetResourceManager()->GetItemTexture(itemID);
    if (!pTex)
        return false;

    int32 spriteX = -1;
    int32 spriteY = -1;

    if (pItem->type == ITEM_TYPE_ACHIEVEMENT)
    {
        TileExtra_Achievement* pExtra = pTile->GetExtra<TileExtra_Achievement>();
        if (!pExtra)
            return false;

        pTex = m_sheets.pAmorkolgTex;
        if (!pTex)
            return false;

        spriteX = pExtra->achievementID / 16;
        spriteY = pExtra->achievementID % 16;
    }
    else
    {
        Vector2Int coords = GetSpriteCoords(pWorld, pTile, pItem);
        spriteX = coords.x;
        spriteY = coords.y;
    }

    if (!pTex->HasFrame(spriteX, spriteY))
        return false;

    Vector2Int& vTilePos = pTile->GetPos();
    ItemInfo* pSeed = (pItem->visualEffect == VISUAL_EFFECT_DISCOLOR || pItem->visualEffect == VISUAL_EFFECT_COLORIZE)
                          ? GetItemInfoManager()->GetItemByID(ITEM_TO_SEED_ID(itemID + 1))
                          : nullptr;

    BLRgba32 color(255, 255, 255, 255);
    if (pItem->visualEffect == VISUAL_EFFECT_DISCOLOR && pSeed)
    {
        color = BLRgba32(pSeed->seedFgColor.GetAsUINT());
    }

    BLRgba32 overlayColor = color;
    if (pItem->visualEffect == VISUAL_EFFECT_COLORIZE && color.value == 0xFFFFFFFF && pSeed)
    {
        color = BLRgba32(pSeed->seedBgColor.GetAsUINT());
        overlayColor = BLRgba32(pSeed->seedFgColor.GetAsUINT());
    }

    if (pItem->visualEffect == VISUAL_EFFECT_RAINBOW_SHIFT)
    {
        color = HsvToRgba((float)((vTilePos.x * 5 + vTilePos.y * 8) % 360), 1.0f, 1.0f);
    }

    // color = BLRgba32(RandomRangeInt(0, 255), RandomRangeInt(0, 255), RandomRangeInt(0, 255), 255);

    out.layerCount = 0;
    auto addLayer = [&](int32 frameX, int32 frameY, BLRgba32 color)
    {
        if (!pTex->HasFrame(frameX, frameY) || out.layerCount >= 2)
            return;

        out.layers[out.layerCount].src = pTex->GetFrameRect(frameX, frameY);
        out.layers[out.layerCount].color = color;
        ++out.layerCount;
    };

    if (!isBackground && pItem->visualEffect == VISUAL_EFFECT_BACKFORE)
    {
        addLayer(spriteX, spriteY + 1, color);
    }

    addLayer(spriteX, spriteY, color);

    if (pItem->visualEffect == VISUAL_EFFECT_COLORIZE)
    {
        addLayer(spriteX + 1, spriteY, overlayColor);
    }

    if (out.layerCount == 0)
        return false;

    out.pImage = pTex->GetImage();
    out.dst = BLRect(vTilePos.x * WORLD_UNITS_PER_TILE, vTilePos.y * WORLD_UNITS_PER_TILE, WORLD_UNITS_PER_TILE,
                     WORLD_UNITS_PER_TILE);
    out.flipX = pTile->HasFlag(TILE_FLAG_FLIPPED_X) && pItem->HasFlag(ITEM_FLAG_FLIPPABLE);
    out.castsShadow = !isBackground || !pItem->HasFlag(ITEM_FLAG_NOSHADOW);
    out.pTree = nullptr;
    return true;
}

void WorldRenderer::DrawTileLayer(WorldInfo* pWorld, const std::vector<TileDraw>& layer)
{
    for (const TileDraw& draw : layer)
    {
        if (draw.pTree)
        {
            DrawTree(pWorld, draw.pTree, false);
            continue;
        }

        for (uint8 i = 0; i < draw.layerCount; ++i)
        {
            m_pRenderer->DrawSprite(draw.pImage, draw.dst, draw.layers[i].src, draw.layers[i].color, 1.0f, 1.0f, 0.0f,
                                    draw.flipX);
        }
    }
}

void WorldRenderer::DrawTileShadows(WorldInfo* pWorld)
{
    float offsetX = m_config.tileShadowOffsetX * WORLD_UNITS_PER_TILE;
    float offsetY = m_config.tileShadowOffsetY * WORLD_UNITS_PER_TILE;
    BLRgba32 shadowColor(0, 0, 0, 255);

    auto drawLayer = [&](const std::vector<TileDraw>& layer)
    {
        for (const TileDraw& draw : layer)
        {
            if (!draw.castsShadow)
                continue;

            if (draw.pTree)
            {
                DrawTree(pWorld, draw.pTree, true);
                continue;
            }

            BLRect rect = draw.dst;
            rect.x += offsetX;
            rect.y += offsetY;
            for (uint8 i = 0; i < draw.layerCount; ++i)
            {
                m_pRenderer->DrawSprite(draw.pImage, rect, draw.layers[i].src, shadowColor, 1.0f, 1.0f, 0.0f,
                                        draw.flipX);
            }
        }
    };

    m_pRenderer->BeginLayer();
    drawLayer(m_bgDraws);
    drawLayer(m_fgDraws);
    m_pRenderer->EndLayer(m_config.shadowAlpha / 255.0f);
}

void WorldRenderer::DrawLockAreas(WorldInfo* pWorld)
{
    Texture* pTex = m_sheets.pLockOutlineTex;
    if (!pTex)
        return;

    WorldTileManager* pTileManager = pWorld->GetTileManager();
    Vector2Int& vWorldSize = pTileManager->GetSize();

    static const int32 sides[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

    for (int32 y = 0; y < vWorldSize.y; ++y)
    {
        for (int32 x = 0; x < vWorldSize.x; ++x)
        {
            TileInfo* pTile = pTileManager->GetTile(x, y);
            if (!pTile)
                continue;

            int32 parentIndex = pTile->GetParent();
            ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());

            if (parentIndex == 0 && (!(pItem && pItem->type == ITEM_TYPE_LOCK) || IsWorldLock(pItem->id)))
                continue;

            TileInfo* pLock = pTile;
            if (!(pItem && pItem->type == ITEM_TYPE_LOCK))
            {
                if (parentIndex < 0 || parentIndex >= (vWorldSize.x * vWorldSize.y))
                    continue;

                pLock = pTileManager->GetTile(parentIndex);
            }

            if (!pLock)
                continue;

            BLRgba32 color =
                pLock->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC) ? BLRgba32(255, 255, 0, 255) : BLRgba32(255, 0, 0, 255);
            BLRect rect(x * WORLD_UNITS_PER_TILE, y * WORLD_UNITS_PER_TILE, WORLD_UNITS_PER_TILE, WORLD_UNITS_PER_TILE);

            for (int32 side = 0; side < 4; ++side)
            {
                TileInfo* pNeighbor = pTileManager->GetTile(x + sides[side][0], y + sides[side][1]);

                if (!pNeighbor || pNeighbor == pLock || pNeighbor->GetParent() == pLock->GetMapIndex())
                    continue;

                m_pRenderer->DrawFrame(pTex->GetImage(), side, 0, rect, color);
            }
        }
    }
}

void WorldRenderer::DrawTree(WorldInfo* pWorld, TileInfo* pTile, bool shadow)
{
    if (!pTile || !pTile->IsTree())
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem || pItem->type != ITEM_TYPE_SEED)
        return;

    Texture* pTex = GetResourceManager()->GetItemTexture(ITEM_ID_DIRT);
    if (!pTex)
        return;

    float bloom = pTile->GetGrowthPercent() / 100.0f;
    float treeScale = 1.0f;

    // todo here
    if (pTile->HasFlag(TILE_FLAG_IS_SEEDLING))
    {
        treeScale = Round((bloom + 2.0f) * 16.0f) / 16.0f;
    }

    Vector2Int& vTilePos = pTile->GetPos();
    BLRect drawRect(vTilePos.x * WORLD_UNITS_PER_TILE, vTilePos.y * WORLD_UNITS_PER_TILE, WORLD_UNITS_PER_TILE,
                    WORLD_UNITS_PER_TILE);

    BLRgba32 baseColor(pItem->seedBgColor.GetAsUINT());
    BLRgba32 overlayColor(pItem->seedFgColor.GetAsUINT());

    if (shadow)
    {
        drawRect.x += m_config.tileShadowOffsetX * WORLD_UNITS_PER_TILE;
        drawRect.y += m_config.tileShadowOffsetY * WORLD_UNITS_PER_TILE;
        baseColor = overlayColor = BLRgba32(0, 0, 0, 255);
    }

    m_pRenderer->DrawFrame(pTex->GetImage(), pItem->treeBg, 19, drawRect, baseColor, treeScale, treeScale, 0.0f, false,
                           false, ALIGNMENT_DOWN_CENTER);
    m_pRenderer->DrawFrame(pTex->GetImage(), pItem->treeFg, 18, drawRect, overlayColor, treeScale, treeScale, 0.0f,
                           false, false, ALIGNMENT_DOWN_CENTER);

    if (shadow || bloom < 1.0f)
        return;

    TileExtra_Seed* pExtra = pTile->GetExtra<TileExtra_Seed>();
    if (!pExtra || pExtra->fruitCount <= 0)
        return;

    static const float fruitOffsets[5][2] = {
        {10.0f, 3.0f}, {22.0f, 3.0f}, {4.0f, 14.0f}, {15.0f, 14.0f}, {28.0f, 14.0f}};

    int32 fruitCount = Min(pExtra->fruitCount, 5);
    for (int32 i = 0; i < fruitCount; ++i)
    {
        ObjectDrawParams fruit;
        fruit.itemID = pTile->GetFG() - 1;
        fruit.pos.x = vTilePos.x * WORLD_UNITS_PER_TILE;
        fruit.pos.y = vTilePos.y * WORLD_UNITS_PER_TILE;
        fruit.offset.x = fruitOffsets[i][0] * 0.5f;
        fruit.offset.y = fruitOffsets[i][1] * 0.5f;
        fruit.drawAlternate = true;
        fruit.scale = 0.55f;
        DrawObject(pWorld, fruit);
    }
}

void WorldRenderer::DrawObjects(WorldInfo* pWorld, bool shadow)
{
    if (shadow)
    {
        m_pRenderer->BeginLayer();
    }

    for (auto& obj : pWorld->GetObjectManager()->GetObjects())
    {
        ObjectDrawParams params;
        params.itemID = obj.itemID;
        params.pos = obj.pos;
        params.pObject = &obj;
        params.objectFlags = obj.flags;
        params.shadow = shadow;
        DrawObject(pWorld, params);
    }

    m_pRenderer->EndLayer(m_config.shadowAlpha / 255.0f);
}

bool WorldRenderer::IsObjectHiddenByTile(WorldInfo* pWorld, WorldObject* pObject)
{
    if (!pWorld || !pObject)
        return true;

    TileInfo* pTile = pWorld->GetTileManager()->GetTileByWorldPos(pObject->GetCenterPos());
    if (!pTile)
        return false;

    ItemInfo* pTileItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pTileItem || pTileItem->collisionType != COLLISION_IF_OFF || pTile->HasFlag(TILE_FLAG_IS_ON))
        return false;

    return pTileItem->type < ITEM_TYPE_ACHIEVEMENT &&
           (pTileItem->type == ITEM_TYPE_SWITCHEROO || pTileItem->type == ITEM_TYPE_CHEST ||
            pTileItem->type == ITEM_TYPE_PINATA);
}

void WorldRenderer::DrawObjectFrame(const ObjectDrawState& state, Texture* pTex, int32 frameX, int32 frameY,
                                    uint32 color, bool isPickupBox)
{
    if (!pTex || !pTex->HasFrame(frameX, frameY))
        return;

    float sizePx = isPickupBox ? state.boxPx : state.itemPx;
    float half = sizePx * 0.5f;
    BLRect rect(state.pos.x + state.offset.x - half, state.pos.y + state.offset.y - half, sizePx, sizePx);

    BLRgba32 tint = state.shadow ? BLRgba32(0, 0, 0, 255) : BLRgba32(color);
    m_pRenderer->DrawFrame(*pTex, frameX, frameY, rect, tint, 1.0f, 1.0f, state.rotation, false, false,
                           ALIGNMENT_CENTER);
}

void WorldRenderer::DrawObject(WorldInfo* pWorld, const ObjectDrawParams& params)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(params.itemID);
    if (!pItem || params.scale < 0.1f)
        return;

    if (params.pObject && IsObjectHiddenByTile(pWorld, params.pObject))
        return;

    ObjectDrawState state;
    state.pos = params.pos;
    state.offset = params.offset;
    state.rotation = params.ghost ? -10.0f : 0.0f;
    state.itemPx = 11.0f * params.scale;
    state.boxPx = state.itemPx * 1.25f;
    state.shadow = params.shadow;

    if (params.shadow)
    {
        state.offset.x += m_config.objectShadowOffsetX * WORLD_UNITS_PER_TILE;
        state.offset.y += m_config.objectShadowOffsetY * WORLD_UNITS_PER_TILE;
    }

    if (pItem->type == ITEM_TYPE_ROOT_CUTTING ||
        (pItem->type == ITEM_TYPE_SEED && params.itemID != ITEM_ID_BUNNY_EGG && params.itemID != ITEM_ID_MUTATED_SEED))
    {
        if (!DrawSeedObject(state, pItem, params))
            return;
    }
    else
    {
        DrawItemObject(state, pItem, params);
    }

    if (!params.shadow)
    {
        DrawObjectCount(state, pItem, params);
    }
}

bool WorldRenderer::DrawSeedObject(ObjectDrawState& state, ItemInfo* pItem, const ObjectDrawParams& params)
{
    ItemInfo* pSeed = pItem;
    Texture* pTex = m_sheets.pSeedTex;

    if (pItem->type == ITEM_TYPE_ROOT_CUTTING)
    {
        pTex = m_sheets.pRtsTex;
        pSeed = GetItemInfoManager()->GetItemByID(ITEM_TO_SEED_ID(params.itemID + 1));
    }

    if (!pTex || !pSeed)
        return false;

    uint32 baseColor = params.colorOverride ? params.colorOverride : pSeed->seedBgColor.GetAsUINT();
    if (params.ghost)
        baseColor = Proton::ColorCombine(baseColor, 0xFFFFFFFF, 0.7f);

    DrawObjectFrame(state, pTex, pSeed->seedBg, 0, baseColor, false);

    if (params.colorOverride == 0)
    {
        uint32 overlayColor = Proton::ColorCombine(pSeed->seedFgColor.GetAsUINT(), 0xFFFFFFFF, 0.6f);
        DrawObjectFrame(state, pTex, pSeed->seedFg, 1, overlayColor, false);
    }

    return true;
}

void WorldRenderer::DrawItemObject(ObjectDrawState& state, ItemInfo* pItem, const ObjectDrawParams& params)
{
    Texture* pTex = nullptr;
    int32 frameX = 0;
    int32 frameY = 0;

    if (params.drawAlternate && pItem->HasFlag(ITEM_FLAG_PERMANENT))
    {
        pTex = GetResourceManager()->GetItemTexture(ITEM_ID_DIRT);
        frameX = 12;
        frameY = 7;
    }
    else
    {
        pTex = GetResourceManager()->GetItemTexture(params.itemID);

        switch (pItem->storage)
        {
            case STORAGE_SINGLE_FRAME:
            case STORAGE_RANDOM:
                frameX = pItem->textureX;
                frameY = pItem->textureY;
                break;

            case STORAGE_SMART_EDGE:
            case STORAGE_SMART_EDGE_OUTER:
                frameX = pItem->textureX + 4;
                frameY = pItem->textureY + 1;
                break;

            case STORAGE_SMART_EDGE_HORIZ:
            case STORAGE_SMART_EDGE_VERT:
            case STORAGE_SMART_EDGE_HORIZ_CAVE:
            case STORAGE_SMART_CLING2:
            case STORAGE_SMART_EDGE_DIAGON:
                frameX = pItem->textureX + 3;
                frameY = pItem->textureY;
                break;

            case STORAGE_SMART_CLING:
                frameX = pItem->textureX + 4;
                frameY = pItem->textureY;
                break;

            default:
                break;
        }

        if (pItem->type == ITEM_TYPE_CLOTHES && pItem->GetBodyPart() == BODY_PART_SHOE)
            frameY *= 2;

        if (pItem->id == ITEM_ID_SWORD_AND_SHIELD || pItem->id == ITEM_ID_GIVING_TREE)
            frameX += 1;
    }

    uint32 color = params.colorOverride ? params.colorOverride : 0xFFFFFFFF;

    if (params.hideAmount && (params.objectFlags & 8))
    {
        color = (uint32)(RandomRangeInt(0, 254) - 256);
        state.offset.x += RandomRangeFloat(-2.0f, 2.0f) * 0.5f;
        state.offset.y += RandomRangeFloat(-2.0f, 2.0f) * 0.5f;
    }

    if (params.colorOverride == 0 && pItem->visualEffect == VISUAL_EFFECT_DISCOLOR)
    {
        if (ItemInfo* pSeed = GetItemInfoManager()->GetItemByID(ITEM_TO_SEED_ID(params.itemID + 1)))
        {
            color = pSeed->seedFgColor.GetAsUINT();
        }
    }

    if (pItem->type == ITEM_TYPE_GEMS && params.pObject)
    {
        frameX = GemsToIconFrame(params.pObject->count);
        frameY = 1;
    }

    auto applyGhost = [&](uint32 c)
    { return params.ghost ? ((c & 0xFFFFFF00) | (uint32)((float)(c & 0xFF) * 0.7f)) : c; };

    bool twoTone =
        pItem->visualEffect <= VISUAL_EFFECT_SILKFOOT &&
        (pItem->visualEffect == VISUAL_EFFECT_COLORIZE || pItem->visualEffect == VISUAL_EFFECT_COLORIZE_ANIM ||
         pItem->visualEffect == VISUAL_EFFECT_COLORIZE_WITH_SKIN || pItem->visualEffect == VISUAL_EFFECT_PETCOLORED ||
         pItem->visualEffect == VISUAL_EFFECT_SILKFOOT) &&
        !(params.drawAlternate && pItem->HasFlag(ITEM_FLAG_PERMANENT));
    if (twoTone)
    {
        if (ItemInfo* pSeed = GetItemInfoManager()->GetItemByID(ITEM_TO_SEED_ID(params.itemID + 1)))
        {
            if (params.colorOverride == 0)
            {
                color = pSeed->seedBgColor.GetAsUINT();
            }

            DrawObjectFrame(state, pTex, frameX, frameY, applyGhost(color), false);

            if (params.colorOverride == 0)
            {
                uint32 overlay =
                    (pItem->visualEffect == VISUAL_EFFECT_PETCOLORED) ? 0xFFC00000 : pSeed->seedFgColor.GetAsUINT();
                DrawObjectFrame(state, pTex, frameX + 1, frameY, applyGhost(overlay), false);
            }
        }

        color = params.colorOverride ? params.colorOverride : 0xFFFFFFFF;
    }
    else
    {
        if (!(pItem->HasFlag2(ITEM_FLAG_GUILD_ITEM)) || (params.drawAlternate && pItem->HasFlag(ITEM_FLAG_PERMANENT)))
        {
            uint32 frameColor = applyGhost(color);
            if (pItem->visualEffect == VISUAL_EFFECT_BACKFORE)
            {
                DrawObjectFrame(state, pTex, frameX, frameY + 1, frameColor, false);
            }

            DrawObjectFrame(state, pTex, frameX, frameY, frameColor, false);

            if (params.itemID == ITEM_ID_COMET_BEANIE)
            {
                DrawObjectFrame(state, pTex, frameX + 1, frameY, frameColor, false);
            }
        }
        else if (pTex)
        {
            if (params.itemID == ITEM_ID_GUILD_LOCK)
            {
                DrawObjectFrame(state, pTex, frameX, frameY, color, false);
                DrawObjectFrame(state, pTex, 4, 0, color, false);
                DrawObjectFrame(state, pTex, 7, 0, color, false);
            }
        }

        if (params.colorOverride == 0 &&
            (pItem->visualEffect == VISUAL_EFFECT_RAINBOW_SHIFT || pItem->visualEffect == VISUAL_EFFECT_DISCOLOR))
        {
            color = 0xFFFFFFFF;
        }
    }

    if (!params.drawAlternate && !params.ghost && params.drawBox && pItem->id != ITEM_ID_GEMS &&
        pItem->id != ITEM_ID_BUNNY_EGG && pItem->id != ITEM_ID_CARRIED_GAME_FLAG &&
        pItem->id != ITEM_ID_CARRIED_GAME_EGG && pItem->id != ITEM_ID_COMET_DUST &&
        pItem->id != ITEM_ID_ANTIMATTER_DUST && pItem->id != ITEM_ID_MUTATED_SEED && pItem->id != ITEM_ID_MUTATED_CELL)
    {
        DrawObjectFrame(state, m_sheets.pPickupBoxTex, GetPickupBoxFrameFromType(pItem->type), 0, color, true);
    }
}

void WorldRenderer::DrawObjectCount(const ObjectDrawState& state, ItemInfo* pItem, const ObjectDrawParams& params)
{
    if (params.hideAmount || params.ghost || !params.pObject)
        return;

    if (pItem->type == ITEM_TYPE_GEMS || params.pObject->itemID == ITEM_ID_CARRIED_GAME_FLAG ||
        params.pObject->itemID == ITEM_ID_CARRIED_GAME_EGG)
        return;

    if (params.pObject->count < 2)
        return;

    if (pItem->type == ITEM_TYPE_PETFISH)
        return;

    BLFont* pFont = GetResourceManager()->GetFont(FONT_TYPE_CENTURY_GOTHIC_BOLD);
    if (!pFont)
        return;

    float viewScale = m_pRenderer->GetViewScale();
    string text = ToString(params.pObject->count);

    float targetLineHeight = state.itemPx * 0.6f * viewScale;
    float unitLineHeight = m_pRenderer->GetTextHeight(pFont, 1.0f);
    float fontSize = unitLineHeight > 0.0f ? targetLineHeight / unitLineHeight : targetLineHeight;

    float rightX = (state.pos.x + state.offset.x + state.itemPx * 0.5f - 0.5f) * viewScale;
    float topY = (state.pos.y + state.offset.y) * viewScale;
    BLPoint origin(rightX - m_pRenderer->GetTextWidth(pFont, text, fontSize), topY);

    m_pRenderer->DrawGTText(pFont, BLPoint(origin.x + 1.0f, origin.y + 1.0f), "`b" + text, fontSize);
    m_pRenderer->DrawGTText(pFont, origin, text, fontSize);
}

Vector2Int WorldRenderer::GetSpriteCoords(WorldInfo* pWorld, TileInfo* pTile, ItemInfo* pItem)
{
    if (!pItem)
        return {0, 0};

    uint8 coordX = pItem->textureX;
    uint8 coordY = pItem->textureY;

    WorldTileManager* pTileMgr = pWorld->GetTileManager();

    Vector2Int& vTilePos = pTile->GetPos();
    Vector2Int& vWorldSize = pTileMgr->GetSize();

    bool isBackground = pItem->IsBackground();

    switch (pItem->storage)
    {
        case STORAGE_SMART_EDGE:
        {
            bool topLeft = (vTilePos.x > 0 && vTilePos.y > 0)
                               ? pTileMgr->IsSameTile(pTile, vTilePos.x - 1, vTilePos.y - 1, isBackground)
                               : false;

            bool midLeft =
                (vTilePos.x > 0) ? pTileMgr->IsSameTile(pTile, vTilePos.x - 1, vTilePos.y, isBackground) : false;

            bool botLeft = (vTilePos.x > 0 && vTilePos.y < vWorldSize.y - 1)
                               ? pTileMgr->IsSameTile(pTile, vTilePos.x - 1, vTilePos.y + 1, isBackground)
                               : false;

            bool bottom = (vTilePos.y < vWorldSize.y - 1)
                              ? pTileMgr->IsSameTile(pTile, vTilePos.x, vTilePos.y + 1, isBackground)
                              : false;

            bool bottomRight = (vTilePos.x < vWorldSize.x - 1 && vTilePos.y < vWorldSize.y - 1)
                                   ? pTileMgr->IsSameTile(pTile, vTilePos.x + 1, vTilePos.y + 1, isBackground)
                                   : false;

            bool midRight = (vTilePos.x < vWorldSize.x - 1)
                                ? pTileMgr->IsSameTile(pTile, vTilePos.x + 1, vTilePos.y, isBackground)
                                : false;

            bool topRight = (vTilePos.x < vWorldSize.x - 1 && vTilePos.y > 0)
                                ? pTileMgr->IsSameTile(pTile, vTilePos.x + 1, vTilePos.y - 1, isBackground)
                                : false;

            bool top = (vTilePos.y > 0) ? pTileMgr->IsSameTile(pTile, vTilePos.x, vTilePos.y - 1, isBackground) : false;

            uint8 mask = (topLeft << 0) | (top << 1) | (topRight << 2) | (midRight << 3) | (bottomRight << 4) |
                         (bottom << 5) | (botLeft << 6) | (midLeft << 7);

            int32 lutVisual = sWorldTileSmartEdgeLut[mask];

            coordX += lutVisual % 8;
            coordY += lutVisual / 8;
            break;
        }

        case STORAGE_SMART_EDGE_HORIZ:
        {
            bool right = pTileMgr->IsSameTile(pTile, vTilePos.x + 1, vTilePos.y, isBackground);
            bool left = pTileMgr->IsSameTile(pTile, vTilePos.x - 1, vTilePos.y, isBackground);

            if (right)
                coordX += left ? 1 : 0;
            else
                coordX += left ? 0 : 3;
            break;
        }

        case STORAGE_SMART_EDGE_DIAGON:
        {
            bool isFlipped = (pTile->HasFlag(TILE_FLAG_FLIPPED_X));

            int32 diag1X = 0, diag1Y = 0;
            int32 diag2X = 0, diag2Y = 0;

            if (!isFlipped)
            {
                diag1X = vTilePos.x - 1;
                diag1Y = vTilePos.y + 1;
                diag2X = vTilePos.x + 1;
                diag2Y = vTilePos.y - 1;
            }
            else
            {
                diag1X = vTilePos.x + 1;
                diag1Y = vTilePos.y + 1;
                diag2X = vTilePos.x - 1;
                diag2Y = vTilePos.y - 1;
            }

            auto checkDiagonal = [&](int32 tx, int32 ty) -> bool
            {
                if (tx < 0 || tx >= vWorldSize.x || ty < 0 || ty >= vWorldSize.y)
                    return true;

                TileInfo* pTarget = pTileMgr->GetTile(tx, ty);
                if (!pTarget)
                    return true;

                uint16 currentID = isBackground ? pTile->GetBG() : pTile->GetFG();
                uint16 targetID = isBackground ? pTarget->GetBG() : pTarget->GetFG();

                if (targetID != ITEM_ID_BLANK && pTarget->HasFlag(TILE_FLAG_GLUED))
                {
                    if (!isBackground && (targetID & 1) == 0)
                        return true;
                    if (isBackground)
                        return true;
                }

                if (targetID == currentID)
                    return pTarget->HasFlag(TILE_FLAG_FLIPPED_X) == isFlipped;

                return false;
            };

            bool bVar2 = checkDiagonal(diag1X, diag1Y);
            bool bVar3 = checkDiagonal(diag2X, diag2Y);

            static const uint8 diagonalLut[2][2] = {{3, 0}, {2, 1}};

            int32 lutVisual = diagonalLut[bVar2][bVar3];

            coordX += lutVisual % 8;
            coordY += lutVisual / 8;
            break;
        }

        case STORAGE_SMART_EDGE_OUTER:
        {
            uint16 currentID = isBackground ? pTile->GetBG() : pTile->GetFG();

            auto checkOuterConnection = [&](int32 tx, int32 ty) -> bool
            {
                if (tx < 0 || tx >= vWorldSize.x || ty < 0 || ty >= vWorldSize.y)
                    return true;

                TileInfo* pTarget = pTileMgr->GetTile(tx, ty);
                if (!pTarget)
                    return true;

                if (isBackground)
                {
                    uint16 targetBG = pTarget->GetFG();

                    if (targetBG == ITEM_ID_BLANK || !pTarget->HasFlag(TILE_FLAG_GLUED))
                        return false;

                    if (currentID == ITEM_ID_WEEPING_WILLOW_BRANCH)
                        return (targetBG == ITEM_ID_WEEPING_WILLOW);

                    return (targetBG == currentID);
                }

                return (pTarget->GetFG() == currentID);
            };

            bool right = checkOuterConnection(vTilePos.x + 1, vTilePos.y);
            bool bottom = checkOuterConnection(vTilePos.x, vTilePos.y + 1);
            bool left = checkOuterConnection(vTilePos.x - 1, vTilePos.y);
            bool top = checkOuterConnection(vTilePos.x, vTilePos.y - 1);

            uint8 mask = (right << 0) | (bottom << 1) | (left << 2) | (top << 3);
            int32 lutVisual = sWorldTileSmartEdgeOuterLut[mask];

            coordX += lutVisual % 8;
            coordY += lutVisual / 8;
            break;
        }

        case STORAGE_SMART_EDGE_VERT:
        {
            bool top = pTileMgr->IsSameTile(pTile, vTilePos.x, vTilePos.y - 1, isBackground);
            bool bottom = pTileMgr->IsSameTile(pTile, vTilePos.x, vTilePos.y + 1, isBackground);

            if (top)
                coordX += bottom ? 1 : 0;
            else
                coordX += bottom ? 0 : 3;
            break;
        }
    }

    if (pTile->HasFlag(TILE_FLAG_IS_ON))
        coordX += 1;

    return {coordX, coordY};
}

int32 WorldRenderer::GemsToIconFrame(int32 gemValue)
{
    static const int32 gemValues[5] = {1, 5, 10, 50, 100};

    for (int32 i = 0; i < 5; ++i)
    {
        if (gemValues[i] == gemValue)
            return 22 + i;
    }

    return 0;
}

int32 WorldRenderer::GetPickupBoxFrameFromType(int32 itemType)
{
    switch (itemType)
    {
        case ITEM_TYPE_LOCK:
            return 6;
        case ITEM_TYPE_CONSUMABLE:
            return 5;
        case ITEM_TYPE_BACKGROUND:
        case ITEM_TYPE_BACKGD_SFX_EXTRA_FRAME:
        case ITEM_TYPE_BACK_BOOMBOX:
        case ITEM_TYPE_MUSICNOTE:
            return 1;
        case ITEM_TYPE_SEED:
            return 3;
        case ITEM_TYPE_CLOTHES:
            return 8;
        default:
            return 0;
    }
}