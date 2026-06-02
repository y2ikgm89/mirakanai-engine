# Engine Performance Optimization Foundation v1

## Status

Retained analysis record.

## Goal

Define a measurement-first optimization foundation for MIRAIKANAI Engine across CPU execution, memory management, renderer/RHI GPU memory, and optional GPU compute. This record organizes official vendor guidance and recommended systems papers into engine-facing design rules. It is not an implementation plan and does not claim new runtime performance readiness.

## Scope

This analysis covers:

- CPU multi-core utilization on current Intel and AMD x86-64 hosts.
- Memory allocation, cache behavior, false sharing, per-thread scratch, frame arenas, resource budgets, and NUMA/topology concerns.
- GPU memory, command recording, synchronization, barriers, upload staging, async compute/transfer overlap, and NVIDIA/AMD/Intel vendor profiling.
- Optional CUDA/HIP-style compute research as a tool or adapter concern, not as a gameplay-facing runtime API.

Out of scope:

- A global allocator replacement policy.
- Public native CPU/GPU handles.
- CUDA/HIP as required engine runtime dependencies.
- Any ready claim for broad performance, async overlap, backend parity, or platform-specific tuning without fresh trace evidence.

## Source Anchors

Use these sources as design anchors, not as code to copy.

| Area | Primary sources | Engine implication |
| ---- | --------------- | ------------------ |
| Intel CPU architecture | Intel 64 and IA-32 Architectures Optimization Reference Manual and throughput/latency tables: <https://www.intel.com/content/www/us/en/developer/articles/technical/intel64-and-ia32-architectures-optimization.html> | Keep hot loops data-local, branch-light, vectorization-friendly, and measurable against instruction/cache stalls instead of relying on generic "more threads" claims. |
| Intel profiling | Intel VTune Profiler User Guide: <https://www.intel.com/content/www/us/en/docs/vtune-profiler/user-guide/2024-2/overview.html> | Treat CPU utilization, threading, memory access, and vectorization analysis as host evidence for optimization changes. |
| Intel microarchitecture analysis | Intel VTune Profiler Performance Analysis Cookbook and Top-down Microarchitecture Analysis Method: <https://www.intel.com/content/www/us/en/docs/vtune-profiler/cookbook/2024-0/overview.html>, <https://www.intel.com/content/www/us/en/docs/vtune-profiler/cookbook/2024-0/top-down-microarchitecture-analysis-method.html> | Classify CPU stalls as front-end, bad-speculation, back-end, memory, or retiring work before changing code structure. |
| Intel hybrid CPU scheduling | Intel Performance Hybrid Architecture material: <https://www.intel.com/content/www/us/en/developer/articles/technical/hybrid-architecture.html> | Treat P-core/E-core systems as asymmetric until measured; keep latency-critical frame work, driver/helper threads, and background work separable. |
| Intel vectorization | Intel Advisor and Intel Intrinsics Guide: <https://www.intel.com/content/www/us/en/developer/tools/oneapi/advisor.html>, <https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html> | Use compiler vectorization reports, Advisor, and narrow runtime-dispatched kernels before hand-writing broad AVX2/AVX-512 code. |
| Intel GPU graphics | Intel Arc A-series Graphics Gaming API Developer and Optimization Guide: <https://www.intel.com/content/www/us/en/developer/articles/guide/arc-a-series-gaming-api-developer-optimization.html> | Query hardware features through D3D12/Vulkan APIs instead of vendor IDs, profile multi-queue choices, batch barriers/submissions, avoid shader assumptions about fixed wave size, and leave CPU headroom for driver/background shader work. |
| Intel GPU profiling | Intel Graphics Performance Analyzers and VTune GPU analysis: <https://www.intel.com/content/www/us/en/developer/tools/graphics-performance-analyzers/overview.html>, <https://www.intel.com/content/www/us/en/docs/vtune-profiler/user-guide/2023-2/gpu-offload-analysis.html> | Use Intel GPA for DirectX/Vulkan graphics frame, trace, shader, and system analysis on Intel GPUs; use VTune GPU Offload or GPU Compute/Media Hotspots for Intel GPU compute and CPU/GPU offload evidence. |
| Intel upscaling/latency | Intel XeSS developer documentation: <https://www.intel.com/content/www/us/en/developer/topic-technology/gamedev/xess.html> | Treat XeSS as an optional renderer feature behind explicit platform/device support, not as a substitute for core frame-time, latency, memory, or shader profiling evidence. |
| AMD CPU profiling | AMD uProf User Guide: <https://docs.amd.com/r/en-US/57368-uProf-user-guide/uProf-User-Guide> | Use AMD host evidence for Zen-specific instruction, cache, memory, and IBS-derived bottlenecks rather than extrapolating from Intel traces. |
| AMD CPU hardware events | AMD uProf Hardware Sources: <https://docs.amd.com/r/en-US/57368-uProf-user-guide/Hardware-Sources> | Use IBS, L3 cache performance counters, timechart, and memory subsystem evidence for Zen/EPYC/Ryzen bottlenecks. |
| AMD CPU tuning | AMD EPYC Software Optimization Guide: <https://docs.amd.com/v/u/en-US/56305> | Keep topology, cache, memory bandwidth, SMT, and NUMA effects visible in the scheduler and benchmark design. |
| AMD Zen optimization guides | AMD uProf useful links to Family 17h, Family 19h, Zen4, and Zen5 software optimization guides: <https://docs.amd.com/r/en-US/57368-uProf-user-guide/Useful-URLs?contentId=egjf_QgPZoI0hKkpK387fw> | Use generation-specific Zen guidance for cache, branch, SIMD, prefetch, ccNUMA, and multithreading decisions. |
| AMD compiler and libraries | AMD AOCC and AOCL: <https://www.amd.com/en/developer/aocc.html>, <https://www.amd.com/en/developer/aocl.html> | Treat AMD-tuned compilers/libraries as optional toolchain or app-level evidence, not as default engine dependencies. |
| Windows CPU topology | Microsoft Processor Groups: <https://learn.microsoft.com/en-us/windows/win32/procthread/processor-groups> | Do not assume one process automatically spans every logical processor on high-core systems; processor groups matter above 64 logical processors. |
| Windows memory and NUMA | Microsoft NUMA support and `VirtualAlloc2`: <https://learn.microsoft.com/en-us/windows/win32/procthread/numa-support>, <https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc2> | Prefer first-touch locality by default, and use explicit NUMA placement only after topology and trace evidence prove the need. |
| C++ ownership | C++ Core Guidelines resource rules and ISO C++ smart pointer guidance: <https://isocpp.org/guidelines>, <https://isocpp.org/wiki/faq/cpp11-library> | Treat raw pointers as non-owning observers by default, use `std::unique_ptr` for exclusive ownership transfer, reserve `std::shared_ptr` for real shared lifetime, and avoid smart pointer parameters when ownership is not part of the call. |
| C++ standard library pointers | Microsoft C++ smart pointer docs: <https://learn.microsoft.com/en-us/cpp/cpp/smart-pointers-modern-cpp?view=msvc-170> | Use RAII smart pointers for heap ownership, but keep hot engine data contiguous and handle/index based instead of pointer-graph based. |
| C++ bit operations | Microsoft `<bit>` function docs and WG21 bit-operations proposal lineage: <https://learn.microsoft.com/en-us/cpp/standard-library/bit-functions?view=msvc-170>, <https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1915r0.pdf> | Prefer standard C++20/C++23 bit APIs such as `std::popcount`, `std::countr_zero`, and `std::countl_zero` before compiler-specific intrinsics. |
| CPU bit manipulation | Intel Intrinsics Guide and AMD optimization guidance: <https://www.intel.com/content/dam/develop/public/us/en/include/intrinsics-guide/index.html>, <https://www.amd.com/content/dam/amd/en/documents/archived-tech-docs/software-optimization-guides/47414_15h_sw_opt_guide.pdf> | Use POPCNT/LZCNT/TZCNT/BMI-style instructions only through standard library or narrow runtime-dispatched kernels with fallback tests. |
| Task scheduling model | oneTBB scheduler and allocator docs: <https://uxlfoundation.github.io/oneTBB/main/tbb_userguide/How_Task_Scheduler_Works.html>, <https://uxlfoundation.github.io/oneTBB/main/tbb_userguide/Memory_Allocation.html> | Favor bounded task graphs, fork-join/data-parallel ranges, work stealing, cache affinity, and false-sharing-aware allocation patterns. |
| ECS archetype masks | Unity Entities archetype concepts and Flecs query docs: <https://docs.unity.cn/Packages/com.unity.entities%401.0/manual/concepts-archetypes.html>, <https://www.flecs.dev/flecs/md_docs_2Queries.html> | Component sets and query matching benefit from compact masks/archetype ids, but systems should iterate chunked data rather than per-entity pointer graphs. |
| Direct3D 12 memory | Microsoft D3D12 Memory Management: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/memory-management> | Follow "classify, budget and stream" for GPU memory; make residency, upload, streaming, and budgets explicit. |
| Direct3D 12 pipeline state | Microsoft D3D12 pipeline state guidance and PSO cache sample: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12>, <https://learn.microsoft.com/en-us/samples/microsoft/directx-graphics-samples/d3d12-pipeline-state-cache-sample-uwp/> | Treat PSO creation and shader compilation as offline/cache-managed work; runtime stutter from pipeline creation is a performance bug. |
| Direct3D 12 enhanced barriers | Microsoft D3D12 Enhanced Barriers documentation and DirectX-Specs: <https://learn.microsoft.com/en-us/windows-hardware/drivers/display/enhanced-barriers>, <https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html> | Treat barrier lowering as a frame-graph/backend policy with before/after GPU captures; do not hand-spread backend-specific barrier shortcuts through gameplay or renderer-neutral APIs. |
| Direct3D 12 work graphs | Microsoft DirectX Work Graphs announcement/spec: <https://devblogs.microsoft.com/directx/d3d12-work-graphs/>, <https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html> | Treat GPU-driven work creation as an optional future path for highly scalable rendering/simulation workloads, gated by hardware tier, shader model, fallback path, and profiler evidence. |
| DirectStorage | Microsoft DirectStorage documentation: <https://learn.microsoft.com/en-us/gaming/gdk/docs/features/common/directstorage/directstorage-overview> | Treat high-volume package IO, decompression, and GPU upload as a streaming pipeline with explicit CPU/GPU cost tradeoffs. |
| PIX | Microsoft PIX documentation: <https://learn.microsoft.com/en-us/windows/win32/direct3dtools/pix/articles/general/pix-overview> | Use GPU captures and timing captures for D3D12 frame evidence. |
| Vulkan threading | Vulkan Guide Threading: <https://docs.vulkan.org/guide/latest/threading.html> | Host-side multi-threading is application-managed; use per-thread command/descriptor pools and explicit synchronization. |
| Vulkan command recording | Khronos command buffer performance sample: <https://docs.vulkan.org/samples/latest/samples/performance/command_buffer_usage/README.html> | Parallel recording can reduce CPU frame time, but too many tiny secondary command buffers add overhead. |
| Vulkan pipeline cache | Vulkan Guide and Khronos pipeline cache sample: <https://docs.vulkan.org/guide/latest/pipeline_cache.html>, <https://docs.vulkan.org/samples/latest/samples/performance/pipeline_cache/README.html> | Persist and validate pipeline cache data per device/driver; avoid draw-time pipeline creation stutter. |
| Vulkan descriptor scaling | Khronos `VK_EXT_descriptor_buffer` overview: <https://www.khronos.org/blog/vk-ext-descriptor-buffer> | Treat descriptor-buffer/bindless-style models as backend feature gates with simpler descriptor-set fallbacks and trace evidence; do not assume the model is universally faster. |
| Vulkan memory | AMD GPUOpen Vulkan Memory Allocator: <https://gpuopen.com/vulkan-memory-allocator/> | GPU memory managers should suballocate, track stats, annotate allocations, and support linear/ring allocation patterns where appropriate. |
| NVIDIA Vulkan practice | NVIDIA Vulkan Dos and Don'ts: <https://developer.nvidia.com/blog/vulkan-dos-donts/> | On NVIDIA paths, measure task-graph command recording, descriptor updates, pipeline creation, and memory allocation/binding parallelism separately from AMD, Intel, and Metal behavior. |
| Metal profiling and resource binding | Apple Metal overview, game performance guidance, and argument buffer guidance: <https://developer.apple.com/metal/>, <https://developer.apple.com/documentation/metal/improving-your-games-graphics-performance-and-settings>, <https://developer.apple.com/documentation/metal/buffers/improving_cpu_performance_by_using_argument_buffers> | Keep Metal performance claims Apple-host gated; use Instruments/Metal HUD/captures, argument buffers, heaps, and explicit resource-use evidence only behind private Metal backend contracts. |
| Shader profiling | NVIDIA Nsight Graphics Shader Profiler: <https://docs.nvidia.com/nsight-graphics/UserGuide/shader-profiler.html> | Treat shader stalls, divergence, occupancy, and source-level shader hotspots as first-class GPU optimization evidence. |
| Texture compression | Khronos KTX 2.0 and Microsoft Direct3D block compression: <https://www.khronos.org/ktx>, <https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression> | Use GPU-native compressed texture formats, mip chains, and platform-appropriate transcodes to reduce memory, bandwidth, load time, and power. |
| Geometry compression | Khronos `EXT_meshopt_compression` and `KHR_draco_mesh_compression`: <https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Vendor/EXT_meshopt_compression/README.md>, <https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_draco_mesh_compression/README.md> | Treat mesh/animation compression, vertex cache ordering, overdraw, and decode placement as cook-time optimization surfaces. |
| Mesh shaders and meshlets | Microsoft D3D12 mesh shader samples and NVIDIA mesh shader guidance: <https://learn.microsoft.com/en-us/samples/microsoft/directx-graphics-samples/d3d12-mesh-shader-samples-win32/>, <https://developer.nvidia.com/blog/advanced-api-performance-mesh-shaders/> | Treat meshlets, cluster culling, and mesh shaders as optional geometry-scaling paths that require feature gates and fallback render paths. |
| Subset transforms | Bjorklund, Husfeldt, Kaski, and Koivisto, "Fourier meets Mobius: fast subset convolution": <https://arxiv.org/abs/cs/0611101> | Use subset DP/zeta/subset-convolution techniques for small bounded combinatorial problems, usually offline/tooling or heavily constrained runtime use. |
| Bitboards | Fenner and Levene, "Move Generation with Perfect Hash Functions": <https://journals.sagepub.com/doi/10.3233/ICG-2008-31102> | Board-game and tile-pattern systems can use precomputed bitboards/perfect-hash lookup tables when the domain fits fixed-width masks. |
| NVIDIA GPU compute | NVIDIA CUDA Best Practices Guide: <https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/index.html> | GPU compute kernels need coalesced memory access, occupancy/resource balance, async transfer planning, and profiler-backed decisions. |
| AMD GPU compute | ROCm HIP performance guidelines: <https://rocm.docs.amd.com/projects/HIP/en/develop/how-to/performance_guidelines.html> | HIP-style compute has the same core pressure points: memory coalescing, shared memory/LDS reuse, occupancy, divergence, and streams. |
| AMD RDNA graphics | AMD GPUOpen RDNA Performance Guide: <https://gpuopen.com/learn/rdna-performance-guide/> | Minimize and batch barriers, use optimized states/layouts, suballocate GPU heaps, avoid unnecessary dynamic state, and prove async overlap. |
| AMD GPU profiling | Radeon GPU Profiler manual: <https://gpuopen.com/manuals/rgp_manual/> | RGP is the AMD-side proof tool for queues, barriers, async compute, and RDNA hardware behavior across D3D12/Vulkan/HIP/OpenCL. |
| NVIDIA GPU profiling | Nsight Graphics GPU Trace: <https://docs.nvidia.com/nsight-graphics/UserGuide/gpu-trace-overview.html> | Nsight GPU Trace is the NVIDIA-side proof tool for D3D12/Vulkan frame bottlenecks and GPU unit utilization. |
| Compiler PGO | Microsoft MSVC Profile-Guided Optimizations and LLVM PGO documentation: <https://learn.microsoft.com/en-us/cpp/build/profile-guided-optimizations?view=msvc-170>, <https://clang.llvm.org/docs/UsersManual.html#profile-guided-optimization> | Treat PGO/LTO as a release-lane optimization with representative training workloads, not as a default debug/editor build behavior. |

