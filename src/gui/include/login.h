#ifndef LOGIN_H
#define LOGIN_H

#include <QMainWindow>
#include "QtDebug"
#include "QtSql"
#include "QFileInfo"
#include "dialog.h"

#include "maininterface.h"

namespace Ui {
class Login;
}

class Login : public QMainWindow
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = 0, QString dbpath = "");
    ~Login();
    
private slots:
    void on_pushButton_clicked();

private:
    std::unique_ptr<MainInterface> mvc_w = nullptr;
    Ui::Login *ui;
    QSqlDatabase mydb;
    QString dbpath_;
};

#endif // LOGIN_H
