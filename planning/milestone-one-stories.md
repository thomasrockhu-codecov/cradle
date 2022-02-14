# Milestone one stories
Milestone one has three parts:

1. External C++ API
2. Isolating and improving inner core
3. Coroutines introspection

Details about contents in [cppapi.md](cppapi.md) for the external API,
or [milestone-one-details.md](generated/milestone-one-details.md) for the rest.


## External C++ API
### Story
As an Astroid developer working on C++ desktop applications, I'd like to be able to interface to OpenCradle using a native, in-process C++ API instead of WebSockets. I'd like to access all the primary functionality that OpenCradle already provides plus be able to specify my own calculation functions as in-process C++ functions.

Things to do:
* Define the external C++ API
* Create (integration) tests covering all exposed functions
* Implementation in C++

Definition of done:
* API defined
* All tests pass
* Some PUMA functtionality ported to C++, and working with the new API


### Tasks
1. Define the external C++ API
   - See [cppapi.md](cppapi.md)
   - Will include the new blob
1. Initial setup:
   - Initial tests exercising the new API
   - Empty/dummy implementation
   - Extend build system
1. Implementation in C++
   - Add more tests on the new API, if needed
1. Demo (run new tests)


## Inner core
Tasks:
1. Define C++ API to the inner core
   - Will include the new blob
   - No cppcoro here!? Not `cppcoro::shared_task<blob>` itself, but a wrapper providing introspection hooks.
1. Move inner core code into a separate (directory tree) component
   - Create initial glue between inner core and "the rest of CRADLE"
1. Update build system accordingly (distinct inner core library)
1. New unit tests accessing the inner core API, distinct unit test runner
1. Move unwanted code (e.g. dynamic) from inner core to the "rest" or glue; part one
1. Idem, part two, three, ...
1. Demo (run unit tests?)


## Introspection
Tasks:
1. Clarify requirements: what introspection information do we need?
   - What exactly is a "task"? (Related to but not equal to a coroutine)
   - Also non-requirements? If supporting all websocket messages and all Thinknode messages
     is too much work, then what are the important ones?
1. Come up with ideas and a design how to add introspection to the C++ inner core
   - Tracking the many-to-one task relationships may be the tricky part.
   - Maybe this could be an additional service using the inner core API, but that API
     should then expose whatever the introspection needs
   - It could be that supporting all messages implies an impractically large amount of work.
   - When introspection is disabled, performance impact of the new code should be negligible.
1. Define new websocket messages:
   - Enable/disable introspection
   - Get all types of introspection data
1. Revive Python integration test
   - More flexibility, cover more messages
   - Possibility to run a single test
   - Accessible from Python CLI
1. Add introspection messages to Python
   - Dummy implementation in C++
1. C++: container to store introspection data
1. C++: collect introspection data and store it in the container; part one
1. Idem, part two, three, ...
1. Port new websocket messages to PUMA
1. Test with Astroid
1. Demo
