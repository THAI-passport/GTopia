#ifdef ENABLE_LUA_SCRIPTING

#include "../LuaManager.h"
#include "Math/Random.h"
#include "Math/Rect.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/WeightRand.h"
#include "Utils/Timer.h"

void RegisterMathBinding()
{
    LuaManager* pLuaManager = GetLuaManager();

    auto color = pLuaManager->BindClass<Color>("Color");
    color.DefMember("r", &Color::r)
        .DefMember("g", &Color::g)
        .DefMember("b", &Color::b)
        .DefMember("a", &Color::a)
        .DefMethod("getAsUINT", &Color::GetAsUINT)
        .DefMethod("getAsUINTSwap", &Color::GetAsUINTSwap)
        .DefMethod("setUINT", &Color::SetUINT)
        .DefMethod("setUINTSwap", &Color::SetUINTSwap);

    auto vec2i = pLuaManager->BindClass<Vector2Int>("Vector2Int");
    vec2i.DefMember("x", &Vector2Int::x).DefMember("y", &Vector2Int::y);

    auto vec2f = pLuaManager->BindClass<Vector2Float>("Vector2Float");
    vec2f.DefMember("x", &Vector2Float::x).DefMember("y", &Vector2Float::y);

    auto vec3i = pLuaManager->BindClass<Vector3Int>("Vector3Int");
    vec3i.DefMember("x", &Vector3Int::x).DefMember("y", &Vector3Int::y).DefMember("z", &Vector3Int::z);

    auto vec3f = pLuaManager->BindClass<Vector3Float>("Vector3Float");
    vec3f.DefMember("x", &Vector3Float::x).DefMember("y", &Vector3Float::y).DefMember("z", &Vector3Float::z);

    auto recti = pLuaManager->BindClass<RectInt>("RectInt");
    recti.DefMember("left", &RectInt::left)
        .DefMember("top", &RectInt::top)
        .DefMember("right", &RectInt::right)
        .DefMember("bottom", &RectInt::bottom)
        .DefMethod("setWidth", &RectInt::SetWidth)
        .DefMethod("setHeight", &RectInt::SetHeight)
        .DefMethod("inFlate", &RectInt::InFlate)
        .DefMethod("width", &RectInt::Width)
        .DefMethod("height", &RectInt::Height)
        .DefMethod("isInside", &RectInt::IsInside)
        .DefMethod("intersects", &RectInt::Intersects);

    auto rectf = pLuaManager->BindClass<RectFloat>("RectFloat");
    rectf.DefMember("left", &RectFloat::left)
        .DefMember("top", &RectFloat::top)
        .DefMember("right", &RectFloat::right)
        .DefMember("bottom", &RectFloat::bottom)
        .DefMethod("setWidth", &RectFloat::SetWidth)
        .DefMethod("setHeight", &RectFloat::SetHeight)
        .DefMethod("inFlate", &RectFloat::InFlate)
        .DefMethod("width", &RectFloat::Width)
        .DefMethod("height", &RectFloat::Height)
        .DefMethod("isInside", &RectFloat::IsInside)
        .DefMethod("intersects", &RectFloat::Intersects);

    auto weightRand = pLuaManager->BindClass<WeightRand>("WeightRand");
    weightRand.DefMethod("add", &WeightRand::Add)
        .DefMethod("getTotalWeight", &WeightRand::GetTotalWeight)
        .DefMethod("roll", &WeightRand::RollPair);

    pLuaManager->DefFunction("RandomRangeInt", &RandomRangeInt);
    pLuaManager->DefFunction("RandomRangeFloat", &RandomRangeFloat);

    auto timer = pLuaManager->BindClass<Timer>("Timer");
    timer.DefMethod("getRemainingTime", &Timer::GetRemainingTime)
        .DefMethodDefault("getElapsedTime", &Timer::GetElapsedTime, (bool)false)
        .DefMethod("getStartTime", &Timer::GetStartTime);

    pLuaManager->DefFunction("getSystemTime", &Time::GetSystemTime);
}

#endif