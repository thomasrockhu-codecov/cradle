#ifndef CRADLE_INNER_UTILITIES_ARRAYS_H
#define CRADLE_INNER_UTILITIES_ARRAYS_H

namespace cradle {

template<typename T>
struct array_deleter
{
    void
    operator()(T* p)
    {
        delete[] p;
    }
};

} // namespace cradle

#endif
