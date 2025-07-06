#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QtSql>
#include <QDialog>
#include <QMessageBox>

namespace Ui {
class registerdialog;
}

class registerdialog : public QDialog
{
	Q_OBJECT

public:
	explicit registerdialog(QWidget *parent = nullptr);
	~registerdialog();

private slots:
	void on_pushButton_clicked();

private:
	Ui::registerdialog *ui;
	QSqlDatabase mydb;
};

#endif // REGISTERDIALOG_H
