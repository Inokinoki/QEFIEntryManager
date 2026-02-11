#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>

#include "qefientryview.h"
#include "qefipartitionview.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

    QTabWidget *m_tabWidget;
    QEFIEntryView *m_entryView;
    QEFIPartitionView *m_partitionView;
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};
#endif // MAINWINDOW_H
