#include "MainWindow.h"
#include <QtWidgets/QApplication>
#include <QtCore/QMetaObject>
#include <QtCore/QDateTime>
#include <sstream>
#include <iomanip>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), isRunning(false) {
    setupUI();
    setupLayout();
}

MainWindow::~MainWindow() {
    if (inputReader && isRunning) {
        inputReader->stop();
    }
}

void MainWindow::setupUI() {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Create main layout
    mainLayout = new QVBoxLayout(centralWidget);
    buttonLayout = new QHBoxLayout();
    splitter = new QSplitter(Qt::Vertical, this);
    
    // Create typing area
    typeLabel = new QLabel("Type here while input events are being logged:", this);
    typeArea = new QTextEdit(this);
    typeArea->setPlaceholderText("Start typing to test input capture...");
    typeArea->setMinimumHeight(150);
    typeArea->setMaximumHeight(200);
    
    // Create log area
    logLabel = new QLabel("Input Event Log:", this);
    logArea = new QPlainTextEdit(this);
    logArea->setReadOnly(true);
    logArea->setFont(QFont("Consolas", 9));
    logArea->setPlaceholderText("Input events will appear here...");
    
    // Create buttons
    startStopButton = new QPushButton("Start Logging", this);
    clearLogButton = new QPushButton("Clear Log", this);
    
    // Connect signals
    connect(startStopButton, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);
    connect(clearLogButton, &QPushButton::clicked, this, &MainWindow::onClearLogClicked);
}

void MainWindow::setupLayout() {
    // Add typing area to top of splitter
    QWidget *typeWidget = new QWidget();
    QVBoxLayout *typeLayout = new QVBoxLayout(typeWidget);
    typeLayout->addWidget(typeLabel);
    typeLayout->addWidget(typeArea);
    typeLayout->setContentsMargins(0, 0, 0, 0);
    
    // Add log area to bottom of splitter
    QWidget *logWidget = new QWidget();
    QVBoxLayout *logLayout = new QVBoxLayout(logWidget);
    logLayout->addWidget(logLabel);
    logLayout->addWidget(logArea);
    logLayout->setContentsMargins(0, 0, 0, 0);
    
    splitter->addWidget(typeWidget);
    splitter->addWidget(logWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    
    // Add buttons to button layout
    buttonLayout->addWidget(startStopButton);
    buttonLayout->addWidget(clearLogButton);
    buttonLayout->addStretch();
    
    // Add everything to main layout
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(splitter);
    
    // Set window properties
    setWindowTitle("Deep Input Logger");
    setMinimumSize(800, 600);
    resize(1000, 800);
}

void MainWindow::setInputReader(std::unique_ptr<IInputReader> reader) {
    if (inputReader && isRunning) {
        inputReader->stop();
    }
    
    inputReader = std::move(reader);
    
    if (inputReader) {
        inputReader->setCallback([this](const PointerEvent& event) {
            // Use Qt's thread-safe method to invoke the slot
            QMetaObject::invokeMethod(this, "onPointerEvent", Qt::QueuedConnection,
                                    Q_ARG(const PointerEvent&, event));
        });
    }
}

void MainWindow::onPointerEvent(const PointerEvent& event) {
    QString eventText = formatEvent(event);
    logArea->appendPlainText(eventText);
    
    // Auto-scroll to bottom and limit log size
    logArea->ensureCursorVisible();
    
    // Keep only last 1000 lines to prevent memory issues
    if (logArea->document()->lineCount() > 1000) {
        QTextCursor cursor = logArea->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, 100);
        cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }
}

void MainWindow::onStartStopClicked() {
    if (!inputReader) {
        logArea->appendPlainText("ERROR: No input reader available");
        return;
    }
    
    if (isRunning) {
        inputReader->stop();
        isRunning = false;
        startStopButton->setText("Start Logging");
        logArea->appendPlainText("=== Input logging stopped ===");
    } else {
        try {
            inputReader->start();
            isRunning = true;
            startStopButton->setText("Stop Logging");
            logArea->appendPlainText("=== Input logging started ===");
        } catch (const std::exception& e) {
            logArea->appendPlainText(QString("ERROR: Failed to start logging: %1").arg(e.what()));
        }
    }
}

void MainWindow::onClearLogClicked() {
    logArea->clear();
}

QString MainWindow::formatEvent(const PointerEvent& event) const {
    std::stringstream ss;
    
    // Format timestamp
    auto timestamp = QDateTime::fromMSecsSinceEpoch(event.timestamp / 1000);
    ss << timestamp.toString("hh:mm:ss.zzz").toStdString() << " ";
    
    // Format event type
    switch (event.type) {
        case PointerEvent::Move:
            ss << "MOVE";
            break;
        case PointerEvent::ButtonDown:
            ss << "BTN_DOWN";
            break;
        case PointerEvent::ButtonUp:
            ss << "BTN_UP";
            break;
        case PointerEvent::Gesture:
            ss << "GESTURE";
            break;
        case PointerEvent::Pressure:
            ss << "PRESSURE";
            break;
    }
    
    // Add coordinates if relevant
    if (event.type == PointerEvent::Move || event.type == PointerEvent::ButtonDown || 
        event.type == PointerEvent::ButtonUp) {
        ss << " x=" << event.x << " y=" << event.y;
    }
    
    // Add button info if relevant
    if (event.type == PointerEvent::ButtonDown || event.type == PointerEvent::ButtonUp) {
        ss << " btn=" << event.button;
    }
    
    // Add pressure if relevant
    if (event.pressure > 0.0f) {
        ss << " pressure=" << std::fixed << std::setprecision(3) << event.pressure;
    }
    
    // Add gesture info if relevant
    if (event.type == PointerEvent::Gesture && !event.gesture.empty()) {
        ss << " gesture=\"" << event.gesture << "\"";
    }
    
    return QString::fromStdString(ss.str());
}

