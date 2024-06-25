#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Set window attributes
    this->setWindowTitle(tr("EFI Entry Manager"));
    this->resize(800, 600);

    // Create a class for each page amd keep a ref to update
    m_entryView = new QEFIEntryView(this);

    this->setCentralWidget(m_entryView);
}

MainWindow::~MainWindow()
{
    if (m_entryView != nullptr) m_entryView->deleteLater();
}

