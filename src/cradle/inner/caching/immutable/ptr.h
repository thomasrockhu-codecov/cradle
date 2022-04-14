#ifndef CRADLE_INNER_CACHING_IMMUTABLE_PTR_H
#define CRADLE_INNER_CACHING_IMMUTABLE_PTR_H

#include <cppcoro/shared_task.hpp>

#include <cradle/inner/caching/immutable/cache.h>
#include <cradle/inner/caching/immutable/internals.h>
#include <cradle/inner/core/type_interfaces.h>
#include <cradle/inner/utilities/functional.h>

// This file provides the interface for consuming cache entries.

namespace cradle {

struct immutable_cache;

namespace detail {

struct immutable_cache_record;

// untyped_immutable_cache_ptr provides all of the functionality of
// immutable_cache_ptr without compile-time knowledge of the data type.
struct untyped_immutable_cache_ptr
{
    untyped_immutable_cache_ptr()
    {
    }

    ~untyped_immutable_cache_ptr()
    {
        reset();
    }

    untyped_immutable_cache_ptr(untyped_immutable_cache_ptr const& other)
    {
        copy(other);
    }
    untyped_immutable_cache_ptr(untyped_immutable_cache_ptr&& other)
    {
        move_in(std::move(other));
    }

    untyped_immutable_cache_ptr&
    operator=(untyped_immutable_cache_ptr const& other)
    {
        reset();
        copy(other);
        return *this;
    }
    untyped_immutable_cache_ptr&
    operator=(untyped_immutable_cache_ptr&& other)
    {
        reset();
        move_in(std::move(other));
        return *this;
    }

    void
    reset();

    void
    reset(
        cradle::immutable_cache& cache,
        captured_id const& key,
        function_view<
            std::any(immutable_cache& cache, id_interface const& key)> const&
            create_task)
    {
        this->reset();
        acquire(cache, key, create_task);
    }

    bool
    is_initialized() const
    {
        return record_ != nullptr;
    }

    // Everything below here should only be called if the pointer is
    // initialized...

    id_interface const&
    key() const
    {
        return *record_->key;
    }

    immutable_cache_record*
    record() const
    {
        return record_;
    }

 private:
    void
    copy(untyped_immutable_cache_ptr const& other);

    void
    move_in(untyped_immutable_cache_ptr&& other);

    void
    acquire(
        cradle::immutable_cache& cache,
        captured_id const& key,
        function_view<
            std::any(immutable_cache& cache, id_interface const& key)> const&
            create_task);

    // the internal cache record for the entry
    detail::immutable_cache_record* record_ = nullptr;
};

void
record_immutable_cache_value(
    immutable_cache& cache, id_interface const& key, size_t size);

void
record_immutable_cache_failure(
    immutable_cache& cache, id_interface const& key);

template<class Value>
cppcoro::shared_task<Value>
cache_task_wrapper(
    immutable_cache& cache, id_interface const& key, cppcoro::task<Value> task)
{
    try
    {
        Value value = co_await task;
        record_immutable_cache_value(cache, key, deep_sizeof(value));
        co_return value;
    }
    catch (...)
    {
        record_immutable_cache_failure(cache, key);
        throw;
    }
}

template<class Value, class CreateTask>
auto
wrap_task_creator(CreateTask&& create_task)
{
    return [create_task = std::forward<CreateTask>(create_task)](
               immutable_cache& cache, id_interface const& key) {
        return cache_task_wrapper<Value>(cache, key, create_task(key));
    };
}

} // namespace detail

// immutable_cache_ptr<T> represents one's interest in a particular immutable
// value (of type T). The value is assumed to be the result of performing some
// operation (with reproducible results). If there are already other parties
// interested in the result, the pointer will immediately pick up whatever
// progress has already been made in computing that result.
//
// This is a polling-based approach to observing a cache value.
//
template<class T>
struct immutable_cache_ptr
{
    immutable_cache_ptr()
    {
    }

    immutable_cache_ptr(detail::untyped_immutable_cache_ptr& untyped)
        : untyped_(untyped)
    {
    }

    template<class CreateTask>
    immutable_cache_ptr(
        cradle::immutable_cache& cache,
        captured_id const& key,
        CreateTask&& create_task)
    {
        reset(cache, key, std::forward<CreateTask>(create_task));
    }

    void
    reset()
    {
        untyped_.reset();
    }

    template<class CreateTask>
    void
    reset(
        cradle::immutable_cache& cache,
        captured_id const& key,
        CreateTask&& create_task)
    {
        untyped_.reset(
            cache,
            key,
            detail::wrap_task_creator<T>(
                std::forward<CreateTask>(create_task)));
    }

    // Access the underlying untyped pointer.
    detail::untyped_immutable_cache_ptr const&
    untyped() const
    {
        return untyped_;
    }
    detail::untyped_immutable_cache_ptr&
    untyped()
    {
        return untyped_;
    }

    bool
    is_initialized() const
    {
        return untyped_.is_initialized();
    }

    // Everything below here should only be called if the pointer is
    // initialized...

    cppcoro::shared_task<T> const&
    task() const
    {
        return std::any_cast<cppcoro::shared_task<T> const&>(
            untyped_.record()->task);
    }

    immutable_cache_entry_state
    state() const
    {
        return untyped_.record()->state.load(std::memory_order_relaxed);
    }
    bool
    is_loading() const
    {
        return state() == immutable_cache_entry_state::LOADING;
    }
    bool
    is_ready() const
    {
        return state() == immutable_cache_entry_state::READY;
    }
    bool
    is_failed() const
    {
        return state() == immutable_cache_entry_state::FAILED;
    }

    id_interface const&
    key() const
    {
        return untyped_.key();
    }

 private:
    detail::untyped_immutable_cache_ptr untyped_;
};

} // namespace cradle

#endif
