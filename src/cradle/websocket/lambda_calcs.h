#ifndef CRADLE_LAMBDA_CALCS_H
#define CRADLE_LAMBDA_CALCS_H

#include <cradle/core/id.h>

namespace cradle {

template<class Return, class... Args>
auto make_function_id(Return (*ptr)(Args... args))
{
    return make_id(ptr);
}

// Make an ID for a lambda function (or potentially another function object).
//
// This relies on two facts: a) Lambda functions are implemented as
// compiler-generated function objects with unique types, and b) the CRADLE ID
// system distinguishes IDs based on type. Thus, simply including the type of
// the lambda function in the ID type produces a unique ID that's specific to
// the lambda function.
//
// Note that we assume here that the lambda doesn't capture any variables that
// would need to be accounted for in the ID. That's considered a usage error.
// It could probably be checked for in the future.
//
template<class Function>
auto
make_function_id(Function f)
{
    return make_id(static_cast<Function*>(nullptr));
}

} // namespace cradle

#endif