## Recommended Papers

Read these before designing a broad job system or allocator surface:

- Gene Amdahl, "Validity of the Single Processor Approach to Achieving Large Scale Computing Capabilities" (1967): <https://www.cs.cmu.edu/~18742/papers/Amdahl1967.pdf>. Use it to keep serial frame work, synchronization, and main-thread ownership visible in every speedup claim.
- John L. Gustafson, "Reevaluating Amdahl's Law" (1988): <https://cacm.acm.org/research/reevaluating-amdahls-law/>. Use it to distinguish fixed-work frame-time speedups from larger-workload throughput scaling.
- Robert D. Blumofe and Charles E. Leiserson, "Scheduling Multithreaded Computations by Work Stealing" (1994/1999): <https://epubs.siam.org/doi/10.1137/S0097539793259471>. Use it as the theoretical baseline for local deques plus steal-on-starvation schedulers.
- Ulrich Drepper, "What Every Programmer Should Know About Memory" (2007): <https://www.cs.tufts.edu/cs/40/docs/WhatEveryProgrammerShouldKnowAboutMemory.pdf>. Use it as background for cache, memory hierarchy, NUMA, locality, and prefetch effects.
- Edward A. Lee, "The Problem with Threads" (2006): <https://www2.eecs.berkeley.edu/Pubs/TechRpts/2006/EECS-2006-1.html>. Use it as a warning that raw shared-memory threading reduces determinism unless coordination is explicit and composable.
- Emery D. Berger, Kathryn S. McKinley, Robert D. Blumofe, and Paul R. Wilson, "Hoard: A Scalable Memory Allocator for Multithreaded Applications" (2000): <https://www.cs.utexas.edu/ftp/techreports/tr00-07.pdf>. Use it to understand heap contention, false sharing, and memory blowup in threaded allocators.
- Daan Leijen, Ben Zorn, and Leonardo de Moura, "Mimalloc: Free List Sharding in Action" (2019): <https://www.microsoft.com/en-us/research/publication/mimalloc-free-list-sharding-in-action/>. Use it to understand modern allocator sharding tradeoffs before considering an optional app-level allocator.

