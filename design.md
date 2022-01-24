# Service Architecture

## Overview

core
  - asynchronous execution logic (using cppcoro & thread pools)
  - task management mechanics
  - immutable memory cache & blob storage
  - mutable data cache [someday]
  - component interface definitions & registration mechanisms
  - configuration
  - logging
  - HTTP requests
  - lowest-level client API (in-process, direct C++ function calls)

external components (extensible via plugins)
  - calculation providers
    - in-process C++ functions [built-in]
    - local, via IPC
    - local, via Docker (Thinknode protocol) [MGH-specific]
    - remote, via Thinknode [MGH-specific]
    - ...
  - immutable data sources (possibly writable)
    - native C++ data [built-in]
    - GitHub revisions
    - Thinknode ISS [MGH-specific]
    - ...
  - caches
    - disk cache [built-in]
    - ...
  - mutable data sources [someday]
    - local files [built-in]
    - GitHub repositories
    - Thinknode RKS [MGH-specific]
  - hash object storage (Git-style)
    - internal, native C++ implementation

external client APIs
  - IPC (via iceoryx)
  - WebSockets
  - REST [maybe] [someday]

## Notes

- Components labeled as "[built-in]" would automatically be provided by the
  core.

- Some core components may be implemented differently depending on the type of
  deployment.

  Examples of deployments:
    - in-process within a C++ desktop application
    - running as a local service on a desktop supporting C++ desktop apps
      and/or R&D computing tasks
    - running on a remote server supporting thinner clients
    - running inside a web browser [someday]
    - running on a mobile device [someday]

  Note that these should be distinguishable at compile-time.

- Some external APIs/components may only be applicable for certain deployment
  types.

- "Tasks" are essentially noteworthy coroutines. The task tracking system
  provides (optional) information about the progress of a task and its
  relationship to other tasks. For example, if a client request triggers an
  HTTP request whose result happens to be cached to disk, which in turn
  triggers a disk read, I should be able to see (for debugging/diagnostic
  purposes) those relationships.

- The immutable caching system is tied in with task management in that tasks
  produce immutable results to place in the cache. Multiple entities should be
  able to request the same immutable result *while it is being computed*
  without spawning duplicate tasks for that result.

- The logging system should be structured around tasks and coroutine thread
  pools. It should be able to answer questions like "What HTTP requests are
  currently outstanding?", "Which client requests caused those to run?" and
  "What log messages are associated with those requests?"

# Data Types

## Notes on Application Data Types

Unlike Thinknode, the goal of CRADLE isn't to impose a type system on
applications. Instead, its goal is to be as flexible and unobtrusive as
possible, so it should impose as few requirements as possible on the data types
that the application tries to pass through CRADLE (e.g., just require that the
types be serializable if that's what's needs to do its job).

That said, there are still benefits to having *some* knowledge of these types:

- Documentation of the functions and data that an application exposes to CRADLE
  can be richer and more informative. (It can even be useful to search
  functions by the data types involved.)

- CRADLE can check that a calculation is structured properly (i.e., the inputs
  to a function are the correct types) without actually issuing the calculation
  and relying on the calculation provider to detect the error.

- Tools that visualize calculation trees would be more useful with a richer
  understanding of the data that exists at each level in the tree.

- Thinknode features a way to 'automatically' upgrade data:
    - When app developers release app versions with changed data formats, they
      provide functions that upgrade data from older formats to the newer
      format.
    - Clients specify what version they understand the formats of.
    - When clients request data, Thinknode automatically upgrades it to the
      format that the client understands.

  We've found this feature to be useful. It could likely be implemented as a
  layer above the core of CRADLE, but wherever it's done, it needs some
  information about data types and their relationships.

So all this means that CRADLE should likely support at least some form of type
tags/descriptions for data and function arguments/results, but it could (at the
core, anyway) consider those descriptions to be somewhat opaque.

It'd be nice to use some standard method for describing application data
types, but I'm not sure what that would be, and it would have to be flexible
enough to be compatible with the other goals here and not get in the way
during development.

Of course, CRADLE does still have to define certain data types related to the
actual service that it provides, which is what the rest of this section is
about.

## Calculation Requests

### Notes on What's Currently In Astroid/Thinknode

Thinknode defines the following types of calculations:

- `reference`
- `value`
- `function`
- `array`
- `item`
- `object`
- `property`
- `cast`
- `let`
- `variable`
- `meta`

`reference` is used to refer to other calculations (or data) by ID, so this
option will remain, but there's some design consideration around what an ID
will look like.

`value` is used to specify a value directly. It will remain.

`function` is used to specify a function application, so obviously this will
remain in some form. The main questions on this are:

  1. How do we refer to the function?

  2. What other overrides on the execution/behavior of the function are needed
     at this level?

     Astroid/Thinknode provide:
     - priority level
     - whether to execute the function locally or remotely (It's unclear at the
       moment how this should be generalized.)

     At higher levels (i.e., when you define a function), Astroid/Thinknode
     also provide:
     - how much caching to use:
       - none (always execute it)
       - cache to memory (but not disk)
       - both memory and disk
     - Does this function provide progress updates?
     - Is this function significant enough for its progress to be reported to
       the user?
     - How much computing power (CPU cores and RAM) should be reserved for the
       execution of this function?

`array`, `item`, `object`, `property`, and `cast` are all specific to the
Thinknode type system, so while they are still useful to Astroid applications,
it probably doesn't make sense to have them defined at the core of CRADLE.
Instead, they should be implemented as *functions* that can be applied just
like any other calculation function.

`let` and `variable` are used to avoid repeating large subrequests within a
parent request. However, those subrequests ultimately end up with their own IDs
anyway, so the utility of this depends on how difficult it is to just generate
those IDs in the first place. With a more responsive service (especially one
that's running locally), it may be fine to omit these request types and just
allow clients to submit the subrequests first, get back the IDs for them, and
then use those IDs in the parent request. It may even be the case that we
decide to use hashes as IDs, in which case the clients can generate them
themselves.

`meta` is used to specify that you are providing a calculation request that
generates another calculation request and that you want the result of the
*generated* request. There may be some utility in keeping a feature like this,
but another way to achieve this functionality is to simply allow calculation
providers access to the CRADLE client API. (In Thinknode, calculation providers
are sandboxed and don't have this access. However, I don't think it should be a
goal of CRADLE to run untrusted calculation functions, so I don't think this is
necessary.) This would be more generally useful.

## IDs

In Thinknode, IDs are assigned server-side using some combination of UUID
tricks and incremental counters.

Since CRADLE is meant to be more decentralized than Thinknode, it probably
makes sense to move away from this technique towards using some a hash of the
data object as its ID.

To the extent that objects have to be stored, their storage would work like
Git's hash-object storage. (i.e., The system would maintain a table mapping
from hashes to object data.)

SHA256 seems like a very safe pick for the hash function, but it might be
desireable to find something that's faster. We don't require cryptographic
security (just a reasonable guarantee that collisions won't happen).

## Contexts

TODO

## App Manifests

TODO

# Other Concerns

## Encryption

TODO

(This matters for HIPAA.)

## Key Management

(This may not matter for a while if we don't have any shared resources that are
managed by CRADLE.)

## Composition Caches

TODO
