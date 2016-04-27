libnstd
=======

*C++ is nice but its STL is an abomination.*

libnstd is a cross platform non-standard standard library for C++. In contrast to the Standard Template
Library (STL) its aim is to provide frequently used features with minimalistic classes and as little usage of templates
as possible. Right now it consists of following headers:

* [Base.h](include/nstd/Base.h): Basic data types and allocation operators.
* [Debug.h](include/nstd/Debug.h): Low level debugging tools.
* [Memory.h](include/nstd/Memory.h): Memory allocation and other memory tools.
* [Error.h](include/nstd/Error.h): Abstraction layer for operating system errors.
* [Math.h](include/nstd/Math.h): Basic mathematics functions.
* Data types
    * [String.h](include/nstd/String.h): A lazy copying string class.
    * [Variant.h](include/nstd/Variant.h): A variable data type for strings, lists, maps, integers and floats.
    * [Buffer.h](include/nstd/Buffer.h): A dynamic byte buffer.
    * [Time.h](include/nstd/Time.h): Date and time functions.
* Containers
    * [List.h](include/nstd/List.h): A double linked list.
    * [HashMap.h](include/nstd/HashMap.h): An associative container based on a hash map with fixed table size.
    * [HashSet.h](include/nstd/HashSet.h):  A container for unique elements based on a hash set with fixed table size.
    * [Map.h](include/nstd/Map.h), MultiMap.h: An associative container based on a balanced tree.
    * [Array.h](include/nstd/Array.h): A dynamically growing container based on an array.
* Concurrency
    * [Thread.h](include/nstd/Thread.h): Abstraction layer for native threads.
    * [Atomic.h](include/nstd/Atomic.h): Abstraction layer for atomic opeations.
    * [Mutex.h](include/nstd/Mutex.h): Abstraction layer for native mutual exclusion mechanisms.
    * [Semaphore.h](include/nstd/Semaphore.h): Abstraction layer for native semaphores.
    * [Signal.h](include/nstd/Signal.h): Abstraction layer for native signals.
    * [Monitor.h](include/nstd/Monitor.h): Abstraction layer for native monitor constructs.
* Input/Output
    * [File.h](include/nstd/File.h): Abstraction layer for file input/output.
    * [Directory.h](include/nstd/Directory.h): Abstraction layer to access directories.
    * [Console.h](include/nstd/Console.h): Helper function for Console I/O and command prompting.
    * [Log.h](include/nstd/Log.h): Console logging helper functions.
* Processes
    * [Process.h](include/nstd/Process.h): Abstraction layer for process launching with or without I/O redirecting.
    * [Library.h](include/nstd/Library.h): Abstraction layer for dll / shared object loading.
* Signals/Slots
    * [Emitter.h](include/nstd/Emitter.h): Base class for an object that emits signals.
    * [Receiver.h](include/nstd/Receiver.h): Base class for an object that receives signals.
* Sockets (optional)
    * [Socket.h](include/nstd/Socket/Socket.h): An abstraction layer for native sockets.
    * [Server.h](include/nstd/Socket/Server.h): An asynchronous TCP/IP server multiplexer.

Design Principles
-----------------

 * Be as much optimized as currently possible. (Unfortunately I do not have the time to optimize the shit out of each function, but some functions area already pretty optimized.)
 * Do not use macros except for code that should be excluded in release builds.
 * Use templates only when it is required to improve performance.
 * Do not include system header files in libnstd header files.
 * Design class interfaces to be minimalistic and easy to use.
 * Do not use exceptions.
 * Avoid dynamic memory allocation when possible.
 
Supported Platforms
-------------------

 * Windows x86/x86_64 (since Windows Vista / Server 2008)
 * Windows x86/x86_64 Unicode (since Windows Vista / Server 2008)
 * Cygwin i686 (since Windows Vista / Server 2008)
 * Linux i686/x86_64
 * Linux armv61