## Design Principles

### 1. Optimize Only With Evidence

Optimization work must start with a reproducible baseline:

- Frame metrics: average, p95, p99, max frame time, simulation time, render submit time, GPU frame time, upload bytes, draw/dispatch counts, and allocation counters.
- CPU evidence: first-party trace JSON, ETW/WPA where relevant, Intel VTune on Intel hosts, AMD uProf on AMD hosts.
- GPU evidence: PIX for D3D12, Nsight Graphics on NVIDIA, Radeon GPU Profiler on AMD, Intel GPA or VTune GPU analysis on Intel, and Vulkan validation/perf samples when Vulkan-specific behavior is under review.
- Claim rule: no "fast", "parallel", "async overlap", "GPU-ready", or "all cores used" claim without captured evidence and a narrow workload definition.

### 2. Treat Core Saturation As A Symptom, Not The Goal

The goal is stable frame time and throughput under the selected workload, not 100 percent utilization of every logical processor.

Good utilization means:

- The main thread is not blocked on avoidable locks, uploads, shader/pipeline work, or synchronous resource destruction.
- Worker threads execute coarse enough jobs to amortize scheduling overhead.
- Work stealing handles imbalance without turning every subsystem into shared mutable state.
- The renderer/RHI thread and GPU queues remain fed without creating hidden CPU/GPU sync points.
- Audio, IO, platform event handling, and editor UI have bounded work and do not starve frame-critical jobs.
- Driver/helper threads are not starved. Intel's Arc/Xe guidance explicitly warns that fully subscribing all CPU hardware threads with application work can reduce D3D12/Vulkan driver and background shader optimization performance.

### 3. Build A Deterministic Job Graph Before Broad Threading

