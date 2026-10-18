#pragma once

#ifdef ENABLE_LUA_SCRIPTING

/**
 * Thanks to https://github.com/gengyong/luaaa - It really helped with functions
 * also the stackoverflow guys
 */

#include "Precompiled.h"
#include <string>
#include <type_traits> // shitty bitch
#include <utility>

extern "C"
{
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

template <typename T> struct LuaTypeRegistry
{
    static inline string className;
};

// holy shit im going crazy
// fucking lua support

template <typename T> struct LuaArgExtractor
{
    using Plain = std::remove_cv_t<std::remove_reference_t<T>>;

    // fix the unneccasry pushs
    // i mean already doing defclass so using that instead of pushing it in here
    static Plain Get(lua_State* L, int offset)
    {
        if constexpr (std::is_same_v<Plain, string>)
        {
            return LuaArgExtractor<string>::Get(L, offset);
        }
        else
        {
            Plain** ud = (Plain**)(lua_touserdata(L, offset));
            if (!ud || !*ud)
                luaL_error(L, "Null or invalid object reference");

            return **ud;
        }
    }
};

template <> struct LuaArgExtractor<int8>
{
    static int8 Get(lua_State* L, int offset) { return (int8)luaL_checkinteger(L, offset); }
};

template <> struct LuaArgExtractor<uint8>
{
    static uint8 Get(lua_State* L, int offset) { return (uint8)luaL_checkinteger(L, offset); }
};

template <> struct LuaArgExtractor<int16>
{
    static int16 Get(lua_State* L, int offset) { return (int16)luaL_checkinteger(L, offset); }
};

template <> struct LuaArgExtractor<uint16>
{
    static uint16 Get(lua_State* L, int offset) { return (uint16)luaL_checkinteger(L, offset); }
};

template <> struct LuaArgExtractor<int32>
{
    static int32 Get(lua_State* L, int offset) { return (int32)luaL_checkinteger(L, offset); }
};

template <> struct LuaArgExtractor<uint32>
{
    static uint32 Get(lua_State* L, int offset) { return (uint32)luaL_checkinteger(L, offset); }
};

template <> struct LuaArgExtractor<int64>
{
    static int64 Get(lua_State* L, int offset) { return (int64)luaL_checkinteger(L, offset); }
};

template <> struct LuaArgExtractor<uint64>
{
    static uint64 Get(lua_State* L, int offset) { return (uint64)luaL_checkinteger(L, offset); }
};

template <> struct LuaArgExtractor<float>
{
    static float Get(lua_State* L, int offset) { return (float)luaL_checknumber(L, offset); }
};

template <> struct LuaArgExtractor<double>
{
    static double Get(lua_State* L, int offset) { return (double)luaL_checknumber(L, offset); }
};

template <> struct LuaArgExtractor<bool>
{
    static bool Get(lua_State* L, int offset) { return lua_toboolean(L, offset) != 0; }
};

template <> struct LuaArgExtractor<string>
{
    static string Get(lua_State* L, int offset)
    {
        if (offset > lua_gettop(L) || lua_isnoneornil(L, offset))
            return "";

        usize len = 0;
        const char* str = luaL_checklstring(L, offset, &len);

        return string(str, len);
    }
};

template <> struct LuaArgExtractor<const char*>
{
    static const char* Get(lua_State* L, int offset)
    {
        if (offset > lua_gettop(L) || lua_isnoneornil(L, offset))
            return "";

        return luaL_checkstring(L, offset);
    }
};

template <typename T> struct LuaArgExtractor<T*>
{
    static T* Get(lua_State* L, int offset)
    {
        T** ud = (T**)lua_touserdata(L, offset);
        if (!ud)
            return nullptr;

        return *ud;
    }
};

inline void PushArg(lua_State* L, int8 val)
{
    lua_pushinteger(L, val);
}

inline void PushArg(lua_State* L, uint8 val)
{
    lua_pushinteger(L, (lua_Integer)val);
}

inline void PushArg(lua_State* L, int16 val)
{
    lua_pushinteger(L, val);
}

inline void PushArg(lua_State* L, uint16 val)
{
    lua_pushinteger(L, (lua_Integer)val);
}

inline void PushArg(lua_State* L, int32 val)
{
    lua_pushinteger(L, val);
}

inline void PushArg(lua_State* L, uint32 val)
{
    lua_pushinteger(L, (lua_Integer)val);
}

inline void PushArg(lua_State* L, int64 val)
{
    lua_pushinteger(L, (lua_Integer)val);
}

inline void PushArg(lua_State* L, uint64 val)
{
    lua_pushinteger(L, (lua_Integer)val);
}

inline void PushArg(lua_State* L, float val)
{
    lua_pushnumber(L, (lua_Number)val);
}

inline void PushArg(lua_State* L, double val)
{
    lua_pushnumber(L, (lua_Number)val);
}

inline void PushArg(lua_State* L, bool val)
{
    lua_pushboolean(L, val ? 1 : 0);
}

inline void PushArg(lua_State* L, const char* val)
{
    if (val)
        lua_pushstring(L, val);
    else
        lua_pushnil(L);
}

inline void PushArg(lua_State* L, const string& val)
{
    lua_pushlstring(L, val.data(), val.size());
}

template <typename T> inline void PushArg(lua_State* L, T* val)
{
    if (!val)
    {
        lua_pushnil(L);
        return;
    }

    const string& className = LuaTypeRegistry<T>::className;
    if (!className.empty())
    {
        T** ud = (T**)lua_newuserdata(L, sizeof(T*));
        *ud = val;

        luaL_getmetatable(L, className.c_str());
        lua_setmetatable(L, -2);
    }
    else
    {
        lua_pushlightuserdata(L, (void*)val);
    }
}

// yea... i guess this fucntion explains why i love people who using c++ instead of
// lua lua lua!!!!!!!! no need to modify source LUAAAAAAAAAAAAAAAAAAA
template <typename T>
inline typename std::enable_if_t<std::is_class_v<std::decay_t<T>> && !std::is_same_v<std::decay_t<T>, string>, void>
PushArg(lua_State* L, const T& val)
{
    typedef std::decay_t<T> PlainT;
    const string& className = LuaTypeRegistry<PlainT>::className;
    if (!className.empty())
    {
        PlainT** ud = (PlainT**)lua_newuserdata(L, sizeof(PlainT*));
        *ud = (PlainT*)(&val);

        luaL_getmetatable(L, className.c_str());
        lua_setmetatable(L, -2);
    }
    else
    {
        lua_pushnil(L);
    }
}

// send the return type
template <typename R> struct ReturnInvoker
{
    template <typename Instance, typename Method, typename... Args>
    static int CallMember(lua_State* L, Instance* inst, Method method, Args&&... args)
    {
        PushArg(L, (inst->*method)(std::forward<Args>(args)...));
        return 1;
    }

    template <typename Instance, typename Func, typename... Args>
    static int CallFuncMethod(lua_State* L, Instance* inst, Func func, Args&&... args)
    {
        PushArg(L, func(inst, std::forward<Args>(args)...));
        return 1;
    }

    template <typename Func, typename... Args> static int Call(lua_State* L, Func func, Args&&... args)
    {
        PushArg(L, func(std::forward<Args>(args)...));
        return 1;
    }
};

// test void
// just forward it, forward fits tho wtf is move lol
template <> struct ReturnInvoker<void>
{
    template <typename Instance, typename Method, typename... Args>
    static int CallMember(lua_State* L, Instance* inst, Method method, Args&&... args)
    {
        (inst->*method)(std::forward<Args>(args)...);
        return 0;
    }

    template <typename Instance, typename Func, typename... Args>
    static int CallFuncMethod(lua_State* L, Instance* inst, Func func, Args&&... args)
    {
        func(inst, std::forward<Args>(args)...);
        return 0;
    }

    template <typename Func, typename... Args> static int Call(lua_State* L, Func func, Args&&... args)
    {
        func(std::forward<Args>(args)...);
        return 0;
    }
};

template <typename T> static T LuaGetOrDefault(lua_State* L, int offset, const T& defaultValue)
{
    if (offset > lua_gettop(L) || lua_isnoneornil(L, offset))
        return defaultValue;

    return LuaArgExtractor<T>::Get(L, offset);
}

// hollllllllly epic pull for index_sequence
// thanks the guy who answered that question on forum :*
// also im happy i've aimed c++17 lol
template <typename Ret, typename... Args, usize... Index>
static int CallLuaFunction(lua_State* L, Ret (*func)(Args...), std::index_sequence<Index...>)
{
    return ReturnInvoker<Ret>::Call(L, func, LuaArgExtractor<Args>::Get(L, (int32)Index + 1)...);
}

template <typename Ret, typename... Args> inline void DefFunction(lua_State* L, const char* name, Ret (*func)(Args...))
{
    typedef Ret (*FuncPtr)(Args...);

    FuncPtr* ud = (FuncPtr*)lua_newuserdata(L, sizeof(FuncPtr));
    *ud = func;

    lua_pushcclosure(
        L,
        [](lua_State* L) -> int
        {
            FuncPtr* fn = (FuncPtr*)lua_touserdata(L, lua_upvalueindex(1));
            if (!fn || !*fn)
                return luaL_error(L, "Null or invalid function pointer");

            return CallLuaFunction(L, *fn, std::index_sequence_for<Args...>{});
        },
        1);

    lua_setglobal(L, name);
}

// check methods, (script crash) probably push shit
template <typename T> class LuaClass
{
public:
    LuaClass(lua_State* L, const char* className) : m_L(L), m_className(className)
    {
        LuaTypeRegistry<T>::className = className;

        luaL_newmetatable(m_L, m_className.c_str());

        lua_pushvalue(m_L, -1);
        lua_setfield(m_L, -2, "__index");
        lua_pop(m_L, 1);
    }

    template <typename M> LuaClass& DefMember(const char* name, M T::* member)
    {
        typedef M T::* MemberPtr;
        luaL_getmetatable(m_L, m_className.c_str());

        MemberPtr* ud = (MemberPtr*)lua_newuserdata(m_L, sizeof(MemberPtr));
        *ud = member;

        lua_pushcclosure(
            m_L,
            [](lua_State* L) -> int
            {
                typedef M T::* MemberPtr;
                MemberPtr* memPtr = (MemberPtr*)lua_touserdata(L, lua_upvalueindex(1));

                T* instance = LuaArgExtractor<T*>::Get(L, 1);
                if (!instance)
                    return luaL_error(L, "Null or invalid object reference");

                if (lua_gettop(L) >= 2 && !lua_isnoneornil(L, 2))
                {
                    instance->*(*memPtr) = LuaArgExtractor<M>::Get(L, 2);
                }

                if constexpr (std::is_class_v<M> && !std::is_same_v<M, string>)
                {
                    PushArg(L, &(instance->*(*memPtr)));
                }
                else
                {
                    // debug, err here (something happening on string lol)
                    PushArg(L, instance->*(*memPtr));
                }

                return 1;
            },
            1);

        lua_setfield(m_L, -2, name);
        lua_pop(m_L, 1);
        return *this;
    }

    template <typename Ret, typename... Args> LuaClass& DefMethod(const char* name, Ret (*func)(T*, Args...))
    {
        using FuncPtr = Ret (*)(T*, Args...);

        luaL_getmetatable(m_L, m_className.c_str());

        FuncPtr* ud = (FuncPtr*)(lua_newuserdata(m_L, sizeof(FuncPtr)));
        *ud = func;

        lua_pushcclosure(
            m_L,
            [](lua_State* L) -> int
            {
                FuncPtr* fn = (FuncPtr*)lua_touserdata(L, lua_upvalueindex(1));
                if (!fn || !*fn)
                    return luaL_error(L, "Null or invalid function pointer");

                T* instance = LuaArgExtractor<T*>::Get(L, 1);
                if (!instance)
                    return luaL_error(L, "Null or invalid object reference");

                return CallFunctionMethod(L, instance, *fn, std::index_sequence_for<Args...>{});
            },
            1);

        lua_setfield(m_L, -2, name);
        lua_pop(m_L, 1);

        return *this;
    }

    template <typename Ret, typename... Args> LuaClass& DefMethod(const char* name, Ret (T::*method)(Args...))
    {
        return DefMethodImpl<Ret, Args...>(name, method);
    }

    template <typename Ret, typename... Args> LuaClass& DefMethod(const char* name, Ret (T::*method)(Args...) const)
    {
        return DefMethodImpl<Ret, Args...>(name, method);
    }

    template <typename C, typename Ret, typename... Args>
    LuaClass& DefMethod(const char* name, Ret (C::*method)(Args...))
    {
        return DefMethodImpl<Ret, Args...>(name, (Ret (T::*)(Args...))method);
    }

    template <typename C, typename Ret, typename... Args>
    LuaClass& DefMethod(const char* name, Ret (C::*method)(Args...) const)
    {
        return DefMethodImpl<Ret, Args...>(name, (Ret (T::*)(Args...) const)method);
    }

    template <typename Ret, typename... Args, typename... Defs>
    LuaClass& DefMethodDefault(const char* name, Ret (T::*method)(Args...), Defs... defaults)
    {
        return DefMethodDefaultImpl<Ret, Args...>(name, method, defaults...);
    }

    template <typename Ret, typename... Args, typename... Defs>
    LuaClass& DefMethodDefault(const char* name, Ret (T::*method)(Args...) const, Defs... defaults)
    {
        return DefMethodDefaultImpl<Ret, Args...>(name, method, defaults...);
    }

private:
    template <typename Ret, typename... Args, typename MethodPtr, usize... Index>
    static int CallMemberMethod(lua_State* L, T* instance, MethodPtr method, std::index_sequence<Index...>)
    {
        return ReturnInvoker<Ret>::CallMember(L, instance, method, LuaArgExtractor<Args>::Get(L, (int32)Index + 2)...);
    }

    template <typename Ret, typename... Args, usize... Index>
    static int CallFunctionMethod(lua_State* L, T* instance, Ret (*func)(T*, Args...), std::index_sequence<Index...>)
    {
        return ReturnInvoker<Ret>::CallFuncMethod(L, instance, func,
                                                  LuaArgExtractor<Args>::Get(L, (int32)Index + 2)...);
    }

    template <usize DefaultCount, typename Ret, typename MethodPtr, typename... Args, usize... Index>
    static int CallDefaultMemberMethod(lua_State* L, T* instance, MethodPtr method, std::index_sequence<Index...>)
    {
        // lets just throw this to compiler

        constexpr usize ArgCount = sizeof...(Args);
        constexpr usize RequiredCount = ArgCount - DefaultCount;

        return ReturnInvoker<Ret>::CallMember(L, instance, method,
                                              GetDefaultMethodArg<Args, Index, RequiredCount>(L)...);
    }

    template <typename Ret, typename... Args, typename MethodPtr>
    LuaClass& DefMethodImpl(const char* name, MethodPtr method)
    {
        luaL_getmetatable(m_L, m_className.c_str());

        MethodPtr* ud = (MethodPtr*)lua_newuserdata(m_L, sizeof(MethodPtr));
        *ud = method;

        lua_pushcclosure(
            m_L,
            [](lua_State* L) -> int
            {
                MethodPtr* methodPtr = (MethodPtr*)lua_touserdata(L, lua_upvalueindex(1));

                T* instance = LuaArgExtractor<T*>::Get(L, 1);
                if (!instance)
                    return luaL_error(L, "Null or invalid object reference");

                return CallMemberMethod<Ret, Args...>(L, instance, *methodPtr, std::index_sequence_for<Args...>{});
            },
            1);

        lua_setfield(m_L, -2, name);
        lua_pop(m_L, 1);

        return *this;
    }

    template <typename Arg, usize Index, usize RequiredCount> static std::decay_t<Arg> GetDefaultMethodArg(lua_State* L)
    {
        int32 luaIndex = (int32)Index + 2;

        if (Index < RequiredCount)
            return LuaArgExtractor<Arg>::Get(L, luaIndex);

        if (lua_gettop(L) >= luaIndex && !lua_isnoneornil(L, luaIndex))
            return LuaArgExtractor<Arg>::Get(L, luaIndex);

        int32 defaultIndex = (int32)(Index - RequiredCount) + 2;

        return LuaArgExtractor<Arg>::Get(L, lua_upvalueindex(defaultIndex));
    }

    // bare minimum :P (gonna looooose my mind but ok)
    // is it really ok :/
    // kinda dont understand why gt people want lua scripting -_-
    // nvm... fitur is fitur
    template <typename Ret, typename... Args, typename MethodPtr, typename... Defs>
    LuaClass& DefMethodDefaultImpl(const char* name, MethodPtr method, Defs... defaults)
    {
        luaL_getmetatable(m_L, m_className.c_str());

        MethodPtr* ud = (MethodPtr*)lua_newuserdata(m_L, sizeof(MethodPtr));
        *ud = method;

        (PushArg(m_L, defaults), ...);

        lua_pushcclosure(
            m_L,
            [](lua_State* L) -> int
            {
                MethodPtr* methodPtr = (MethodPtr*)lua_touserdata(L, lua_upvalueindex(1));
                if (!methodPtr)
                    return luaL_error(L, "Null or invalid method pointer");

                T* instance = LuaArgExtractor<T*>::Get(L, 1);
                if (!instance)
                    return luaL_error(L, "Null or invalid object reference");

                return CallDefaultMemberMethod<sizeof...(Defs), Ret, MethodPtr, Args...>(
                    L, instance, *methodPtr, std::index_sequence_for<Args...>{});
            },
            (int32)sizeof...(Defs) + 1);

        lua_setfield(m_L, -2, name);
        lua_pop(m_L, 1);
        return *this;
    }

private:
    lua_State* m_L;
    string m_className;
};

#endif