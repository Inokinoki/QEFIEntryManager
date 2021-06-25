#ifndef QEFIENTRYVIEW_H
#define QEFIENTRYVIEW_H

#include <QWidget>

#include <QBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>

class QEFIEntryView: public QWidget
{
    Q_OBJECT

    QBoxLayout *m_topLevelLayout;

    QListWidget *m_entries;

    QBoxLayout *m_buttonLayout;
    QPushButton *m_moveUpEntryButton;
    QPushButton *m_moveDownEntryButton;
    QPushButton *m_setCurrentButton;
    QPushButton *m_saveButton;
    QPushButton *m_resetButton;
public:
    QEFIEntryView(QWidget *parent = nullptr);
    ~QEFIEntryView();
};

#endif // QEFIENTRYVIEW_H
