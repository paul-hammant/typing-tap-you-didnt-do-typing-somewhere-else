#pragma once
#include "IInputReader.h"
#include <libinput.h>
#include <libudev.h>
#include <thread>
#include <atomic>
#include <functional>

class LibinputReader : public IInputReader {
public:
    LibinputReader();
    ~LibinputReader();
    
    void start() override;
    void stop() override;
    void setCallback(std::function<void(const PointerEvent&)> callback) override;

private:
    void processEvents();
    void handleEvent(struct libinput_event *event);
    PointerEvent translateEvent(struct libinput_event *event);
    
    static int openRestricted(const char *path, int flags, void *user_data);
    static void closeRestricted(int fd, void *user_data);
    
    struct udev *udev;
    struct libinput *libinput;
    std::thread eventThread;
    std::atomic<bool> running;
    std::function<void(const PointerEvent&)> callback;
    
    static const struct libinput_interface interface;
};