#ifndef QEFIPARTITIONVIEW_H
#define QEFIPARTITIONVIEW_H

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "qefipartitionmanager.h"

class QEFIPartitionView : public QWidget
{
    Q_OBJECT

public:
    explicit QEFIPartitionView(QWidget *parent = nullptr);
    ~QEFIPartitionView();

public slots:
    void refreshPartitions();
    void toggleMountSelectedPartition();
    void openMountPoint();
    void createBootEntryFromFile();
    void selectionChanged();

private:
    void setupUI();
    void updatePartitionTable();
    void updateButtonStates();
    QString formatSize(quint64 bytes);

    QEFIPartitionManager *m_partitionManager;

    QVBoxLayout *m_mainLayout;
    QTableWidget *m_partitionTable;

    QHBoxLayout *m_buttonLayout;
    QPushButton *m_refreshButton;
    QPushButton *m_mountUnmountButton;
    QPushButton *m_openButton;
    QPushButton *m_createBootEntryButton;

    QLabel *m_statusLabel;

    int m_selectedRow;
};

#endif // QEFIPARTITIONVIEW_H
