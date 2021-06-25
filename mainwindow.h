#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QTabWidget>
#include <QLabel>

class MainWindow : public QMainWindow
{
    Q_OBJECT

    QTabWidget *m_tab;
    QLabel *m_firstLabel;
    QLabel *m_secondLabel;
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};
#endif // MAINWINDOW_H
