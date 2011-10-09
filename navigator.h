#ifndef NAVIGATOR_H
#define NAVIGATOR_H

#include <QWidget>

namespace Ui {
    class Navigator;
}

class Navigator : public QWidget
{
    Q_OBJECT

public:
    explicit Navigator(QWidget *parent = 0);
    ~Navigator();

signals:
    void activated(const QString& fileName);

private:
    Ui::Navigator *ui;
};

#endif // NAVIGATOR_H
