#pragma once
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <memory>
#include "IInputReader.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    void setInputReader(std::unique_ptr<IInputReader> reader);

private slots:
    void onPointerEvent(const PointerEvent& event);
    void onStartStopClicked();
    void onClearLogClicked();

private:
    void setupUI();
    void setupLayout();
    QString formatEvent(const PointerEvent& event) const;
    
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    QHBoxLayout *buttonLayout;
    QSplitter *splitter;
    
    QLabel *typeLabel;
    QTextEdit *typeArea;
    QLabel *logLabel;
    QPlainTextEdit *logArea;
    
    QPushButton *startStopButton;
    QPushButton *clearLogButton;
    
    std::unique_ptr<IInputReader> inputReader;
    bool isRunning;
};