Prefer phase-owned job graphs over arbitrary subsystem threads:

- Define frame phases such as input, fixed simulation, animation/skin prep, culling, scene extraction, render packet build, command recording, upload submission, GPU submit, and presentation.
- Make each phase expose read/write sets, dependencies, budget rows, and replay diagnostics.
- Split parallel work into ranges over SoA/chunked data, not into one job per tiny entity.
- Use per-worker local outputs and deterministic merge steps instead of global locked appenders.
- Keep job granularity adaptive: target enough work to occupy cores, but avoid thousands of tiny jobs when task overhead dominates.
- Reserve fixed-purpose threads only where the ownership boundary is real: platform/main, audio callback/mixer, IO, render/RHI submission, and long-running tool/editor services.

### 4. Make Memory Layout A First-Class Optimization Surface

Memory work should reduce traffic before replacing allocators:

- Use frame arenas for frame-temporary data with free-at-once lifetime.
- Use per-thread scratch arenas for job-local temporary data.
- Use pools/slabs for high-churn fixed-size objects and stable handles.
- Prefer SoA or chunked AoS for culling, transform, animation, particle, tile, and sprite hot paths.
- Split hot/cold data so tight loops load only fields they need.
- Align and pad cross-thread counters, queues, and per-worker state to avoid false sharing.
- Batch diagnostic and telemetry writes per thread, then merge once per frame.
- Track allocation bytes/counts by subsystem and lifetime class: frame, persistent CPU, package CPU, upload, transient GPU, resident GPU, and editor/tooling.

Do not introduce a process-wide mutable allocator singleton into public gameplay APIs. If an app-level allocator such as mimalloc, jemalloc, or a oneTBB allocator becomes attractive, gate it behind a measured host/application policy and keep engine containers explicit about ownership and lifetime.

### 5. Separate Memory Lifetimes Before Optimizing Allocators

Allocator design should start from lifetime and ownership, not from replacing `malloc` globally:

- Frame temporary memory: short-lived scratch created and reset once per frame or frame phase; no pointers may survive the owning frame boundary.
- Worker scratch memory: per-worker or per-job temporary storage that avoids cross-thread allocation contention and is reset by the worker owner.
- Persistent engine memory: long-lived subsystem state with explicit owner modules, shutdown order, leak diagnostics, and no hidden global mutable allocator state.
- Package resident CPU memory: cooked package records, decoded/runtime-ready CPU resources, safe-point unload state, and hot-reload generations.
- Upload/staging memory: CPU-visible staging buffers, upload rings, fence retirement, and failure diagnostics when ring capacity is exhausted.
- Resident GPU memory: backend-owned resources, budget rows, residency pressure, debug names, safe-point teardown, and no gameplay-facing native handles.
- Transient GPU memory: frame-graph-owned render targets, compute outputs, alias groups, lifetime intervals, and aliasing barriers.
- Readback memory: explicit latency and ownership for GPU-to-CPU data, never hidden synchronous waits in generic helpers.
- Editor/tooling memory: visible diagnostics and cache budgets that cannot silently become runtime requirements.

Ownership rules:

- Each allocation class needs an owner, lifetime, reset/release point, high-water mark, and diagnostic category.
- Cross-thread ownership transfer should use explicit queues or value rows; avoid freeing hot objects on a different worker without measurement.
- Per-thread caches must have a global reclamation path so idle workers do not hold unbounded memory.
- Budget overflow must fail closed or degrade predictably; it must not silently evict live resources behind public gameplay handles.
- Debug builds should detect leaked frame allocations, stale generation handles, use-after-safe-point rows, and missing resource teardown evidence.

Phase 2 memory class map:

| Memory class | Owner and reset/release boundary | Existing evidence | Missing before optimization claims |
| ------------ | -------------------------------- | ----------------- | ---------------------------------- |
| Frame temporary | Frame phase or frame graph owner; reset at the frame/safe-point boundary. | Frame graph transient lifetime rows, alias groups, aliasing barriers, and `RhiStats::transient_*` counters. | Frame allocation byte/count counters, high-water marks, leak-on-frame-exit diagnostics, and per-phase scratch budgets. |
| Worker scratch | Future worker/job owner; reset by worker, job, or deterministic merge point. | No production worker scratch API is claimed yet. | Job graph ownership rows, per-worker scratch high-water marks, false-sharing diagnostics, and reclamation rules for idle workers. |
| Persistent CPU | Owning engine module, host adapter, or editor subsystem; released by explicit owner shutdown. | C++ RAII/value ownership rules, `std::unique_ptr` guidance, module boundaries, and diagnostics rows where implemented. | Cross-module allocation category counters, leak summaries, persistent-cache budgets, and shutdown-order evidence. |
| Package resident CPU | Runtime package, resident mount set, and catalog cache; changed only at reviewed safe points. | `RuntimeResourceResidencyBudgetV2`, resident mount/cache safe-point results, `estimated_resident_bytes`, package resident byte counters, and budget-failure diagnostics. | Automatic/LRU policy evidence, broad/background streaming evidence, package cache high-water marks, and allocator enforcement. |
| Upload/staging | Host-owned RHI upload plan, upload ring, staging pool lease, and submitted fence retirement. | `RhiUploadStagingPlan`, `RhiUploadRing`, `RhiStagingBufferLease`, stale-generation diagnostics, uploaded-byte counters, submitted fences, and queue-wait evidence. | Runtime-wide staging-pool ownership, ring pressure high-water marks, reusable pool fragmentation counters, and measured async-overlap wins. |
| Resident GPU | Renderer/RHI device and backend-private resource owners; released through lifetime registry or safe-point teardown. | `RhiResourceLifetimeRegistry`, `RhiDeviceMemoryDiagnostics`, `GpuMemoryPolicyPlan`, backend memory evidence gates, and package-visible `gpu_memory_policy_*` counters. | GPU allocator/suballocation policy, residency enforcement, automatic eviction, Metal memory parity, and cross-vendor/backend performance proof. |
| Transient GPU | Frame graph and backend transient lease owner; retired after pass lifetime, fence, or alias-group interval. | `TransientResourceHandle`, transient texture alias groups, aliasing barrier counters, and D3D12/Vulkan placed/alias allocation counters. | Metal alias allocation evidence, content-preservation policy, alias fragmentation counters, and production render graph ownership. |
| Readback | RHI readback resource owner and caller-visible result rows; latency explicit at readback call sites. | CPU buffer readback commands, `bytes_read`, readback-domain buffers, and explicit copy/read counters. | Readback latency budgets, no-hidden-wait diagnostics, and profiler-visible readback stall evidence. |
| Editor/tooling | Editor/tool adapter or host tool; never a runtime requirement unless promoted through a separate contract. | Editor Resources diagnostics, capture handoff rows, trace import/export rows, and host-gated profiler/capture evidence. | Cache-size budgets, editor-only allocator diagnostics, and host-tool memory pressure rows that stay separate from generated-game runtime claims. |

