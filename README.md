# TLS Core Allocator

Small C++20 allocator using thread-local free lists and a shared arena.

I wrote this mostly to play with thread-local allocation and see how far I could get without putting a lock in the normal allocation path. It is not intended to replace `malloc`.

The basic setup is:

- 1 GiB `mmap`'d arena
- 64 KiB slabs
- five size classes: 64, 128, 256, 512, 1024 bytes
- one free-list set per thread
- one shared atomic offset for handing out slabs

The pool only handles relatively small allocations. Larger allocations fall back to `malloc`.

## Layout

A pool allocation has a 16-byte header in front of the object.

The header is reused depending on whether the chunk is allocated or free. While the chunk is allocated, the first 8 bytes contain the chunk size. When the chunk is on a free list, those bytes are used for the intrusive `next` pointer.

The remaining 8 bytes are currently unused.

So a 1024 byte chunk gives 1008 bytes to the object:

```text
1024 bytes total
+----------------+
| 16 byte header |
+----------------+
| 1008 byte data |
+----------------+
```

The requested size is increased by the 16-byte header and then rounded up to a power-of-two size class.

The relevant code is basically:

```cpp
size_t total_size = request_size + HEADER_SIZE;
size_t chunk_size =
    std::max(MIN_CHUNK_SIZE, std::bit_ceil(total_size));
```

The index for the free-list array comes from `std::bit_width`.

For example, a 12-byte object ends up using a 64-byte chunk:

```text
12 byte object
    +
16 byte header
    =
28 bytes

next class = 64 bytes
```

Anything requiring more than 1008 bytes of payload does not fit in the pool and returns `nullptr` from the pool allocator. The global `operator new` then falls back to `malloc`.

## Getting slabs

`MemoryBlock` owns the 1 GiB arena.

A slab is obtained with an atomic `fetch_add` on the current arena offset:

```cpp
m_global_offset.fetch_add(
    slab_size,
    std::memory_order_relaxed
);
```

The arena offset is the only shared state involved in handing out slabs.

Once a thread gets a slab, it is all local. The slab is carved into chunks of one size class and linked into that thread's free list.

For example, if a thread needs a 256-byte chunk and its 256-byte list is empty, it gets a new 64 KiB slab and turns the whole slab into a list of 256-byte chunks.

There is no attempt to split one slab between different size classes.

## Free lists

`FreeList` keeps five heads:

```cpp
std::array<Node*, 5> m_heads;
```

A free chunk is itself the `Node`, so there is no separate allocation for list metadata.

The normal allocation path is just:

1. pick a size class
2. get a new slab if that list is empty
3. pop a node from the list
4. write the chunk size into the header
5. return the address after the header

Deallocation does the reverse and pushes the chunk back onto the local list.

There are no mutexes involved in these operations.

## Global new/delete

`src/bridges/GlobalOverride.cpp` provides the global `operator new` and `operator delete` used by the test and benchmark programs.

That means normal code can use the allocator without calling a custom allocation function:

```cpp
SmallStruct* s = new SmallStruct();
delete s;
```

On allocation, the pool is tried first. If the requested size does not fit in a pool class, or the arena has no slab left, `operator new` calls `std::malloc`.

On delete, the implementation checks whether the pointer address is inside the arena range.

```text
inside arena  -> thread-local FreeList
outside arena -> std::free
```

This is just an address-range check. There is no separate allocation table.

The usual unsized and sized `delete` forms are provided, along with `new[]` and `delete[]`.

I have not implemented the aligned `new`/`delete` overloads. In particular, this means the pool should not be used for over-aligned types.

## `make_core`

There is a small RAII wrapper in `src/raii/SmartPtr.h`:

```cpp
auto obj = raii::make_core<SmallStruct>();
```

It is just a `std::unique_ptr` alias with a helper around `std::make_unique`.

The helper has a compile-time size check:

```cpp
static_assert(sizeof(T) <= 1008);
```

The point of the check is mostly to catch a type getting too large for the pool.

The actual allocation still goes through the global `new` override.

This helper is intended for normally aligned types. Since aligned allocation is not overridden, I would not use it as a general-purpose smart-pointer factory for over-aligned objects.

## Cross-thread frees

This is the main weakness right now.

The free lists belong to the thread, not to the slab or to the allocation itself.

So this:

```text
thread A
   |
   | new
   v
 object
   |
   | hand object to B
   v
thread B
   |
   | delete
   v
B's free list
```

means that thread B now owns the freed chunk from the allocator's point of view.

If thread A keeps allocating, it cannot use that chunk. It may have to claim another slab even though there are already free chunks sitting on thread B's lists.

With enough cross-thread traffic, the 1 GiB arena can therefore run out of slabs even though there is still free memory in the process.

I have not tried to fix this yet.

The obvious solution is some kind of per-thread return queue or another way of sending frees back to the allocating thread. That also means putting some synchronization back into the free path, which is the tradeoff I'm trying to avoid here.

For this experiment I'm leaving it as-is.

## Arena

The arena is currently fixed at 1 GiB:

