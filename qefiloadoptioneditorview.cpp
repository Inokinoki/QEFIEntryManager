#include "qefiloadoptioneditorview.h"
#include "qefidpeditorview.h"

#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>

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

    m_idSpinBox = new QSpinBox(this);
    m_idSpinBox->setMinimum(0);
    m_idSpinBox->setMaximum(0xFFFF);
    m_idSpinBox->setValue(0x1000);
    m_idSpinBox->setSingleStep(1);
    m_idSpinBox->setDisplayIntegerBase(16);

    m_topLevelLayout->addRow(tr("ID:"), m_idSpinBox);
    m_nameTextEdit = new QLineEdit(option == nullptr ? tr("") :
        option->name(), this);
    m_topLevelLayout->addRow(tr("Name:"), m_nameTextEdit);
    m_optionalDataTextEdit = new QLineEdit(option == nullptr ? tr("") :
        QString(option->optionalData().toHex()), this);
    m_topLevelLayout->addRow(tr("Optional Data:"), m_optionalDataTextEdit);
    #define DP_BEGIN_INDEX 3
    QPushButton *button = new QPushButton(tr("Add Device Path"), this);
    m_topLevelLayout->addRow(tr(""), button);
    connect(button, &QPushButton::clicked,
        this, &QEFILoadOptionEditorView::createDPClicked);
    QPushButton *clearButton = new QPushButton(tr("Clear Device Path"), this);
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
