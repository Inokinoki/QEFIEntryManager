#include "qefiloadoptioneditorview.h"
#include "qefidpeditorview.h"

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
        m_topLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);
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

    QSpinBox *idSpinBox = new QSpinBox(this);
    idSpinBox->setMinimum(0);
    idSpinBox->setMaximum(0xFFFF);
    idSpinBox->setDisplayIntegerBase(16);

    m_topLevelLayout->addRow(QStringLiteral("ID:"), idSpinBox);
    m_topLevelLayout->addRow(QStringLiteral("Name:"), new QLineEdit(option == nullptr ?
        QStringLiteral("") : option->name(), this));
    m_topLevelLayout->addRow(QStringLiteral("Optional Data:"),
        new QLineEdit(option == nullptr ? QStringLiteral("") :
            QString(option->optionalData().toHex()), this));
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
}

void QEFILoadOptionEditorView::createDPClicked(bool checked)
{
    Q_UNUSED(checked);
    EditorDialog dialog(this);
    dialog.exec();
    // TODO: Get Device Path and add it
}
