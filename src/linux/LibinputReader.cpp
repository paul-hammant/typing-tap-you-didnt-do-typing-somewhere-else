#include "LibinputReader.h"
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <cstring>

const struct libinput_interface LibinputReader::interface = {
    .open_restricted = LibinputReader::openRestricted,
    .close_restricted = LibinputReader::closeRestricted,
};

LibinputReader::LibinputReader() : udev(nullptr), libinput(nullptr), running(false) {
    udev = udev_new();
    if (!udev) {
        throw std::runtime_error("Failed to create udev context");
    }
    
    libinput = libinput_udev_create_context(&interface, this, udev);
    if (!libinput) {
        udev_unref(udev);
        throw std::runtime_error("Failed to create libinput context");
    }
    
    if (libinput_udev_assign_seat(libinput, "seat0") != 0) {
        libinput_unref(libinput);
        udev_unref(udev);
        throw std::runtime_error("Failed to assign seat");
    }
}

LibinputReader::~LibinputReader() {
    stop();
    if (libinput) {
        libinput_unref(libinput);
    }
    if (udev) {
        udev_unref(udev);
    }
}

void LibinputReader::start() {
    if (running) {
        return;
    }
    
    running = true;
    eventThread = std::thread(&LibinputReader::processEvents, this);
}

void LibinputReader::stop() {
    if (!running) {
        return;
    }
    
    running = false;
    if (eventThread.joinable()) {
        eventThread.join();
    }
}

void LibinputReader::setCallback(std::function<void(const PointerEvent&)> cb) {
    callback = cb;
}

void LibinputReader::processEvents() {
    int fd = libinput_get_fd(libinput);
    fd_set fds;
    struct timeval tv;
    
    while (running) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms timeout
        
        int ret = select(fd + 1, &fds, nullptr, nullptr, &tv);
        if (ret < 0) {
            if (errno == EINTR) continue;
            std::cerr << "select() failed: " << strerror(errno) << std::endl;
            break;
        }
        
        if (ret > 0 && FD_ISSET(fd, &fds)) {
            libinput_dispatch(libinput);
            
            struct libinput_event *event;
            while ((event = libinput_get_event(libinput))) {
                handleEvent(event);
                libinput_event_destroy(event);
            }
        }
    }
}

void LibinputReader::handleEvent(struct libinput_event *event) {
    if (!callback) {
        return;
    }
    
    enum libinput_event_type type = libinput_event_get_type(event);
    
    switch (type) {
        case LIBINPUT_EVENT_POINTER_MOTION:
        case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
        case LIBINPUT_EVENT_POINTER_BUTTON:
        case LIBINPUT_EVENT_POINTER_AXIS:
        case LIBINPUT_EVENT_POINTER_SCROLL_WHEEL:
        case LIBINPUT_EVENT_POINTER_SCROLL_FINGER:
        case LIBINPUT_EVENT_POINTER_SCROLL_CONTINUOUS:
        case LIBINPUT_EVENT_TOUCH_DOWN:
        case LIBINPUT_EVENT_TOUCH_UP:
        case LIBINPUT_EVENT_TOUCH_MOTION:
        case LIBINPUT_EVENT_TOUCH_CANCEL:
        case LIBINPUT_EVENT_TOUCH_FRAME:
        case LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN:
        case LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE:
        case LIBINPUT_EVENT_GESTURE_SWIPE_END:
        case LIBINPUT_EVENT_GESTURE_PINCH_BEGIN:
        case LIBINPUT_EVENT_GESTURE_PINCH_UPDATE:
        case LIBINPUT_EVENT_GESTURE_PINCH_END:
        case LIBINPUT_EVENT_GESTURE_HOLD_BEGIN:
        case LIBINPUT_EVENT_GESTURE_HOLD_END:
            try {
                PointerEvent pointerEvent = translateEvent(event);
                callback(pointerEvent);
            } catch (const std::exception& e) {
                std::cerr << "Error translating event: " << e.what() << std::endl;
            }
            break;
        default:
            break;
    }
}

PointerEvent LibinputReader::translateEvent(struct libinput_event *event) {
    PointerEvent pe = {};
    pe.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    enum libinput_event_type type = libinput_event_get_type(event);
    
    switch (type) {
        case LIBINPUT_EVENT_POINTER_MOTION: {
            pe.type = PointerEvent::Move;
            struct libinput_event_pointer *ptr = libinput_event_get_pointer_event(event);
            pe.x = static_cast<int>(libinput_event_pointer_get_dx(ptr));
            pe.y = static_cast<int>(libinput_event_pointer_get_dy(ptr));
            break;
        }
        case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE: {
            pe.type = PointerEvent::Move;
            struct libinput_event_pointer *ptr = libinput_event_get_pointer_event(event);
            pe.x = static_cast<int>(libinput_event_pointer_get_absolute_x(ptr));
            pe.y = static_cast<int>(libinput_event_pointer_get_absolute_y(ptr));
            break;
        }
        case LIBINPUT_EVENT_POINTER_BUTTON: {
            struct libinput_event_pointer *ptr = libinput_event_get_pointer_event(event);
            enum libinput_button_state state = libinput_event_pointer_get_button_state(ptr);
            pe.type = (state == LIBINPUT_BUTTON_STATE_PRESSED) ? PointerEvent::ButtonDown : PointerEvent::ButtonUp;
            pe.button = libinput_event_pointer_get_button(ptr);
            break;
        }
        case LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN:
        case LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE:
        case LIBINPUT_EVENT_GESTURE_SWIPE_END: {
            pe.type = PointerEvent::Gesture;
            struct libinput_event_gesture *gesture = libinput_event_get_gesture_event(event);
            int finger_count = libinput_event_gesture_get_finger_count(gesture);
            pe.gesture = "swipe" + std::to_string(finger_count);
            break;
        }
        case LIBINPUT_EVENT_GESTURE_PINCH_BEGIN:
        case LIBINPUT_EVENT_GESTURE_PINCH_UPDATE:
        case LIBINPUT_EVENT_GESTURE_PINCH_END: {
            pe.type = PointerEvent::Gesture;
            pe.gesture = "pinch";
            break;
        }
        case LIBINPUT_EVENT_TOUCH_DOWN:
        case LIBINPUT_EVENT_TOUCH_UP:
        case LIBINPUT_EVENT_TOUCH_MOTION: {
            struct libinput_event_touch *touch = libinput_event_get_touch_event(event);
            pe.type = (type == LIBINPUT_EVENT_TOUCH_DOWN) ? PointerEvent::ButtonDown : 
                      (type == LIBINPUT_EVENT_TOUCH_UP) ? PointerEvent::ButtonUp : PointerEvent::Move;
            pe.x = static_cast<int>(libinput_event_touch_get_x(touch));
            pe.y = static_cast<int>(libinput_event_touch_get_y(touch));
            pe.pressure = 1.0f; // libinput doesn't provide pressure info directly
            break;
        }
        default:
            pe.type = PointerEvent::Move;
            break;
    }
    
    return pe;
}

int LibinputReader::openRestricted(const char *path, int flags, void *user_data) {
    int fd = open(path, flags);
    if (fd < 0) {
        std::cerr << "Failed to open " << path << ": " << strerror(errno) << std::endl;
    }
    return fd;
}

void LibinputReader::closeRestricted(int fd, void *user_data) {
    close(fd);
}