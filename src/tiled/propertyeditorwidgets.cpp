/*
 * propertyeditorwidgets.cpp
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

#include "propertyeditorwidgets.h"

#include "utils.h"

#include <QGridLayout>
#include <QPainter>
#include <QResizeEvent>
#include <QStyle>
#include <QStyleOption>
#include <QStylePainter>

namespace Tiled {

/**
 * Strips a floating point number representation of redundant trailing zeros.
 * Examples:
 *
 *  0.01000 -> 0.01
 *  3.000   -> 3.0
 */
static QString removeRedundantTrialingZeros(const QString &text)
{
    const QString decimalPoint = QLocale::system().decimalPoint();
    const auto decimalPointIndex = text.lastIndexOf(decimalPoint);
    if (decimalPointIndex < 0) // return if there is no decimal point
        return text;

    const auto afterDecimalPoint = decimalPointIndex + decimalPoint.length();
    int redundantZeros = 0;

    for (int i = text.length() - 1; i > afterDecimalPoint && text.at(i) == QLatin1Char('0'); --i)
        ++redundantZeros;

    return text.left(text.length() - redundantZeros);
}


SpinBox::SpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    // Allow the full range by default.
    setRange(std::numeric_limits<int>::lowest(),
             std::numeric_limits<int>::max());

    // Don't respond to keyboard input immediately.
    setKeyboardTracking(false);

    // Allow the widget to shrink horizontally.
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

QSize SpinBox::minimumSizeHint() const
{
    // Don't adjust the horizontal size hint based on the maximum value.
    auto hint = QSpinBox::minimumSizeHint();
    hint.setWidth(Utils::dpiScaled(50));
    return hint;
}


DoubleSpinBox::DoubleSpinBox(QWidget *parent)
    : QDoubleSpinBox(parent)
{
    // Allow the full range by default.
    setRange(std::numeric_limits<double>::lowest(),
             std::numeric_limits<double>::max());

    // Increase possible precision.
    setDecimals(9);

    // Don't respond to keyboard input immediately.
    setKeyboardTracking(false);

    // Allow the widget to shrink horizontally.
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

QSize DoubleSpinBox::minimumSizeHint() const
{
    // Don't adjust the horizontal size hint based on the maximum value.
    auto hint = QDoubleSpinBox::minimumSizeHint();
    hint.setWidth(Utils::dpiScaled(50));
    return hint;
}

QString DoubleSpinBox::textFromValue(double val) const
{
    auto text = QDoubleSpinBox::textFromValue(val);

    // remove redundant trailing 0's in case of high precision
    if (decimals() > 3)
        return removeRedundantTrialingZeros(text);

    return text;
}


ResponsivePairswiseWidget::ResponsivePairswiseWidget(QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QGridLayout(this);
    layout->setContentsMargins(QMargins());
    layout->setColumnStretch(1, 1);
    layout->setSpacing(Utils::dpiScaled(3));
}

void ResponsivePairswiseWidget::setWidgetPairs(const QVector<WidgetPair> &widgetPairs)
{
    const int horizontalMargin = Utils::dpiScaled(3);

    for (auto &pair : widgetPairs) {
        pair.label->setAlignment(Qt::AlignCenter);
        pair.label->setContentsMargins(horizontalMargin, 0, horizontalMargin, 0);
    }

    m_widgetPairs = widgetPairs;

    addWidgetsToLayout();
}

void ResponsivePairswiseWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    const auto orientation = event->size().width() < minimumHorizontalWidth()
            ? Qt::Vertical : Qt::Horizontal;

    if (m_orientation != orientation) {
        m_orientation = orientation;

        auto layout = this->layout();

        // Remove all widgets from layout, without deleting them
        for (auto &pair : m_widgetPairs) {
            layout->removeWidget(pair.label);
            layout->removeWidget(pair.widget);
        }

        addWidgetsToLayout();

        // This avoids flickering when the layout changes
        layout->activate();
    }
}

