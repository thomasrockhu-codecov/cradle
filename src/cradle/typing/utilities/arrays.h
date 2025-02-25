#ifndef CRADLE_TYPING_UTILITIES_ARRAYS_H
#define CRADLE_TYPING_UTILITIES_ARRAYS_H

#include <string>

#include <cradle/inner/core/exception.h>
#include <cradle/inner/utilities/arrays.h>

namespace cradle {

// Check that an index is in bounds.
// :index must be nonnegative and strictly less than :upper_bound to pass.
void
check_index_bounds(std::string const& label, size_t index, size_t upper_bound);

// If the above check fails, it throws this exception.
CRADLE_DEFINE_EXCEPTION(index_out_of_bounds)
CRADLE_DEFINE_ERROR_INFO(std::string, index_label)
CRADLE_DEFINE_ERROR_INFO(size_t, index_value)
CRADLE_DEFINE_ERROR_INFO(size_t, index_upper_bound)

// Check that an array size matches an expected size.
void
check_array_size(size_t expected_size, size_t actual_size);

// If the above check fails, it throws this exception.
CRADLE_DEFINE_EXCEPTION(array_size_mismatch)
CRADLE_DEFINE_ERROR_INFO(size_t, expected_size)
CRADLE_DEFINE_ERROR_INFO(size_t, actual_size)

} // namespace cradle

#endif