```cpp
static MemoryBlock instance(1024 * 1024 * 1024);
```

Slabs are fixed at 64 KiB:

```cpp
constexpr size_t SLAB_SIZE = 64 * 1024;
```

Neither is configurable.

`MemoryBlock` uses `mmap` with `MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE`, so this is currently aimed at Linux rather than being a portable allocator implementation.

The global slab offset is cache-line aligned as a member:

```cpp
alignas(64) std::atomic<size_t> m_global_offset;
```

The idea there is mostly to keep the frequently-updated atomic away from unrelated members.

## Size classes

The pool has five classes:

```text
64
128
256
512
1024
```

The 16-byte header is part of those sizes.

That means the largest payload is 1008 bytes:

```text
1024 - 16 = 1008
```

Anything above that goes through `malloc`.

The classes are deliberately simple. A request just over a boundary gets promoted to the next class, so there is some internal waste.

For example, a request that needs 129 bytes of payload needs 145 bytes including the header, so it becomes a 256-byte chunk.

## Slabs are not recycled

Once a slab is handed to a thread, it stays with that thread.

There is no slab reclamation, and there is no mechanism for moving an unused slab to another thread.

There also isn't any attempt to detect that every chunk in a slab is free and return the slab to the arena.

That keeps the ownership model simple, but it also means memory usage can only move in one direction inside the arena.

## Metadata

Each pool chunk has the same 16-byte header whether it is allocated or free.

Allocated:

```text
+------------------+
| chunk size (8)   |
| unused (8)       |
+------------------+
| object data      |
+------------------+
```

Free:

```text
+------------------+
| next pointer     |
|                  |
+------------------+
| unused chunk     |
+------------------+
```

The free-list node is therefore stored inside the chunk itself.

This saves a separate metadata allocation, although 16 bytes is still noticeable for the smaller classes.

## Telemetry

There is some basic allocation telemetry in `MemoryTracker.h`.

The counters are thread-local:

```cpp
thread_local ThreadMetrics t_metrics;
```

So allocations normally just increment local counters.

When a thread's `ThreadMetrics` is destroyed, its counts are added to the global atomic counters.

A final report is registered with `std::atexit`.

The report is mainly for looking at allocator behavior while experimenting. I would not treat it as a profiling system.

## Tests

`tests/main.cpp` is pretty small at the moment.

It starts two threads and allocates:

- a 12-byte object
- a 100-byte object
- a 2000-byte object

The first two should come from the arena. The 2000-byte allocation should fall back to `malloc`.

The test also checks that the small allocation is inside the arena and the large allocation is outside it.

There is not much more to the correctness test yet. In particular, it does not try to exhaust the arena or exercise complicated cross-thread ownership.

## Benchmarks

`benchmarks/AllocatorBench.cpp` compares the global overrides against `std::malloc`.

There are two basic benchmark shapes:

```text
allocate -> free
```

and:

```text
allocate 1000 objects
free 1000 objects
```

Both are run with 8 threads.

The current benchmark ranges are:

```cpp
Range(32, 512)
```

for the single-allocation tests and:

```cpp
Range(64, 256)
```

for the burst tests.

The benchmark is mainly there so I can make an allocator change and see whether it moved the numbers.

It is not meant to establish that this allocator is faster than `malloc` in general.

It also does not currently cover the 1024-byte class in the benchmarks, or the `malloc` fallback caused by going over the pool limit.

## Build

The project uses C++20, CMake, and Google Benchmark.

The current build flags are:

```text
-O3
-march=native
-fno-exceptions
-fno-rtti
-Wall
-Wextra
-Werror
```

`-march=native` is intentional. The resulting binary is therefore tied to the CPU features available on the build machine.

The project currently expects a Linux/POSIX environment with the `mmap` APIs used by `MemoryBlock`.

Build:

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Then:

```bash
./allocator_test
./allocator_bench
```

If the arena cannot provide another slab, the pool returns `nullptr` and the global `operator new` falls back to `malloc`.

If `malloc` also fails, the program prints an error and calls `std::abort()`.

## Why I made it

Mostly curiosity.

I wanted to be able to read the whole allocation path without having to keep a large allocator implementation in my head.

The part I was interested in was the combination of:

- thread-local free lists
- fixed size classes
- intrusive nodes
- one atomic counter for handing out slabs
- no mutex on the normal pool path

There are plenty of things it does not handle well.

The biggest one is cross-thread frees. Slabs also never get recycled, the size classes are fairly crude, and the global `new`/`delete` coverage is intentionally incomplete.

That's okay for what this is.

I was more interested in having something small enough to poke at than in trying to build a production allocator.

For an actual application, I'd use `malloc` or an allocator that already solves these problems.

## Things I'd probably try next

The ones that seem most useful are:

- make the arena and slab sizes configurable
- recycle empty slabs
- add some kind of cross-thread free queue
- experiment with the size classes
- see whether all of the header is really necessary

The cross-thread free problem is probably the interesting one, because fixing it starts bringing synchronization and ownership rules back into the design.

That may end up making the allocator better, but it will also make it much less small.
