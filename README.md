# Deep Input Logger

A cross-platform C++ application that captures and logs all mouse and touchpad events (moves, clicks, pressure, gestures) while providing a live typing textarea.

## Features

- **Real-time input capture**: Logs all mouse movements, clicks, and touchpad gestures
- **Cross-platform**: Works on Linux (libinput) and macOS (IOHID/Quartz)
- **Live typing area**: Interactive text field for testing while logging
- **Detailed event logging**: Timestamps, coordinates, button states, pressure, and gestures
- **Qt-based UI**: Clean, responsive interface with separate typing and logging areas

## Building

### Prerequisites

**Linux (Ubuntu/Debian):**
```bash
sudo apt install qtbase5-dev libqt5widgets5 libinput-dev libudev-dev cmake build-essential
```

**macOS:**
```bash
# Install Xcode command line tools
xcode-select --install
# Install Qt (via Homebrew)
brew install qt@5 cmake
```

### Build Instructions

```bash
git clone <repository>
cd <project-directory>
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Running

### Linux

1. **Set up permissions** (required for input device access):
   ```bash
   # Copy udev rules
   sudo cp third_party/99-deepinput.rules /etc/udev/rules.d/
   
   # Reload udev rules
   sudo udevadm control --reload-rules && sudo udevadm trigger
   
   # Add user to input group
   sudo usermod -a -G input $USER
   
   # Log out and back in for group changes to take effect
   ```

2. **Run the application**:
   ```bash
   cd build
   ./DeepInputLogger
   ```

### macOS

1. **Grant permissions**: The app requires Accessibility and Input Monitoring permissions:
   - Go to System Preferences → Security & Privacy → Privacy
   - Add the application to both "Accessibility" and "Input Monitoring"

2. **Run the application**:
   ```bash
   cd build
   ./DeepInputLogger
   ```

## Usage

1. Launch the application
2. Click "Start Logging" to begin capturing input events
3. Use the typing area to test keyboard input while mouse/touchpad events are logged
4. View real-time event logs in the bottom panel
5. Click "Stop Logging" to stop capture
6. Use "Clear Log" to clear the event history

## Event Types

The application captures and logs:

- **MOVE**: Mouse/touchpad movement with x,y coordinates
- **BTN_DOWN/BTN_UP**: Button press/release events with button number
- **GESTURE**: Touchpad gestures (pinch, swipe, scroll)
- **PRESSURE**: Pressure-sensitive input (when supported)

## Troubleshooting

### Linux

- **Permission denied**: Ensure udev rules are installed and user is in input group
- **No events captured**: Check that input devices are accessible via `/dev/input/event*`
- **Build errors**: Install all required development packages

### macOS

- **No events captured**: Grant Accessibility and Input Monitoring permissions
- **App crashes**: Check that private frameworks are accessible (may require disabling SIP for development)

## Architecture

- **IInputReader**: Abstract interface for platform-specific input readers
- **LibinputReader**: Linux implementation using libinput
- **HIDReader**: macOS implementation using IOHIDManager and Quartz Event Tap
- **MainWindow**: Qt-based UI with typing area and event log display

## License

This project is for educational and development purposes. Handle input device access responsibly and respect user privacy.