#include "Renderer2D.h"
#include "Math/Math.h"

static BLRgba32 GetGTColor(char code)
{
    switch (code)
    {
        case '1':
            return BLRgba32(173, 244, 255);
        case '2':
            return BLRgba32(73, 252, 0);
        case '3':
            return BLRgba32(191, 218, 255);
        case '4':
            return BLRgba32(255, 39, 29);
        case '5':
            return BLRgba32(235, 183, 255);
        case '6':
            return BLRgba32(255, 202, 111);
        case '7':
            return BLRgba32(230, 230, 230);
        case '8':
            return BLRgba32(255, 148, 69);
        case '9':
            return BLRgba32(255, 238, 125);
        case '!':
            return BLRgba32(209, 255, 249);
        case '@':
            return BLRgba32(255, 205, 201);
        case '#':
            return BLRgba32(255, 143, 243);
        case '$':
            return BLRgba32(255, 252, 197);
        case '^':
            return BLRgba32(181, 255, 151);
        case '&':
            return BLRgba32(254, 235, 255);
        case 'o':
            return BLRgba32(252, 230, 186);
        case 'b':
            return BLRgba32(0, 0, 0);
        case 'p':
            return BLRgba32(255, 223, 241);
        case 'q':
            return BLRgba32(12, 96, 164);
        case 'e':
            return BLRgba32(25, 185, 255);
        case 'r':
            return BLRgba32(111, 211, 87);
        case 't':
            return BLRgba32(47, 131, 13);
        case 'a':
            return BLRgba32(81, 81, 81);
        case 's':
            return BLRgba32(158, 158, 158);
        case 'c':
            return BLRgba32(80, 255, 255);
        default:
            return BLRgba32(255, 255, 255);
    }
}

static void AlignmentToFactors(eAlignment alignment, float& rx, float& ry)
{
    switch (alignment)
    {
        case ALIGNMENT_UPPER_CENTER:
            rx = 0.5f, ry = 0.0f;
            break;
        case ALIGNMENT_UPPER_RIGHT:
            rx = 1.0f, ry = 0.0f;
            break;
        case ALIGNMENT_LEFT_CENTER:
            rx = 0.0f, ry = 0.5f;
            break;
        case ALIGNMENT_CENTER:
            rx = 0.5f, ry = 0.5f;
            break;
        case ALIGNMENT_RIGHT_CENTER:
            rx = 1.0f, ry = 0.5f;
            break;
        case ALIGNMENT_DOWN_LEFT:
            rx = 0.0f, ry = 1.0f;
            break;
        case ALIGNMENT_DOWN_CENTER:
            rx = 0.5f, ry = 1.0f;
            break;
        case ALIGNMENT_DOWN_RIGHT:
            rx = 1.0f, ry = 1.0f;
            break;
        case ALIGNMENT_UPPER_LEFT:
        default:
            rx = 0.0f, ry = 0.0f;
            break;
    }
}

static void ResampleNearest(const BLImageData& src, const BLRectI& rect, const BLImageData& dst, int32 outW, int32 outH)
{
    uint8* srcBase = (uint8*)src.pixel_data;
    uint8* dstBase = (uint8*)dst.pixel_data;

    std::vector<int32> columns(outW);
    for (int32 x = 0; x < outW; ++x)
    {
        columns[x] = Min((x * rect.w) / outW, rect.w - 1);
    }

    for (int32 y = 0; y < outH; ++y)
    {
        int32 sy = Min(rect.y + (y * rect.h) / outH, rect.y + rect.h - 1);
        uint8* srcRow = srcBase + sy * src.stride + rect.x * 4;
        uint8* dstRow = dstBase + y * dst.stride;

        for (int32 x = 0; x < outW; ++x)
        {
            memcpy(dstRow + x * 4, srcRow + columns[x] * 4, 4);
        }
    }
}

