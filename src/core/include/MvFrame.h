#ifndef MVFRAME_H
#define MVFRAME_H

#include <vector>
#include <iostream>
#include <atomic>

#include <QThread>
#include <QWidget>
#include <QMutex>

#include "MvDefine.h"
#include "MvCamera.h"


class MvFrame : public QThread
{
	Q_OBJECT

public:
	explicit MvFrame(QObject *parent = nullptr, HWND hwnd = nullptr, CMvCamera* mCamera = nullptr);
	~MvFrame();

	void Start(int times = 9);
	void Stop();

	void ShowImage();
protected:
	void run() override;

signals:
	void ImageAvailable();

private:
	QMutex m_mutex;
	HWND p_hwnd = NULL;
	CMvCamera* p_mCamera = nullptr;

	std::atomic<bool> quit;
	std::atomic<int> m_times;

	std::vector<MV_CC_IMAGE> *ImageDataList = nullptr;
};

#endif // MVFRAME_H
