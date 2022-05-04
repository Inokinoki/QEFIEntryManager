#include "qefiloadoptioneditorview.h"
#include "qefidpeditorview.h"

#include <QDebug>
#include <QDialog>

class EditorDialog : public QDialog
{
    QEFIDPEditorView *m_view;
    QBoxLayout *m_topLevelLayout;
public:
    EditorDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        m_view = new QEFIDPEditorView(nullptr, this);
        setWindowTitle(QStringLiteral("Add Device Path"));
        m_topLevelLayout = new QBoxLayout(QBoxLayout::Down, this);
        m_topLevelLayout->addWidget(m_view);
    }

    ~EditorDialog()
    {
        if (m_topLevelLayout) m_topLevelLayout->deleteLater();
        if (m_view) m_view->deleteLater();
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

    m_topLevelLayout->addRow(QStringLiteral("ID:"), m_idSpinBox);
    m_nameTextEdit = new QLineEdit(option == nullptr ? QStringLiteral("") :
        option->name(), this);
    m_topLevelLayout->addRow(QStringLiteral("Name:"), m_nameTextEdit);
    m_optionalDataTextEdit = new QLineEdit(option == nullptr ? QStringLiteral("") :
        QString(option->optionalData().toHex()), this);
    m_topLevelLayout->addRow(QStringLiteral("Optional Data:"), m_optionalDataTextEdit);
    #define DP_BEGIN_INDEX 3
    QPushButton *button = new QPushButton(QStringLiteral("Add Device Path"), this);
    m_topLevelLayout->addRow(QStringLiteral(""), button);
    connect(button, &QPushButton::clicked,
        this, &QEFILoadOptionEditorView::createDPClicked);

    setLayout(m_topLevelLayout);
}

QEFILoadOptionEditorView::~QEFILoadOptionEditorView()
{
    if (m_topLevelLayout != nullptr) {
        m_topLevelLayout->deleteLater();
        m_topLevelLayout = nullptr;
    }
    if (m_idSpinBox != nullptr) m_idSpinBox->deleteLater();
}

void QEFILoadOptionEditorView::createDPClicked(bool checked)
{
    Q_UNUSED(checked);
    EditorDialog dialog(this);
    if (dialog.exec() == QDialog::Rejected) return;
    // TODO: Get Device Path and add it
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
    // TODO: Format DPs

    return newLoadOption.format();
}

quint16 QEFILoadOptionEditorView::getBootEntryID()
{
    if (m_idSpinBox == nullptr) return 0;
    return (quint16)(m_idSpinBox->value() & 0xFFFF);
}
