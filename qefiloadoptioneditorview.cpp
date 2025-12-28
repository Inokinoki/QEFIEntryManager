#include "qefiloadoptioneditorview.h"
#include "qefidpeditorview.h"
#include "qefifileselectiondialog.h"

#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>

#include "helpers.h"

class EditorDialog : public QDialog
{
    QEFIDPEditorView *m_view;
    QBoxLayout *m_topLevelLayout;
    QDialogButtonBox *m_buttonBox;

    QEFIDevicePath * m_currentDP;
public:
    EditorDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        m_view = new QEFIDPEditorView(nullptr, this);
        setWindowTitle(tr("Add Device Path"));
        m_topLevelLayout = new QBoxLayout(QBoxLayout::Down, this);
        m_topLevelLayout->addWidget(m_view);

        m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                           QDialogButtonBox::Cancel);
        m_topLevelLayout->addWidget(m_buttonBox);

        connect(m_buttonBox, &QDialogButtonBox::accepted, this, &EditorDialog::onAccept);
        connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    ~EditorDialog()
    {
        if (m_topLevelLayout) m_topLevelLayout->deleteLater();
        if (m_view) m_view->deleteLater();
        if (m_buttonBox) m_buttonBox->deleteLater();
    }

    QEFIDevicePath *takeCurrentDP()
    {
        QEFIDevicePath *dp = m_currentDP; m_currentDP = nullptr; return dp;
    }

public slots:
    void onAccept() {
        // Init the class from the view
        if (m_view != nullptr) {
            QEFIDevicePath *dp = m_view->getDevicePath();
            if (dp == nullptr) {
                reject();
                return;
            }
            m_currentDP = dp;
            accept();
            return;
        }
        reject();
    }
};

QEFILoadOptionEditorView::QEFILoadOptionEditorView(QEFILoadOption *option, QWidget *parent)
    : QWidget(parent)
{
    m_topLevelLayout = new QFormLayout(this);
    m_topLevelLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    m_topLevelLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_topLevelLayout->setHorizontalSpacing(10);
    m_topLevelLayout->setVerticalSpacing(8);

    m_idSpinBox = new QSpinBox(this);
    m_idSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_idSpinBox->setMinimum(0);
    m_idSpinBox->setMaximum(0xFFFF);
    m_idSpinBox->setValue(0x1000);
    m_idSpinBox->setSingleStep(1);
    m_idSpinBox->setDisplayIntegerBase(16);

    m_topLevelLayout->addRow(tr("ID:"), m_idSpinBox);
    m_nameTextEdit = new QLineEdit(option == nullptr ? tr("") :
        option->name(), this);
    m_nameTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_topLevelLayout->addRow(tr("Name:"), m_nameTextEdit);
    m_optionalDataTextEdit = new QLineEdit(option == nullptr ? tr("") :
        QString(option->optionalData().toHex()), this);
    m_optionalDataTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_topLevelLayout->addRow(tr("Optional Data:"), m_optionalDataTextEdit);
    #define DP_BEGIN_INDEX 3

    QPushButton *selectEFIButton = new QPushButton(tr("Select from EFI Partition"), this);
    m_topLevelLayout->addRow(tr(""), selectEFIButton);
    connect(selectEFIButton, &QPushButton::clicked,
        this, &QEFILoadOptionEditorView::selectFromEFIClicked);

    QPushButton *button = new QPushButton(tr("Add Device Path"), this);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_topLevelLayout->addRow(tr(""), button);
    connect(button, &QPushButton::clicked,
        this, &QEFILoadOptionEditorView::createDPClicked);
    QPushButton *clearButton = new QPushButton(tr("Clear Device Path"), this);
    clearButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_topLevelLayout->addRow(tr(""), clearButton);
    connect(clearButton, &QPushButton::clicked,
        this, &QEFILoadOptionEditorView::clearDPClicked);

    setLayout(m_topLevelLayout);
}

QEFILoadOptionEditorView::~QEFILoadOptionEditorView()
{
    if (m_topLevelLayout != nullptr) {
        m_topLevelLayout->deleteLater();
        m_topLevelLayout = nullptr;
    }
    if (m_idSpinBox != nullptr) m_idSpinBox->deleteLater();

    // Clear and delete
    for (const auto &dp: std::as_const(m_dps)) {
        delete dp;
    }
    m_dps.clear();
}

void QEFILoadOptionEditorView::createDPClicked(bool checked)
{
    Q_UNUSED(checked);
    EditorDialog dialog(this);
    if (dialog.exec() == QDialog::Rejected) return;
    // Get Device Path and add it
    QEFIDevicePath *dp = dialog.takeCurrentDP();
    if (dp != nullptr) {
        m_dps << dp;
        if (m_topLevelLayout->rowCount() > DP_BEGIN_INDEX) {
            m_topLevelLayout->insertRow(DP_BEGIN_INDEX + m_dps.size() - 1,
                tr("Device Path:"), new QLabel(
                    convert_device_path_type_to_name(dp->type()) + " " +
                    convert_device_path_subtype_to_name(dp->type(), dp->subType())));
        }
    }
}

