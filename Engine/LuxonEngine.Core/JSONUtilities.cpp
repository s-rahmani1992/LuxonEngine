#include "JSONUtilities.h"

boost::json::object Vector3ToJSON(const LuxonEngine::Vector3& vec3)
{
	boost::json::object vecObject;
	vecObject.emplace("x", vec3.x);
	vecObject.emplace("y", vec3.y);
	vecObject.emplace("z", vec3.z);
	return vecObject;
}

LuxonEngine::Vector3 JSONToVector3(const boost::json::object& vec3Json)
{
	return LuxonEngine::Vector3(vec3Json.at("x").as_double(), vec3Json.at("y").as_double(), vec3Json.at("z").as_double());
}
