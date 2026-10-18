#include "WeatherRenderer.h"
#include "Math/Math.h"
#include "Math/Random.h"
#include "Utils/ResourceManager.h"
#include "Utils/StringUtils.h"

WeatherRenderer::WeatherRenderer(Renderer2D* pRenderer) : m_pRenderer(pRenderer)
{
    if (m_pRenderer)
    {
        m_width = (float)m_pRenderer->GetSurfaceWidth();
        m_height = (float)m_pRenderer->GetSurfaceHeight();
    }
}

WeatherRenderer::~WeatherRenderer() {}

void WeatherRenderer::DrawDefault(bool spring)
{
    if (!m_pRenderer)
        return;

    ResourceManager* pResMgr = GetResourceManager();

    m_pRenderer->DrawRectFilled(BLRect(0, 0, m_width, m_height),
                                spring ? BLRgba32(36, 130, 242, 255) : BLRgba32(96, 215, 242, 255));

    if (BLImage* pSun = pResMgr->GetTileSheet("game/sun.rttex"))
    {
        float sunScale = (m_width / 161.0f) / 6.0f;
        m_pRenderer->DrawImageAt(pSun, BLPoint(m_width * 0.7f, m_height * 0.1f), ALIGNMENT_UPPER_LEFT,
                                 BLRgba32(255, 255, 255, 255), sunScale, sunScale);
    }

    DrawHill(0, spring);
    DrawHill(1, spring);
    DrawHill(2, spring);
}

void WeatherRenderer::DrawHill(int32 hillIndex, bool spring)
{
    if (!m_pRenderer)
        return;

    BLImage* pHill = GetResourceManager()->GetTileSheet("game/hills" + ToString(3 - hillIndex) + ".rttex");
    if (!pHill)
        return;

    float hillScale = m_width / ((float)pHill->width() - 1.0f);
    float y = m_height + 2.0f;

    if (hillIndex == 0)
        y -= hillScale * 46.0f;
    else if (hillIndex == 1)
        y += hillScale * 60.0f;

    BLRgba32 tint = spring ? BLRgba32(0, 255, 40, 255) : BLRgba32(255, 255, 255, 255);
    m_pRenderer->DrawImageAt(pHill, BLPoint(0.0f, y), ALIGNMENT_DOWN_LEFT, tint, hillScale, hillScale);
}

void WeatherRenderer::DrawHarvest()
{
    if (!m_pRenderer)
        return;

    ResourceManager* pResMgr = GetResourceManager();

    if (BLImage* pSky = pResMgr->GetTileSheet("game/sunset.rttex"))
        m_pRenderer->DrawImageAt(pSky, BLPoint(0.0f, -10.0f), ALIGNMENT_UPPER_LEFT, BLRgba32(255, 255, 255, 255),
                                 m_width / 32.0f, m_height / 28.0f);

    if (BLImage* pMoon = pResMgr->GetTileSheet("game/moon2.rttex"))
    {
        float moonScale = (m_width * 0.00390625f) / 2.5f; // muhehehehehe
        m_pRenderer->DrawImageAt(pMoon, BLPoint(m_width, m_height * 0.15f), ALIGNMENT_UPPER_RIGHT,
                                 BLRgba32(255, 255, 255, 255), moonScale, moonScale);
    }

    DrawFence(0);
    DrawFence(1);
}

void WeatherRenderer::DrawFence(int32 layer)
{
    if (!m_pRenderer)
        return;

    BLImage* pImg = GetResourceManager()->GetTileSheet(layer == 0 ? "game/field_back.rttex" : "game/field_front.rttex");

    float scale = m_width / 1024.0f;
    float period = scale * (layer == 1 ? 512.0f : 1024.0f);
    float bottomY = (m_height + 2.0f) - m_height * (layer == 0 ? 0.25f : 0.125f);

    if (pImg && period > 0.0f)
    {
        for (float x = 0.0f; x <= m_width; x += period)
            m_pRenderer->DrawImageAt(pImg, BLPoint(x, bottomY), ALIGNMENT_DOWN_LEFT, BLRgba32(255, 255, 255, 255),
                                     scale, scale);
    }

    BLRgba32 fill = layer == 0 ? BLRgba32(0, 0, 0, 255) : BLRgba32(16, 24, 12, 255);
    float top = bottomY - scale;
    m_pRenderer->DrawRectFilled(BLRect(0.0f, top, m_width, m_height - top), fill);
}

void WeatherRenderer::DrawSunset()
{
    if (!m_pRenderer)
        return;

    BLImage* pBackground = GetResourceManager()->GetTileSheet("game/sunset.rttex");
    if (!pBackground)
        return;
}