static float ProcessTextLine(BLContext* ctx, BLFont* pFont, BLGlyphBuffer& gb, const char* line, usize size, float x,
                             float y, BLRgba32& color)
{
    float width = 0.0f;
    const char* end = line + size;
    const char* src = line;

    while (src < end)
    {
        if (*src == '`' && src + 1 < end)
        {
            color = GetGTColor(src[1]);
            src += 2;
            continue;
        }

        const char* next = (const char*)(memchr(src + 1, '`', end - (src + 1)));
        usize len = (next ? next : end) - src;

        gb.set_utf8_text(src, len);
        pFont->shape(gb);

        BLTextMetrics tm;
        pFont->get_text_metrics(gb, tm);

        if (ctx)
        {
            ctx->fill_glyph_run(BLPoint(x + width, y), *pFont, gb.glyph_run(), color);
        }

        width += tm.advance.x;
        src += len;
    }

    return width;
}

Renderer2D::Renderer2D() : m_layerActive(false), m_threadCount(0), m_viewScale(1.0f), m_tintBytes(0), m_scaledBytes(0)
{
}

Renderer2D::~Renderer2D() {}

void Renderer2D::Init(int32 width, int32 height)
{
    if (width <= 0 || height <= 0)
        return;

    if (m_surface.width() == width && m_surface.height() == height)
        return;

    ReleaseLayer();

    if (m_context.is_valid())
    {
        m_context.end();
    }

    m_context = BLContext();
    m_surface = BLImage(width, height, BL_FORMAT_PRGB32);
}

void Renderer2D::ReleaseResources()
{
    ReleaseLayer();

    if (m_context.is_valid())
    {
        m_context.end();
    }

    m_context = BLContext();
    m_surface = BLImage();

    FreeCaches();
}

void Renderer2D::ReleaseLayer()
{
    if (m_layerActive)
        m_layerContext.end();

    m_layerActive = false;
    m_layerContext = BLContext();
    m_layerImage = BLImage();
}

void Renderer2D::FreeCaches()
{
    m_tintCache.clear();
    m_scaledCache.clear();
    m_tintBytes = 0;
    m_scaledBytes = 0;
}

void Renderer2D::BeginLayer()
{
    if (m_layerActive || !m_context.is_valid())
        return;

    if (m_layerImage.width() != m_surface.width() || m_layerImage.height() != m_surface.height())
    {
        m_layerImage = BLImage(m_surface.width(), m_surface.height(), BL_FORMAT_PRGB32);
    }

    BLContextCreateInfo createInfo;
    if (m_threadCount > 0)
    {
        createInfo.thread_count = m_threadCount;
    }

    m_layerContext = BLContext(m_layerImage, createInfo);
    m_layerContext.clear_all();
    m_layerActive = true;
}

void Renderer2D::EndLayer(float opacity)
{
    if (!m_layerActive)
        return;

    m_layerContext.end();
    m_layerContext = BLContext();
    m_layerActive = false;

    m_context.save();
    m_context.set_global_alpha(Max(0.0f, Min(1.0f, opacity)));
    m_context.blit_image(BLPointI(0, 0), m_layerImage);
    m_context.restore();
}

void Renderer2D::Begin()
{
    if (!m_context.is_valid())
    {
        BLContextCreateInfo createInfo;
        if (m_threadCount > 0)
        {
            createInfo.thread_count = m_threadCount;
        }

        m_context = BLContext(m_surface, createInfo);
    }

    m_context.clear_all();
}

void Renderer2D::End()
{
    ReleaseLayer();

    if (m_context.is_valid())
    {
        m_context.end();
    }
    m_context = BLContext();
}

bool Renderer2D::WriteToFile(const string& path)
{
    if (m_context.is_valid())
    {
        m_context.flush(BL_CONTEXT_FLUSH_SYNC);
    }

    return m_surface.write_to_file(path.c_str()) == BL_SUCCESS;
}

void Renderer2D::ClearTintCache()
{
    m_tintCache.clear();
    m_tintBytes = 0;
}

void Renderer2D::ClearScaledCache()
{
    ClearTintCache();
    m_scaledCache.clear();
    m_scaledBytes = 0;
}

BLRect Renderer2D::ToScreen(const BLRect& r) const
{
    return BLRect(r.x * m_viewScale, r.y * m_viewScale, r.w * m_viewScale, r.h * m_viewScale);
}

