#include "qefidpeditordialog.h"
#include "qefidpfields.h"
#include "helpers.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>

QEFIDPEditorDialog::QEFIDPEditorDialog(QEFIDevicePath *dp, QWidget *parent)
    : QDialog(parent)
    , m_formLayout(nullptr)
    , m_buttonBox(nullptr)
{
    if (dp) {
        m_type = dp->type();
        m_subtype = dp->subType();
    } else {
        m_type = QEFIDevicePathType::DP_Hardware;
        m_subtype = QEFIDevicePathHardwareSubType::HW_PCI;
    }

    setupUI(dp);
}

QEFIDPEditorDialog::QEFIDPEditorDialog(QEFIDevicePathType type, quint8 subtype, QWidget *parent)
    : QDialog(parent)
    , m_formLayout(nullptr)
    , m_buttonBox(nullptr)
    , m_type(type)
    , m_subtype(subtype)
{
    // Create a dummy device path to get field definitions
    QEFIDevicePath *dummy = create_dummy_device_path(type, subtype);
    setupUI(dummy);
    delete dummy;
}

QEFIDPEditorDialog::~QEFIDPEditorDialog()
{
}

void QEFIDPEditorDialog::setupUI(QEFIDevicePath *dp)
{
    setWindowTitle(tr("Edit Device Path"));
    setMinimumWidth(500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Add header with type info
    QString typeInfo = QString("%1 / %2")
        .arg(convert_device_path_type_to_name(m_type))
        .arg(convert_device_path_subtype_to_name(m_type, m_subtype));
    QLabel *headerLabel = new QLabel(typeInfo, this);
    QFont headerFont = headerLabel->font();
    headerFont.setPointSize(headerFont.pointSize() + 2);
    headerFont.setBold(true);
    headerLabel->setFont(headerFont);
    mainLayout->addWidget(headerLabel);

    // Create form layout for fields
    m_formLayout = new QFormLayout();
    m_formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    m_formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_formLayout->setHorizontalSpacing(10);
    m_formLayout->setVerticalSpacing(8);
    mainLayout->addLayout(m_formLayout);

    // Create field widgets based on device path type
    QList<QEFIFieldMeta> fields;
    if (dp) {
        fields = getDevicePathFields(dp);
    } else {
        // Create dummy to get fields
        QEFIDevicePath *dummy = create_dummy_device_path(m_type, m_subtype);
        if (dummy) {
            fields = getDevicePathFields(dummy);
            delete dummy;
        }
    }

    createFieldWidgets(fields, dp);

    // Add button box
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(m_buttonBox);

    setLayout(mainLayout);
}

void QEFIDPEditorDialog::createFieldWidgets(const QList<QEFIFieldMeta> &fields, QEFIDevicePath *dp)
{
    m_fieldWidgets.clear();

    for (const QEFIFieldMeta &field : fields) {
        // Create widget for this field
        QWidget *widget = QEFIFieldWidgetFactory::createWidget(field, this);
        if (widget) {
            // Set size policy to ensure widget fills available width
            widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

            // Add to form layout
            m_formLayout->addRow(field.name + ":", widget);

            // Store widget with field metadata
            m_fieldWidgets[field.propertyName] = qMakePair(field, widget);
        }
    }

    // Populate values if we have an existing device path
    if (dp) {
        populateFieldValues(dp);
    }
}

void QEFIDPEditorDialog::populateFieldValues(QEFIDevicePath *dp)
{
    if (!dp) return;

    // Get the values from the device path using the existing helper
    QList<QPair<QString, QString>> attrs = convert_device_path_attrs(dp);

    // Map attribute names to values
    QMap<QString, QString> attrMap;
    for (const auto &attr : attrs) {
        attrMap[attr.first] = attr.second;
    }

    // Set widget values based on field metadata
    for (auto it = m_fieldWidgets.begin(); it != m_fieldWidgets.end(); ++it) {
        const QEFIFieldMeta &field = it.value().first;
        QWidget *widget = it.value().second;

        // Try to find the value in the attributes
        QString attrValue = attrMap.value(field.name, "");

        if (!attrValue.isEmpty()) {
            // Convert string value to appropriate type and set widget
            QVariant value;

            switch (field.type) {
            case QEFIFieldType::UInt8:
            case QEFIFieldType::UInt16:
            case QEFIFieldType::UInt32:
            case QEFIFieldType::Port:
                value = attrValue.toUInt();
                break;
            case QEFIFieldType::UInt64:
                value = attrValue.toULongLong();
                break;
            case QEFIFieldType::String:
            case QEFIFieldType::Path:
            case QEFIFieldType::URL:
                value = attrValue;
                break;
            case QEFIFieldType::UUID:
                value = QUuid(attrValue);
                break;
            case QEFIFieldType::HexData:
            case QEFIFieldType::ByteArray:
                value = QByteArray::fromHex(attrValue.toLatin1());
                break;
            case QEFIFieldType::IPv4Address:
            case QEFIFieldType::IPv6Address:
            case QEFIFieldType::MACAddress:
                value = attrValue;
                break;
            default:
                value = attrValue;
                break;
            }

            QEFIFieldWidgetFactory::setWidgetValue(widget, field, value);
        }
    }
}

QMap<QString, QVariant> QEFIDPEditorDialog::getFieldValues()
{
    QMap<QString, QVariant> values;

    for (auto it = m_fieldWidgets.begin(); it != m_fieldWidgets.end(); ++it) {
        const QString &propName = it.key();
        const QEFIFieldMeta &field = it.value().first;
        QWidget *widget = it.value().second;

        QVariant value = QEFIFieldWidgetFactory::getWidgetValue(widget, field);
        if (!value.isNull()) {
            values[propName] = value;
        }
    }

    return values;
}

QEFIDevicePath *QEFIDPEditorDialog::getDevicePath()
{
    // Get field values
    QMap<QString, QVariant> values = getFieldValues();

    // Create device path from values using the helper function
    return create_device_path_from_values(m_type, m_subtype, values);
}