Memory Diagnostics v1 now covers the core data model, summary API, and selected `sample_desktop_runtime_game --require-memory-diagnostics` package-visible `memory_diagnostics_*` counters for package resident CPU bytes, resident GPU bytes/budget, upload/staging bytes, and transient GPU allocation counts. Frame/Thread Scratch v1 then added frame temporary and worker scratch ownership APIs, first-party frame arenas, per-worker scratch arenas, high-water marks, false-sharing diagnostics, and selected package evidence. Job Scheduling Evidence v1 is now complete through Phase 3: dependency-free `MK_core` `JobWorkerTopologyRow`, `JobQueueCounterRow`, `JobSchedulingWorkItemRow`, `JobSchedulingExecutionOptions`, `JobSchedulingExecutionOrderRow`, `JobSchedulingExecutionEvidence`, `JobSchedulingDiagnosticsSummary`, `JobSchedulingDiagnosticsCode`, `JobSchedulingDiagnosticsStatus`, `summarize_job_scheduling_diagnostics`, `build_job_scheduling_execution_evidence`, and label helpers provide worker topology, bounded queue capacity/high-water, explicit dependency ids, deterministic execution order, worker-local output merge order, queue overflow, steal/wait, blocked dependency, dependency cycle, per-worker scratch `MemoryCounterRow` charges, scratch misuse, processor-group, NUMA, and batch-size evidence. The selected package path `sample_desktop_runtime_game --require-job-scheduling-evidence` now reports `job_scheduling_evidence_status=ready`, two workers, two queue rows, three submitted/completed/executed jobs, three deterministic merges, positive scratch byte/high-water counters, clean diagnostics, and zero native-thread/thread-pool/affinity/NUMA/SIMD/GPU-overlap side-effect flags. No selected optimization wave remains open; allocator replacement, all-core CPU scheduling, thread pools, work stealing execution, affinity pinning, NUMA placement, SIMD dispatch, GPU residency, and async-upload changes need new focused plans.

### 6. Use Topology Carefully

CPU topology tuning should be late-stage and measured:

- Detect logical processor count, core count, SMT siblings, processor groups, and NUMA nodes through platform adapters.
- Default to OS scheduling unless traces show migration, group, or NUMA problems.
- On Windows hosts above 64 logical processors, explicitly account for processor groups before claiming all-core scaling.
- Keep affinity optional and workload-specific; hard pinning can hurt heterogeneous Intel P-core/E-core systems, laptop power behavior, editor responsiveness, and host multitasking.
- Prefer first-touch allocation, job/data locality, and per-worker data ownership before manual NUMA placement.

### 7. Tune Intel And AMD CPUs Separately

Vendor CPU optimization should be a matrix of measured policies, not one generic x86 path.

Intel-specific rules:

- Use VTune Top-down Microarchitecture Analysis to classify hotspots before changing code: front-end bound, bad speculation, back-end bound, memory bound, or retiring.
- Treat Intel P-core/E-core systems as asymmetric. Do not assume all cores have equal throughput, latency, frequency, AVX behavior, cache behavior, or scheduling priority.
- Keep latency-critical frame work, render submission, audio, and platform work separable from background jobs so hybrid schedulers and explicit policies can be tested without broad refactors.
- Leave CPU headroom for graphics driver/helper threads, shader compilation, file IO, and editor/platform services instead of trying to occupy every logical processor with engine jobs.
- Prefer AVX2 as a practical portable x86 SIMD target; treat AVX-512 as a narrow runtime-dispatched path that needs host support, thermal/frequency evidence, and E-core compatibility checks.
- Use Intel Advisor or compiler vectorization reports before intrinsics, and use the Intel Intrinsics Guide only for kernels where the scalar/reference path and benchmark already exist.

AMD-specific rules:

- Use AMD uProf IBS, L3 cache counters, timechart, and memory analysis to distinguish instruction, cache, memory-bandwidth, branch, and synchronization stalls.
- Treat Zen CCX/CCD/L3 locality as relevant for large job graphs. Prefer job/data placement that keeps hot working sets near the cache group that owns them.
- Treat EPYC/Threadripper NUMA and NPS settings as host evidence, not assumptions. First-touch allocation and worker/data locality come before manual NUMA placement.
- Test SMT with workload classes. SMT can help latency-hidden or mixed workloads, but memory-bound or cache-thrashing jobs may need physical-core-biased scheduling.
- Use generation-specific Zen guidance for Zen3/Zen4/Zen5 SIMD, prefetch, branch, and memory behavior. Do not assume Intel AVX tuning transfers to AMD.
- AOCC/AOCL may be useful for selected math, memory, string, compression, or numeric kernels, but adopting them must stay optional and dependency-gated.

Shared policy:

- Runtime CPU dispatch should select only a small number of reviewed kernel variants, such as scalar, SSE/AVX baseline, AVX2, and optional AVX-512, instead of multiplying whole-engine code paths.
- Public APIs must not expose vendor-specific CPU choices. Vendor selection belongs in private dispatch, toolchain configuration, or host diagnostics.
- Every vendor-tuned path needs scalar/reference tests, correctness parity, benchmark evidence, and a documented fallback.

### 8. Make Pointer Semantics Explicit

Pointer choices are correctness tools first and optimization tools second. The engine should use them to express ownership and lifetime clearly:

- Prefer values, contiguous containers, indices, spans, and generational handles for hot runtime data.
- Use `std::unique_ptr` for exclusive heap ownership, polymorphic implementation objects, PIMPL, backend-private objects, and ownership transfer through factories.
- Use raw pointers only as non-owning observers. A nullable observer is `T*`; a required non-null object should normally be `T&` or a documented non-null precondition.
- Use `std::span<T>` or `std::span<const T>` for contiguous ranges instead of passing `T*` plus length by convention.
- Use `std::shared_ptr` only when multiple owners genuinely share lifetime. Do not use it for simple access, hot component references, render rows, job payloads, or resource handles that already have an owning registry.
- Use `std::weak_ptr` only to observe objects already managed by `std::shared_ptr`; do not use it as a general handle system.
- Avoid `new` and `delete` in ordinary engine code. Wrap C/native API ownership immediately in RAII types, `std::unique_ptr` with a deleter, or first-party backend-private owner classes.
- Avoid storing raw pointers into `std::vector` or pool elements across mutations unless the storage stability is part of the contract. Prefer indices/generations for long-lived references.
- Do not pass a smart pointer parameter unless the function participates in ownership. Use `T&`, `T*`, or `std::span` for pure observation.

Performance rules:

