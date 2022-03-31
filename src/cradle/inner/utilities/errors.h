#ifndef CRADLE_INNER_UTILITIES_ERRORS_H
#define CRADLE_INNER_UTILITIES_ERRORS_H

#include <string>

#include <cradle/inner/core/exception.h>

namespace cradle {

// If an error occurs internally within library that provides its own
// error messages, this is used to convey that message.
CRADLE_DEFINE_ERROR_INFO(std::string, internal_error_message)

// This can be used to flag errors that represent failed checks on conditions
// that should be guaranteed internally.
CRADLE_DEFINE_EXCEPTION(internal_check_failed)

// This exception is used when a low-level system call fails (one that should
// generally always work) and there is really no point in creating a specific
// exception type for it.
CRADLE_DEFINE_EXCEPTION(system_call_failed)
CRADLE_DEFINE_ERROR_INFO(std::string, failed_system_call)

} // namespace cradle

#endif
