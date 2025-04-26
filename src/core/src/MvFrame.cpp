#include "MvFrame.h"
#include <QImage>
#include <QDateTime>


MvFrame::MvFrame(QObject *parent, HWND hwnd, CMvCamera *mCamera)
	: p_hwnd(hwnd), p_mCamera(mCamera), quit(false), m_times(0)  // 显式初始化原子变量
{
	ImageDataList = new std::vector<MV_CC_IMAGE>();
}

MvFrame::~MvFrame()
{
	if (m_currentFrame.pImageBuf) {
		delete[] m_currentFrame.pImageBuf;
	}
	for (auto &img : *ImageDataList) {
		delete[] img.pImageBuf;
	}
	ImageDataList->clear();
	delete ImageDataList; // 确保释放
	ImageDataList = nullptr;
}

void MvFrame::Stop()
{
	quit.store(true);
}
void MvFrame::Start(int times)
{
	if (isRunning()) {      // 检查线程是否仍在运行
		quit.store(true);    // 请求退出
		wait();             // 等待线程完全结束
	}
	quit.store(false);
	if( times == 1 ) setDisplayMode(SingleFrameMode);
	else setDisplayMode(ContinuousMode);
	m_times.store(times);
	this->start();
}

void MvFrame::run()
{
	auto id = std::this_thread::get_id();
	std::cout << "Thread " << id << "running."<< std::endl;

	int nRet = MV_OK;
	MV_FRAME_OUT stImageInfo = {0};
	MV_CC_IMAGE stImageData = {0};
	do{
		p_mCamera->GetImageBuffer(&stImageInfo, 1000);
		if (nRet == MV_OK)
		{
			DEBUG() << "Get Image Buffer: Width[" << stImageInfo.stFrameInfo.nExtendWidth << "], "
					  << "Height[" << stImageInfo.stFrameInfo.nExtendHeight << "], "
					  << "FrameNum[" << stImageInfo.stFrameInfo.nFrameNum << "]\n";
			if (p_hwnd)
			{
				stImageData.nWidth = stImageInfo.stFrameInfo.nExtendWidth;
				stImageData.nHeight = stImageInfo.stFrameInfo.nExtendHeight;
				stImageData.enPixelType = stImageInfo.stFrameInfo.enPixelType;
				stImageData.nImageLen = stImageInfo.stFrameInfo.nFrameLenEx;
				// 申请新内存，确保数据独立
				stImageData.pImageBuf = new unsigned char[stImageData.nImageLen];
				memcpy(stImageData.pImageBuf, stImageInfo.pBufAddr, stImageData.nImageLen);

				{
					QMutexLocker locker(&m_mutex);

					// 先检查并释放旧数据，避免内存泄漏
					if (ImageDataList->size() >= 20) {
						delete[] ImageDataList->front().pImageBuf;
						ImageDataList->erase(ImageDataList->begin());
					}

					// 存入拷贝后的数据
					ImageDataList->push_back(stImageData);
				}
				emit ImageAvailable();
			}
			// 回收资源
			nRet = p_mCamera->FreeImageBuffer(&stImageInfo);
			if(nRet != MV_OK)
			{
				ERROR() << "Free Image Buffer fail! nRet: "<<nRet << "\n";
				continue;
			}
		} else
		{
			ERROR() << "Get Image fail! nRet: "<< nRet << "\n";
			break;
		}
		if( m_times.load() == 1 ) break;
	} while(!quit.load()); // 退出标志位和运行次数
	// 退出前清理数据
	if(quit.load() == true)
	{
		QMutexLocker locker(&m_mutex);
		for (auto &img : *ImageDataList) {
			delete[] img.pImageBuf;
		}
		ImageDataList->clear();
	}else{
		// 不清理队列直接退出采集线程
		quit.store(true);
		m_times.store(0);
	}
	std::cout << "Thread " << id << "stoped."<< std::endl;
}

// MvFrame.cpp 修改部分
void MvFrame::ShowImage() {
	QMutexLocker locker(&m_mutex);
	if (ImageDataList->empty()) return;

	MV_CC_IMAGE &frontImage = ImageDataList->front();

	// 深拷贝显示用数据
	MV_CC_IMAGE pImage;
	pImage.nWidth = frontImage.nWidth;
	pImage.nHeight = frontImage.nHeight;
	pImage.enPixelType = frontImage.enPixelType;
	pImage.nImageLen = frontImage.nImageLen;
	pImage.pImageBuf = new unsigned char[pImage.nImageLen];
	memcpy(pImage.pImageBuf, frontImage.pImageBuf, pImage.nImageLen);

	// 模式判断
	if (m_displayMode == SingleFrameMode) {
		// 单帧模式：保存当前帧副本
		if (m_currentFrame.pImageBuf) {
			delete[] m_currentFrame.pImageBuf;
		}
		m_currentFrame = pImage; // 直接转移所有权
	} else {
		// 连续模式：释放队列数据
		delete[] frontImage.pImageBuf;
		ImageDataList->erase(ImageDataList->begin());
	}

	// 显示图像
	p_mCamera->DisplayOneFrame(p_hwnd, &pImage);

	// 释放显示拷贝
	if (m_displayMode == ContinuousMode) {
		delete[] pImage.pImageBuf;
	}
}

void MvFrame::setDisplayMode(DisplayMode mode)
{
	m_displayMode.store(mode);
}

void MvFrame::saveCurrentFrame(MV_SAVE_IAMGE_TYPE type) {
	QMutexLocker locker(&m_mutex);

	if (!m_currentFrame.pImageBuf) {
		qWarning() << "No frame available for saving";
		return;
	}

	MV_CC_SAVE_IMAGE_PARAM stSaveParam;
	memset(&stSaveParam, 0, sizeof(MV_CC_SAVE_IMAGE_PARAM));
	stSaveParam.enImageType = type;
	stSaveParam.nQuality = 99;

	QString extension;
	switch (type) {
	case MV_Image_Bmp: extension = "bmp"; break;
	case MV_Image_Jpeg: extension = "jpg"; break;
	case MV_Image_Tif: extension = "tif"; break;
	case MV_Image_Png: extension = "png"; break;
	default: extension = "raw"; break;
	}

	// 生成带时间戳的文件名
	QString filename = QString("Capture_%1_%2x%3.%4")
		.arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmsszzz"))
		.arg(m_currentFrame.nWidth)
		.arg(m_currentFrame.nHeight)
		.arg(extension);

	// 调用SDK保存
	int ret = p_mCamera->SaveImageToFile(
		&m_currentFrame,
		&stSaveParam,
		filename.toLocal8Bit().constData()
	);

	if (ret != MV_OK) {
		qCritical() << "Save failed:" << ret;
	}
}
