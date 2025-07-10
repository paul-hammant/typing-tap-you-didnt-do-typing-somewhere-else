#pragma once
#include <CoreFoundation/CoreFoundation.h>

// Private MultitouchSupport framework stubs
typedef struct {
    int frame;
    double timestamp;
    int identifier;
    int state;
    int unknown1;
    int unknown2;
    int unknown3;
    float normalized_x;
    float normalized_y;
    float size;
    int unknown4;
    float angle;
    float major_axis;
    float minor_axis;
    float unknown5;
    float unknown6;
    float unknown7;
    float density;
} MTTouch;

typedef void (*MTContactFrameCallback)(void *device, MTTouch *touches, int numTouches, double timestamp, int frame);

extern "C" {
    CFMutableArrayRef MTDeviceCreateList(void);
    void MTRegisterContactFrameCallback(void *device, MTContactFrameCallback callback);
    void MTDeviceStart(void *device, int unknown);
    void MTDeviceStop(void *device);
    void MTDeviceRelease(void *device);
}