void ResponsivePairswiseWidget::addWidgetsToLayout()
{
    auto layout = qobject_cast<QGridLayout *>(this->layout());

    const int maxColumns = m_orientation == Qt::Horizontal ? 4 : 2;
    int row = 0;
    int column = 0;

    for (auto &pair : m_widgetPairs) {
        layout->addWidget(pair.label, row, column);
        layout->addWidget(pair.widget, row, column + 1);
        column += 2;

        if (column == maxColumns) {
            column = 0;
            ++row;
        }
    }

    layout->setColumnStretch(3, m_orientation == Qt::Horizontal ? 1 : 0);
}

int ResponsivePairswiseWidget::minimumHorizontalWidth() const
{
    const int spacing = layout()->spacing();
    int sum = 0;
    int minimum = 0;
    int index = 0;

    for (auto &pair : m_widgetPairs) {
        sum += (pair.label->minimumSizeHint().width() +
                pair.widget->minimumSizeHint().width() +
                spacing * 2);

        if (++index % 2 == 0) {
            minimum = std::max(sum - spacing, minimum);
            sum = 0;
        }
    }

    return minimum;
}


SizeEdit::SizeEdit(QWidget *parent)
    : ResponsivePairswiseWidget(parent)
    , m_widthLabel(new QLabel(QStringLiteral("W"), this))
    , m_heightLabel(new QLabel(QStringLiteral("H"), this))
    , m_widthSpinBox(new SpinBox(this))
    , m_heightSpinBox(new SpinBox(this))
{
    setWidgetPairs({
                       { m_widthLabel, m_widthSpinBox },
                       { m_heightLabel, m_heightSpinBox },
                   });

    connect(m_widthSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &SizeEdit::valueChanged);
    connect(m_heightSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &SizeEdit::valueChanged);
}

void SizeEdit::setValue(const QSize &size)
{
    m_widthSpinBox->setValue(size.width());
    m_heightSpinBox->setValue(size.height());
}

QSize SizeEdit::value() const
{
    return QSize(m_widthSpinBox->value(),
                 m_heightSpinBox->value());
}

void SizeEdit::setMinimum(int minimum)
{
    m_widthSpinBox->setMinimum(minimum);
    m_heightSpinBox->setMinimum(minimum);
}


SizeFEdit::SizeFEdit(QWidget *parent)
    : ResponsivePairswiseWidget(parent)
    , m_widthLabel(new QLabel(QStringLiteral("W"), this))
    , m_heightLabel(new QLabel(QStringLiteral("H"), this))
    , m_widthSpinBox(new DoubleSpinBox(this))
    , m_heightSpinBox(new DoubleSpinBox(this))
{
    setWidgetPairs({
                       { m_widthLabel, m_widthSpinBox },
                       { m_heightLabel, m_heightSpinBox },
                   });

    connect(m_widthSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &SizeFEdit::valueChanged);
    connect(m_heightSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &SizeFEdit::valueChanged);
}

void SizeFEdit::setValue(const QSizeF &size)
{
    m_widthSpinBox->setValue(size.width());
    m_heightSpinBox->setValue(size.height());
}

QSizeF SizeFEdit::value() const
{
    return QSizeF(m_widthSpinBox->value(),
                  m_heightSpinBox->value());
}


PointEdit::PointEdit(QWidget *parent)
    : ResponsivePairswiseWidget(parent)
    , m_xLabel(new QLabel(QStringLiteral("X"), this))
    , m_yLabel(new QLabel(QStringLiteral("Y"), this))
    , m_xSpinBox(new SpinBox(this))
    , m_ySpinBox(new SpinBox(this))
{
    setWidgetPairs({
                       { m_xLabel, m_xSpinBox },
                       { m_yLabel, m_ySpinBox },
                   });

    connect(m_xSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &PointEdit::valueChanged);
    connect(m_ySpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &PointEdit::valueChanged);
}

void PointEdit::setValue(const QPoint &point)
{
    m_xSpinBox->setValue(point.x());
    m_ySpinBox->setValue(point.y());
}

QPoint PointEdit::value() const
{
    return QPoint(m_xSpinBox->value(),
                  m_ySpinBox->value());
}