void Renderer2D::ApplyTransform(const BLRect& screenRect, float scaleX, float scaleY, float rotateAngle, bool flipX,
                                bool flipY, eAlignment alignment)
{
    float rx = 0.0f, ry = 0.0f;
    AlignmentToFactors(alignment, rx, ry);

    float anchorX = (float)(screenRect.x + screenRect.w * rx);
    float anchorY = (float)(screenRect.y + screenRect.h * ry);
    float localAnchorX = (float)(screenRect.w * rx);
    float localAnchorY = (float)(screenRect.h * ry);

    Ctx().translate(anchorX, anchorY);
    if (rotateAngle != 0.0f)
    {
        Ctx().rotate(rotateAngle * (M_PI / 180.0));
    }

    Ctx().scale(flipX ? -scaleX : scaleX, flipY ? -scaleY : scaleY);
    Ctx().translate(-localAnchorX, -localAnchorY);
}

void Renderer2D::DrawRectFilled(const BLRect& rect, const BLRgba32& color, float scaleX, float scaleY,
                                float rotateAngle, bool flipX, bool flipY, eAlignment alignment)
{
    if (color.a() == 0)
        return;

    BLRect screen = ToScreen(rect);

    Ctx().save();
    ApplyTransform(screen, scaleX, scaleY, rotateAngle, flipX, flipY, alignment);
    Ctx().fill_rect(BLRect(0, 0, screen.w, screen.h), color);
    Ctx().restore();
}

void Renderer2D::DrawRectFilled(const BLRect& rect, const BLRgba32& color, float scale, float rotateAngle, bool flipX,
                                bool flipY, eAlignment alignment)
{
    DrawRectFilled(rect, color, scale, scale, rotateAngle, flipX, flipY, alignment);
}

void Renderer2D::DrawRect(const BLRect& rect, const BLRgba32& color, float strokeWidth, float scaleX, float scaleY,
                          float rotateAngle, bool flipX, bool flipY, eAlignment alignment)
{
    if (color.a() == 0)
        return;

    BLRect screen = ToScreen(rect);

    Ctx().save();
    ApplyTransform(screen, scaleX, scaleY, rotateAngle, flipX, flipY, alignment);
    Ctx().set_stroke_width(strokeWidth);
    Ctx().stroke_rect(BLRect(0, 0, screen.w, screen.h), color);
    Ctx().restore();
}

void Renderer2D::DrawImage(BLImage* pImage, const BLRect& drawRect, const BLRgba32& color, float scaleX, float scaleY,
                           float rotateAngle, bool flipX, bool flipY, eAlignment alignment)
{
    if (!pImage)
        return;

    BLRectI full(0, 0, pImage->width(), pImage->height());
    DrawSprite(pImage, drawRect, full, color, scaleX, scaleY, rotateAngle, flipX, flipY, alignment);
}

void Renderer2D::DrawImageAt(BLImage* pImage, const BLPoint& pivot, eAlignment alignment, const BLRgba32& color,
                             float scaleX, float scaleY, float rotateAngle, bool flipX, bool flipY)
{
    if (!pImage)
        return;

    Texture full(pImage, (int32)pImage->width(), (int32)pImage->height());
    DrawFrameAt(full, 0, 0, pivot, alignment, color, scaleX, scaleY, rotateAngle, flipX, flipY);
}

void Renderer2D::DrawFrameAt(const Texture& sheet, int32 frameX, int32 frameY, const BLPoint& pivot,
                             eAlignment alignment, const BLRgba32& color, float scaleX, float scaleY, float rotateAngle,
                             bool flipX, bool flipY)
{
    if (!sheet.HasFrame(frameX, frameY))
        return;

    float w = (float)sheet.GetFrameWidth() / m_viewScale;
    float h = (float)sheet.GetFrameHeight() / m_viewScale;

    float rx = 0.0f, ry = 0.0f;
    AlignmentToFactors(alignment, rx, ry);

    BLRect box(pivot.x - w * rx, pivot.y - h * ry, w, h);
    DrawSprite(sheet.GetImage(), box, sheet.GetFrameRect(frameX, frameY), color, scaleX, scaleY, rotateAngle, flipX,
               flipY, alignment);
}

void Renderer2D::DrawFrame(const Texture& sheet, int32 frameX, int32 frameY, const BLRect& drawRect,
                           const BLRgba32& color, float scaleX, float scaleY, float rotateAngle, bool flipX, bool flipY,
                           eAlignment alignment)
{
    if (!sheet.HasFrame(frameX, frameY))
        return;

    DrawSprite(sheet.GetImage(), drawRect, sheet.GetFrameRect(frameX, frameY), color, scaleX, scaleY, rotateAngle,
               flipX, flipY, alignment);
}

