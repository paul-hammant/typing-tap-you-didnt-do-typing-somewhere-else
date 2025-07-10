#include "HIDReader.h"
#include "MultitouchSupport.h"
#include <iostream>
#include <chrono>
#include <dlfcn.h>

HIDReader::HIDReader() : hidManager(nullptr), eventTap(nullptr), tapRunLoopSource(nullptr), 
                         running(false), touchDevices(nullptr) {
    
    // Create HID Manager
    hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!hidManager) {
        throw std::runtime_error("Failed to create IOHIDManager");
    }
    
    // Set up device matching for mouse and pointer devices
    CFMutableDictionaryRef matchingDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                                                    &kCFTypeDictionaryKeyCallBacks,
                                                                    &kCFTypeDictionaryValueCallBacks);
    
    // Match mouse devices
    int usagePage = kHIDPage_GenericDesktop;
    int usage = kHIDUsage_GD_Mouse;
    CFNumberRef usagePageRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usagePage);
    CFNumberRef usageRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
    
    CFDictionarySetValue(matchingDict, CFSTR(kIOHIDDeviceUsagePageKey), usagePageRef);
    CFDictionarySetValue(matchingDict, CFSTR(kIOHIDDeviceUsageKey), usageRef);
    
    IOHIDManagerSetDeviceMatching(hidManager, matchingDict);
    
    CFRelease(usagePageRef);
    CFRelease(usageRef);
    CFRelease(matchingDict);
    
    // Register input callback
    IOHIDManagerRegisterInputValueCallback(hidManager, hidInputValueCallback, this);
}

HIDReader::~HIDReader() {
    stop();
    if (hidManager) {
        CFRelease(hidManager);
    }
}

void HIDReader::start() {
    if (running) {
        return;
    }
    
    running = true;
    
    // Schedule HID manager with run loop
    IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOReturn result = IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone);
    if (result != kIOReturnSuccess) {
        std::cerr << "Failed to open IOHIDManager: " << result << std::endl;
        return;
    }
    
    // Create event tap for system-wide mouse events
    CGEventMask eventMask = CGEventMaskBit(kCGEventMouseMoved) |
                           CGEventMaskBit(kCGEventLeftMouseDown) |
                           CGEventMaskBit(kCGEventLeftMouseUp) |
                           CGEventMaskBit(kCGEventRightMouseDown) |
                           CGEventMaskBit(kCGEventRightMouseUp) |
                           CGEventMaskBit(kCGEventOtherMouseDown) |
                           CGEventMaskBit(kCGEventOtherMouseUp) |
                           CGEventMaskBit(kCGEventScrollWheel) |
                           CGEventMaskBit(kCGEventOtherMouseDragged) |
                           CGEventMaskBit(kCGEventLeftMouseDragged) |
                           CGEventMaskBit(kCGEventRightMouseDragged);
    
    eventTap = CGEventTapCreate(kCGHIDEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly,
                               eventMask, eventTapCallback, this);
    
    if (eventTap) {
        tapRunLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
        CFRunLoopAddSource(CFRunLoopGetCurrent(), tapRunLoopSource, kCFRunLoopCommonModes);
        CGEventTapEnable(eventTap, true);
    } else {
        std::cerr << "Failed to create event tap. Check accessibility permissions." << std::endl;
    }
    
    // Setup multitouch support
    setupMultitouchSupport();
}

void HIDReader::stop() {
    if (!running) {
        return;
    }
    
    running = false;
    
    // Cleanup multitouch
    cleanupMultitouchSupport();
    
    // Cleanup event tap
    if (eventTap) {
        CGEventTapEnable(eventTap, false);
        if (tapRunLoopSource) {
            CFRunLoopRemoveSource(CFRunLoopGetCurrent(), tapRunLoopSource, kCFRunLoopCommonModes);
            CFRelease(tapRunLoopSource);
            tapRunLoopSource = nullptr;
        }
        CFRelease(eventTap);
        eventTap = nullptr;
    }
    
    // Cleanup HID manager
    if (hidManager) {
        IOHIDManagerUnscheduleFromRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        IOHIDManagerClose(hidManager, kIOHIDOptionsTypeNone);
    }
}

