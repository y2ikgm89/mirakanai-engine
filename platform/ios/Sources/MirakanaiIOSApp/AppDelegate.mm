// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <UIKit/UIKit.h>

#include "mirakana/platform/mobile.hpp"

#include <cstdint>
#include <string>

@interface MirakanaiMetalView : UIView
@end

@implementation MirakanaiMetalView

+ (Class)layerClass {
    return [CAMetalLayer class];
}

@end

namespace {

std::string utf8_string(NSString* value) {
    if (value == nil) {
        return {};
    }

    const char* text = [value UTF8String];
    return text == nullptr ? std::string{} : std::string{text};
}

NSString* path_for_directory(NSSearchPathDirectory directory) {
    NSArray<NSURL*>* urls = [[NSFileManager defaultManager] URLsForDirectory:directory inDomains:NSUserDomainMask];
    NSURL* url = [urls firstObject];
    return url == nil ? @"" : [url path];
}

void create_directory_if_needed(NSString* path) {
    if ([path length] == 0U) {
        return;
    }

    NSError* error = nil;
    [[NSFileManager defaultManager] createDirectoryAtPath:path
                              withIntermediateDirectories:YES
                                               attributes:nil
                                                    error:&error];
}

bool check_ios_metal_feature_set(id<MTLDevice> device) {
    if (device == nil) {
        return false;
    }

    if (@available(iOS 13.0, *)) {
        (void)[device supportsFamily:MTLGPUFamilyApple1];
        return true;
    }

    (void)[device supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily1_v1];
    return true;
}

bool write_ios_metal_evidence(NSString* cache_path,
                              bool feature_set_checked,
                              bool command_queue_ready,
                              bool pipeline_ready,
                              bool command_buffer_ready,
                              bool readback_ready) {
    if ([cache_path length] == 0U) {
        return false;
    }

    NSString* evidence_path = [cache_path stringByAppendingPathComponent:@"mirakanai_ios_metal_evidence.txt"];
    NSString* evidence = [NSString
        stringWithFormat:@"ios_metal_feature_set_checked=%d\n"
                         "ios_metal_command_queue_ready=%d\n"
                         "ios_metal_pipeline_ready=%d\n"
                         "ios_metal_command_buffer_ready=%d\n"
                         "ios_metal_readback_ready=%d\n",
                         feature_set_checked ? 1 : 0,
                         command_queue_ready ? 1 : 0,
                         pipeline_ready ? 1 : 0,
                         command_buffer_ready ? 1 : 0,
                         readback_ready ? 1 : 0];
    NSError* error = nil;
    return [evidence writeToFile:evidence_path atomically:YES encoding:NSUTF8StringEncoding error:&error] == YES;
}

bool run_ios_metal_evidence(id<MTLDevice> device) {
    NSString* cache_path = path_for_directory(NSCachesDirectory);
    create_directory_if_needed(cache_path);

    bool feature_set_checked = check_ios_metal_feature_set(device);
    bool command_queue_ready = false;
    bool pipeline_ready = false;
    bool command_buffer_ready = false;
    bool readback_ready = false;

    id<MTLCommandQueue> command_queue = [device newCommandQueue];
    command_queue_ready = command_queue != nil;

    id<MTLLibrary> library = device == nil ? nil : [device newDefaultLibrary];
    id<MTLFunction> function = library == nil ? nil : [library newFunctionWithName:@"mirakanai_ios_evidence_kernel"];
    NSError* pipeline_error = nil;
    id<MTLComputePipelineState> pipeline =
        function == nil ? nil : [device newComputePipelineStateWithFunction:function error:&pipeline_error];
    pipeline_ready = pipeline != nil;

    id<MTLBuffer> output_buffer =
        device == nil ? nil : [device newBufferWithLength:sizeof(std::uint32_t) options:MTLResourceStorageModeShared];
    if (output_buffer != nil) {
        std::uint32_t* contents = static_cast<std::uint32_t*>([output_buffer contents]);
        if (contents != nullptr) {
            *contents = 0U;
        }
    }

    id<MTLCommandBuffer> command_buffer = command_queue == nil ? nil : [command_queue commandBuffer];
    command_buffer_ready = command_buffer != nil;

    id<MTLComputeCommandEncoder> encoder =
        command_buffer == nil ? nil : [command_buffer computeCommandEncoder];
    if (encoder != nil && pipeline != nil && output_buffer != nil) {
        [encoder setComputePipelineState:pipeline];
        [encoder setBuffer:output_buffer offset:0 atIndex:0];
        [encoder dispatchThreads:MTLSizeMake(1U, 1U, 1U) threadsPerThreadgroup:MTLSizeMake(1U, 1U, 1U)];
        [encoder endEncoding];
        [command_buffer commit];
        [command_buffer waitUntilCompleted];

        const std::uint32_t* contents = static_cast<const std::uint32_t*>([output_buffer contents]);
        readback_ready = [command_buffer status] == MTLCommandBufferStatusCompleted && contents != nullptr &&
                         *contents == 0x4D4B494FU;
    }

    write_ios_metal_evidence(cache_path,
                             feature_set_checked,
                             command_queue_ready,
                             pipeline_ready,
                             command_buffer_ready,
                             readback_ready);
    return feature_set_checked && command_queue_ready && pipeline_ready && command_buffer_ready && readback_ready;
}

mirakana::MobileStorageRoots make_ios_storage_roots() {
    NSString* save = path_for_directory(NSApplicationSupportDirectory);
    NSString* cache = path_for_directory(NSCachesDirectory);
    NSString* shared = path_for_directory(NSDocumentDirectory);

    create_directory_if_needed(save);
    create_directory_if_needed(cache);
    create_directory_if_needed(shared);

    return mirakana::MobileStorageRoots{
        utf8_string(save),
        utf8_string(cache),
        utf8_string(shared),
    };
}

} // namespace

