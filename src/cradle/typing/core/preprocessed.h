#ifndef CRADLE_TYPING_CORE_PREPROCESSED_H
#define CRADLE_TYPING_CORE_PREPROCESSED_H

#include <cradle/typing/core/api_types.hpp>
#include <cradle/typing/core/dynamic.h>
#include <cradle/typing/core/immutable.h>
#include <cradle/typing/core/monitoring.h>
#include <cradle/typing/core/omissible.h>
#include <cradle/typing/core/upgrades.hpp>

// So that IDEs don't get confused by the macro in the original source file.
#define api(...)

namespace cradle {

// invalid_enum_value is thrown when an enum's raw (integer) value is invalid.
CRADLE_DEFINE_EXCEPTION(invalid_enum_value)
CRADLE_DEFINE_ERROR_INFO(string, enum_id)
CRADLE_DEFINE_ERROR_INFO(int, enum_value)

// invalid_enum_string is thrown when attempting to convert a string value to
// an enum and the string doesn't match any of the enum's cases.
CRADLE_DEFINE_EXCEPTION(invalid_enum_string)
// Note that this also uses the enum_id info declared above.
CRADLE_DEFINE_ERROR_INFO(string, enum_string)

} // namespace cradle

#endif
