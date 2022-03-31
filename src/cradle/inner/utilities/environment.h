#ifndef CRADLE_INNER_UTILITIES_ENVIRONMENT_H
#define CRADLE_INNER_UTILITIES_ENVIRONMENT_H

#include <optional>
#include <string>

#include <cradle/inner/core/exception.h>

namespace cradle {

// Get the value of an environment variable.
std::string
get_environment_variable(std::string const& name);
// If the variable isn't set, the following exception is thrown.
CRADLE_DEFINE_EXCEPTION(missing_environment_variable)
CRADLE_DEFINE_ERROR_INFO(std::string, variable_name)

// Get the value of an optional environment variable.
// If the variable isn't set, this simply returns none.
std::optional<std::string>
get_optional_environment_variable(std::string const& name);

// Set the value of an environment variable.
void
set_environment_variable(std::string const& name, std::string const& value);

} // namespace cradle

#endif