@interface MirakanaiAppDelegate : UIResponder <UIApplicationDelegate>
@property(strong, nonatomic) UIWindow* window;
@end

@implementation MirakanaiAppDelegate {
    mirakana::VirtualLifecycle lifecycle_;
    mirakana::MobileStorageRoots storage_roots_;
}

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions {
    (void)application;
    (void)launchOptions;
    storage_roots_ = make_ios_storage_roots();
    if (!mirakana::is_valid_mobile_storage_roots(storage_roots_)) {
        return NO;
    }

    mirakana::push_mobile_lifecycle_event(lifecycle_, mirakana::MobileLifecycleEventKind::started);
    mirakana::push_mobile_lifecycle_event(lifecycle_, mirakana::MobileLifecycleEventKind::resumed);

    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    UIViewController* viewController = [[UIViewController alloc] init];
    MirakanaiMetalView* metalView = [[MirakanaiMetalView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    CAMetalLayer* metalLayer = (CAMetalLayer*)[metalView layer];
    metalLayer.device = MTLCreateSystemDefaultDevice();
    if (metalLayer.device == nil) {
        return NO;
    }
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.framebufferOnly = YES;
    metalLayer.contentsScale = [[UIScreen mainScreen] scale];
    if (!run_ios_metal_evidence(metalLayer.device)) {
        return NO;
    }
    viewController.view = metalView;
    self.window.rootViewController = viewController;
    [self.window makeKeyAndVisible];
    return YES;
}

- (void)applicationWillResignActive:(UIApplication*)application {
    (void)application;
    mirakana::push_mobile_lifecycle_event(lifecycle_, mirakana::MobileLifecycleEventKind::paused);
}

- (void)applicationDidEnterBackground:(UIApplication*)application {
    (void)application;
    mirakana::push_mobile_lifecycle_event(lifecycle_, mirakana::MobileLifecycleEventKind::stopped);
}

- (void)applicationWillTerminate:(UIApplication*)application {
    (void)application;
    mirakana::push_mobile_lifecycle_event(lifecycle_, mirakana::MobileLifecycleEventKind::destroyed);
}

@end

int main(int argc, char* argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([MirakanaiAppDelegate class]));
    }
}
