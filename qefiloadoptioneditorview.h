#ifndef QEFILOADOPTIONEDITORVIEW_H
#define QEFILOADOPTIONEDITORVIEW_H

#include <QWidget>

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

#include <qefi.h>

class QEFILoadOptionEditorView: public QWidget
{
    Q_OBJECT

    QFormLayout *m_topLevelLayout;

    QSpinBox *m_idSpinBox;
    QLineEdit *m_nameTextEdit;
    QLineEdit *m_optionalDataTextEdit;

public:
    QEFILoadOptionEditorView(QEFILoadOption *option = nullptr,
        QWidget *parent = nullptr);
    ~QEFILoadOptionEditorView();

    QByteArray generateLoadOption();
    quint16 getBootEntryID();

public slots:
    void createDPClicked(bool checked);
};

#endif // QEFILOADOPTIONEDITORVIEW_H