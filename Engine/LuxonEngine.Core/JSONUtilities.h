#pragma once
#include "BasicTypes.h"
#include "Core/Vector3.h"
#include <boost/json.hpp>

boost::json::object Vector3ToJSON(const LuxonEngine::Vector3& vec3);

LuxonEngine::Vector3 JSONToVector3(const boost::json::object& vec3Json);