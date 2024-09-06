/*
 * varianteditor.cpp
 * Copyright 2024, Thorbjørn Lindeijer <bjorn@lindeijer.nl>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "varianteditor.h"

#include "colorbutton.h"
#include "fileedit.h"
#include "utils.h"
#include "propertyeditorwidgets.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QResizeEvent>
#include <QSpacerItem>
#include <QSpinBox>
#include <QStringListModel>

namespace Tiled {

AbstractProperty::AbstractProperty(const QString &name,
                                   EditorFactory *editorFactory,
                                   QObject *parent)
    : Property(name, parent)
    , m_editorFactory(editorFactory)
{}

QWidget *AbstractProperty::createEditor(QWidget *parent)
{
    return m_editorFactory ? m_editorFactory->createEditor(this, parent)
                           : nullptr;
}


GetSetProperty::GetSetProperty(const QString &name,
                               std::function<QVariant ()> get,
                               std::function<void (const QVariant &)> set,
                               EditorFactory *editorFactory,
                               QObject *parent)
    : AbstractProperty(name, editorFactory, parent)
    , m_get(std::move(get))
    , m_set(std::move(set))
{}


QWidget *StringProperty::createEditor(QWidget *parent)
{
    auto editor = new QLineEdit(parent);
    auto syncEditor = [=] {
        editor->setText(m_get());
    };
    syncEditor();

    QObject::connect(this, &Property::valueChanged, editor, syncEditor);
    QObject::connect(editor, &QLineEdit::textEdited, this, m_set);

    return editor;
}

QWidget *UrlProperty::createEditor(QWidget *parent)
{
    auto editor = new FileEdit(parent);
    editor->setFilter(m_filter);

    auto syncEditor = [=] {
        editor->setFileUrl(m_get());
    };
    syncEditor();

    QObject::connect(this, &Property::valueChanged, editor, syncEditor);
    QObject::connect(editor, &FileEdit::fileUrlChanged, this, m_set);

    return editor;
}

QWidget *IntProperty::createEditor(QWidget *parent)
{
    auto editor = new SpinBox(parent);
    auto syncEditor = [=] {
        const QSignalBlocker blocker(editor);
        editor->setValue(m_get());
    };
    syncEditor();

    QObject::connect(this, &Property::valueChanged, editor, syncEditor);
    QObject::connect(editor, qOverload<int>(&SpinBox::valueChanged),
                     this, m_set);

    return editor;
}

QWidget *FloatProperty::createEditor(QWidget *parent)
{
    auto editor = new DoubleSpinBox(parent);
    editor->setSuffix(m_suffix);

    auto syncEditor = [=] {
        const QSignalBlocker blocker(editor);
        editor->setValue(m_get());
    };
    syncEditor();

    QObject::connect(this, &Property::valueChanged, editor, syncEditor);
    QObject::connect(editor, qOverload<double>(&DoubleSpinBox::valueChanged),
                     this, m_set);

    return editor;
}

QWidget *BoolProperty::createEditor(QWidget *parent)
{
    auto editor = new QCheckBox(parent);
    auto syncEditor = [=] {
        const QSignalBlocker blocker(editor);
        bool checked = m_get();
        editor->setChecked(checked);
        editor->setText(checked ? tr("On") : tr("Off"));
    };
    syncEditor();

    QObject::connect(this, &Property::valueChanged, editor, syncEditor);
    QObject::connect(editor, &QCheckBox::toggled, this, [=](bool checked) {
        editor->setText(checked ? QObject::tr("On") : QObject::tr("Off"));
        m_set(checked);
    });

    return editor;
}

QWidget *PointProperty::createEditor(QWidget *parent)
{
    auto editor = new PointEdit(parent);
    auto syncEditor = [this, editor] {
        const QSignalBlocker blocker(editor);
        editor->setValue(m_get());
    };
    syncEditor();

    QObject::connect(this, &Property::valueChanged, editor, syncEditor);
    QObject::connect(editor, &PointEdit::valueChanged, this,
                     [this, editor] {
        m_set(editor->value());
    });

    return editor;
}

QWidget *PointFProperty::createEditor(QWidget *parent)
{
    auto editor = new PointFEdit(parent);
    auto syncEditor = [this, editor] {
        const QSignalBlocker blocker(editor);
        editor->setValue(this->value().toPointF());
    };
    syncEditor();

    QObject::connect(this, &Property::valueChanged, editor, syncEditor);
    QObject::connect(editor, &PointFEdit::valueChanged, this,
                     [this, editor] {
        this->setValue(editor->value());
    });

    return editor;
}

QWidget *SizeProperty::createEditor(QWidget *parent)
{
    auto editor = new SizeEdit(parent);
    auto syncEditor = [this, editor] {
        const QSignalBlocker blocker(editor);
        editor->setValue(m_get());
    };
    syncEditor();

    QObject::connect(this, &Property::valueChanged, editor, syncEditor);
    QObject::connect(editor, &SizeEdit::valueChanged, this,
                     [this, editor] {
        m_set(editor->value());
    });

    return editor;
}

QWidget *SizeFProperty::createEditor(QWidget *parent)
{
    auto editor = new SizeFEdit(parent);
    auto syncEditor = [this, editor] {
        const QSignalBlocker blocker(editor);
        editor->setValue(this->value().toSizeF());
    };
    syncEditor();

    QObject::connect(this, &Property::valueChanged, editor, syncEditor);
    QObject::connect(editor, &SizeFEdit::valueChanged, this,
                     [this, editor] {
        this->setValue(editor->value());
    });

    return editor;
}

QWidget *RectProperty::createEditor(QWidget *parent)
{
    auto editor = new RectEdit(parent);
    auto syncEditor = [this, editor] {
        const QSignalBlocker blocker(editor);
        editor->setValue(this->value().toRect());
    };
    syncEditor();

    QObject::connect(this, &Property::valueChanged, editor, syncEditor);
    QObject::connect(editor, &RectEdit::valueChanged, this,
                     [this, editor] {
                         this->setValue(editor->value());
                     });

    return editor;
}

QWidget *RectFProperty::createEditor(QWidget *parent)
{
    auto editor = new RectFEdit(parent);
    auto syncEditor = [this, editor] {
        const QSignalBlocker blocker(editor);
        editor->setValue(this->value().toRectF());
    };
    syncEditor();

    QObject::connect(this, &Property::valueChanged, editor, syncEditor);
    QObject::connect(editor, &RectFEdit::valueChanged, this,
                     [this, editor] {
        this->setValue(editor->value());
    });

    return editor;
}

// todo: needs to handle invalid color (unset value)
QWidget *ColorProperty::createEditor(QWidget *parent)
{
    auto editor = new ColorButton(parent);
    auto syncEditor = [=] {
        const QSignalBlocker blocker(editor);
        editor->setColor(this->value().value<QColor>());
    };
    syncEditor();

    QObject::connect(this, &Property::valueChanged, editor, syncEditor);
    QObject::connect(editor, &ColorButton::colorChanged, this,
                     [this, editor] {
        this->setValue(editor->color());
    });

    return editor;
}

QWidget *FontProperty::createEditor(QWidget *parent)
{
    auto editor = new QWidget(parent);
    auto layout = new QVBoxLayout(editor);
    auto fontComboBox = new QFontComboBox(editor);
    auto sizeSpinBox = new QSpinBox(editor);
    auto boldCheckBox = new QCheckBox(tr("Bold"), editor);
    auto italicCheckBox = new QCheckBox(tr("Italic"), editor);
    auto underlineCheckBox = new QCheckBox(tr("Underline"), editor);
    auto strikeoutCheckBox = new QCheckBox(tr("Strikeout"), editor);
    auto kerningCheckBox = new QCheckBox(tr("Kerning"), editor);
    sizeSpinBox->setRange(1, 999);
    sizeSpinBox->setSuffix(tr(" px"));
    sizeSpinBox->setKeyboardTracking(false);
    layout->setContentsMargins(QMargins());
    layout->setSpacing(Utils::dpiScaled(3));
    layout->addWidget(fontComboBox);
    layout->addWidget(sizeSpinBox);
    layout->addWidget(boldCheckBox);
    layout->addWidget(italicCheckBox);
    layout->addWidget(underlineCheckBox);
    layout->addWidget(strikeoutCheckBox);
    layout->addWidget(kerningCheckBox);

    auto syncEditor = [=] {
        const auto font = this->value().value<QFont>();
        const QSignalBlocker fontBlocker(fontComboBox);
        const QSignalBlocker sizeBlocker(sizeSpinBox);
        const QSignalBlocker boldBlocker(boldCheckBox);
        const QSignalBlocker italicBlocker(italicCheckBox);
        const QSignalBlocker underlineBlocker(underlineCheckBox);
        const QSignalBlocker strikeoutBlocker(strikeoutCheckBox);
        const QSignalBlocker kerningBlocker(kerningCheckBox);
        fontComboBox->setCurrentFont(font);
        sizeSpinBox->setValue(font.pixelSize());
        boldCheckBox->setChecked(font.bold());
        italicCheckBox->setChecked(font.italic());
        underlineCheckBox->setChecked(font.underline());
        strikeoutCheckBox->setChecked(font.strikeOut());
        kerningCheckBox->setChecked(font.kerning());
    };

    auto syncProperty = [=] {
        auto font = fontComboBox->currentFont();
        font.setPixelSize(sizeSpinBox->value());
        font.setBold(boldCheckBox->isChecked());
        font.setItalic(italicCheckBox->isChecked());
        font.setUnderline(underlineCheckBox->isChecked());
        font.setStrikeOut(strikeoutCheckBox->isChecked());
        font.setKerning(kerningCheckBox->isChecked());
        this->setValue(font);
    };

    syncEditor();

    QObject::connect(this, &Property::valueChanged, fontComboBox, syncEditor);
    QObject::connect(fontComboBox, &QFontComboBox::currentFontChanged, this, syncProperty);
    QObject::connect(sizeSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, syncProperty);
    QObject::connect(boldCheckBox, &QCheckBox::toggled, this, syncProperty);
    QObject::connect(italicCheckBox, &QCheckBox::toggled, this, syncProperty);
    QObject::connect(underlineCheckBox, &QCheckBox::toggled, this, syncProperty);
    QObject::connect(strikeoutCheckBox, &QCheckBox::toggled, this, syncProperty);
    QObject::connect(kerningCheckBox, &QCheckBox::toggled, this, syncProperty);

    return editor;
}

QWidget *AlignmentProperty::createEditor(QWidget *parent)
{
    auto editor = new QWidget(parent);
    auto layout = new QGridLayout(editor);
    layout->setContentsMargins(QMargins());
    layout->setSpacing(Utils::dpiScaled(3));

    auto horizontalLabel = new ElidingLabel(tr("Horizontal"), editor);
    layout->addWidget(horizontalLabel, 0, 0);

    auto verticalLabel = new ElidingLabel(tr("Vertical"), editor);
    layout->addWidget(verticalLabel, 1, 0);

    auto horizontalComboBox = new QComboBox(editor);
    horizontalComboBox->addItem(tr("Left"), Qt::AlignLeft);
    horizontalComboBox->addItem(tr("Center"), Qt::AlignHCenter);
    horizontalComboBox->addItem(tr("Right"), Qt::AlignRight);
    horizontalComboBox->addItem(tr("Justify"), Qt::AlignJustify);
    layout->addWidget(horizontalComboBox, 0, 1);

    auto verticalComboBox = new QComboBox(editor);
    verticalComboBox->addItem(tr("Top"), Qt::AlignTop);
    verticalComboBox->addItem(tr("Center"), Qt::AlignVCenter);
    verticalComboBox->addItem(tr("Bottom"), Qt::AlignBottom);
    layout->addWidget(verticalComboBox, 1, 1);

    layout->setColumnStretch(1, 1);

    auto syncEditor = [=] {
        const QSignalBlocker horizontalBlocker(horizontalComboBox);
        const QSignalBlocker verticalBlocker(verticalComboBox);
        const auto alignment = this->value().value<Qt::Alignment>();
        horizontalComboBox->setCurrentIndex(horizontalComboBox->findData(static_cast<int>(alignment & Qt::AlignHorizontal_Mask)));
        verticalComboBox->setCurrentIndex(verticalComboBox->findData(static_cast<int>(alignment & Qt::AlignVertical_Mask)));
    };

    auto syncProperty = [=] {
        const Qt::Alignment alignment(horizontalComboBox->currentData().toInt() |
                                      verticalComboBox->currentData().toInt());
        this->setValue(QVariant::fromValue(alignment));
    };

    syncEditor();

    QObject::connect(this, &Property::valueChanged, editor, syncEditor);
    QObject::connect(horizontalComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, syncProperty);
    QObject::connect(verticalComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, syncProperty);

    return editor;
}


ValueProperty::ValueProperty(const QString &name,
                             const QVariant &value,
                             EditorFactory *editorFactory,
                             QObject *parent)
    : AbstractProperty(name, editorFactory, parent)
    , m_value(value)
{}

void ValueProperty::setValue(const QVariant &value)
{
    if (m_value != value) {
        m_value = value;
        emit valueChanged();
    }
}


EnumProperty::EnumProperty(const QString &name,
                           QObject *parent)
    : AbstractProperty(name, &m_editorFactory, parent)
{}

void EnumProperty::setEnumNames(const QStringList &enumNames)
{
    m_editorFactory.setEnumNames(enumNames);
}

void EnumProperty::setEnumValues(const QList<int> &enumValues)
{
    m_editorFactory.setEnumValues(enumValues);
}


VariantEditor::VariantEditor(QWidget *parent)
    : QScrollArea(parent)
{
    m_widget = new QWidget;
    m_widget->setBackgroundRole(QPalette::AlternateBase);
    auto verticalLayout = new QVBoxLayout(m_widget);
    m_gridLayout = new QGridLayout;
    verticalLayout->addLayout(m_gridLayout);
    verticalLayout->addStretch();
    verticalLayout->setContentsMargins(0, 0, 0, Utils::dpiScaled(6));

    setWidget(m_widget);
    setWidgetResizable(true);

    m_gridLayout->setContentsMargins(QMargins());
    m_gridLayout->setSpacing(Utils::dpiScaled(3));

    m_gridLayout->setColumnStretch(LabelColumn, 2);
    m_gridLayout->setColumnStretch(WidgetColumn, 3);
    m_gridLayout->setColumnMinimumWidth(LeftSpacing, Utils::dpiScaled(3));
    m_gridLayout->setColumnMinimumWidth(MiddleSpacing, Utils::dpiScaled(2));
    m_gridLayout->setColumnMinimumWidth(RightSpacing, Utils::dpiScaled(3));

    // setValue(QVariantMap {
    //              { QStringLiteral("Name"), QVariant(QLatin1String("Hello")) },
    //              { QStringLiteral("Position"), QVariant(QPoint(15, 50)) },
    //              { QStringLiteral("Size"), QVariant(QSize(35, 400)) },
    //              { QStringLiteral("Rectangle"), QVariant(QRectF(15, 50, 35, 400)) },
    //              { QStringLiteral("Margin"), QVariant(10) },
    //              { QStringLiteral("Opacity"), QVariant(0.5) },
    //              { QStringLiteral("Visible"), true },
    //              { QStringLiteral("Object Alignment"), QVariant::fromValue(TopLeft) },
    //          });


    // setValue(QVariantList {
    //              QVariant(QLatin1String("Hello")),
    //              QVariant(10),
    //              QVariant(3.14)
    //          });
}

void VariantEditor::clear()
{
    QLayoutItem *item;
    while ((item = m_gridLayout->takeAt(0))) {
        delete item->widget();
        delete item;
    }
    m_rowIndex = 0;
}

void VariantEditor::addHeader(const QString &text)
{
    auto label = new ElidingLabel(text, m_widget);
    label->setBackgroundRole(QPalette::Dark);
    const int verticalMargin = Utils::dpiScaled(3);
    const int horizontalMargin = Utils::dpiScaled(6);
    label->setContentsMargins(horizontalMargin, verticalMargin,
                              horizontalMargin, verticalMargin);

    label->setAutoFillBackground(true);

    m_gridLayout->addWidget(label, m_rowIndex, 0, 1, ColumnCount);
    ++m_rowIndex;
}

void VariantEditor::addSeparator()
{
    auto separator = new QFrame(m_widget);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Plain);
    separator->setForegroundRole(QPalette::Mid);
    m_gridLayout->addWidget(separator, m_rowIndex, 0, 1, ColumnCount);
    ++m_rowIndex;
}

void VariantEditor::addProperty(Property *property)
{
    auto label = new LineEditLabel(property->name(), m_widget);
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    label->setToolTip(property->toolTip());
    label->setEnabled(property->isEnabled());
    connect(property, &Property::toolTipChanged, label, &QWidget::setToolTip);
    connect(property, &Property::enabledChanged, label, &QLabel::setEnabled);
    m_gridLayout->addWidget(label, m_rowIndex, LabelColumn, Qt::AlignTop/* | Qt::AlignRight*/);

    if (auto editor = createEditor(property)) {
        editor->setToolTip(property->toolTip());
        editor->setEnabled(property->isEnabled());
        connect(property, &Property::toolTipChanged, editor, &QWidget::setToolTip);
        connect(property, &Property::enabledChanged, editor, &QWidget::setEnabled);
        m_gridLayout->addWidget(editor, m_rowIndex, WidgetColumn);
    }

    ++m_rowIndex;
}

