# Proposed Milestone One Overview

This is what we discussed in the last meeting.

- Generalize the core CRADLE concepts of defining calculation functions,
  constructing requests and resolving them. (Abstract away Thinknode.)
- Preserve the interface to Thinknode, including local calculation.
  - At MGH, calculation apps would still be created via Thinknode (independent
    of the new CRADLE), but can be used via the new CRADLE.
- Provide an in-process C++ API, both on the client side (requesting
  calculation results) and the calculation provider side (i.e., allow normal
  C++ functions to be exposed to CRADLE).
- Improve the debugging/information capabilities. (See below.)
- The WebSocket interface would likely remain for exposing debugging
  information but doesn't need to provide the full client API.

Optional:

- Implement a client-side C++ interface as IPC/shared memory.

Pieter pointed out that when you take into account the work of abstracting the
code, the above is likely too much for a single milestone.

# Possible Use Cases

## PUMA Performance Improvements

PUMA is a Python library and command-line utility that exposes various Astroid
support functions. (Some of these are 'generic' (i.e., not specific to
radiotherapy) Thinknode/CRADLE-related functions, while others are very
specific to Astroid treatment planning.)

PUMA uses CRADLE under the hood. Certain tasks in PUMA take an unreasonable
amount of time to run, but many potential performance improvements in PUMA
would require improvements in CRADLE.

### Implementation Strategy

1. Get a better understanding of where PUMA spends all its time (especially
   where CRADLE is spending time under the hood).
2. Eliminate/mitigate some of the inefficiencies. Most likely, this would
   include the following, but it's hard to say for sure before step #1 is done:
   - Port some of PUMA to C++. (There are actually already a couple CRADLE
     requests that are Astroid-specific just because it makes sense for them to
     be there performance-wise. So the proper long-term solution to this is to
     move those onto the PUMA side but in a C++ module that communicates more
     efficiently with CRADLE.)
   - Directly improve the performance of certain parts of CRADLE. (e.g., CRADLE
     could improve certain aspects of its handling of Thinknode data and the
     Thinknode type system.)
   - (Follow through on whatever other insights we gain.)

### CRADLE Requirements

- Implement better tools for understanding what CRADLE is doing under the hood,
  especially with respect to tasks (coroutines). i.e., I should be able to have
  a conversation with CRADLE that looks something like the following:
  - What tasks are currently running on the HTTP request thread pool?
    ```
    active: [7, 8, 9]
    queued: [10, 11, 12]
    ```
  - Describe task 7.
    ```
    started: today 16:01:01.123456
    status: ongoing
    progress: unknown
    type: HTTP
    (HTTP-specific stuff...)
    method: "GET"
    URL: https://mgh.thinknode.io/..."
    ```
  - Who requested task 7?
    ```
    [4]
    ```
    (It's possible for multiple tasks to require the same result (and thus be
    waiting on the same subtask), so this is a list.)
  - Who requested task 4?
    ```
    [3]
    ```
  - Who requested task 3?
    ```
    []
    ```
    (i.e., Task 3 is a 'root' task, meaning that it was explicitly requested by
    a client.)
  - Describe task 3.
    ```
    (info about the requesting, including who requested it, etc.)
    ```

  (This functionality would all have to be extremely efficient and/or optional
  so that it doesn't have a noticeable impact on normal usage.)

- Provide a C++ client API.
  - This would allow some of the PUMA functionality to be implemented in C++
    with a more efficient interface to CRADLE.
  - There's some incentive for this interface to use IPC so that the Python and
    C++ parts of PUMA can talk to the same CRADLE service. However, that could
    also be achieved (for now) by bundling the CRADLE service inside the PUMA
    C++ app.

- I suspect there are some performance improvements to be gained by isolating
  CRADLE more from the Thinknode type system and processing things more as
  blobs. (i.e., Working towards the abstraction goal would help here.)

- (Again, insights might reveal other useful work.)

- (Also note that the full WebSocket client API would need to remain in this
  case.)

## Astroid Support Apps

These would be C++ desktop apps that allow users to perform various support
functions for Astroid (via Thinknode). e.g.:
- copy/move imported data between realms
- copy patient plans between realms
- deploy realm configurations
- perform robust analyses (extra dose calculations)

(Those functions are currently performed via PUMA, but that is are hard to set
up and run, which limits the number of users that are able to run them.)

### Implementation Strategy

- Polish the alia/Qt integration and use that to implement the desktop UIs.

- Integrate the open-source alia with the open-source CRADLE.

- Port over some of the PUMA tasks from Python to C++.

### CRADLE Requirements

Definitely:

- Provide a C++ client API (either in-process or IPC should be fine).

Probably:

- Implement improved task tracking tools. (See above.)

## In-Process C++ Open-Source Release

This would be a polished release of CRADLE in a form that could be used to
apply CRADLE concepts internally within C++ applications.

It should:

- Be well-documented.
- Be easy to integrate into an existing C++ application.

### CRADLE Requirements

Definitely:

- Generalize the core CRADLE concepts of defining calculation functions,
  constructing requests and resolving them. (Abstract away Thinknode.)
- Provide an in-process C++ client API.
- Provide an in-process C++ API for defining calculation functions.

Probably:

- Implement improved task tracking tools. (See above.)
