#include "GuidUtilities.h"
#include <boost/uuid/uuid_generators.hpp>

LuxonEditor::GUID LuxonEditor::GuidGenerator::GenerateGUID()
{
    static boost::uuids::random_generator generator;
    return generator();
}