#if 0
void VariantEditor::addValue(const QVariant &value)
{
    const int type = value.userType();
    switch (type) {
    case QMetaType::QVariantList: {
        const auto list = value.toList();
        for (const auto &item : list)
            addValue(item);
        break;
    }
    case QMetaType::QVariantMap: {
        const auto map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it)
            addValue(it.key(), it.value());
        break;
    }
    default: {
        if (auto editor = createEditor(value))
            m_gridLayout->addWidget(editor, m_rowIndex, LabelColumn, 1, 3);
        else
            qDebug() << "No editor factory for type" << type;

        ++m_rowIndex;
    }
    }
}
#endif

QWidget *VariantEditor::createEditor(Property *property)
{
    if (const auto editor = property->createEditor(m_widget)) {
        editor->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        return editor;
    } else {
        qDebug() << "No editor for property" << property->name();
    }
    return nullptr;
}


EnumEditorFactory::EnumEditorFactory(const QStringList &enumNames,
                                     const QList<int> &enumValues)
    : m_enumNamesModel(enumNames)
    , m_enumValues(enumValues)
{}

void EnumEditorFactory::setEnumNames(const QStringList &enumNames)
{
    m_enumNamesModel.setStringList(enumNames);
}

void EnumEditorFactory::setEnumIcons(const QMap<int, QIcon> &enumIcons)
{
    // todo: add support for showing these icons in the QComboBox
    m_enumIcons = enumIcons;
}

