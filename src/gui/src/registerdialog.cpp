/*
 * @Author: SweerItTer xxxzhou.xian@gmail.com
 * @Date: 2025-07-05 17:19:35
 * @LastEditors: SweerItTer xxxzhou.xian@gmail.com
 * @LastEditTime: 2025-07-07 00:58:41
 * @FilePath: \mvsbase\src\gui\src\registerdialog.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "registerdialog.h"
#include "ui_registerdialog.h"

registerdialog::registerdialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::registerdialog)
{
	ui->setupUi(this);
}

registerdialog::~registerdialog()
{
	delete ui;
}

void registerdialog::on_pushButton_clicked()
{
	QString username = ui->lineEdit_username->text();
	QString password = ui->lineEdit_password->text();
	QString confirmpwd = ui->lineEdit_cpwd->text();

	if(username.isEmpty() || password.isEmpty()) {
		QMessageBox::warning(this, "warning", "The username and password cannot be empty.");
		return;
	} else if (confirmpwd != password){
		QMessageBox::warning(this,"warning","ConfirmPassword don't match.");
	}

	QSqlQuery qry;
	// 先检查用户名是否已存在
	qry.prepare("SELECT * FROM login WHERE name = :username");
	qry.bindValue(":username", username);
	if(!qry.exec()) {
		QMessageBox::critical(this, "Error", "Query failed:" + qry.lastError().text());
		return;
	}
	if(qry.next()) {
		QMessageBox::warning(this, "warning", "The username already exists");
		return;
	}

	// 插入新用户
	qry.prepare("INSERT INTO login (name, password) VALUES (:username, :password)");
	qry.bindValue(":username", username);
	qry.bindValue(":password", password);

	if(!qry.exec()) {
		QMessageBox::critical(this, "Error", "Registration failed:" + qry.lastError().text());
		return;
	}

	QMessageBox::information(this, "succeed", "The registration is successful, please return to log in");
	accept();  // 关闭对话框
}

