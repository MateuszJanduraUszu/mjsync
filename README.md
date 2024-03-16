# MJSYNC

MJSYNC is an internal module designed for use in future projects.
It is written in C++20.

## Setup

1. Download the appropriate package based on your system architecture:

    * For 64-bit systems, download `Bin-x64.zip`.
    * For 32-bit systems, download `Bin-x86.zip`.

2. Extract the downloaded package. You should see the following directories:

    * `bin` - Debug and release binaries.
    * `inc` - Include headers.

3. Include the `inc\mjsync` directory to your project as an additional include directory.
4. Link `bin\{Debug|Release}\mjsync.lib` library to your project.
5. Don't forget to include the `bin\{Debug|Release}\mjsync.dll` in your project's
   output directory if your application relies on it during runtime.

## Usage

To integrate MJSYNC into your project, you can include the appropriate header files
based on your requirements:

* **<mjsync/api.hpp>**: Export/import macro, don't include it directly.
* **<mjsync/async.hpp>**: `async()` function for asynchronous execution of user-defined callables.
* **<mjsync/shared_resource.hpp>**: Manages access to shared resources across multiple threads
* **<mjsync/srwlock.hpp>**: Slim reader/writer lock (SRW Lock).
* **<mjsync/sync_flag.hpp>**: Provides a thread-safe synchronization flag management.
* **<mjsync/task.hpp>**: Observable scheduled task object.
* **<mjsync/thread.hpp>**: Threads management.
* **<mjsync/thread_pool.hpp>**: Manages multiple threads for asynchronous work execution.
* **<mjsync/waitable_event.hpp>**: `waitable_event` class for multithreaded waiting and signaling mechanisms.

## Compatibility

MJSYNC is compatible with Windows Vista and later operating systems,
and it requires C++20 support.

## Questions and support

If you have any questions, encounter issues, or need assistance with the MJSYNC,
feel free to reach out. You can reach me through the `Issues` section or email
([mjandura03@gmail.com](mailto:mjandura03@gmail.com)).

## License

Copyright © Mateusz Jandura.

SPDX-License-Identifier: Apache-2.0