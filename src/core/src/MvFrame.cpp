#include "MvFrame.h"
#include <QImage>

MvFrame::MvFrame(QObject *parent, HWND hwnd, CMvCamera *mCamera)
	: p_hwnd(hwnd), p_mCamera(mCamera), quit(false), m_times(0)  // 显式初始化原子变量
{
	ImageDataList = new std::vector<MV_CC_IMAGE>();
}

MvFrame::~MvFrame()
{
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
	} while(!quit.load() && m_times.load() > 1); // 退出标志位和运行次数
	// 退出前清理数据
	{
		QMutexLocker locker(&m_mutex);
		for (auto &img : *ImageDataList) {
			delete[] img.pImageBuf;
		}
		ImageDataList->clear();
	}
	std::cout << "Thread " << id << "stoped."<< std::endl;
}

void MvFrame::ShowImage() {
	QMutexLocker locker(&m_mutex); // 线程安全
	if (ImageDataList->empty()) return;

	MV_CC_IMAGE &frontImage = ImageDataList->front();

	// 1. 先拷贝元数据
	MV_CC_IMAGE pImage;
	pImage.nWidth = frontImage.nWidth;
	pImage.nHeight = frontImage.nHeight;
	pImage.enPixelType = frontImage.enPixelType;
	pImage.nImageLen = frontImage.nImageLen;

	// 2. 申请新内存，深拷贝数据
	pImage.pImageBuf = new unsigned char[pImage.nImageLen];
	memcpy(pImage.pImageBuf, frontImage.pImageBuf, pImage.nImageLen);
	DEBUG() << "Got Image.";

	// 3. 释放队列中的旧数据
	delete[] frontImage.pImageBuf;
	ImageDataList->erase(ImageDataList->begin());

	// 4. 传递给显示函数
	DEBUG() << "Showing.";
	p_mCamera->DisplayOneFrame(p_hwnd, &pImage);

	// 5. 释放深拷贝的数据
	delete[] pImage.pImageBuf;
}