void EnumEditorFactory::setEnumValues(const QList<int> &enumValues)
{
    m_enumValues = enumValues;
}

QWidget *EnumEditorFactory::createEditor(Property *property, QWidget *parent)
{
    auto editor = new QComboBox(parent);
    // This allows the combo box to shrink horizontally.
    editor->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    editor->setModel(&m_enumNamesModel);

    auto syncEditor = [property, editor, this] {
        const QSignalBlocker blocker(editor);
        if (m_enumValues.isEmpty())
            editor->setCurrentIndex(property->value().toInt());
        else
            editor->setCurrentIndex(m_enumValues.indexOf(property->value().toInt()));
    };
    syncEditor();

    QObject::connect(property, &Property::valueChanged, editor, syncEditor);
    QObject::connect(editor, qOverload<int>(&QComboBox::currentIndexChanged), property,
                     [property, this](int index) {
        property->setValue(m_enumValues.isEmpty() ? index : m_enumValues.at(index));
    });

    return editor;
}


void PropertyFactory::registerEditorFactory(int type, std::unique_ptr<EditorFactory> factory)
{
    m_factories[type] = std::move(factory);
}

Property *PropertyFactory::createQObjectProperty(QObject *qObject,
                                                 const char *name,
                                                 const QString &displayName)
{
    auto metaObject = qObject->metaObject();
    auto propertyIndex = metaObject->indexOfProperty(name);
    if (propertyIndex < 0)
        return nullptr;

    auto metaProperty = metaObject->property(propertyIndex);
    auto property = createProperty(displayName.isEmpty() ? QString::fromUtf8(name)
                                                         : displayName,
                                   [=] {
                                       return metaProperty.read(qObject);
                                   },
                                   [=] (const QVariant &value) {
                                       metaProperty.write(qObject, value);
                                   });

    // If the property has a notify signal, forward it to valueChanged
    auto notify = metaProperty.notifySignal();
    if (notify.isValid()) {
        auto propertyMetaObject = property->metaObject();
        auto valuePropertyIndex = propertyMetaObject->indexOfProperty("value");
        auto valueProperty = propertyMetaObject->property(valuePropertyIndex);
        auto valueChanged = valueProperty.notifySignal();

        QObject::connect(qObject, notify, property, valueChanged);
    }

    property->setEnabled(metaProperty.isWritable());

    return property;
}

