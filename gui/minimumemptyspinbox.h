#ifndef MINIMUMEMPTYSPINBOX_H
#define MINIMUMEMPTYSPINBOX_H

#include <QSpinBox>

namespace QtGui {

class MinimumEmptySpinBox : public QSpinBox
{
    Q_OBJECT
public:
    explicit MinimumEmptySpinBox(QWidget *parent = nullptr);
        
protected:
    virtual int valueFromText(const QString &text) const;
    virtual QString textFromValue(int val) const;

};

}

#endif // MINIMUMEMPTYSPINBOX_H
