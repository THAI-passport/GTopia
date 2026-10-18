#pragma once

#include "Precompiled.h"
#include "Renderer2D.h"

class WeatherRenderer
{
public:
    WeatherRenderer(Renderer2D* pRenderer);
    ~WeatherRenderer();

public:
    void DrawDefault(bool spring = false);
    void DrawHill(int32 hillIndex, bool spring = false);

    void DrawHarvest();
    void DrawFence(int32 layer);
    void DrawSunset();

private:
    struct Cloud
    {
        float x = 0.0f;
        float y = 0.0f;
        float distance = 1.0f;
        bool flip = false;
    };

private:
    Renderer2D* m_pRenderer;

    float m_width;
    float m_height;
};