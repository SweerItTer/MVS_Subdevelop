#ifndef MAININTERFACE_H
#define MAININTERFACE_H

#include <iostream>

#include <QWidget>
#include <QFutureWatcher>
#include <qDebug>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>

#include "MvDefine.h"
#include "MvCamera.h"
#include "MvFrame.h"

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class MainInterface; }
QT_END_NAMESPACE

class MainInterface : public QWidget
{
	Q_OBJECT

public:
	MainInterface(QWidget *parent = nullptr);
	~MainInterface();

private slots:
	// 保存图像
	void on_saveTiffpbt_clicked();
	void on_saveJPGpbt_clicked();
	void on_saveBMPpbt_clicked();
	void on_savePNGpbt_clicked();
	// 设置参数
	void on_setParameter_clicked();
	void on_getParameter_clicked();

	void on_tigger1_released();
	void on_continuousMode_clicked();
	void on_triggerMode_clicked();
	void on_startpbt_released();
	void on_stoppbt_released();

	void onFindDevicesFinished();

private:
	// 查找设备
	int findDevices();
	// 打开摄像头
	int OpenCamera();
	// 关闭摄像头
	int CloseCamera();
	// 设置控件状态
	void setComponentsStatus(bool status);
	// 控件初始化及信号绑定
	int uiInitialize();
	// 关闭取图线程
	void CloseThread();
	// 状态
	int grabbingflag = 0;
	//显示控件的句柄
	HWND m_hwndDisplay = NULL;

	QString byteArrayToQString(const unsigned char* byteArray, int maxLength);
	QString intToIpString(unsigned int ip);

	MV_CC_DEVICE_INFO_LIST* m_devList = nullptr;
	QFutureWatcher<int> m_findDevicesWatcher; // 用于监控线程状态

	MvFrame* m_pFrame = nullptr;
	CMvCamera *m_pMvcamera = nullptr;
	Ui::MainInterface *ui;
};
#endif // MAININTERFACE_H