- Pointer chasing is a common frame-time cost. Avoid linked lists, deep object graphs, per-entity heap allocations, and virtual dispatch in measured hot loops unless traces show the cost is irrelevant.
- Keep hot loops over packed arrays or chunked SoA/AoS storage so hardware prefetch, cache lines, and SIMD have usable access patterns.
- Move ownership resolution to phase boundaries. Resolve handles into compact work arrays, then execute hot loops over those arrays.
- Async jobs must not capture raw pointers unless the owner outlives the job fence by construction. Prefer copied value rows, stable handles resolved inside a phase, or explicit ownership transfer.
- Resource systems should expose stable first-party handles, not backend-native pointers. Backend pointers may exist only behind private RHI/platform/editor owners.

### 9. Keep SIMD Kernel Boundaries Narrow

SIMD should be applied to stable, measured kernels:

- Keep scalar reference implementations for tests.
- Use compiler vectorization reports and disassembly/profiler evidence before hand-written intrinsics.
- Prefer structure and alignment changes that let the compiler vectorize.
- Runtime-dispatch only narrow kernels where host capability matters: AVX2, AVX-512, FMA, BMI, or platform-specific NEON on future non-x86 lanes.
- Do not bake Intel-only or AMD-only SIMD assumptions into public APIs.

### 10. GPU Optimization Belongs Behind RHI Contracts

Renderer and GPU memory work should follow explicit API practice:

- Classify resources by lifetime and usage: immutable, per-frame, streaming, readback, transient render target, transient compute, descriptor-visible, and CPU-visible upload.
- Budget and report GPU memory by heap/type, resident bytes, transient bytes, upload bytes, alias groups, and pressure state.
- Stream resources through upload rings/staging pools with fence retirement rather than helper-owned CPU waits.
- Batch barriers and resource transitions; avoid redundant read-to-read barriers and generic states/layouts where optimized states are known.
- Reuse command, descriptor, upload, and transient pools per frame/thread where backend rules allow it.
- Use frame graph ownership for pass dependencies, transient lifetime, aliasing, queue scheduling, and diagnostics.
- Prove async compute/copy overlap with queue timestamps and profiler captures; do not infer overlap from multiple queues existing.
- Keep vendor-specific guidance additive. NVIDIA, AMD, and Intel all support D3D12/Vulkan-style explicit optimization, but each vendor's profiler evidence, shader behavior, memory hierarchy, and queue behavior must be recorded separately.

CUDA/HIP should not become the core runtime path. For a game engine, first prefer backend-neutral compute through D3D12/Vulkan/Metal RHI abstractions for runtime rendering/simulation work. CUDA/HIP may be reconsidered only as optional tool-side acceleration or a private adapter for workloads that are demonstrably worth a vendor-specific path.

### 11. Cover High-Level Engine Bottlenecks

Low-level CPU/GPU tuning is not enough for a game engine. The following optimization surfaces should be treated as first-class work, each with evidence and fallback rules:

- Shader and pipeline caches: compile shaders offline where possible, persist PSO/pipeline caches per device/driver, bound shader permutation growth, and make runtime pipeline creation visible as stutter evidence.
- Asset conditioning: cook textures into GPU-native compressed formats with mip chains, choose KTX2/BasisU/BCn/ASTC-style formats by target, and keep source image decoding out of runtime hot paths.
- Geometry conditioning: optimize vertex/index ordering, overdraw, mesh compression, animation keyframes, LODs, and optional meshlet data during cook rather than during frame execution.
- Visibility and LOD: combine frustum culling, chunk/region culling, occlusion/Hi-Z where justified, distance/material/animation LOD, and draw/dispatch budgets before adding deeper GPU features.
- IO and streaming: batch package reads, separate IO, decompression, CPU residency, GPU upload, and resource activation; report streaming latency, decompression time, upload bytes, and pop-in/budget misses.
- Frame pacing and latency: track CPU frame time, GPU frame time, queue depth, present latency, input-to-present latency, vsync mode, pacing jitter, and dynamic quality changes separately from average FPS.
- Power and thermal behavior: record throttling, laptop power modes, sustained clocks, fan/noise-sensitive modes, and battery/mobile constraints where a host lane cares about them.
- Performance regression gates: keep representative benchmark scenes/packages, fixed settings, warm-up policy, repeat counts, confidence bands, and fail-closed budget thresholds for CI or operator-run perf lanes.

### 12. Make Optimization AI-Operable

AI can only create optimized games when optimization intent is represented as data the agent can read, validate, and repair:

- Put optimization budgets in `game.agent.json` or generated package evidence rows, not only in prose. Budgets should name frame time, CPU time, GPU time, draw/dispatch counts, upload bytes, resident memory, package bytes, streaming latency, shader/pipeline cache behavior, and unsupported claims.
- Tie every budget to a selected production recipe, host gate, validation recipe, and package-smoke counter. A 2D package budget does not prove 3D package readiness, and D3D12 evidence does not prove Vulkan, Metal, NVIDIA, AMD, or Intel parity.
- Let AI choose simpler content when evidence is missing: fewer active entities, lower draw rows, smaller texture sets, coarser simulation budgets, simpler shaders, precomputed/cooked data, and reviewed placeholder assets.
- Keep optimization remediation game-local. If a game needs engine-level job systems, allocator changes, shader cache systems, package streaming, renderer/RHI residency, or backend-specific profiler integration that the selected recipe cannot express, the agent must file an engine capability handoff.
- Preserve explainability. The agent should be able to answer which budget it optimized, which validation recipe proved it, which host/backend it applies to, and which performance claims remain unsupported.

### 13. Use Bitmask Algorithms Deliberately

Bit exhaustive enumeration is useful in a game engine only when the set is small, bounded, and visible to validation. Treat it as a precise tool, not as a general optimization answer.

Good uses:

- ECS/component matching, archetype signatures, feature flags, collision layers, visibility layers, input state, ability conditions, and compact permission sets.
- Tile adjacency masks, autotiling, local neighborhood rules, small tactical sets, board-game bitboards, and precomputed lookup tables.
- Offline/cook/tooling tasks such as shader/material feature review, asset pack choices, small LOD combinations, validation matrix expansion, and deterministic test generation.
- Runtime subset DP only for bounded gameplay problems with a documented maximum `N`, fixed iteration budget, and deterministic fallback.

Rules:

- Record the maximum set size `N`, mask width, worst-case iteration count, and whether the loop runs per frame, per load, per package cook, or only in tests.
- Prefer unsigned fixed-width masks (`std::uint32_t`, `std::uint64_t`) for small sets. Use multiword bitsets only when the size and iteration cost are explicitly budgeted.
- Use standard bit operations such as `std::popcount`, `std::countr_zero`, `std::countl_zero`, and `std::has_single_bit` before compiler or vendor intrinsics.
- Use submask enumeration only when the cost is bounded: `for (sub = mask; sub != 0; sub = (sub - 1) & mask)`.
- Avoid signed shifts, `1 << n` on `int`, shifting by the bit width or more, and assumptions that `N <= 63` remains true after content growth.
- Do not generate shader/material permutations by brute force unless every combination is required, cached, and budgeted. Prefer reviewed feature sets and explicit invalid-combination pruning.
- Keep per-frame exhaustive search small. If `N` grows, move work to cook time, use pruning, meet-in-the-middle, dynamic programming, cached tables, or an engine capability handoff.

