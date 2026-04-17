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

boost::json::object ColorToJSON(LuxonEngine::Color& color)
{
	boost::json::object vecObject;
	auto array = color.GetColorArray();
	vecObject.emplace("r", array[0]);
	vecObject.emplace("g", array[1]);
	vecObject.emplace("b", array[2]);
	vecObject.emplace("a", array[3]);
	return vecObject;
}

LuxonEngine::Color JSONToColor(const boost::json::object& colorJson)
{
	return LuxonEngine::Color(colorJson.at("r").as_double(), colorJson.at("g").as_double(), colorJson.at("b").as_double(), colorJson.at("a").as_double());
}
