#include "SerializationStream.h"

#include "../Core/Color.h"
#include "../Core/Vector2.h"
#include "../Core/Vector3.h"

#include <boost/uuid/string_generator.hpp>
#include <boost/json.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <fstream>
#include <sstream>

#ifdef _DEBUG
#pragma comment(lib, "libboost_json-vc143-mt-gd-x64-1_90.lib")
#pragma comment(lib, "libboost_container-vc143-mt-gd-x64-1_90.lib")
#else
#pragma comment(lib, "libboost_json-vc143-mt-x64-1_90.lib")
#pragma comment(lib, "libboost_container-vc143-mt-x64-1_90.lib")
#endif

namespace LuxonEngine {

    SerializationStream::SerializationStream()
        : m_jsonData(boost::json::object())
    {
    }

    SerializationStream::SerializationStream(boost::json::value& data)
        : m_jsonData(data)
    {
    }

    bool SerializationStream::LoadFromFile(const std::string& filePath)
    {
        std::ifstream file(filePath, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            m_lastError = "Failed to open file: " + filePath;
            return false;
        }

        boost::system::error_code ec;
        m_jsonData = boost::json::parse(file, ec);
        file.close();
        if (ec) {
            m_lastError = "Failed to parse JSON: " + ec.message();
            return false;
        }

        return true;
    }

    bool SerializationStream::SaveToFile(const std::string& filePath)
    {
        std::ofstream file(filePath, std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            m_lastError = "Failed to open file for writing: " + filePath;
            return false;
        }

		PrettyPrint(file, m_jsonData);
        file.close();

        return true;
    }

    std::string SerializationStream::ToString() const
    {
        std::stringstream ss;
        PrettyPrint(ss, m_jsonData);
        return ss.str();
    }

    int SerializationStream::GetInt(const std::string& fieldName, int defaultValue)
    {
        try {
            auto& obj = m_jsonData.as_object();
            if (obj.contains(fieldName)) {
                return static_cast<int>(obj[fieldName].as_int64());
            }
        }
        catch (const std::exception&) {
            // Return default on error
        }
        return defaultValue;
    }

    Float SerializationStream::GetFloat(const std::string& fieldName, Float defaultValue)
    {
        try {
            auto& obj = m_jsonData.as_object();
            if (obj.contains(fieldName)) {
                return static_cast<Float>(obj[fieldName].as_double());
            }
        }
        catch (const std::exception&) {
            // Return default on error
        }
        return defaultValue;
    }

    bool SerializationStream::GetVector2(const std::string& fieldName, Vector2& vec)
    {
        try {
            auto& obj = m_jsonData.as_object();
            if (obj.contains(fieldName)) {
                auto& vecObj = obj[fieldName].as_object();
                Float x = static_cast<Float>(vecObj["x"].as_double());
                Float y = static_cast<Float>(vecObj["y"].as_double());
                vec = Vector2(x, y);
                return true;
            }
        }
        catch (const std::exception&) {
            // Return default on error
        }
        vec = Vector2();
        return false;
    }

    bool SerializationStream::GetVector3(const std::string& fieldName, Vector3& vec)
    {
        try {
            auto& obj = m_jsonData.as_object();
            if (obj.contains(fieldName)) {
                auto& vecObj = obj[fieldName].as_object();
                Float x = static_cast<Float>(vecObj["x"].as_double());
                Float y = static_cast<Float>(vecObj["y"].as_double());
                Float z = static_cast<Float>(vecObj["z"].as_double());
                vec = Vector3(x, y, z);
                return true;
            }
        }
        catch (const std::exception&) {
            // Return default on error
        }
        vec = Vector3();
        return false;
    }

    bool SerializationStream::GetColor(const std::string& fieldName, Color& color)
    {
        try {
            auto& obj = m_jsonData.as_object();
            if (obj.contains(fieldName)) {
                auto& colorObj = obj[fieldName].as_object();
                Float r = static_cast<Float>(colorObj["r"].as_double());
                Float g = static_cast<Float>(colorObj["g"].as_double());
                Float b = static_cast<Float>(colorObj["b"].as_double());
                Float a = static_cast<Float>(colorObj["a"].as_double());
                color = Color(r, g, b, a);
                return true;
            }
        }
        catch (const std::exception&) {
            // Return default on error
        }
        color = Color();
    }

    bool SerializationStream::GetString(const std::string& fieldName, std::string& str)
    {
        try {
            auto& obj = m_jsonData.as_object();
            if (obj.contains(fieldName)) {
                str = obj[fieldName].as_string();
                return true;
            }
        }
        catch (const std::exception&) {
            // Return default on error
        }
        str = std::string();
        return false;
    }

    bool SerializationStream::GetString(const std::string& fieldName, char** str)
    {
        try {
            auto& obj = m_jsonData.as_object();
            if (obj.contains(fieldName)) {
                *str = const_cast<char*>(obj[fieldName].as_string().data());
                return true;
            }
        }
        catch (const std::exception&) {
            // Return default on error
        }
        *str = nullptr;
        return false;
    }

