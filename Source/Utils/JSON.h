#pragma once

#include "../Math/Math.h"
#include "../Precompiled.h"
#include "StringUtils.h"
#include <stdexcept>

// its all distraction

class JSON
{
public:
    JSON() : m_type(Type::NONE) {}
    JSON(const char* val) : m_type(Type::STRING) { m_value.str = string(val); }
    JSON(const string& val) : m_type(Type::STRING) { m_value.str = val; }
    JSON(bool val) : m_type(Type::BOOL) { m_value.b_0 = val; }

    template <typename T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int32> = 0>
    JSON(T val) : m_type(Type::INT)
    {
        m_value.i_0 = (int64)val;
    }

    template <typename T, std::enable_if_t<std::is_floating_point_v<T>, int32> = 0> JSON(T val) : m_type(Type::FLOAT)
    {
        m_value.d_0 = (double)val;
    }

public:
    JSON& operator[](const string& key)
    {
        if (m_type == Type::NONE)
            m_type = Type::OBJECT;
        else if (m_type != Type::OBJECT)
            throw std::logic_error("not an object");

        for (int32 i = 0; i < m_keys.size(); ++i)
        {
            if (m_keys[i] == key)
                return m_items[i];
        }

        m_keys.push_back(key);
        return m_items.emplace_back();
    }

    JSON& push_back(JSON val)
    {
        if (m_type == Type::NONE)
            m_type = Type::ARRAY;
        else if (m_type != Type::ARRAY)
            throw std::logic_error("not an array");

        return m_items.emplace_back(std::move(val));
    }

    string Get(int32 indent = 2, int32 depth = 0)
    {
        string out;
        Write(out, indent, depth);
        return out;
    }

private:
    string Escape(const string& str)
    {
        string escape;
        escape.reserve(str.size());
        for (char c : str)
        {
            switch (c)
            {
                case '\"':
                    escape += "\\\"";
                    break;
                case '\\':
                    escape += "\\\\";
                    break;
                case '\b':
                    escape += "\\b";
                    break;
                case '\f':
                    escape += "\\f";
                    break;
                case '\n':
                    escape += "\\n";
                    break;
                case '\r':
                    escape += "\\r";
                    break;
                case '\t':
                    escape += "\\t";
                    break;
                default:
                    if ((uint8)c < 0x20)
                    {
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\u%04x", (unsigned)(uint8)c);
                        escape += buf;
                    }
                    else
                        escape += c;
                    break;
            }
        }
        return escape;
    }

    void NewLine(string& out, int32 indent, int32 depth)
    {
        if (indent == 0)
            return;

        out += '\n';
        out.append(indent * depth, ' ');
    }

    void Write(string& out, int32 indent, int32 depth)
    {
        switch (m_type)
        {
            case Type::INT:
                out += ToString(m_value.i_0);
                break;
            case Type::FLOAT:
            {
                if (!IsFinite(m_value.d_0))
                {
                    out += "null";
                    return;
                }
                out += ToString(m_value.d_0);
                break;
            }
            case Type::STRING:
                out += "\"" + Escape(m_value.str) + "\"";
                break;
            case Type::BOOL:
                out += (m_value.b_0 ? "true" : "false");
                break;
            case Type::OBJECT:
            {
                if (m_items.empty())
                {
                    out += "{}";
                    break;
                }

                out += '{';
                for (int32 i = 0; i < m_items.size(); ++i)
                {
                    if (i > 0)
                        out += ',';
                    NewLine(out, indent, depth + 1);
                    out += "\"" + Escape(m_keys[i]) + "\"";
                    out += (indent > 0 ? ": " : ":");
                    m_items[i].Write(out, indent, depth + 1);
                }
                NewLine(out, indent, depth);
                out += '}';
                break;
            }
            case Type::ARRAY:
            {
                if (m_items.empty())
                {
                    out += "[]";
                    break;
                }

                out += '[';
                for (int32 i = 0; i < m_items.size(); ++i)
                {
                    if (i > 0)
                        out += ',';
                    NewLine(out, indent, depth + 1);
                    m_items[i].Write(out, indent, depth + 1);
                }
                NewLine(out, indent, depth);
                out += ']';
                break;
            }
        }
    }

private:
    struct JSONValue
    {
        union
        {
            int64 i_0;
            double d_0;
            bool b_0;
        };

        string str;
    };

    enum class Type
    {
        NONE,
        INT,
        STRING,
        BOOL,
        FLOAT,
        ARRAY,
        OBJECT
    };

    Type m_type;
    JSONValue m_value;

    // key is parallel with items
    // tested with unordered_map too, i dont think we even need 100+ keys so we may keep
    // 2 vector trick
    std::vector<string> m_keys;
    std::vector<JSON> m_items;
};