#ifndef MVDEFINE_H
#define MVDEFINE_H
#include <QLoggingCategory>

#define DEBUG	    QMessageLogger(__FILE__, __LINE__, __FUNCTION__).debug
#define INFO		QMessageLogger(__FILE__, __LINE__, __FUNCTION__).info
#define WARNING		QMessageLogger(__FILE__, __LINE__, __FUNCTION__).warning
#define ERROR   	QMessageLogger(__FILE__, __LINE__, __FUNCTION__).critical

#endif // MVDEFINE_H
