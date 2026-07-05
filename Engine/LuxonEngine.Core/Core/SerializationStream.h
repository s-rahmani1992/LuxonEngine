#pragma once

#include "../BasicTypes.h"
#include "../export.h"

#include <boost/uuid/uuid.hpp>
#include <boost/json.hpp>

#include <string>
#include <fstream>

namespace LuxonEngine {
    using GUID = boost::uuids::uuid;
    // Forward declarations
    class Color;
    class Vector2;
    class Vector3;

    class LUXON_CORE_API SerializationStream {
    public:
        // Constructor - initializes empty JSON
        SerializationStream();

        // Load JSON from file
        bool LoadFromFile(const std::string& filePath);

        // Save JSON to file
        bool SaveToFile(const std::string& filePath);

		std::string ToString() const;

        // Get methods
        int GetInt(const std::string& fieldName, int defaultValue = 0);
        Float GetFloat(const std::string& fieldName, Float defaultValue = 0.0f);
        bool GetVector2(const std::string& fieldName, Vector2& vec);
        bool GetVector3(const std::string& fieldName, Vector3& vec);
        bool GetColor(const std::string& fieldName, Color& color);
        bool GetString(const std::string& fieldName, std::string& str);
        bool GetString(const std::string& fieldName, char** str);
        GUID GetGuid(const std::string& fieldName);
        SerializationStream Object(const std::string& fieldName);

        // Set methods
        void SetInt(const std::string& fieldName, int value);
        void SetFloat(const std::string& fieldName, Float value);
        void SetVector2(const std::string& fieldName, const Vector2& vec);
        void SetVector3(const std::string& fieldName, const Vector3& vec);
        void SetColor(const std::string& fieldName, const Color& color);
        void SetString(const std::string& fieldName, const std::string& str);
        void SetGuid(const std::string& fieldName, const GUID& guid);
        void SetObject(const std::string& fieldName, const SerializationStream& object);
    private:
        SerializationStream(boost::json::value& data);
        static void PrettyPrint(std::ostream& os, boost::json::value const& jv, std::string* indent = nullptr);
        boost::json::value m_jsonData;
        std::string m_lastError;

        // Helper method to ensure field exists as object
        void EnsureFieldExists(const std::string& fieldName);
    };
}