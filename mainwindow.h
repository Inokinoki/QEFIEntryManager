#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QTabWidget>
#include <QLabel>

#include "qefientryrebootview.h"
#include "qefientryview.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

    QTabWidget *m_tab;
    QEFIEntryRebootView *m_rebootView;
    QEFIEntryView *m_entryView;
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};
#endif // MAINWINDOW_H
