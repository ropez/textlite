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
    static Navigator* instance();

    explicit Navigator(QWidget *parent = 0);
    ~Navigator();

public slots:
    void setFileFocus();

signals:
    void activated(const QString& fileName);
    void themeChange(const QString& themeName);

private slots:
    void activate();

private:
    Ui::Navigator *ui;
    static Navigator* s_instance;
};

#endif // NAVIGATOR_H
