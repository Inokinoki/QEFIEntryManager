#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Set window attributes
    this->setWindowTitle(QStringLiteral("EFI Entry Manager"));
    this->resize(800, 600);

    // Create tabs
    m_tab = new QTabWidget(this);

    // Create a class for each page amd keep a ref to update
    m_entryView = new QEFIEntryView(m_tab);
    m_rebootView = new QEFIEntryRebootView(m_tab);

    m_tab->insertTab(0, m_entryView, QStringLiteral("Boot Order"));
    m_tab->insertTab(1, m_rebootView, QStringLiteral("Reboot with"));

    this->setCentralWidget(m_tab);
}

MainWindow::~MainWindow()
{
    if (m_tab != nullptr) delete m_tab;
    m_tab = nullptr;

    // Ownership has been passed to tab
    m_entryView = nullptr;
    m_rebootView = nullptr;
}

