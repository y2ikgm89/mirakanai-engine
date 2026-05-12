// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <UIKit/UIKit.h>

#include "mirakana/platform/mobile.hpp"

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
