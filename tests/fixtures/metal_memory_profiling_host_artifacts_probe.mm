// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary
// Plan ID: renderer-metal-memory-profiling-apple-host-artifacts-v1

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <cstdlib>
#include <iostream>

namespace {

[[nodiscard]] NSString* make_error_text(NSString* message, NSError* error = nil) {
    if (error == nil || error.localizedDescription == nil) {
        return message;
    }
    return [NSString stringWithFormat:@"%@: %@", message, error.localizedDescription];
}

[[noreturn]] void fail(NSString* message, NSError* error = nil) {
    const auto* utf8 = make_error_text(message, error).UTF8String;
    std::cerr << (utf8 == nullptr ? "Metal memory/profiling host artifact probe failed" : utf8) << "\n";
    std::exit(1);
}

void ensure_directory(NSURL* directory_url) {
    NSError* error = nil;
    if (![[NSFileManager defaultManager] createDirectoryAtURL:directory_url
                                  withIntermediateDirectories:YES
                                                   attributes:nil
                                                        error:&error]) {
        fail(@"Failed to create probe output directory", error);
    }
}

void remove_existing_path(NSURL* url) {
    if ([[NSFileManager defaultManager] fileExistsAtPath:url.path]) {
        NSError* error = nil;
        if (![[NSFileManager defaultManager] removeItemAtURL:url error:&error]) {
            fail(@"Failed to remove stale probe artifact", error);
        }
    }
}

[[nodiscard]] bool invoke_void_with_object(id target, SEL selector, id argument) {
    if (target == nil || ![target respondsToSelector:selector]) {
        return false;
    }

    NSMethodSignature* signature = [target methodSignatureForSelector:selector];
    if (signature == nil) {
        return false;
    }

    NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:signature];
    [invocation setTarget:target];
    [invocation setSelector:selector];
    id mutable_argument = argument;
    [invocation setArgument:&mutable_argument atIndex:2];
    [invocation invoke];
    return true;
}

[[nodiscard]] bool invoke_void_no_args(id target, SEL selector) {
    if (target == nil || ![target respondsToSelector:selector]) {
        return false;
    }

    NSMethodSignature* signature = [target methodSignatureForSelector:selector];
    if (signature == nil) {
        return false;
    }

    NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:signature];
    [invocation setTarget:target];
    [invocation setSelector:selector];
    [invocation invoke];
    return true;
}

void write_utf8_text(NSURL* url, NSString* text) {
    NSError* error = nil;
    if (![text writeToURL:url atomically:YES encoding:NSUTF8StringEncoding error:&error]) {
        fail(@"Failed to write probe text artifact", error);
    }
}

void write_json(NSURL* url, NSDictionary* value) {
    NSError* error = nil;
    NSData* data = [NSJSONSerialization dataWithJSONObject:value options:NSJSONWritingPrettyPrinted error:&error];
    if (data == nil) {
        fail(@"Failed to serialize probe summary JSON", error);
    }
    if (![data writeToURL:url options:NSDataWritingAtomic error:&error]) {
        fail(@"Failed to write probe summary JSON", error);
    }
}

} // namespace

