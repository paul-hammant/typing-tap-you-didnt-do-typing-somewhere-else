#pragma once
#include "IInputReader.h"
#include <IOKit/hid/IOHIDManager.h>
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <functional>
#include <atomic>

class HIDReader : public IInputReader {
public:
    HIDReader();
    ~HIDReader();
    
    void start() override;
    void stop() override;
    void setCallback(std::function<void(const PointerEvent&)> callback) override;

private:
    static void hidInputValueCallback(void *context, IOReturn result, void *sender, IOHIDValueRef value);
    static CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);
    static void touchCallback(void *device, void *touches, int numTouches, double timestamp, int frame);
    
    void handleHIDValue(IOHIDValueRef value);
    void handleCGEvent(CGEventType type, CGEventRef event);
    void handleTouchFrame(void *touches, int numTouches, double timestamp);
    
    IOHIDManagerRef hidManager;
    CFMachPortRef eventTap;
    CFRunLoopSourceRef tapRunLoopSource;
    std::atomic<bool> running;
    std::function<void(const PointerEvent&)> callback;
    
    // Multitouch support
    CFMutableArrayRef touchDevices;
    void setupMultitouchSupport();
    void cleanupMultitouchSupport();
};