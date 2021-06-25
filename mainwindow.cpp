#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Set window attributes
    this->setWindowTitle(QStringLiteral("EFI Entry Manager"));
    this->resize(800, 600);

    // Create tabs
    m_tab = new QTabWidget(this);

    // TODO: Add class for each page
    m_firstLabel = new QLabel(QStringLiteral("Boot entry page"), m_tab);
    m_secondLabel = new QLabel(QStringLiteral("Reboot page"), m_tab);

    m_tab->insertTab(0, m_firstLabel, QStringLiteral("Boot Entry"));
    m_tab->insertTab(1, m_secondLabel, QStringLiteral("Reboot into"));

    this->setCentralWidget(m_tab);
}

MainWindow::~MainWindow()
{
    if (m_tab != nullptr) delete m_tab;
    m_tab = nullptr;

    // TODO: Remove pages instead of such placeholders
    if (m_firstLabel != nullptr) delete m_firstLabel;
    if (m_secondLabel != nullptr) delete m_secondLabel;
    m_firstLabel = nullptr;
    m_secondLabel = nullptr;
}