Validation:

- Exhaustively test small masks against a scalar/reference implementation.
- Fuzz larger masks within documented limits.
- Report `N`, iteration count, table size, cache/memory footprint, branch count risk, `popcount`/bit-scan usage, determinism order, and overflow/invalid-shift guards.
- Treat missing bounds as unsupported for runtime use.

### 14. Keep Compiler And Link-Time Optimization Evidence-Based

Compiler flags can improve release performance, but they can also hide poor data layout, increase iteration time, or bias toward the wrong workload.

Rules:

- Keep debug, editor, validation, and release-performance presets distinct. Do not make PGO/LTO required for default development validation.
- Define a representative training workload before using PGO: selected package launch, scene load, warm-up, steady-state frame loop, streaming event, UI pass, and shutdown.
- Record compiler, linker, target architecture, optimization flags, profile collection command, profile-use command, binary size, startup time, p95/p99 frame time, and any changed crash/debug behavior.
- Keep PGO results host/toolchain-specific. MSVC PGO evidence does not prove Clang/GCC behavior, and one game/package trace does not prove every generated game.
- Prefer algorithm, memory layout, and renderer/RHI fixes before relying on PGO to recover performance.

### 15. Treat GPU-Driven Rendering As A Future Feature Gate

Work graphs, mesh shaders, indirect draws, descriptor-buffer/bindless models, argument buffers, and GPU culling can reduce CPU submission pressure, but they increase backend complexity and portability risk.

Rules:

- First prove the CPU bottleneck: command generation, draw sorting, descriptor updates, pipeline switches, culling, upload, or synchronization.
- Add a simpler fallback path for every advanced GPU-driven path.
- Gate by backend, device feature tier, shader model, driver/toolchain, and package recipe.
- Report draw/dispatch counts, command-list or command-buffer counts, descriptor updates, barrier count, GPU occupancy, GPU time, CPU submit time, and shader/pipeline cache behavior.
- Keep D3D12 Work Graphs, Vulkan descriptor buffers, Metal argument buffers, mesh shaders, and CUDA/HIP optional and private to backend/tooling adapters until broad evidence exists.

## Repository Optimization Candidates

The current codebase already exposes narrow candidates where this foundation can be applied after measurement:

- Diagnostics/profiling: use existing `MK_core` diagnostics, profile zones, counters, and Trace Event JSON as the baseline collection path before adding external tool recipes.
- Memory diagnostics: extend existing runtime/RHI/editor resource counters into allocation lifetime rows, high-water marks, budget pressure, stale-generation diagnostics, and safe-point teardown evidence before changing allocator policy.
- Pointer/handle audit: classify current raw pointers, smart pointers, resource handles, and backend-private native owners so hot loops and public APIs do not accidentally encode ownership transfer, pointer stability, or native-handle access.
- Intel/AMD CPU profiling matrix: collect representative traces on Intel hybrid, Intel server/workstation, AMD Ryzen, and AMD EPYC/Threadripper-class hosts before making broad scheduler, SIMD, or allocator claims.
- Runtime scheduler: reduce repeated scans and make fixed-step gameplay scheduling expose budget and work distribution evidence before adding worker execution.
- UI/editor retained models: index frequently queried ids and avoid full document rebuilds on every view pass where persistent model state can be retained safely.
- Sprite/tile rendering: move from adjacent-only batching and ordered temporary sets toward measured chunk-local draw planning, stable sort keys, and batch budget diagnostics.
- Animation/audio CPU kernels: evaluate CPU skinning, morph prep, and audio mixing as scalar-reference plus SIMD/data-layout candidates.
- Upload/ring/frame graph: measure interval sorting, staging reuse, command-list counts, queue waits, barriers, and fence retirement before claiming async upload or overlap wins.
- Shader/material pipeline: make shader permutation counts, compile/cache hits, PSO creation latency, pipeline cache validity, and runtime fallback paths visible before broad material quality work.
- Compiler optimization lane: evaluate MSVC/Clang PGO and link-time optimization only after a stable benchmark package exists, and keep the result separate from default validation/editor builds.
- GPU-driven rendering lane: evaluate indirect draw, meshlet/mesh shader, descriptor/bindless, D3D12 Work Graph, Vulkan descriptor-buffer, and Metal argument-buffer paths only after CPU submit or descriptor pressure is proven.
- Asset/package pipeline: measure cooked texture/mesh sizes, decode/transcode time, mip/LOD coverage, streaming latency, and package IO granularity before claiming loading or streaming improvements.
- Visibility/LOD/frame pacing: add package-visible budgets for visible objects, draw rows, animation rows, queue depth, present pacing, and dynamic quality decisions before subjective renderer-quality claims.
- Bitmask algorithms: audit component masks, layer masks, tile adjacency masks, shader feature masks, validation matrix expansion, and small AI/tactics subsets for explicit `N`, overflow guards, and runtime/offline placement.

## Implementation Plan Notes

Do not create a dated implementation plan for this analysis until optimization becomes the selected capability or milestone. When it is selected, the first plan should be a capability-level milestone such as `AI-Operable Performance Budget And Evidence v1`, not a broad "optimize everything" plan.

The plan should include:

- Goal: AI agents can generate games with explicit performance budgets and evidence rows, without claiming broad optimization.
- Context: current AI game development already uses recipes, `game.agent.json`, package smokes, quality rubrics, and engine capability handoffs.
- Constraints: no public native handles, no broad backend parity claims, no global allocator replacement, no unbounded per-frame exhaustive search, no vendor-specific proof reuse.
- Done when: selected 2D/3D generated package lanes can carry budget rows for frame, CPU, GPU, memory, draw/dispatch, upload, package size, shader/pipeline cache, streaming, and unsupported performance claims.
- Phase 1: add descriptor/schema rows for game-local optimization budgets and evidence references.
- Phase 2: expose package-visible counters for the smallest ready lane first, likely 2D package budgets plus bitmask/tile adjacency evidence.
- Phase 3: add trace/profiler artifact references and fail-closed host-gate rows without requiring profiler tools in default validation.
- Phase 4: add memory/pointer/bitmask audits as review-only surfaces before changing allocators or hot loops.
- Phase 5: add CPU/GPU vendor evidence matrices only after representative Intel, AMD, NVIDIA, and Intel GPU host traces exist.
- Validation: run focused unit/package checks for each phase, `check-json-contracts`, `check-agents`, and full `tools/validate.ps1` only when C++/runtime/schema/manifest contract changes require it.
- Agent-surface drift: update `docs/ai-game-development.md`, generated-game validation scenarios, `game.agent.json` schemas, manifest fragments, and static checks in the same slice when AI-operable contracts change.

