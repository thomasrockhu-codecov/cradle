#include <cradle/caching/immutable/production.h>

#include <cradle/caching/immutable/internals.h>

namespace cradle {

void
report_immutable_cache_loading_progress(
    immutable_cache& cache_object, id_interface const& key, float progress)
{
    auto& cache = *cache_object.impl;

    {
        std::scoped_lock<std::mutex> lock(cache.mutex);

        auto i = cache.records.find(&key);
        if (i == cache.records.end())
            return;

        detail::immutable_cache_record* r = i->second.get();
        r->progress.store(encode_progress(progress));
    }
}

void
set_immutable_cache_data(
    immutable_cache& cache_object,
    id_interface const& key,
    untyped_immutable&& value)
{
    auto& cache = *cache_object.impl;

    {
        std::scoped_lock<std::mutex> lock(cache.mutex);

        auto i = cache.records.find(&key);
        if (i == cache.records.end())
            return;

        detail::immutable_cache_record* r = i->second.get();
        r->data = value;
        r->state.store(immutable_cache_data_state::READY);
        r->progress.store(encoded_optional_progress());
        // Ideally, the job controller should be reset here, since we don't
        // really need it anymore, but this causes some tricky synchronization
        // issues with the UI code that's observing it.
        // r->job->reset();
    }

    // Setting this data could've made it possible for any of the waiting
    // calculation jobs to run.
    // TODO
    // wake_up_waiting_jobs(
    //     *system.impl_->pools[int(background_job_queue_type::CALCULATION)]
    //          .queue);
}

} // namespace cradle