void HIDReader::setCallback(std::function<void(const PointerEvent&)> cb) {
    callback = cb;
}

void HIDReader::setupMultitouchSupport() {
    // Try to load MultitouchSupport framework dynamically
    void *handle = dlopen("/System/Library/PrivateFrameworks/MultitouchSupport.framework/MultitouchSupport", RTLD_LAZY);
    if (!handle) {
        std::cerr << "Failed to load MultitouchSupport framework" << std::endl;
        return;
    }
    
    // Get function pointers
    typedef CFMutableArrayRef (*MTDeviceCreateListFunc)(void);
    typedef void (*MTRegisterContactFrameCallbackFunc)(void*, void*);
    typedef void (*MTDeviceStartFunc)(void*, int);
    
    MTDeviceCreateListFunc MTDeviceCreateList = (MTDeviceCreateListFunc)dlsym(handle, "MTDeviceCreateList");
    MTRegisterContactFrameCallbackFunc MTRegisterContactFrameCallback = (MTRegisterContactFrameCallbackFunc)dlsym(handle, "MTRegisterContactFrameCallback");
    MTDeviceStartFunc MTDeviceStart = (MTDeviceStartFunc)dlsym(handle, "MTDeviceStart");
    
    if (!MTDeviceCreateList || !MTRegisterContactFrameCallback || !MTDeviceStart) {
        std::cerr << "Failed to get MultitouchSupport function pointers" << std::endl;
        dlclose(handle);
        return;
    }
    
    touchDevices = MTDeviceCreateList();
    if (touchDevices) {
        CFIndex deviceCount = CFArrayGetCount(touchDevices);
        for (CFIndex i = 0; i < deviceCount; i++) {
            void *device = (void*)CFArrayGetValueAtIndex(touchDevices, i);
            MTRegisterContactFrameCallback(device, (void*)touchCallback);
            MTDeviceStart(device, 0);
        }
    }
}

void HIDReader::cleanupMultitouchSupport() {
    if (touchDevices) {
        void *handle = dlopen("/System/Library/PrivateFrameworks/MultitouchSupport.framework/MultitouchSupport", RTLD_LAZY);
        if (handle) {
            typedef void (*MTDeviceStopFunc)(void*);
            typedef void (*MTDeviceReleaseFunc)(void*);
            
            MTDeviceStopFunc MTDeviceStop = (MTDeviceStopFunc)dlsym(handle, "MTDeviceStop");
            MTDeviceReleaseFunc MTDeviceRelease = (MTDeviceReleaseFunc)dlsym(handle, "MTDeviceRelease");
            
            if (MTDeviceStop && MTDeviceRelease) {
                CFIndex deviceCount = CFArrayGetCount(touchDevices);
                for (CFIndex i = 0; i < deviceCount; i++) {
                    void *device = (void*)CFArrayGetValueAtIndex(touchDevices, i);
                    MTDeviceStop(device);
                    MTDeviceRelease(device);
                }
            }
            dlclose(handle);
        }
        
        CFRelease(touchDevices);
        touchDevices = nullptr;
    }
}

void HIDReader::hidInputValueCallback(void *context, IOReturn result, void *sender, IOHIDValueRef value) {
    HIDReader *reader = static_cast<HIDReader*>(context);
    if (reader && reader->running) {
        reader->handleHIDValue(value);
    }
}

CGEventRef HIDReader::eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
    HIDReader *reader = static_cast<HIDReader*>(refcon);
    if (reader && reader->running) {
        reader->handleCGEvent(type, event);
    }
    return event; // Pass through the event
}

