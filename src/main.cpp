#include <QtWidgets/QApplication>
#include <QtCore/QDebug>
#include <memory>
#include "ui/MainWindow.h"

#ifdef Q_OS_LINUX
#include "linux/LibinputReader.h"
#elif defined(Q_OS_MACOS)
#include "macos/HIDReader.h"
#endif

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Deep Input Logger");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Jules");
    
    try {
        // Create the main window
        MainWindow window;
        
        // Create platform-specific input reader
        std::unique_ptr<IInputReader> reader;
        
#ifdef Q_OS_LINUX
        qDebug() << "Creating Linux libinput reader...";
        reader = std::make_unique<LibinputReader>();
#elif defined(Q_OS_MACOS)
        qDebug() << "Creating macOS HID reader...";
        reader = std::make_unique<HIDReader>();
#else
        qWarning() << "Unsupported platform";
        return 1;
#endif
        
        // Set the input reader for the main window
        window.setInputReader(std::move(reader));
        
        // Show the window
        window.show();
        
        qDebug() << "Application started successfully";
        
        // Run the application event loop
        return app.exec();
        
    } catch (const std::exception& e) {
        qCritical() << "Fatal error:" << e.what();
        return 1;
    }
}