void Renderer2D::DrawSprite(BLImage* pImage, const BLRect& drawRect, const BLRectI& spriteRect, const BLRgba32& color,
                            float scaleX, float scaleY, float rotateAngle, bool flipX, bool flipY, eAlignment alignment)
{
    if (!pImage || color.a() == 0 || spriteRect.w <= 0 || spriteRect.h <= 0)
        return;

    BLRect screen = ToScreen(drawRect);

    int32 outW = Max(1, Round(screen.w * Abs(scaleX)));
    int32 outH = Max(1, Round(screen.h * Abs(scaleY)));

    BLImage* pSource = pImage;
    BLRectI srcRect = spriteRect;
    BLRect rect = screen;
    float useScaleX = scaleX;
    float useScaleY = scaleY;

    bool needsResize = (outW != spriteRect.w) || (outH != spriteRect.h);
    bool cacheable = outW <= 256 && outH <= 256;

    BLImage* pScaled = (needsResize && cacheable) ? GetScaledSprite(pImage, spriteRect, outW, outH) : nullptr;
    if (!needsResize || pScaled)
    {
        float rx = 0.0f, ry = 0.0f;
        AlignmentToFactors(alignment, rx, ry);

        double anchorX = screen.x + screen.w * rx;
        double anchorY = screen.y + screen.h * ry;
        rect = BLRect(anchorX - outW * rx, anchorY - outH * ry, outW, outH);

        if (rotateAngle == 0.0f)
        {
            rect.x = Round(rect.x);
            rect.y = Round(rect.y);
        }

        if (pScaled)
        {
            pSource = pScaled;
            srcRect = BLRectI(0, 0, outW, outH);
        }

        useScaleX = scaleX < 0.0f ? -1.0f : 1.0f;
        useScaleY = scaleY < 0.0f ? -1.0f : 1.0f;
    }

    Ctx().save();
    Ctx().set_pattern_quality(BL_PATTERN_QUALITY_NEAREST);
    ApplyTransform(rect, useScaleX, useScaleY, rotateAngle, flipX, flipY, alignment);

    BLRect target(0, 0, rect.w, rect.h);
    bool hasColorTint = color.r() != 255 || color.g() != 255 || color.b() != 255;

    if (hasColorTint)
    {
        if (BLImage* pTinted = GetTintedSprite(pSource, srcRect, color))
        {
            Ctx().blit_image(target, *pTinted);
        }
    }
    else
    {
        if (color.a() != 255)
        {
            Ctx().set_global_alpha(color.a() / 255.0);
        }
        Ctx().blit_image(target, *pSource, srcRect);
    }

    Ctx().restore();
}

BLImage* Renderer2D::GetScaledSprite(BLImage* pSrc, const BLRectI& rect, int32 outW, int32 outH)
{
    if (!pSrc || outW <= 0 || outH <= 0 || rect.w <= 0 || rect.h <= 0)
        return nullptr;

    if (rect.x < 0 || rect.y < 0 || rect.x + rect.w > pSrc->width() || rect.y + rect.h > pSrc->height())
        return nullptr;

    ScaledKey key{pSrc, rect.x, rect.y, rect.w, rect.h, outW, outH};
    auto it = m_scaledCache.find(key);
    if (it != m_scaledCache.end())
        return &it->second;

    usize bytes = outW * outH * 4;
    if (m_scaledBytes + bytes > RENDERER_MAX_CACHE_SCALE)
    {
        ClearScaledCache();
    }

    BLImage out(outW, outH, BL_FORMAT_PRGB32);

    BLImageData srcData;
    BLImageData dstData;
    if (pSrc->get_data(&srcData) != BL_SUCCESS || out.get_data(&dstData) != BL_SUCCESS)
        return nullptr;

    ResampleNearest(srcData, rect, dstData, outW, outH);

    m_scaledBytes += bytes;
    auto inserted = m_scaledCache.emplace(key, std::move(out));
    return &inserted.first->second;
}

