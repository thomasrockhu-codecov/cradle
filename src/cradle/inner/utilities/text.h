#ifndef CRADLE_INNER_UTILITIES_TEXT_H
#define CRADLE_INNER_UTILITIES_TEXT_H

#include <string>

#include <boost/lexical_cast.hpp>

#include <cradle/inner/core/exception.h>

namespace cradle {

using boost::lexical_cast;

// If a simple parsing operation fails, this exception can be thrown.
CRADLE_DEFINE_EXCEPTION(parsing_error)
CRADLE_DEFINE_ERROR_INFO(std::string, expected_format)
CRADLE_DEFINE_ERROR_INFO(std::string, parsed_text)
CRADLE_DEFINE_ERROR_INFO(std::string, parsing_error)

} // namespace cradle

#endif