## Suggested Work Waves

1. Performance Baseline v1: add reproducible benchmark scenes/packages, trace export recipes, subsystem counters, and p95/p99 frame budget reporting.
2. Memory Lifetime Taxonomy v1: define frame, worker scratch, persistent, package resident, upload, transient GPU, resident GPU, readback, and editor/tooling memory classes.
3. Memory Diagnostics v1: expose allocation counters, lifetime budgets, leak/stale-generation diagnostics, high-water marks, and budget pressure rows for each memory class.
4. Pointer And Handle Semantics Audit v1: classify owning, observing, nullable, stable, generation-checked, backend-private, and hot-loop pointer/handle usage before broad refactors.
5. Intel/AMD CPU Profiling Matrix v1: define representative Intel/AMD host classes, trace recipes, core topology fields, ISA dispatch rows, and evidence requirements.
6. Frame/Thread Scratch v1: add first-party frame arenas and per-worker scratch arenas behind explicit ownership APIs.
7. Job Graph v1: introduce deterministic frame-phase job planning with work ranges, dependency rows, worker-local outputs, and serial fallback.
8. CPU Hotpath Data Layout v1: apply SoA/chunk layout to culling, sprites/tiles, animation prep, and other measured hot loops.
9. ISA Dispatch Policy v1: add narrow scalar/SIMD kernel dispatch rules, fallback tests, and vendor evidence for selected hot kernels.
10. Shader And Pipeline Cache v1: add shader permutation budgets, offline compile/cache rows, PSO/pipeline cache evidence, and runtime-stutter diagnostics.
11. Asset Conditioning v1: add texture compression, mesh/animation compression, mip/LOD coverage, and cook-time optimization diagnostics.
12. Streaming IO v1: add package read batching, decompression, upload activation, latency, and budget-miss evidence.
13. Visibility LOD Frame Pacing v1: add visibility, LOD, queue-depth, present-pacing, latency, dynamic quality, and regression-budget rows.
14. Bitmask Algorithm Policy v1: add bounded mask-size rules, standard `<bit>` usage, subset-enumeration helpers, overflow guards, and runtime/offline placement guidance.
15. Renderer GPU Memory vNext: extend existing GPU memory diagnostics into budget enforcement, suballocation policy, aliasing telemetry, and profiler-visible debug names.
16. Parallel Command Recording v1: use per-thread command/descriptor pools and coarse command chunks where D3D12/Vulkan traces show CPU submit/build bottlenecks.
17. Optional GPU Compute Review v1: classify each compute candidate as RHI compute, offline tool acceleration, CUDA/HIP adapter candidate, or non-goal.
18. Release Compiler Optimization v1: add representative training workloads, MSVC/Clang PGO evidence, binary-size/startup checks, and fail-closed release-lane documentation without changing default validation.
19. GPU-Driven Rendering Feature Gate v1: evaluate indirect draw, mesh shader/meshlet, descriptor-buffer/bindless, D3D12 Work Graph, and Metal argument-buffer candidates with fallback paths and per-backend profiler evidence.

## Validation Rules

For any future implementation derived from this analysis:

- Add or update the smallest externally meaningful tests for new contracts.
- Use focused benchmarks and traces before and after the change.
- Report both speedup and regression risk: CPU time, GPU time, frame p95/p99, allocations, memory footprint, command lists, barriers, queue waits, and diagnostics.
- For memory changes, report allocation counts/bytes, high-water marks, peak resident bytes, fragmentation or unused-range evidence where available, cross-thread frees, arena reset counts, stale handles, leaks, budget-overflow behavior, upload-ring exhaustion, and GPU residency pressure.
- For pointer/handle changes, report ownership semantics, nullability, pointer stability, container invalidation behavior, job lifetime fences, raw `new`/`delete` absence, `shared_ptr` use justification, and public API/native-handle exposure checks.
- For Intel/AMD CPU changes, report exact CPU model, core topology, SMT state, processor groups, NUMA/NPS state where available, OS scheduler context, compiler and flags, selected ISA path, thermal/power throttling observations, profiler tool/version, relevant hardware counters, and whether the result is Intel-only, AMD-only, or cross-vendor.
- For shader/pipeline changes, report shader permutation counts, compile time, cache hit/miss behavior, PSO/pipeline creation latency, runtime creation count, driver/device cache keys, fallback behavior, and stutter evidence.
- For asset/streaming changes, report cooked asset sizes, compression format, decode/transcode/decompression time, IO request counts/sizes, upload bytes, activation latency, mip/LOD coverage, and budget misses.
- For frame pacing changes, report queue depth, CPU/GPU frame overlap, present timing, input latency proxy where available, p95/p99 pacing jitter, and dynamic quality transitions.
- For bitmask algorithm changes, report maximum `N`, mask width, worst-case iteration count, table size, runtime/offline placement, standard bit API usage, invalid-shift guards, deterministic order, and reference-test coverage.
- For PGO/LTO changes, report training workload, compiler/linker, profile collection/use commands, binary size, startup time, p95/p99 frame time, representative package list, and debugability/regression notes.
- For GPU-driven rendering changes, report feature tier, fallback path, draw/dispatch count, CPU submit time, command buffer/list count, descriptor update count, barrier count, GPU time, shader/pipeline cache state, and per-vendor profiler evidence.
- Keep vendor-specific evidence vendor-specific. Intel Arc/Xe evidence does not prove NVIDIA or AMD behavior, NVIDIA evidence does not prove RDNA or Intel behavior, AMD RDNA evidence does not prove NVIDIA or Intel behavior, and D3D12 evidence does not prove Vulkan or Metal behavior.
- Keep failure modes fail-closed: missing profiler tool, missing host capability, missing GPU timestamp support, missing processor topology evidence, or missing validation recipe must produce a host-gated or unsupported row, not a ready claim.

## Non-Goals

- "Use all cores" as a standalone success criterion.
- Blind thread pinning or manual NUMA placement before traces prove the need.
- Replacing the standard allocator globally without a measured application policy.
- Exposing allocator, thread, queue, fence, device, or native backend handles to gameplay APIs.
- Using `std::shared_ptr` as the default engine reference type.
- Treating raw pointers as owning pointers in public engine APIs.
- Storing pointer-heavy object graphs in measured per-frame hot loops when contiguous handles or rows would provide the same behavior.
- Unbounded bit exhaustive search in per-frame runtime paths.
- Brute-force shader/material permutation generation without pruning, cache policy, and package evidence.
- Treating CUDA or HIP as required runtime dependencies.
- Claiming async compute, async upload, or multi-queue overlap without calibrated timestamp/profiler evidence.
- Optimizing by copying vendor sample code or unlicensed snippets into the repository.