PointFEdit::PointFEdit(QWidget *parent)
    : ResponsivePairswiseWidget(parent)
    , m_xLabel(new QLabel(QStringLiteral("X"), this))
    , m_yLabel(new QLabel(QStringLiteral("Y"), this))
    , m_xSpinBox(new DoubleSpinBox(this))
    , m_ySpinBox(new DoubleSpinBox(this))
{
    setWidgetPairs({
                       { m_xLabel, m_xSpinBox },
                       { m_yLabel, m_ySpinBox },
                   });

    connect(m_xSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &PointFEdit::valueChanged);
    connect(m_ySpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &PointFEdit::valueChanged);
}

void PointFEdit::setValue(const QPointF &point)
{
    m_xSpinBox->setValue(point.x());
    m_ySpinBox->setValue(point.y());
}

QPointF PointFEdit::value() const
{
    return QPointF(m_xSpinBox->value(),
                   m_ySpinBox->value());
}

void PointFEdit::setSingleStep(double step)
{
    m_xSpinBox->setSingleStep(step);
    m_ySpinBox->setSingleStep(step);
}


RectEdit::RectEdit(QWidget *parent)
    : ResponsivePairswiseWidget(parent)
    , m_xLabel(new QLabel(QStringLiteral("X"), this))
    , m_yLabel(new QLabel(QStringLiteral("Y"), this))
    , m_widthLabel(new QLabel(QStringLiteral("W"), this))
    , m_heightLabel(new QLabel(QStringLiteral("H"), this))
    , m_xSpinBox(new SpinBox(this))
    , m_ySpinBox(new SpinBox(this))
    , m_widthSpinBox(new SpinBox(this))
    , m_heightSpinBox(new SpinBox(this))
{
    setWidgetPairs({
                    { m_xLabel, m_xSpinBox },
                    { m_yLabel, m_ySpinBox },
                    { m_widthLabel, m_widthSpinBox },
                    { m_heightLabel, m_heightSpinBox },
                    });

    m_widthSpinBox->setMinimum(0);
    m_heightSpinBox->setMinimum(0);

    connect(m_xSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &RectEdit::valueChanged);
    connect(m_ySpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &RectEdit::valueChanged);
    connect(m_widthSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &RectEdit::valueChanged);
    connect(m_heightSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &RectEdit::valueChanged);
}

void RectEdit::setValue(const QRect &rect)
{
    m_xSpinBox->setValue(rect.x());
    m_ySpinBox->setValue(rect.y());
    m_widthSpinBox->setValue(rect.width());
    m_heightSpinBox->setValue(rect.height());
}

QRect RectEdit::value() const
{
    return QRect(m_xSpinBox->value(),
                 m_ySpinBox->value(),
                 m_widthSpinBox->value(),
                 m_heightSpinBox->value());
}

void RectEdit::setConstraint(const QRect &constraint)
{
    if (constraint.isNull()) {
        m_xSpinBox->setRange(std::numeric_limits<int>::lowest(),
                             std::numeric_limits<int>::max());
        m_ySpinBox->setRange(std::numeric_limits<int>::lowest(),
                             std::numeric_limits<int>::max());
        m_widthSpinBox->setRange(0, std::numeric_limits<int>::max());
        m_heightSpinBox->setRange(0, std::numeric_limits<int>::max());
    } else {
        m_xSpinBox->setRange(constraint.left(), constraint.right() + 1);
        m_ySpinBox->setRange(constraint.top(), constraint.bottom() + 1);
        m_widthSpinBox->setRange(0, constraint.width());
        m_heightSpinBox->setRange(0, constraint.height());
    }
}


RectFEdit::RectFEdit(QWidget *parent)
    : ResponsivePairswiseWidget(parent)
    , m_xLabel(new QLabel(QStringLiteral("X"), this))
    , m_yLabel(new QLabel(QStringLiteral("Y"), this))
    , m_widthLabel(new QLabel(QStringLiteral("W"), this))
    , m_heightLabel(new QLabel(QStringLiteral("H"), this))
    , m_xSpinBox(new DoubleSpinBox(this))
    , m_ySpinBox(new DoubleSpinBox(this))
    , m_widthSpinBox(new DoubleSpinBox(this))
    , m_heightSpinBox(new DoubleSpinBox(this))
{
    setWidgetPairs({
                       { m_xLabel, m_xSpinBox },
                       { m_yLabel, m_ySpinBox },
                       { m_widthLabel, m_widthSpinBox },
                       { m_heightLabel, m_heightSpinBox },
                   });

    connect(m_xSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &RectFEdit::valueChanged);
    connect(m_ySpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &RectFEdit::valueChanged);
    connect(m_widthSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &RectFEdit::valueChanged);
    connect(m_heightSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &RectFEdit::valueChanged);
}

