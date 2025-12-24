#ifndef QEFIFILESELECTIONDIALOG_H
#define QEFIFILESELECTIONDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QTreeWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QScopedPointer>

#include "qefipartitionscanner.h"
#include "qfatfilesystem/qfatfilesystem.h"

class QEFIFileSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QEFIFileSelectionDialog(QWidget *parent = nullptr);
    ~QEFIFileSelectionDialog();

    QString selectedFilePath() const { return m_selectedFilePath; }
    QEFIPartitionInfo selectedPartition() const { return m_selectedPartition; }
    QFATFileInfo selectedFileInfo() const { return m_selectedFileInfo; }

    // Get the device path suitable for creating boot entries
    QByteArray getDevicePathData() const;

public slots:
    void scanPartitions();
    void partitionChanged(int index);
    void fileItemDoubleClicked(QTreeWidgetItem *item, int column);
    void fileItemClicked(QTreeWidgetItem *item, int column);
    void refreshFileList();
    void navigateToDirectory(const QString &path);

private:
    void setupUI();
    void loadDirectory(const QString &path);
    QTreeWidgetItem *createFileItem(const QFATFileInfo &fileInfo);

    QVBoxLayout *m_mainLayout;
    QLabel *m_partitionLabel;
    QComboBox *m_partitionComboBox;
    QPushButton *m_scanButton;
    QTreeWidget *m_fileTreeWidget;
    QLabel *m_statusLabel;
    QDialogButtonBox *m_buttonBox;

    QVector<QEFIPartitionInfo> m_partitions;
    QEFIPartitionInfo m_selectedPartition;
    QScopedPointer<QFATFileSystem> m_fatFilesystem;
    QString m_currentPath;
    QString m_selectedFilePath;
    QFATFileInfo m_selectedFileInfo;
};

#endif // QEFIFILESELECTIONDIALOG_H
