#pragma once

#include "Precompiled.h"
#include <blend2d/blend2d.h>

class Texture
{
public:
    Texture() {};
    Texture(BLImage* pImage, int32 frameWidth = 32, int32 frameHeight = 32)
        : m_pImage(pImage), m_frameWidth(frameWidth > 0 ? frameWidth : 32),
          m_frameHeight(frameHeight > 0 ? frameHeight : 32)
    {
    }

public:
    bool IsValid() const { return m_pImage != nullptr; }
    BLImage* GetImage() const { return m_pImage; }

    int32 GetWidth() const { return m_pImage ? m_pImage->width() : 0; }
    int32 GetHeight() const { return m_pImage ? m_pImage->height() : 0; }

    Texture& SetFrameSize(int32 width, int32 height)
    {
        if (width > 0)
            m_frameWidth = width;

        if (height > 0)
            m_frameHeight = height;

        return *this;
    }

    Texture& SetGrid(int32 columns, int32 rows)
    {
        if (m_pImage && columns > 0 && rows > 0)
        {
            SetFrameSize(GetWidth() / columns, GetHeight() / rows);
        }

        return *this;
    }

    int32 GetFrameWidth() const { return m_frameWidth; }
    int32 GetFrameHeight() const { return m_frameHeight; }

    int32 GetFramesPerRow() const { return m_pImage ? GetWidth() / m_frameWidth : 0; }
    int32 GetFramesPerColumn() const { return m_pImage ? GetHeight() / m_frameHeight : 0; }
    int32 GetFrameCount() const { return GetFramesPerRow() * GetFramesPerColumn(); }

    bool HasFrame(int32 frameX, int32 frameY) const
    {
        return m_pImage && frameX >= 0 && frameY >= 0 && (frameX + 1) * m_frameWidth <= GetWidth() &&
               (frameY + 1) * m_frameHeight <= GetHeight();
    }

    BLRectI GetFrameRect(int32 frameX, int32 frameY) const
    {
        return BLRectI(frameX * m_frameWidth, frameY * m_frameHeight, m_frameWidth, m_frameHeight);
    }

    BLRectI GetFrameRectByIndex(int32 index) const
    {
        int32 perRow = GetFramesPerRow();
        if (perRow <= 0)
            return BLRectI(0, 0, 0, 0);
        return GetFrameRect(index % perRow, index / perRow);
    }

private:
    BLImage* m_pImage = nullptr; // owned by ResourceManager
    int32 m_frameWidth = 32;
    int32 m_frameHeight = 32;
};
