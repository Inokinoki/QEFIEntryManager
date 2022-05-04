#ifndef QEFIDPEDITORVIEW_H
#define QEFIDPEDITORVIEW_H

#include <QWidget>

#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QList>
#include <QPair>

#include <qefi.h>

class QEFIDPEditorView: public QWidget
{
    Q_OBJECT

    QFormLayout *m_topLevelLayout;

    QComboBox *m_dpTypeSelector;
    QComboBox *m_dpSubtypeSelector;
    int m_dpTypeSelected;
    int m_dpSubtypeSelected;

    QList<QPair<QString, QWidget *> > m_currentWidgets;

    QList<QPair<QString, QWidget *> > constructDPEditView(enum QEFIDevicePathType type, quint8 subType);
    QList<QPair<QString, QWidget *> > constructDPEditView(QEFIDevicePath *dp);
public:
    QEFIDPEditorView(QEFIDevicePath *dp = nullptr,
        QWidget *parent = nullptr);
    ~QEFIDPEditorView();

    QEFIDevicePath *getDevicePath();    // Ownership is yours

public slots:
    void dpTypeComboBoxCurrentIndexChanged(int index);
    void dpSubtypeComboBoxCurrentIndexChanged(int index);
};

#endif // QEFIDPEDITORVIEW_H