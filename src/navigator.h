#ifndef NAVIGATOR_H
#define NAVIGATOR_H

#include <QWidget>

class QLineEdit;
class QComboBox;

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
    void setThemeNames(const QStringList& themeList);

signals:
    void activated(const QString& fileName);
    void themeChange(const QString& themeName);

private slots:
    void activate();

private:
    QLineEdit *pathEdit;
    QComboBox *themeSelector;
};

#endif // NAVIGATOR_H
