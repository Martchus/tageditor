#include "minimumemptyspinbox.h"

namespace QtGui {

MinimumEmptySpinBox::MinimumEmptySpinBox(QWidget *parent) :
    QSpinBox(parent)
{}

int MinimumEmptySpinBox::valueFromText(const QString &text) const
{
    if(text.isEmpty())
        return minimum();
    return QSpinBox::valueFromText(text);
}

QString MinimumEmptySpinBox::textFromValue(int val) const
{
    if(val == minimum())
        return QString();
    return QSpinBox::textFromValue(val);
}

}
