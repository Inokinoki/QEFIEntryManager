#ifndef QEFIPARTITIONVIEW_H
#define QEFIPARTITIONVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>

#include "qefipartitionmanager.h"

class QEFIPartitionView : public QWidget
{
    Q_OBJECT

public:
    explicit QEFIPartitionView(QWidget *parent = nullptr);
    ~QEFIPartitionView();

public slots:
    void refreshPartitions();
    void mountSelectedPartition();
    void unmountSelectedPartition();
    void openMountPoint();
    void selectionChanged();

private:
    void setupUI();
    void updatePartitionTable();
    void updateButtonStates();
    QString formatSize(quint64 bytes);

    QEFIPartitionManager *m_partitionManager;

    QVBoxLayout *m_mainLayout;
    QLabel *m_titleLabel;
    QTableWidget *m_partitionTable;

    QHBoxLayout *m_buttonLayout;
    QPushButton *m_refreshButton;
    QPushButton *m_mountButton;
    QPushButton *m_unmountButton;
    QPushButton *m_openButton;

    QLabel *m_statusLabel;

    int m_selectedRow;
};

#endif // QEFIPARTITIONVIEW_H