BLImage* Renderer2D::GetTintedSprite(BLImage* pSrc, const BLRectI& rect, const BLRgba32& color)
{
    if (!pSrc || rect.w <= 0 || rect.h <= 0)
        return nullptr;

    TintKey key{pSrc, rect.x, rect.y, rect.w, rect.h, color.value};
    auto it = m_tintCache.find(key);
    if (it != m_tintCache.end())
        return &it->second;

    usize bytes = rect.w * rect.h * 4;
    if (m_tintBytes + bytes > RENDERER_MAC_CACHE_TINT)
        ClearTintCache();

    BLImage out(rect.w, rect.h, BL_FORMAT_PRGB32);

    BLImageData srcData;
    BLImageData dstData;
    if (pSrc->get_data(&srcData) != BL_SUCCESS || out.get_data(&dstData) != BL_SUCCESS)
        return nullptr;

    uint32 a = color.a();
    uint32 r = (color.r() * a + 127) / 255;
    uint32 g = (color.g() * a + 127) / 255;
    uint32 b = (color.b() * a + 127) / 255;

    for (int32 y = 0; y < rect.h; ++y)
    {
        uint8* srcRow = (uint8*)srcData.pixel_data + (y + rect.y) * srcData.stride;
        uint8* dstRow = (uint8*)dstData.pixel_data + y * dstData.stride;

        for (int32 x = 0; x < rect.w; ++x)
        {
            uint8* spx = srcRow + (x + rect.x) * 4;
            uint8* dpx = dstRow + x * 4;

            dpx[0] = (uint8)((spx[0] * b + 127) / 255);
            dpx[1] = (uint8)((spx[1] * g + 127) / 255);
            dpx[2] = (uint8)((spx[2] * r + 127) / 255);
            dpx[3] = (uint8)((spx[3] * a + 127) / 255);
        }
    }

    m_tintBytes += bytes;
    auto inserted = m_tintCache.emplace(key, std::move(out));
    return &inserted.first->second;
}

BLImage Renderer2D::TintSprite(BLImage* pSrc, const BLRectI& rect, const BLRgba32& color)
{
    if (BLImage* pTinted = GetTintedSprite(pSrc, rect, color))
        return *pTinted;
    return BLImage();
}

void Renderer2D::DrawGTText(BLFont* pFont, const BLPoint& origin, const string& text, float size)
{
    if (!pFont || text.empty())
        return;

    float oldSize = pFont->size();
    pFont->set_size(size);

    BLFontMetrics fm = pFont->metrics();
    float lineHeight = (float)(fm.ascent + fm.descent + fm.line_gap);

    float x = (float)Round(origin.x);
    float y = (float)Round(origin.y) + float(fm.ascent);

    BLGlyphBuffer gb;
    BLRgba32 color(255, 255, 255);

    const char* src = text.data();
    const char* end = src + text.size();
    while (true)
    {
        const char* nl = (const char*)memchr(src, '\n', end - src);
        const char* lineEnd = nl ? nl : end;

        ProcessTextLine(&Ctx(), pFont, gb, src, lineEnd - src, x, y, color);

        if (!nl)
            break;

        src = nl + 1;
        y += lineHeight;
    }

    pFont->set_size(oldSize);
}

float Renderer2D::GetTextWidth(BLFont* pFont, const string& text, float size)
{
    if (!pFont || text.empty())
        return 0.0f;

    float oldSize = pFont->size();
    pFont->set_size(size);

    BLGlyphBuffer gb;
    BLRgba32 color(255, 255, 255);
    float maxWidth = 0.0f;

    const char* src = text.data();
    const char* end = src + text.size();
    while (true)
    {
        const char* nl = (const char*)memchr(src, '\n', end - src);
        const char* lineEnd = nl ? nl : end;

        maxWidth = Max(maxWidth, ProcessTextLine(nullptr, pFont, gb, src, lineEnd - src, 0.0f, 0.0f, color));

        if (!nl)
            break;

        src = nl + 1;
    }

    pFont->set_size(oldSize);
    return maxWidth;
}

float Renderer2D::GetTextHeight(BLFont* pFont, float size)
{
    if (!pFont)
        return 0.0f;

    float oldSize = pFont->size();
    pFont->set_size(size);

    BLFontMetrics fm = pFont->metrics();
    float textHeight = (float)(fm.ascent + fm.descent + fm.line_gap);

    pFont->set_size(oldSize);
    return textHeight;
}