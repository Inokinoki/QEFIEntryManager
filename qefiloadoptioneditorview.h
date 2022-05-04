#ifndef QEFILOADOPTIONEDITORVIEW_H
#define QEFILOADOPTIONEDITORVIEW_H

#include <QWidget>

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <qefi.h>

class QEFILoadOptionEditorView: public QWidget
{
    Q_OBJECT

    QFormLayout *m_topLevelLayout;

public:
    QEFILoadOptionEditorView(QEFILoadOption *option = nullptr,
        QWidget *parent = nullptr);
    ~QEFILoadOptionEditorView();

public slots:
    void createDPClicked(bool checked);
};

#endif // QEFILOADOPTIONEDITORVIEW_H