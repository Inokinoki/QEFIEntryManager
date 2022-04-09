#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QLabel>

#include "qefientryview.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

    QEFIEntryView *m_entryView;
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};
#endif // MAINWINDOW_H
