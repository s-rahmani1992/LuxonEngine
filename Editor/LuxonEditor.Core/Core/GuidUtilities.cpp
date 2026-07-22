#include "GuidUtilities.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

LuxonEditor::GUID LuxonEditor::GuidGenerator::GenerateGUID()
{
    static boost::uuids::random_generator generator;
    return generator();
}

LuxonEditor::GUID LuxonEditor::GuidGenerator::GenerateGUIDFromString(const std::string& str)
{
    boost::uuids::string_generator gen;
    return gen(str);
}

std::string LuxonEditor::GuidGenerator::GUIDToString(const GUID& guid)
{
    return boost::uuids::to_string(guid);
}
