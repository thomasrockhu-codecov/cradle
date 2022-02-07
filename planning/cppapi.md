
So first off, I think that especially for milestone one, the signatures for the
functions that the WebSocket server uses internally to fulfill its requests
would actually be perfectly reasonable as an in-process C++ client API. For
example:

```cpp
cppcoro::shared_task<dynamic>
get_iss_object(
    service_core& service,
    thinknode_session session,
    string context_id,
    string object_id,
    bool ignore_upgrades = false)
```

Of course, this is a coroutine. I guess it's reasonable to ask if the external
API should use coroutines. I don't have a strong opinion on this, so I'm
tempted to just say that it should for now since that will be a lot easier.

And I suppose maybe if this were an external API, it should take a
`cradle::service` object or something, but I suppose that would be pretty
similar to what I call the `service_core` right now.

And then there's the session object. I suppose we shouldn't just be passing
`thinknode_session` objects around to every API, but it probably does make
sense to include a session object of some sort, so for now we could just
typedef that to be `thinknode_session`.

I don't think we need every WebSocket API either. I think these would suffice:

```cpp
    cradle::websocket_registration_message registration;
    cradle::iss_object_request iss_object;
    cradle::resolve_iss_object_request resolve_iss_object;
    cradle::iss_object_metadata_request iss_object_metadata;
    cradle::post_iss_object_request post_iss_object;
    cradle::copy_iss_object_request copy_iss_object;
    cradle::copy_calculation_request copy_calculation;
    cradle::post_calculation_request post_calculation;
    cradle::calculation_request_message calculation_request;
    cradle::post_calculation_request perform_local_calc;
```

There might be a few other high-level control functions that are needed.
Obviously there needs to be a way to initialize a service with a configuration
and all that, but obviously some of that is there already, and I can't think of
anything that would need to be added.

As far as in-process C++ functions go, I think it's going to take some planning
to figure out how to properly unify the concepts. For milestone one, it might
be worthwhile trying to emulate the functionality that CRADLE provides within
the Astroid C++ code. Basically, there, CRADLE has its own internal definition
of a calculation request that's a superset of what Thinknode can represent. The
app sends CRADLE its calculation requests in this internal format and,
depending on what's in there, CRADLE will processes parts of it locally or send
parts off to Thinknode. (So unlike the code that we currently have, where you
construct a calculation request and then you ask CRADLE to either resolve it
locally or remotely, in Astroid, you could create a single calculation request
where some of it was done locally and some remotely and then the results were
combined by uploading or downloading things.)

So I might start with the Thinknode `calculation_request` structure and make a
few additions:
- Add a new request type called `lambda` where you can just give it an
  in-process function object that it will apply. For now, we could keep it very
  simple and just say that this function has one argument: a `dynamic_array` of
  the actual arguments, and it returns a `dynamic`.
- For function requests, add an optional flag that allows the client to force
  it to run locally or remotely.

That should basically unify `post_calculation` and `perform_local_calc` and
also add in-process C++ functions. And I think most of the code needed is
already in one of those two functions. (You'd need some logic too to decide
when to post calculations to Thinknode vs resolve them locally, but that could
be very simple for now.)

This structure could also include flags like I mentioned before about what
level of caching to use.