ValueProperty *PropertyFactory::createProperty(const QString &name,
                                               const QVariant &value)
{
    auto f = m_factories.find(value.userType());
    return new ValueProperty(name, value,
                             f != m_factories.end() ? f->second.get()
                                                    : nullptr);
}

template<typename PropertyClass>
Property *createTypedProperty(const QString &name,
                              std::function<QVariant ()> get,
                              std::function<void (const QVariant &)> set)
{
    return new PropertyClass(name,
                             [get = std::move(get)] { return get().value<typename PropertyClass::ValueType>(); },
                             [set = std::move(set)] (typename PropertyClass::ValueType v) { set(QVariant::fromValue(v)); });
}

Property *PropertyFactory::createProperty(const QString &name,
                                          std::function<QVariant ()> get,
                                          std::function<void (const QVariant &)> set)
{
    const auto type = get().userType();
    switch (type) {
    case QMetaType::QString:
        return createTypedProperty<StringProperty>(name, get, set);
    case QMetaType::QUrl:
        return createTypedProperty<UrlProperty>(name, get, set);
    case QMetaType::Int:
        return createTypedProperty<IntProperty>(name, get, set);
    case QMetaType::Double:
        return createTypedProperty<FloatProperty>(name, get, set);
    case QMetaType::Bool:
        return createTypedProperty<BoolProperty>(name, get, set);
    case QMetaType::QColor:
        return createTypedProperty<ColorProperty>(name, get, set);
    case QMetaType::QFont:
        return createTypedProperty<FontProperty>(name, get, set);
    case QMetaType::QPoint:
        return createTypedProperty<PointProperty>(name, get, set);
    case QMetaType::QPointF:
        return createTypedProperty<PointFProperty>(name, get, set);
    case QMetaType::QRect:
        return createTypedProperty<RectProperty>(name, get, set);
    case QMetaType::QRectF:
        return createTypedProperty<RectFProperty>(name, get, set);
    case QMetaType::QSize:
        return createTypedProperty<SizeProperty>(name, get, set);
    case QMetaType::QSizeF:
        return createTypedProperty<SizeFProperty>(name, get, set);
    default:
        if (type == qMetaTypeId<Qt::Alignment>())
            return createTypedProperty<AlignmentProperty>(name, get, set);
    }

    // Fall back to registered factories approach (still used for enums)
    auto f = m_factories.find(get().userType());
    return new GetSetProperty(name, get, set,
                              f != m_factories.end() ? f->second.get()
                                                     : nullptr);
}

} // namespace Tiled

#include "moc_varianteditor.cpp"
