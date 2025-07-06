#include "login.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	Login lg_w(0,"D:\\UserFile\\Desktop\\v1.0\\sourceCode\\mvsbase\\src\\model\\database.db");
	lg_w.show();
	return a.exec();
}
