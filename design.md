# Goals

## Core Reason for Being

CRADLE is a service for managing the execution of resource-intensive
calculations and the caching of their results. It attempts to bring a
declarative approach to scientific computing, where instead of directly
executing calculations, clients *declare* their interest in specific
calculation results and let CRADLE figure out what actually needs to be done
(if anything) to deliver those results.

Calculations are specified as compositions of pure functions applied to
immutable inputs (which may simply be raw data or may themselves be calculation
results).

CRADLE seeks to enable this approach in as many environments as possible, but
in a way that's compatible across environments. (For example, when deployed
internally within a C++ application, it may enable calculation compositions to
be specified as highly-efficient native data structures using function
pointers, but it may also allow these same compositions to be exported to JSON
data structures for manipulation in Python.)

## Supportive Goals

In order to achieve its primary goal in a useful manner, CRADLE should:

- Impose an acceptably small performance overhead on the user. (What
  constitutes "acceptably small" may differ depending on the environment.)

- Provide integrations with popular tools/services for storing data, performing
  calculations, and deploying applications/code.

- Provide integrations with popular development tools in targeted environments.
  (e.g., For Python, not only should it provide an SDK for Python itself, but
  ideally also integrations with NumPy, Jupyter, etc.)

- Provide supporting tools for analyzing calculation compositions, tracking
  progress, analyzing logs, debugging problematic calculations, etc.

# Immediate MGH Use Cases

## Astroid Support Apps

These would be C++ desktop apps that allow users to perform various support
functions for Astroid (via Thinknode). e.g.:
- copy/move imported data between realms
- copy patient plans between realms
- deploy realm configurations
- perform robust analyses (extra dose calculations)

(Those functions are currently performed via Python scripts, but those are hard
to set up and run, which limits the number of users who are able to run them.)

## Plan Evaluation App (PEA)

This would be a C++ desktop app that is much more graphically-intensive and
involves visualizing patient images and dose distributions. The purpose would
be to perform more in-depth evaluations and comparisons on Astroid treatment
plans. (Do custom calculations on the plans and visualize the results.)

# The Calculation Resolution Process

The essence of CRADLE is all about quickly resolving calculation requests that
include:

- a function
- some inputs (which are other requests)

This is somewhat abstract and could have different manifestations in different
environments. For example, for a C++ app that only uses CRADLE internally, the
function could be represented simply by a function pointer. In a more
heavy-weight deployment, the function could be represented by a Docker image
tag.

Regardless of the exact environment and manifestation, the resolution process
follows a similar pattern:

- Try resolving it using a cache lookup.
  - The cache lookup is keyed by the content of the request itself. (This
    could involve a hash, either entirely as a replacement or just to speed
    up the lookup. - In the Astroid C++ code, 32-bit hashes are precomputed
    as the requests are constructed.)
  - This can be skipped entirely or partially for functions where it's not
    warranted. (In Astroid, the default is to use the memory cache but not
    the disk cached. Functions can be explicitly marked for disk caching or
    marked as 'trivial' to disable caching entirely.)

- Resolve the inputs to values.

- Try resolving it again using the cache but with the actual values substituted
  into the inputs.
  - This allows you to reuse results in cases where the inputs have changed in
    structure but still produce the same value. (Originally Thinknode didn't do
    this, but it proved frustrating in certain cases where it felt like the
    calculation was obviously the same. It's possible that these cases are
    uncommon enough that this could be conditionally enabled/disabled.)

- Invoke the function.

# Service Architecture

## Components

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

### Notes

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

In Thinknode, almost all operations happen within a 'context'. The context is
an immutable entity that defines the environment in which an operation happens.
In particular, it provides:

- the versions of the (calculation) apps that are installed in the environment
- the data bucket where data is read from (and written to)

In generalizing this, it seems that there are two concerns:

1. Since we are using a more generalized/extensible design, essentially all
   aspects of the CRADLE environment must now be explicitly provided, so some
   sort of context must exist to say where data can be found, where calculation
   app images come from, etc.

2. The context (in Thinknode, at least) allows client requests to refer to
   functions in calculation apps using only the *name* of that app (without
   specifying exactly which version of the app to use or where that app can be
   found). This allows clients to have one central location where they specify
   which versions of apps they depend on. (Essentially, one can think of
   requests as first existing in some unresolved/dependent state (where app
   references are just names) and then being combined with a context to become
   fully resolved/independent.)

## App Manifests

TODO

# Other Concerns

## Encryption

CRADLE needs to be compatible with storing sensitive data that needs to be
encrypted both in transit and at rest). (This matters for HIPAA.)

Obviously, since CRADLE is agnostic to the format of the data it processes, one
could achieve encryption simply by encrypting/decrypting data whenever it
enters/exits CRADLE. This wouldn't require any explicit support from CRADLE.
However, it might result in a) additional complexity on all the boundaries
where clients/apps interact with CRADLE, b) unnecessary work encrypting and
decrypting along boundaries that don't require it (e.g., the memory cache), and
c) decreased usefulness of analysis tools (since data would become more
opaque).

Thus, CRADLE should provide explicit support by allowing data to be denoted as
sensitive. It should even allow this to be the default and require clients to
opt-out for specific data types. (Unclear right now what would be the scope of
this default setting.)

## Key Management

(This may not matter for a while if we don't have any shared resources that are
managed by CRADLE.)

## Composition Caches

TODO

## The Role of the Preprocessor

Astroid uses a preprocessor for generating boilerplate C++ code needed to:
1. Implement certain 'normal' C++ operations that aren't provided by default
   when declaring a data structure (e.g., comparison operators).
2. Serialize data structures for network and disk I/O.
3. Expose C++ functions for external invocation.
4. Generate external descriptions of C++ types and functions (for registration
   with Thinknode and as documentation).

A lot (or all?) or #1 has been made obsolete by improvements in C++.

#2 still has no good solution in C++ without external tools.

#3 is more approachable in C++ but not quite trivial.

#4 is also not trivial but might be approachable with the help of something
like Doxygen (or some Clang-based tool).

From the perspective of a C++ developer wanting to use CRADLE, it would be
*highly* undesirable (often a deal-breaker) for CRADLE to try to impose the use
of an external preprocessor. It would be a bit more palatable to require the
use of an external tool that analyzed source code and extracted information for
use in a manifest/documentation (but didn't actually try to generate C++ code
itself). (And this would be even more acceptable if such a tool was only
required when using CRADLE in an environment that already implied that external
services were running.)

Ideally, from a C++ development perspective, CRADLE should:

- Define the requirements that it imposes on types and functions in order for
  them to be exposed through CRADLE.
    - Note that these requirements might differ depending on the environment.
      (Exposing a function to other C++ code requires a lot less than exposing
      it to Python code.)
    - Requirements within C++ itself should be expressed as C++20 concepts.

- Be flexible about how those requirements are fulfilled.
    - Where appropriate, provide tools that we think make the job easier, but
      also allow developers to fulfill the requirements in other ways that make
      more sense to them.

The preprocessor is likely still a useful tool for fulfilling some of the above
requirements in the core CRADLE code. It *might* also still be useful to
provide as an option for C++ developers who want to use CRADLE, but it
definitely shouldn't be required, and it's worth considering alternative ways
to do the same job.