void RectFEdit::setValue(const QRectF &rect)
{
    m_xSpinBox->setValue(rect.x());
    m_ySpinBox->setValue(rect.y());
    m_widthSpinBox->setValue(rect.width());
    m_heightSpinBox->setValue(rect.height());
}

QRectF RectFEdit::value() const
{
    return QRectF(m_xSpinBox->value(),
                  m_ySpinBox->value(),
                  m_widthSpinBox->value(),
                  m_heightSpinBox->value());
}


ElidingLabel::ElidingLabel(QWidget *parent)
    : ElidingLabel(QString(), parent)
{}

ElidingLabel::ElidingLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
}

QSize ElidingLabel::minimumSizeHint() const
{
    auto hint = QLabel::minimumSizeHint();
    hint.setWidth(std::min(hint.width(), Utils::dpiScaled(30)));
    return hint;
}

void ElidingLabel::paintEvent(QPaintEvent *)
{
    const int m = margin();
    const QRect cr = contentsRect().adjusted(m, m, -m, -m);
    const Qt::LayoutDirection dir = text().isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight;
    const int align = QStyle::visualAlignment(dir, alignment());
    const int flags = align | (dir == Qt::LeftToRight ? Qt::TextForceLeftToRight
                                                      : Qt::TextForceRightToLeft);

    QStyleOption opt;
    opt.initFrom(this);

    const auto elidedText = opt.fontMetrics.elidedText(text(), Qt::ElideRight, cr.width());
    const bool isElided = elidedText != text();

    if (isElided != m_isElided) {
        m_isElided = isElided;
        setToolTip(isElided ? text() : QString());
    }

    QPainter painter(this);
    QWidget::style()->drawItemText(&painter, cr, flags, opt.palette, isEnabled(), elidedText, foregroundRole());
}


HeaderWidget::HeaderWidget(const QString &text, QWidget *parent)
    : ElidingLabel(text, parent)
{
    setBackgroundRole(QPalette::Dark);
    setForegroundRole(QPalette::BrightText);
    setAutoFillBackground(true);

    const int verticalMargin = Utils::dpiScaled(3);
    const int horizontalMargin = Utils::dpiScaled(6);
    const int branchIndicatorWidth = Utils::dpiScaled(14);
    setContentsMargins(horizontalMargin + branchIndicatorWidth,
                       verticalMargin, horizontalMargin, verticalMargin);
}

void HeaderWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_checked = !m_checked;
        emit toggled(m_checked);
    }

    ElidingLabel::mousePressEvent(event);
}

void HeaderWidget::paintEvent(QPaintEvent *)
{
    const QRect cr = contentsRect();
    const Qt::LayoutDirection dir = text().isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight;
    const int align = QStyle::visualAlignment(dir, {});
    const int flags = align | (dir == Qt::LeftToRight ? Qt::TextForceLeftToRight
                                                      : Qt::TextForceRightToLeft);

    QStyleOption branchOption;
    branchOption.initFrom(this);
    branchOption.rect = QRect(0, 0, contentsMargins().left(), height());
    branchOption.state = QStyle::State_Children;
    if (m_checked)
        branchOption.state |= QStyle::State_Open;

    QStylePainter p(this);
    p.drawPrimitive(QStyle::PE_IndicatorBranch, branchOption);

    QStyleOption opt;
    opt.initFrom(this);

    const auto elidedText = opt.fontMetrics.elidedText(text(), Qt::ElideRight, cr.width());

    p.drawItemText(cr, flags, opt.palette, isEnabled(), elidedText, foregroundRole());
}


QSize LineEditLabel::sizeHint() const
{
    auto hint = ElidingLabel::sizeHint();
    hint.setHeight(m_lineEdit.sizeHint().height());
    return hint;
}

} // namespace Tiled

#include "moc_propertyeditorwidgets.cpp"
