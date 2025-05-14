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
	enum DisplayMode {
		ContinuousMode,  // 连续模式（默认）
		SingleFrameMode  // 单帧模式
	};
	explicit MvFrame(QObject *parent = nullptr, HWND hwnd = nullptr, CMvCamera* mCamera = nullptr);
	~MvFrame();

	void Start(int times = 9);
	void Stop();

	void ShowImage();
	void setDisplayMode(DisplayMode mode);
	void saveCurrentFrame(MV_SAVE_IAMGE_TYPE type); // 保存接口
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
	std::atomic<DisplayMode> m_displayMode{ContinuousMode};
	MV_CC_IMAGE currentFrame{}; // 单帧模式下保存的当前帧

	std::vector<MV_CC_IMAGE> *ImageDataList = nullptr;
private:
	void CopyFrame(MV_CC_IMAGE& dst, const MV_CC_IMAGE& src);

};

#endif // MVFRAME_H