QByteArray QEFILoadOptionEditorView::generateLoadOption()
{
    QByteArray data;
    if (m_nameTextEdit == nullptr) return data;
    if (m_optionalDataTextEdit == nullptr) return data;

    QEFILoadOption newLoadOption(data);
    newLoadOption.setIsVisible(true);
    newLoadOption.setName(m_nameTextEdit->text());
    QByteArray optionalData = QByteArray::fromHex(
        m_optionalDataTextEdit->text().toLatin1());
    // Check format
    if (optionalData.size() * 2 != m_optionalDataTextEdit->text().length()) return data;
    newLoadOption.setOptionalData(optionalData);
    // Format DPs and clear it
    for (const auto &dp: std::as_const(m_dps)) {
        newLoadOption.addDevicePath(dp);
    }
    m_dps.clear();

    return newLoadOption.format();
}

quint16 QEFILoadOptionEditorView::getBootEntryID()
{
    if (m_idSpinBox == nullptr) return 0;
    return (quint16)(m_idSpinBox->value() & 0xFFFF);
}

void QEFILoadOptionEditorView::clearDPClicked(bool checked)
{
    Q_UNUSED(checked);
    for (const auto &dp: std::as_const(m_dps)) {
        delete dp;
    }
    m_dps.clear();
}

void QEFILoadOptionEditorView::selectFromEFIClicked(bool checked)
{
    Q_UNUSED(checked);

    QEFIFileSelectionDialog dialog(this);
    if (dialog.exec() == QDialog::Rejected) {
        return;
    }

    QString selectedPath = dialog.selectedFilePath();
    QFATFileInfo fileInfo = dialog.selectedFileInfo();
    QEFIPartitionScanInfo partition = dialog.selectedPartition();

    if (selectedPath.isEmpty()) {
        QMessageBox::warning(this, tr("No File Selected"),
                             tr("Please select a file from the EFI partition."));
        return;
    }

    qDebug() << "Selected file:" << selectedPath;
    qDebug() << "File size:" << fileInfo.size;
    qDebug() << "Partition:" << partition.devicePath;
    qDebug() << "Partition offset:" << partition.partitionOffset;

    // Clear existing device paths
    for (const auto &dp: std::as_const(m_dps)) {
        delete dp;
    }
    m_dps.clear();

    // TODO: Create proper device path structure
    // For now, create a simplified Hard Drive + File Path device path

    // Create Hard Drive Media Device Path
    QEFIDevicePathHardDrive *hdDP = new QEFIDevicePathHardDrive();
    hdDP->setPartitionNumber(1);  // Should be extracted from partition info
    hdDP->setPartitionStart(partition.partitionOffset / 512);  // Convert to LBA
    hdDP->setPartitionSize(partition.partitionSize / 512);
    hdDP->setPartitionFormat(QEFIDevicePathHardDrive::FORMAT_GPT);
    hdDP->setSignatureType(QEFIDevicePathHardDrive::SIGNATURE_GUID);
    // TODO: Set actual partition GUID signature

    m_dps << hdDP;

    // Create File Path Device Path
    QEFIDevicePathFile *fileDP = new QEFIDevicePathFile();
    // Convert Unix path to EFI path (replace / with \)
    QString efiPath = selectedPath;
    efiPath.replace('/', '\\');
    fileDP->setPath(efiPath);

    m_dps << fileDP;

    // Update name field if empty
    if (m_nameTextEdit && m_nameTextEdit->text().isEmpty()) {
        QString fileName = selectedPath.split('/').last();
        fileName.replace(".efi", "", Qt::CaseInsensitive);
        fileName.replace(".EFI", "", Qt::CaseInsensitive);
        m_nameTextEdit->setText(fileName);
    }

    // Update the UI to show device paths
    int rowIndex = DP_BEGIN_INDEX;
    for (const auto &dp: std::as_const(m_dps)) {
        if (m_topLevelLayout->rowCount() > rowIndex) {
            // Remove existing rows
            while (m_topLevelLayout->rowCount() > rowIndex + 2) {
                m_topLevelLayout->removeRow(rowIndex);
            }
        }
        m_topLevelLayout->insertRow(rowIndex++,
            tr("Device Path:"), new QLabel(
                convert_device_path_type_to_name(dp->type()) + " " +
                convert_device_path_subtype_to_name(dp->type(), dp->subType())));
    }

    QMessageBox::information(this, tr("File Selected"),
                             tr("Selected: %1\n\nDevice paths have been automatically configured.")
                             .arg(selectedPath));
}
