#include "registerdialog.h"

#include "login.h"
#include "ui_login.h"
#include "dialog.h"

Login::Login(QWidget *parent, QString dbpath) :
    QMainWindow(parent), dbpath_(dbpath),
    ui(new Ui::Login)
{
    ui->setupUi(this);

    mydb=QSqlDatabase::addDatabase("QSQLITE");
	mydb.setDatabaseName(dbpath_); //change the directory path based on ur database

    if(mydb.open())
        ui->label_4->setText("Connected...");
    else
        ui->label_4->setText("Dis-Connected...");
}

Login::~Login()
{
    delete ui;
}
void Login::on_pushButton_clicked()
{
	QString username = ui->lineEdit_username->text();
	QString password = ui->lineEdit_2_password->text();

	if(!mydb.open())
	{
		qDebug() << "Not Connected to DataBase";
		return;
	}

	QSqlQuery qry;
	qry.prepare("SELECT * FROM login WHERE name = :username AND password = :password");
	qry.bindValue(":username", username);
	qry.bindValue(":password", password);

	if(!qry.exec()) {	
		ui->label_4->setText("Query failed: " + qry.lastError().text());
		return;
	}

	if(qry.next()) {
		mvc_w = std::make_unique<MainInterface>();
		// User found
		ui->label_4->setText("Username and password are correct");
		this->hide();
		mvc_w->show();
	} else {
//		 User not found, show registration dialog
		ui->label_4->setText("User not found, please register first");
		registerdialog regDialog(this);
		if(regDialog.exec() == QDialog::Accepted) {
			ui->label_4->setText("Registration successful, please log in again");
		} else {
			ui->label_4->setText("Registration cancelled");
		}
	}
}
