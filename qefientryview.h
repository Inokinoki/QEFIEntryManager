#ifndef QEFIENTRYVIEW_H
#define QEFIENTRYVIEW_H

#include <QWidget>

#include <QBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>

#include <QMap>
#include <QList>

#include "qefientry.h"

class QEFIEntryView: public QWidget
{
    Q_OBJECT

    QBoxLayout *m_topLevelLayout;

    QListWidget *m_entries;

    QBoxLayout *m_buttonLayout;
    QPushButton *m_moveUpEntryButton;
    QPushButton *m_moveDownEntryButton;
    QPushButton *m_setCurrentButton;
    QPushButton *m_importButton;
    QPushButton *m_saveButton;
    QPushButton *m_resetButton;
    QPushButton *m_rebootTargetButton;

    QLabel *m_bootTimeoutLabel;

    QMap<quint16, QEFIEntry> m_entryItems;
    QList<quint16> m_order;

    int m_selectedItemIndex;

    void updateButtonState();

    void contextMenuEvent(QContextMenuEvent *event);
public:
    QEFIEntryView(QWidget *parent = nullptr);
    ~QEFIEntryView();

public slots:
    void entryChanged(int currentRow);

    void resetClicked(bool checked);
    void resetFromStaticListClicked(bool checked);
    void saveClicked(bool checked);
    void setCurrentClicked(bool checked);
    void visibilityClicked(bool checked);
    void rebootClicked(bool checked);
    void detailClicked(bool checked);
    void importClicked(bool checked);
    void exportClicked(bool checked);
    void deleteClicked(bool checked);

    void moveUpClicked(bool checked);
    void moveDownClicked(bool checked);
};

#endif // QEFIENTRYVIEW_H
