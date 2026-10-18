#pragma once

#include "Utils/StringUtils.h"

#ifdef ENABLE_LUA_SCRIPTING
#include "LuaManager.h"

#define LUA_CALLBACK_HANDLED(eventName, ...)                                                                           \
    GetLuaManager()->InvokeCallbackHandled(CompileTimeHashString(eventName), ##__VA_ARGS__)

#define LUA_CALLBACK_NON_HANDLED(eventName, ...)                                                                       \
    GetLuaManager()->InvokeCallback(CompileTimeHashString(eventName), ##__VA_ARGS__)
#else
#define LUA_CALLBACK_HANDLED(eventName, ...) (false)
#define LUA_CALLBACK_NON_HANDLED(eventName, ...) ((void)0)
#endif