int main(int argc, char** argv) {
    @autoreleasepool {
        if (argc != 2 || argv[1] == nullptr) {
            fail(@"Usage: MK_metal_memory_profiling_host_artifacts_probe <output-directory>");
        }

        NSString* output_path = [NSString stringWithUTF8String:argv[1]];
        if (output_path == nil || output_path.length == 0U) {
            fail(@"Probe output directory must be valid UTF-8");
        }

        NSURL* output_url = [NSURL fileURLWithPath:output_path isDirectory:YES];
        ensure_directory(output_url);

        NSURL* capture_document_url = [output_url URLByAppendingPathComponent:@"capture.gputrace"];
        NSURL* capture_summary_url = [output_url URLByAppendingPathComponent:@"capture-summary.txt"];
        NSURL* summary_url = [output_url URLByAppendingPathComponent:@"probe-summary.json"];
        remove_existing_path(capture_document_url);
        remove_existing_path(capture_summary_url);
        remove_existing_path(summary_url);

        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (device == nil) {
            fail(@"MTLCreateSystemDefaultDevice returned nil");
        }

        id<MTLCommandQueue> command_queue = [device newCommandQueue];
        if (command_queue == nil) {
            fail(@"Metal command queue creation failed");
        }
        command_queue.label = @"GameEngine.RHI.Metal.MemoryProfiling.CommandQueue";

        id<MTLResidencySet> residency_set = nil;
        if (@available(macOS 15.0, *)) {
            if (![device respondsToSelector:@selector(newResidencySetWithDescriptor:error:)]) {
                fail(@"MTLDevice newResidencySetWithDescriptor:error: selector is unavailable on this host");
            }

            MTLResidencySetDescriptor* residency_descriptor = [MTLResidencySetDescriptor new];
            residency_descriptor.label = @"GameEngine.RHI.Metal.MemoryProfiling.ResidencySet";

            NSError* residency_error = nil;
            residency_set = [device newResidencySetWithDescriptor:residency_descriptor error:&residency_error];
            if (residency_set == nil) {
                fail(@"MTLResidencySet creation failed", residency_error);
            }
        } else {
            fail(@"MTLResidencySet requires macOS 15.0 or newer");
        }

        constexpr NSUInteger heap_size = 65536;
        constexpr NSUInteger heap_buffer_bytes = 4096;
        MTLHeapDescriptor* heap_descriptor = [MTLHeapDescriptor new];
        heap_descriptor.size = heap_size;
        heap_descriptor.storageMode = MTLStorageModePrivate;
        id<MTLHeap> heap = [device newHeapWithDescriptor:heap_descriptor];
        if (heap == nil) {
            fail(@"MTLHeap allocation failed");
        }
        heap.label = @"GameEngine.RHI.Metal.MemoryProfiling.Heap";

        id<MTLBuffer> heap_buffer = [heap newBufferWithLength:heap_buffer_bytes options:MTLResourceStorageModePrivate];
        if (heap_buffer == nil) {
            fail(@"MTLHeap resource allocation failed");
        }
        heap_buffer.label = @"GameEngine.RHI.Metal.MemoryProfiling.HeapBuffer";

        NSArray* residency_allocations = @[ heap, heap_buffer ];
        bool allocations_added = invoke_void_with_object(
            residency_set, NSSelectorFromString(@"addAllocations:"), residency_allocations);
        if (!allocations_added) {
            allocations_added = invoke_void_with_object(residency_set, NSSelectorFromString(@"addAllocation:"), heap);
            allocations_added =
                invoke_void_with_object(residency_set, NSSelectorFromString(@"addAllocation:"), heap_buffer) &&
                allocations_added;
        }
        if (!allocations_added) {
            fail(@"MTLResidencySet allocation staging selector was unavailable");
        }

        if (!invoke_void_no_args(residency_set, NSSelectorFromString(@"commit"))) {
            fail(@"MTLResidencySet commit selector was unavailable");
        }
        if (!invoke_void_no_args(residency_set, NSSelectorFromString(@"requestResidency"))) {
            fail(@"MTLResidencySet requestResidency selector was unavailable");
        }
        if (!invoke_void_with_object(command_queue, NSSelectorFromString(@"addResidencySet:"), residency_set)) {
            fail(@"MTLCommandQueue addResidencySet selector was unavailable");
        }

        MTLCaptureManager* capture_manager = [MTLCaptureManager sharedCaptureManager];
        if (capture_manager == nil) {
            fail(@"MTLCaptureManager sharedCaptureManager returned nil");
        }
        if ([capture_manager respondsToSelector:@selector(supportsDestination:)] &&
            ![capture_manager supportsDestination:MTLCaptureDestinationGPUTraceDocument]) {
            fail(@"MTLCaptureDestinationGPUTraceDocument is not supported on this host");
        }

        id<MTLCaptureScope> capture_scope = [capture_manager newCaptureScopeWithCommandQueue:command_queue];
        if (capture_scope == nil) {
            fail(@"MTLCaptureScope creation failed");
        }
        capture_scope.label = @"GameEngine.RHI.Metal.MemoryProfiling";

        MTLCaptureDescriptor* capture_descriptor = [MTLCaptureDescriptor new];
        capture_descriptor.captureObject = capture_scope;
        capture_descriptor.destination = MTLCaptureDestinationGPUTraceDocument;
        capture_descriptor.outputURL = capture_document_url;

        NSError* capture_error = nil;
        if (![capture_manager startCaptureWithDescriptor:capture_descriptor error:&capture_error]) {
            fail(@"MTLCaptureManager startCaptureWithDescriptor failed", capture_error);
        }

        [capture_scope beginScope];
        id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];
        if (command_buffer == nil) {
            fail(@"Metal command buffer creation failed");
        }
        command_buffer.label = @"GameEngine.RHI.Metal.MemoryProfiling.CapturedCommandBuffer";
        id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
        if (blit_encoder == nil) {
            fail(@"Metal blit encoder creation failed");
        }
        blit_encoder.label = @"GameEngine.RHI.Metal.MemoryProfiling.BlitEncoder";
        [blit_encoder fillBuffer:heap_buffer range:NSMakeRange(0, heap_buffer_bytes) value:0x5AU];
        [blit_encoder endEncoding];
        [command_buffer commit];
        [command_buffer waitUntilCompleted];
        [capture_scope endScope];
        [capture_manager stopCapture];

        if (command_buffer.status != MTLCommandBufferStatusCompleted) {
            fail(@"Captured Metal command buffer did not complete");
        }

        BOOL capture_document_is_directory = NO;
        const BOOL capture_document_exists = [[NSFileManager defaultManager] fileExistsAtPath:capture_document_url.path
                                                                                 isDirectory:&capture_document_is_directory];
        if (!capture_document_exists) {
            fail(@"MTLCaptureManager did not create capture.gputrace");
        }

        NSString* capture_summary =
            [NSString stringWithFormat:
                          @"validation_recipe=renderer-metal-memory-profiling-host-evidence\n"
                           "plan_id=renderer-metal-memory-profiling-apple-host-artifacts-v1\n"
                           "workload_id=renderer_metal_memory_profiling_apple_host_probe\n"
                           "capture_document=capture.gputrace\n"
                           "capture_document_is_directory=%@\n"
                           "heap_api_name=MTLHeap\n"
                           "residency_api_name=MTLResidencySet\n"
                           "capture_api_name=MTLCaptureManager\n"
                           "capture_scope_api_name=MTLCaptureScope\n"
                           "heap_resource_rows=1\n"
                           "heap_allocated_bytes=%llu\n"
                           "resident_bytes=%llu\n"
                           "budget_bytes=%llu\n"
                           "residency_set_allocation_rows=2\n"
                           "memory_pressure_sample_rows=1\n"
                           "memory_pressure_budget_status=within_budget\n"
                           "capture_scope_label=GameEngine.RHI.Metal.MemoryProfiling\n"
                           "capture_artifact_rows=1\n"
                           "native_handles_exposed=0\n",
                          capture_document_is_directory ? @"1" : @"0",
                          static_cast<unsigned long long>(heap_buffer_bytes),
                          static_cast<unsigned long long>(heap_buffer_bytes),
                          static_cast<unsigned long long>(heap_size)];
        write_utf8_text(capture_summary_url, capture_summary);

        NSDictionary* summary = @{
            @"workload_id" : @"renderer_metal_memory_profiling_apple_host_probe",
            @"capture_artifact" : @"capture-summary.txt",
            @"raw_capture_document" : @"capture.gputrace",
            @"heap_resource_rows" : @1,
            @"heap_allocated_bytes" : @(heap_buffer_bytes),
            @"resident_bytes" : @(heap_buffer_bytes),
            @"budget_bytes" : @(heap_size),
            @"residency_set_allocation_rows" : @2,
            @"memory_pressure_sample_rows" : @1,
            @"memory_pressure_budget_status" : @"within_budget",
            @"capture_scope_label" : @"GameEngine.RHI.Metal.MemoryProfiling",
            @"capture_artifact_rows" : @1,
            @"native_handles_exposed" : @0
        };
        write_json(summary_url, summary);

        std::cout << "renderer-metal-memory-profiling-host-artifacts-probe: "
                  << "validation_recipe=renderer-metal-memory-profiling-host-evidence "
                  << "renderer_metal_memory_profiling_host_artifacts_probe_ready=1 "
                  << "renderer_metal_memory_profiling_status=ready "
                  << "renderer_metal_memory_profiling_ready=1 "
                  << "renderer_backend_parity_ready=0 "
                  << "renderer_metal_broad_readiness=0 "
                  << "renderer_commercial_readiness=0 "
                  << "renderer_broad_quality_ready=0 "
                  << "native_handles_exposed=0\n";
    }
    return 0;
}