void HIDReader::touchCallback(void *device, void *touches, int numTouches, double timestamp, int frame) {
    // This is a static callback, we need to find the instance
    // For simplicity, we'll use a global pointer or thread-local storage
    // In a real implementation, you'd want a better way to get the instance
}

void HIDReader::handleHIDValue(IOHIDValueRef value) {
    if (!callback) {
        return;
    }
    
    IOHIDElementRef element = IOHIDValueGetElement(value);
    uint32_t usage = IOHIDElementGetUsage(element);
    uint32_t usagePage = IOHIDElementGetUsagePage(element);
    CFIndex integerValue = IOHIDValueGetIntegerValue(value);
    
    PointerEvent pe = {};
    pe.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // Handle different HID usages
    if (usagePage == kHIDPage_GenericDesktop) {
        switch (usage) {
            case kHIDUsage_GD_X:
                pe.type = PointerEvent::Move;
                pe.x = static_cast<int>(integerValue);
                callback(pe);
                break;
            case kHIDUsage_GD_Y:
                pe.type = PointerEvent::Move;
                pe.y = static_cast<int>(integerValue);
                callback(pe);
                break;
        }
    } else if (usagePage == kHIDPage_Button) {
        pe.type = (integerValue > 0) ? PointerEvent::ButtonDown : PointerEvent::ButtonUp;
        pe.button = static_cast<int>(usage);
        callback(pe);
    }
}

void HIDReader::handleCGEvent(CGEventType type, CGEventRef event) {
    if (!callback) {
        return;
    }
    
    PointerEvent pe = {};
    pe.timestamp = CGEventGetTimestamp(event);
    
    CGPoint location = CGEventGetLocation(event);
    pe.x = static_cast<int>(location.x);
    pe.y = static_cast<int>(location.y);
    
    switch (type) {
        case kCGEventMouseMoved:
        case kCGEventLeftMouseDragged:
        case kCGEventRightMouseDragged:
        case kCGEventOtherMouseDragged:
            pe.type = PointerEvent::Move;
            break;
        case kCGEventLeftMouseDown:
            pe.type = PointerEvent::ButtonDown;
            pe.button = 1;
            break;
        case kCGEventLeftMouseUp:
            pe.type = PointerEvent::ButtonUp;
            pe.button = 1;
            break;
        case kCGEventRightMouseDown:
            pe.type = PointerEvent::ButtonDown;
            pe.button = 2;
            break;
        case kCGEventRightMouseUp:
            pe.type = PointerEvent::ButtonUp;
            pe.button = 2;
            break;
        case kCGEventOtherMouseDown:
            pe.type = PointerEvent::ButtonDown;
            pe.button = CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber);
            break;
        case kCGEventOtherMouseUp:
            pe.type = PointerEvent::ButtonUp;
            pe.button = CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber);
            break;
        case kCGEventScrollWheel:
            pe.type = PointerEvent::Gesture;
            pe.gesture = "scroll";
            break;
        default:
            return;
    }
    
    callback(pe);
}

void HIDReader::handleTouchFrame(void *touches, int numTouches, double timestamp) {
    if (!callback) {
        return;
    }
    
    // Handle multitouch data
    MTTouch *touchData = static_cast<MTTouch*>(touches);
    for (int i = 0; i < numTouches; i++) {
        PointerEvent pe = {};
        pe.timestamp = static_cast<uint64_t>(timestamp * 1000000); // Convert to microseconds
        pe.x = static_cast<int>(touchData[i].normalized_x * 1920); // Assume 1920 width
        pe.y = static_cast<int>(touchData[i].normalized_y * 1080); // Assume 1080 height
        pe.pressure = touchData[i].size;
        
        if (touchData[i].state == 1) {
            pe.type = PointerEvent::ButtonDown;
        } else if (touchData[i].state == 4) {
            pe.type = PointerEvent::ButtonUp;
        } else {
            pe.type = PointerEvent::Move;
        }
        
        callback(pe);
    }
}