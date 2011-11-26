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

    QString fileName() const;

public slots:
    void setFileFocus();
    void setFileName(const QString& fileName);

signals:
    void activated(const QString& fileName);
    void themeChange(const QString& themeName);

private slots:
    void activate();

private:
    Ui::Navigator *ui;
};

#endif // NAVIGATOR_H
