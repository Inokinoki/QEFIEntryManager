#ifndef QEFIDPEDITORDIALOG_H
#define QEFIDPEDITORDIALOG_H

#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMap>
#include <QWidget>

#include "qefidpfields.h"
#include <qefi.h>

/**
 * @brief Dialog for editing device path fields using declarative field metadata
 *
 * This dialog provides a clean interface for editing device paths:
 * - Pass in a device path (existing or dummy) to edit
 * - The dialog automatically creates appropriate widgets based on field types
 * - Returns a newly created device path with the edited values
 *
 * Example usage:
 * @code
 *   // Create or edit a device path
 *   QEFIDevicePath *existingDp = ...; // or nullptr for new
 *   QEFIDPEditorDialog dialog(existingDp, this);
 *   if (dialog.exec() == QDialog::Accepted) {
 *       QEFIDevicePath *newDp = dialog.getDevicePath();
 *       // Use the new device path
 *   }
 * @endcode
 */
class QEFIDPEditorDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Construct a device path editor dialog
     * @param dp Device path to edit (can be nullptr to create a new one)
     * @param parent Parent widget
     */
    explicit QEFIDPEditorDialog(QEFIDevicePath *dp = nullptr, QWidget *parent = nullptr);

    /**
     * @brief Construct a device path editor dialog for a specific type/subtype
     * @param type Device path type
     * @param subtype Device path subtype
     * @param parent Parent widget
     */
    QEFIDPEditorDialog(QEFIDevicePathType type, quint8 subtype, QWidget *parent = nullptr);

    ~QEFIDPEditorDialog();

    /**
     * @brief Get the device path created from the dialog fields
     * @return Newly created device path (ownership transferred to caller)
     */
    QEFIDevicePath *getDevicePath();

    /**
     * @brief Get the values from the dialog as a map
     * @return Map of field name to value
     */
    QMap<QString, QVariant> getFieldValues();

private:
    void setupUI(QEFIDevicePath *dp);
    void createFieldWidgets(const QList<QEFIFieldMeta> &fields, QEFIDevicePath *dp);
    void populateFieldValues(QEFIDevicePath *dp);

    QFormLayout *m_formLayout;
    QDialogButtonBox *m_buttonBox;

    QEFIDevicePathType m_type;
    quint8 m_subtype;

    // Map of property name to (field metadata, widget) pairs
    QMap<QString, QPair<QEFIFieldMeta, QWidget*>> m_fieldWidgets;
};

#endif // QEFIDPEDITORDIALOG_H
