#pragma once
#include "BasicTypes.h"
#include "Core/Vector3.h"
#include <boost/json.hpp>
#include <Core/Color.h>

boost::json::object Vector3ToJSON(const LuxonEngine::Vector3& vec3);

LuxonEngine::Vector3 JSONToVector3(const boost::json::object& vec3Json);

boost::json::object ColorToJSON(LuxonEngine::Color& color);

LuxonEngine::Color JSONToColor(const boost::json::object& colorJson);