#ifndef QEFILOADOPTIONEDITORVIEW_H
#define QEFILOADOPTIONEDITORVIEW_H

#include <QWidget>

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QList>
#include <QSharedPointer>

#include <qefi.h>

class QEFILoadOptionEditorView: public QWidget
{
    Q_OBJECT

    QFormLayout *m_topLevelLayout;

    QSpinBox *m_idSpinBox;
    QLineEdit *m_nameTextEdit;
    QLineEdit *m_optionalDataTextEdit;

    QList<QEFIDevicePath *> m_dps;
public:
    QEFILoadOptionEditorView(QEFILoadOption *option = nullptr,
        QWidget *parent = nullptr);
    ~QEFILoadOptionEditorView();

    QByteArray generateLoadOption();
    quint16 getBootEntryID();

public slots:
    void createDPClicked(bool checked);
    void clearDPClicked(bool checked);
    void selectFromEFIClicked(bool checked);
};

#endif // QEFILOADOPTIONEDITORVIEW_H