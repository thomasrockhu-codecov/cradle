#ifndef CRADLE_WEBSOCKET_TYPES_HPP
#define CRADLE_WEBSOCKET_TYPES_HPP

#include <cradle/thinknode/types.hpp>

namespace cradle {

struct lambda_calculation;
struct function_application;
struct array_calc_request;
struct item_calc_request;
struct object_calc_request;
struct property_calc_request;
struct let_calc_request;
struct meta_calc_request;
struct cast_calc_request;

api(union)
union calculation_request
{
    std::string reference;
    dynamic value;
    lambda_calculation lambda;
    function_application function;
    array_calc_request array;
    item_calc_request item;
    object_calc_request object;
    property_calc_request property;
    let_calc_request let;
    std::string variable;
    meta_calc_request meta;
    cast_calc_request cast;
};

// TODO: Maybe come up with a better name for this.
api(enum)
enum class execution_host_selection
{
    ANY,
    THINKNODE,
    LOCAL
};

api(struct)
struct function_application
{
    std::string account;
    std::string app;
    std::string name;
    omissible<execution_host_selection> host;
    omissible<cradle::integer> level;
    std::vector<calculation_request> args;
};

struct lambda_function
{
    captured_id id;
    std::function<dynamic(dynamic_array const& args)> object;
};
inline bool
operator<(lambda_function const& a, lambda_function const& b)
{
    return a.id < b.id;
}
inline bool
operator==(lambda_function const& a, lambda_function const& b)
{
    return a.id == b.id;
}
inline void
to_dynamic(dynamic* v, lambda_function const& x)
{
    throw "unimplemented";
}
inline void
from_dynamic(lambda_function* x, dynamic const& v)
{
    throw "unimplemented";
}
inline size_t
deep_sizeof(lambda_function const& x)
{
    return 0;
}
inline size_t
hash_value(lambda_function const& x)
{
    return x.id.hash();
}
template<>
struct type_info_query<lambda_function>
{
    static void
    get(api_type_info*)
    {
        throw "unimplemented";
    }
};

api(struct)
struct lambda_calculation
{
    lambda_function function;
    std::vector<calculation_request> args;
};

api(struct)
struct array_calc_request
{
    std::vector<calculation_request> items;
    cradle::thinknode_type_info item_schema;
};

api(struct)
struct object_calc_request
{
    std::map<std::string, calculation_request> properties;
    cradle::thinknode_type_info schema;
};

api(struct)
struct item_calc_request
{
    calculation_request array;
    calculation_request index;
    cradle::thinknode_type_info schema;
};

api(struct)
struct property_calc_request
{
    calculation_request object;
    calculation_request field;
    cradle::thinknode_type_info schema;
};

api(struct)
struct meta_calc_request
{
    calculation_request generator;
    cradle::thinknode_type_info schema;
};

api(struct)
struct cast_calc_request
{
    cradle::thinknode_type_info schema;
    calculation_request object;
};

api(struct)
struct let_calc_request
{
    std::map<std::string, calculation_request> variables;
    calculation_request in;
};

api(struct)
struct results_api_generated_request
{
    std::string context_id;
    optional<calculation_request> request;
};

} // namespace cradle

#endif