    GUID SerializationStream::GetGuid(const std::string& fieldName)
    {
        try {
            auto& obj = m_jsonData.as_object();
            if (obj.contains(fieldName)) {
                std::string guidStr = std::string(obj[fieldName].as_string());
                boost::uuids::string_generator gen;
                return gen(guidStr);
            }
        }
        catch (const std::exception&) {
            // Return nil UUID on error
        }
        return boost::uuids::uuid();
    }

    SerializationStream SerializationStream::Object(const std::string& fieldName)
    {
        try {
            auto& obj = m_jsonData.as_object();
            if (obj.contains(fieldName)) {
                return SerializationStream(obj[fieldName]);
            }
        }
        catch (const std::exception&) {
            // Return nil UUID on error
        }
        return SerializationStream();
    }

    void SerializationStream::SetInt(const std::string& fieldName, int value)
    {
        EnsureFieldExists(fieldName);
        m_jsonData.as_object()[fieldName] = value;
    }

    void SerializationStream::SetFloat(const std::string& fieldName, Float value)
    {
        EnsureFieldExists(fieldName);
        m_jsonData.as_object()[fieldName] = static_cast<double>(value);
    }

    void SerializationStream::SetVector2(const std::string& fieldName, const Vector2& vec)
    {
        EnsureFieldExists(fieldName);
        auto& obj = m_jsonData.as_object();
        boost::json::object vecObj;
        vecObj["x"] = static_cast<double>(vec.x);
        vecObj["y"] = static_cast<double>(vec.y);
        obj[fieldName] = vecObj;
    }

    void SerializationStream::SetVector3(const std::string& fieldName, const Vector3& vec)
    {
        EnsureFieldExists(fieldName);
        auto& obj = m_jsonData.as_object();
        boost::json::object vecObj;
        vecObj["x"] = static_cast<double>(vec.x);
        vecObj["y"] = static_cast<double>(vec.y);
        vecObj["z"] = static_cast<double>(vec.z);
        obj[fieldName] = vecObj;
    }

    void SerializationStream::SetColor(const std::string& fieldName, const Color& color)
    {
        EnsureFieldExists(fieldName);
        auto& obj = m_jsonData.as_object();
        boost::json::object colorObj;
        Float* colorArray = const_cast<Color&>(color).GetColorArray();
        colorObj["r"] = static_cast<double>(colorArray[0]);
        colorObj["g"] = static_cast<double>(colorArray[1]);
        colorObj["b"] = static_cast<double>(colorArray[2]);
        colorObj["a"] = static_cast<double>(colorArray[3]);
        obj[fieldName] = colorObj;
    }

    void SerializationStream::SetString(const std::string& fieldName, const std::string& str)
    {
        EnsureFieldExists(fieldName);
        m_jsonData.as_object()[fieldName] = str;
    }

    void SerializationStream::SetGuid(const std::string& fieldName, const GUID& guid)
    {
        EnsureFieldExists(fieldName);
        m_jsonData.as_object()[fieldName] = boost::uuids::to_string(guid);
    }

    void SerializationStream::SetObject(const std::string& fieldName, const SerializationStream& object)
    {
        EnsureFieldExists(fieldName);
        m_jsonData.as_object()[fieldName] = object.m_jsonData;
    }

    void SerializationStream::EnsureFieldExists(const std::string& fieldName)
    {
        auto& obj = m_jsonData.as_object();
        if (!obj.contains(fieldName)) {
            obj[fieldName] = boost::json::value();
        }
    }

    void SerializationStream::PrettyPrint(std::ostream& os, boost::json::value const& jv, std::string* indent)
    {
        namespace json = boost::json;
        std::string indent_;
        if (!indent)
            indent = &indent_;
        switch (jv.kind())
        {
        case json::kind::object:
        {
            os << "{\n";
            indent->append(4, ' ');
            auto const& obj = jv.get_object();
            if (!obj.empty())
            {
                auto it = obj.begin();
                for (;;)
                {
                    os << *indent << json::serialize(it->key()) << " : ";
                    PrettyPrint(os, it->value(), indent);
                    if (++it == obj.end())
                        break;
                    os << ",\n";
                }
            }
            os << "\n";
            indent->resize(indent->size() - 4);
            os << *indent << "}";
            break;
        }

        case json::kind::array:
        {
            os << "[\n";
            indent->append(4, ' ');
            auto const& arr = jv.get_array();
            if (!arr.empty())
            {
                auto it = arr.begin();
                for (;;)
                {
                    os << *indent;
                    PrettyPrint(os, *it, indent);
                    if (++it == arr.end())
                        break;
                    os << ",\n";
                }
            }
            os << "\n";
            indent->resize(indent->size() - 4);
            os << *indent << "]";
            break;
        }

        case json::kind::string:
        {
            os << json::serialize(jv.get_string());
            break;
        }

        case json::kind::uint64:
            os << jv.get_uint64();
            break;

        case json::kind::int64:
            os << jv.get_int64();
            break;

        case json::kind::double_:
            os << jv.get_double();
            break;

        case json::kind::bool_:
            if (jv.get_bool())
                os << "true";
            else
                os << "false";
            break;

        case json::kind::null:
            os << "null";
            break;
        }

        if (indent->empty())
            os << "\n";
    }

    void LuxonEngine::SerializationStream::Clear() {
        m_jsonData = boost::json::object();
    }
}