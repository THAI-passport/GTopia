#pragma once

#include "Precompiled.h"
#include "Texture.h"
#include "Utils/StringUtils.h"
#include <blend2d/blend2d.h>

#define RENDERER_MAX_CACHE_SCALE 64 * 1024 * 1024
#define RENDERER_MAC_CACHE_TINT 16 * 1024 * 1024

enum eAlignment
{
    ALIGNMENT_UPPER_LEFT,
    ALIGNMENT_CENTER,
    ALIGNMENT_DOWN_CENTER,
    ALIGNMENT_UPPER_RIGHT,
    ALIGNMENT_DOWN_LEFT,
    ALIGNMENT_UPPER_CENTER,
    ALIGNMENT_LEFT_CENTER,
    ALIGNMENT_DOWN_RIGHT,
    ALIGNMENT_RIGHT_CENTER,
};

class Renderer2D
{
public:
    Renderer2D();
    ~Renderer2D();

public:
    void Init(int32 width, int32 height);
    void ReleaseResources();

    void SetThreadCount(uint8 threadCount) { m_threadCount = threadCount; }

    void SetViewScale(float scale) { m_viewScale = scale > 0.0f ? scale : 1.0f; }
    float GetViewScale() const { return m_viewScale; }

    void DrawRectFilled(const BLRect& rect, const BLRgba32& color = BLRgba32(255, 255, 255, 255), float scaleX = 1.0f,
                        float scaleY = 1.0f, float rotateAngle = 0.0f, bool flipX = false, bool flipY = false,
                        eAlignment alignment = ALIGNMENT_UPPER_LEFT);

    void DrawRectFilled(const BLRect& rect, const BLRgba32& color, float scale, float rotateAngle = 0.0f,
                        bool flipX = false, bool flipY = false, eAlignment alignment = ALIGNMENT_UPPER_LEFT);

    void DrawRect(const BLRect& rect, const BLRgba32& color = BLRgba32(255, 255, 255, 255), float strokeWidth = 1.0f,
                  float scaleX = 1.0f, float scaleY = 1.0f, float rotateAngle = 0.0f, bool flipX = false,
                  bool flipY = false, eAlignment alignment = ALIGNMENT_UPPER_LEFT);

    void DrawImage(BLImage* pImage, const BLRect& drawRect, const BLRgba32& color = BLRgba32(255, 255, 255, 255),
                   float scaleX = 1.0f, float scaleY = 1.0f, float rotateAngle = 0.0f, bool flipX = false,
                   bool flipY = false, eAlignment alignment = ALIGNMENT_UPPER_LEFT);

    void DrawImageAt(BLImage* pImage, const BLPoint& pivot, eAlignment alignment = ALIGNMENT_UPPER_LEFT,
                     const BLRgba32& color = BLRgba32(255, 255, 255, 255), float scaleX = 1.0f, float scaleY = 1.0f,
                     float rotateAngle = 0.0f, bool flipX = false, bool flipY = false);

    void DrawSprite(BLImage* pImage, const BLRect& drawRect, const BLRectI& spriteRect,
                    const BLRgba32& color = BLRgba32(255, 255, 255, 255), float scaleX = 1.0f, float scaleY = 1.0f,
                    float rotateAngle = 0.0f, bool flipX = false, bool flipY = false,
                    eAlignment alignment = ALIGNMENT_UPPER_LEFT);

    void DrawFrame(const Texture& sheet, int32 frameX, int32 frameY, const BLRect& drawRect,
                   const BLRgba32& color = BLRgba32(255, 255, 255, 255), float scaleX = 1.0f, float scaleY = 1.0f,
                   float rotateAngle = 0.0f, bool flipX = false, bool flipY = false,
                   eAlignment alignment = ALIGNMENT_UPPER_LEFT);

    void DrawFrameAt(const Texture& sheet, int32 frameX, int32 frameY, const BLPoint& pivot,
                     eAlignment alignment = ALIGNMENT_UPPER_LEFT, const BLRgba32& color = BLRgba32(255, 255, 255, 255),
                     float scaleX = 1.0f, float scaleY = 1.0f, float rotateAngle = 0.0f, bool flipX = false,
                     bool flipY = false);

    void DrawGTText(BLFont* pFont, const BLPoint& origin, const string& text, float size = 16.0f);
    float GetTextWidth(BLFont* pFont, const string& text, float size);
    float GetTextHeight(BLFont* pFont, float size);

    void BeginLayer();
    void EndLayer(float opacity);

    void Begin();
    void End();
    void Clear() { m_context.clear_all(); }
    bool WriteToFile(const string& path);

    void ClearTintCache();
    void ClearScaledCache();

    int32 GetSurfaceWidth() const { return m_surface.width(); }
    int32 GetSurfaceHeight() const { return m_surface.height(); }

    BLImage TintSprite(BLImage* pSrc, const BLRectI& rect, const BLRgba32& color);

private:
    BLRect ToScreen(const BLRect& worldRect) const;

    BLContext& Ctx() { return m_layerActive ? m_layerContext : m_context; }

    void ApplyTransform(const BLRect& screenRect, float scaleX, float scaleY, float rotateAngle, bool flipX, bool flipY,
                        eAlignment alignment);

    void ReleaseLayer();
    void FreeCaches();

    BLImage* GetScaledSprite(BLImage* pSrc, const BLRectI& rect, int32 outW, int32 outH);
    BLImage* GetTintedSprite(BLImage* pSrc, const BLRectI& rect, const BLRgba32& color);

private:
    struct TintKey
    {
        BLImage* pImage;
        int32 x, y, w, h;
        uint32 colorValue;

        bool operator==(const TintKey& o) const
        {
            return pImage == o.pImage && x == o.x && y == o.y && w == o.w && h == o.h && colorValue == o.colorValue;
        }
    };

    struct TintKeyHash
    {
        uint32 operator()(const TintKey& k) const
        {
            uint32 data[6] = {
                (uint32)((uintptr_t)k.pImage >> 4), (uint32)k.x, (uint32)k.y, (uint32)k.w, (uint32)k.h, k.colorValue};
            return HashString((const char*)data, sizeof(data));
        }
    };

    struct ScaledKey
    {
        BLImage* pImage;
        int32 x, y, w, h;
        int32 outW, outH;

        bool operator==(const ScaledKey& o) const
        {
            return pImage == o.pImage && x == o.x && y == o.y && w == o.w && h == o.h && outW == o.outW &&
                   outH == o.outH;
        }
    };

    struct ScaledKeyHash
    {
        uint32 operator()(const ScaledKey& k) const
        {
            uint32 data[7] = {(uint32)((uintptr_t)k.pImage >> 4),
                              (uint32)k.x,
                              (uint32)k.y,
                              (uint32)k.w,
                              (uint32)k.h,
                              (uint32)k.outW,
                              (uint32)k.outH};
            return HashString((const char*)data, sizeof(data));
        }
    };

private:
    BLImage m_surface;
    BLContext m_context;
    BLImage m_layerImage;
    BLContext m_layerContext;
    bool m_layerActive;
    uint8 m_threadCount;
    float m_viewScale;

    // lets speed up by caching these
    std::unordered_map<TintKey, BLImage, TintKeyHash> m_tintCache;
    std::unordered_map<ScaledKey, BLImage, ScaledKeyHash> m_scaledCache;
    usize m_tintBytes;
    usize m_scaledBytes;
};