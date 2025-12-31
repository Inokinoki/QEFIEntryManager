#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Set window attributes
    this->setWindowTitle(tr("EFI Entry Manager"));
    this->resize(800, 600);

    // Create tab widget
    m_tabWidget = new QTabWidget(this);

    // Create a class for each page and keep a ref to update
    m_entryView = new QEFIEntryView(this);
    m_partitionView = new QEFIPartitionView(this);

    // Add tabs
    m_tabWidget->addTab(m_entryView, tr("Boot Entries"));
    m_tabWidget->addTab(m_partitionView, tr("EFI Partitions"));

    this->setCentralWidget(m_tabWidget);
}

MainWindow::~MainWindow()
{
    if (m_entryView != nullptr) m_entryView->deleteLater();
    if (m_partitionView != nullptr) m_partitionView->deleteLater();
    if (m_tabWidget != nullptr) m_tabWidget->deleteLater();
}

