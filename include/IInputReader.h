#pragma once
#include <functional>
#include <string>
#include <cstdint>

struct PointerEvent {
    enum Type { Move, ButtonDown, ButtonUp, Gesture, Pressure } type;
    int x, y;                 // for Move
    int button;               // 1=left,2=right,3=middle
    float pressure;           // normalized 0.0–1.0
    std::string gesture;      // e.g. "pinch", "swipe3"
    uint64_t timestamp;       // microseconds
};

class IInputReader {
public:
    virtual ~IInputReader() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void setCallback(std::function<void(const PointerEvent&)>) = 0;
};