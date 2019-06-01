#ifndef MINIMUMEMPTYSPINBOX_H
#define MINIMUMEMPTYSPINBOX_H

#include <QSpinBox>

namespace QtGui {

class MinimumEmptySpinBox : public QSpinBox {
    Q_OBJECT
public:
    explicit MinimumEmptySpinBox(QWidget *parent = nullptr);

protected:
    int valueFromText(const QString &text) const override;
    QString textFromValue(int val) const override;
};

} // namespace QtGui

#endif // MINIMUMEMPTYSPINBOX_H
