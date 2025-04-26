// maininterface.cpp
#include "maininterface.h"
#include "ui_maininterface.h"

#include <QMessageBox>

// 构造
MainInterface::MainInterface(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::MainInterface), m_pMvcamera(new CMvCamera())
{
	ui->setupUi(this);
	m_hwndDisplay = (HWND)ui->frameDisplayer->winId();

	// 初始化SDK和设备枚举
	if(uiInitialize() != MV_OK) {
		qWarning() << "Initialization failed!";
	}
	// 打印一次线程ID
	auto id = std::this_thread::get_id();
	std::cout << "MainInterface::MainInterface ID: " << id << std::endl;
}
// 析构
MainInterface::~MainInterface()
{
	// 先停止所有后台任务
	if (m_findDevicesWatcher.isRunning()) {
		m_findDevicesWatcher.cancel();
		m_findDevicesWatcher.waitForFinished();
	}
	// 关闭线程
	CloseThread();
	CloseCamera();

	// 释放资源
	if (m_devList) {
		delete m_devList;
		m_devList = nullptr;
	}
	// 反初始化SDK
	CMvCamera::FinalizeSDK();
	if (m_pMvcamera) {
		delete m_pMvcamera;
		m_pMvcamera = nullptr;	
	}

	delete ui;
}

// 类型转换		---------------------------------------------------
QString MainInterface::intToIpString(unsigned int ip)
{
	return QString("%1.%2.%3.%4")
		.arg((ip >> 24) & 0xFF)
		.arg((ip >> 16) & 0xFF)
		.arg((ip >> 8) & 0xFF)
		.arg(ip & 0xFF);
}
QString MainInterface::byteArrayToQString(const unsigned char* byteArray, int maxLength)
{
	// 更健壮的实现
	if (!byteArray || maxLength <= 0) return QString();

	// 自动检测字符串长度
	int length = strnlen(reinterpret_cast<const char*>(byteArray), maxLength);
	return QString::fromLocal8Bit(reinterpret_cast<const char*>(byteArray), length);
}

// 查找设备		---------------------------------------------------
int MainInterface::findDevices()
{
	ui->devicesComboBox->clear();
	if (m_findDevicesWatcher.isRunning()) {
		m_findDevicesWatcher.cancel();  // 可选：取消之前的任务
		m_findDevicesWatcher.waitForFinished();
	}

	// 使用QtConcurrent运行设备枚举
	QFuture<int> future = QtConcurrent::run([this]() {
		m_devList = new MV_CC_DEVICE_INFO_LIST;
		// MV_CC_DEVICE_INFO_LIST devList = {0};
		int Ret = CMvCamera::EnumDevices(
			MV_GIGE_DEVICE | MV_USB_DEVICE,
			// &devList
			m_devList
		);

		if (Ret != MV_OK)
		{
			qDebug() << "EnumDevices failed! Error code:" << Ret;
			return Ret;
		}

		// 遍历设备并添加到 ComboBox
		for (unsigned int i = 0; i < m_devList->nDeviceNum; i++)
		{
			MV_CC_DEVICE_INFO* pDeviceInfo = m_devList->pDeviceInfo[i];
			if (!pDeviceInfo)
				continue;

			// 提取设备名称（根据协议类型）
			QString strDeviceName;
			if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
			{
				QString manufacturer = QString::fromLocal8Bit(
					static_cast<const char*>(static_cast<void*>(pDeviceInfo->SpecialInfo.stGigEInfo.chManufacturerName)),
					32);  // 32是数组长度

				QString model = QString::fromLocal8Bit(
					static_cast<const char*>(static_cast<void*>(pDeviceInfo->SpecialInfo.stGigEInfo.chModelName)),
					32);
				QString ipStr = intToIpString(pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp);
				// GigE 设备：使用制造商名 + 型号名 + IP
				strDeviceName = QString("GigE %1 (%2) (%3)")
					.arg(manufacturer.trimmed())  // 去除可能的尾部空白
					.arg(model.trimmed())
					.arg(ipStr);
			}
			else if (pDeviceInfo->nTLayerType == MV_USB_DEVICE)
			{

				// USB 设备：使用制造商名 + 型号名
				QString manufacturer = byteArrayToQString(
					pDeviceInfo->SpecialInfo.stUsb3VInfo.chManufacturerName,
					INFO_MAX_BUFFER_SIZE);

				QString model = byteArrayToQString(
					pDeviceInfo->SpecialInfo.stUsb3VInfo.chModelName,
					INFO_MAX_BUFFER_SIZE);

				strDeviceName = QString("USB %1 (%2)")
					.arg(manufacturer.trimmed())
					.arg(model.trimmed());
			}
			else
			{
				// 其他设备类型
				strDeviceName = "Unknown Device";
			}

			// 使用设备名称作为显示文本，设备信息作为用户数据
			ui->devicesComboBox->addItem(strDeviceName, i);
		}

		return MV_OK;
	});

	m_findDevicesWatcher.setFuture(future);

	return 0;
}
// 打开设备		---------------------------------------------------
int MainInterface::OpenCamera()
{
	int nRet = MV_OK;
	int index = ui->devicesComboBox->currentData().toInt();
	MV_CC_DEVICE_INFO* pDeviceInfo = m_devList->pDeviceInfo[index];

	nRet = m_pMvcamera->Open(pDeviceInfo);
	if (nRet != MV_OK){
		if(nRet == MV_E_CALLORDER){
			QMessageBox::warning(this, QString("Function calling order error:%1").arg(nRet), "Please close device first.");
			WARNING() << "Please close device first.";
			return nRet;
		}
		ERROR() << QString("Device open err: %1").arg(nRet);
		return nRet;
	}
	nRet = m_pMvcamera->SetImageNodeNum(5);
	if (nRet != MV_OK){
		ERROR() << QString("SetImageNodeNum err: %1").arg(nRet);
	}
	// 初始化线程
	CloseThread();
	m_pFrame = new MvFrame(nullptr, m_hwndDisplay, m_pMvcamera);
	INFO() << "Device is ready.";

	setComponentsStatus(true);
	ui->startpbt->setEnabled(true);
	ui->openDevpbt->setEnabled(false); // 禁用打开设备按钮
	ui->stoppbt->setEnabled(false);
	return nRet;
}
// 关闭设备		---------------------------------------------------
int MainInterface::CloseCamera()
{
	int nRet = MV_OK;
	if (grabbingflag){
		// ch:停止抓图 | en:Stop Grabbing
		nRet = m_pMvcamera->StopGrabbing();
		if (nRet != MV_OK){
			ERROR() << QString("StopGrabbing err: %1").arg(nRet);
			CloseCamera();
			return nRet;
		}
		grabbingflag = 0;
	}
	nRet = m_pMvcamera->Close();
	return MV_OK;
}
// 初始化		---------------------------------------------------
int MainInterface::uiInitialize()
{
	// 组件初始化(全部禁用)
	setComponentsStatus(false);

	// 设备查找
	int nRet = CMvCamera::InitSDK();  // 确保这是CMvCamera的有效成员函数
	if (nRet != MV_OK) {
		ERROR() << "InitSDK failed! Error code:" << nRet;
		return nRet;
	}
	findDevices();

	// 槽函数绑定
	connect(&m_findDevicesWatcher, &QFutureWatcher<int>::finished,
			this, &MainInterface::onFindDevicesFinished);
	connect(ui->findDevpbt, &QPushButton::released, [this](){
		INFO() << "find bt Pushed.";
		findDevices();
	});
	connect(ui->openDevpbt, &QPushButton::released, [this](){
		OpenCamera();
		connect(m_pFrame, &MvFrame::ImageAvailable, this, [this]() {
			INFO() << "ImageAvailable";
			// 处理图像数据的逻辑，例如显示、保存等操作
			m_pFrame->ShowImage();
		}, Qt::QueuedConnection);
	});
	connect(ui->closeDevpbt, &QPushButton::released, [this](){
		CloseCamera();
		setComponentsStatus(false);
		ui->openDevpbt->setEnabled(true);
		INFO() << "Device is closed.";
	});
	// 强转触发模式
	connect(ui->spinExposure, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double value){
		if (!m_pMvcamera) return;
		int nRet = m_pMvcamera->SetFloatValue("ExposureTime", value);
		if (nRet != MV_OK) {
			ERROR() << QString("SetExposureTimeValue errer: %1").arg(nRet);
			return;
		}
	});
	connect(ui->spinGain, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double value){
		if (!m_pMvcamera) return;
		int nRet = m_pMvcamera->SetFloatValue("Gain", value);
		if (nRet != MV_OK) {
			ERROR() << QString("SetGainValue errer: %1").arg(nRet);
			return;
		}
	});

	return MV_OK;
}
// 控件状态设置	---------------------------------------------------
void MainInterface::setComponentsStatus(const bool status) {
	// 禁用所有按钮（除"查找设备"）
	QList<QPushButton*> allButtons = findChildren<QPushButton*>();
	for (auto btn : allButtons) {
		btn->setEnabled(status);
	}

	// 禁用所有组合框（除设备选择框）
	QList<QComboBox*> allCombos = findChildren<QComboBox*>();
	for (auto combo : allCombos) {
		combo->setEnabled(status);
	}

	// 禁用其他交互控件（单选按钮、输入框等）
	QList<QRadioButton*> allRadios = findChildren<QRadioButton*>();
	for (auto radio : allRadios) {
		radio->setEnabled(status);
	}
	// 禁用数字输入控件
	QList<QDoubleSpinBox*> allSpinBoxes = findChildren<QDoubleSpinBox*>();
	for (auto spin : allSpinBoxes) {
		spin->setEnabled(status);
	}

	ui->devicesComboBox->setEnabled(true); // 启用设备选择框
	ui->findDevpbt->setEnabled(true); // 启用查找设备按钮
}
// 关闭线程		---------------------------------------------------
void MainInterface::CloseThread(){
	if(m_pFrame){
		if(m_pFrame->isRunning()){
			m_pFrame->Stop();
			m_pFrame->wait();
			std::this_thread::sleep_for(std::chrono::microseconds(10));
		}
		delete m_pFrame;
		m_pFrame = nullptr;
	}
}

// 槽函数		---------------------------------------------------
void MainInterface::onFindDevicesFinished()
{
	int result = m_findDevicesWatcher.result();
	if (result == MV_OK) {
		qDebug() << QString::fromLocal8Bit("设备枚举成功！找到 %1 个设备").arg(ui->devicesComboBox->count());
		if(!ui->closeDevpbt->isEnabled())
			ui->openDevpbt->setEnabled(true);
	} else {
		qDebug() << QString::fromLocal8Bit("设备枚举失败！错误码： %1 ").arg(result);
	}
}
// 单次触发
void MainInterface::on_tigger1_released()
{
	if(ui->softTrigger->isChecked())
	{
		int tigger1 = 1;
		m_pFrame->Start(tigger1);
		INFO() << "on_tigger1_released";
	}
}
// 停止采图
void MainInterface::on_stoppbt_released()
{
	int nRet = MV_OK;
	ui->stoppbt->setEnabled(false);
	ui->startpbt->setEnabled(true);

	if (grabbingflag){
		// ch:停止抓图 | en:Stop Grabbing
		nRet = m_pMvcamera->StopGrabbing();
		if (nRet != MV_OK){
			ERROR() << QString("StopGrabbing err: %1").arg(nRet);
			CloseCamera();
			return;
		}
		grabbingflag = 0;
	}
}
// 开始采图
void MainInterface::on_startpbt_released()
{
	ui->stoppbt->setEnabled(true);
	ui->startpbt->setEnabled(false);
	if(grabbingflag) return;
	// ch:开启抓图 | en:Start Grabbing
	grabbingflag = 1;
	int nRet = m_pMvcamera->StartGrabbing();
	if (nRet != MV_OK){
		ERROR() << QString("StartGrabbing err: %1").arg(nRet);
		CloseCamera();
		return;
	}
	INFO() << "StartGrabbing";
}
// 模式切换
void MainInterface::on_triggerMode_clicked()
{
	// 修改控件状态
	ui->continuousMode->isChecked() == true ? ui->continuousMode->setChecked(false) : 1;
	ui->softTrigger->setEnabled(true);
	ui->tigger1->setEnabled(true);

	// 停止线程
	if(m_pFrame->isRunning()){
		m_pFrame->Stop();
	}
}
void MainInterface::on_continuousMode_clicked()
{
	// 修改控件状态
	ui->triggerMode->isChecked() == true ? ui->continuousMode->setChecked(false):1;
	ui->softTrigger->setEnabled(false);
	ui->tigger1->setEnabled(false);

	// 启动线程
	if(m_pFrame->isRunning()){
		m_pFrame->Stop();
	}
	m_pFrame->Start();
}

// 获取相机相关数据
void MainInterface::on_getParameter_clicked()
{
	int nRet = MV_OK;
	// 获取浮点类型
	MVCC_FLOATVALUE ExposureValue = {0};
	MVCC_FLOATVALUE GainValue = {0};
//	MVCC_FLOATVALUE FrameRate = {0};
	nRet = m_pMvcamera->GetFloatValue("ExposureTime", &ExposureValue);
	if (nRet != MV_OK){
		ERROR() << QString("GetExposureValue error:%1").arg(nRet);
		return;
	}
	nRet = m_pMvcamera->GetFloatValue("Gain", &GainValue);
	if (nRet != MV_OK){
		ERROR() << QString("GetGainValue error:%1").arg(nRet);
		return;
	}
//	nRet = m_pMvcamera->GetFloatValue("ResultingFrameRate ", &FrameRate);
//	if (nRet != MV_OK){
//		ERROR() << QString("GetResultingFrameRate error:%1").arg(nRet);
//		return;
//	}

	// 获取像素格式的当前枚举值
	MVCC_ENUMVALUE PixelFormatEnumValue = {0};
	nRet = m_pMvcamera->GetEnumValue("PixelFormat", &PixelFormatEnumValue);
	if (nRet != MV_OK) {
		ERROR() << QString("GetPixelFormatEnumValue errer: %1").arg(nRet);
		return;
	}

	// 根据枚举值获取对应的符号名称
	MVCC_ENUMENTRY PixelFormatEntry = {0};
	PixelFormatEntry.nValue = PixelFormatEnumValue.nCurValue;
	nRet = m_pMvcamera->GetEnumEntrySymbolic("PixelFormat", &PixelFormatEntry);
	if (nRet != MV_OK) {
		ERROR() << QString("GetPixelFormatEntry errer: %1").arg(nRet);
		return;
	}

	// 更新UI控件
	float ExposureTime = ExposureValue.fCurValue;
	float Gain = GainValue.fCurValue;
//	float Rate = FrameRate.fCurValue;
	QString PixelFormat = QString::fromLatin1(PixelFormatEntry.chSymbolic);

	ui->spinExposure->setValue(static_cast<double>(ExposureTime));
	ui->spinGain->setValue(static_cast<double>(Gain));
//	ui->spinFrameRate->setValue(static_cast<double>(Rate));
	ui->LinePixelFormat->setText(PixelFormat); // 确保控件名和函数名正确
}


void MainInterface::on_setParameter_clicked()
{
	int nRet = MV_OK;
	do{
		nRet = m_pMvcamera->SetFloatValue("ExposureTime", ui->spinExposure->value());
		nRet = m_pMvcamera->SetFloatValue("Gain", ui->spinGain->value());
	} while(0);
	if (nRet != MV_OK) {
		ERROR() << QString("SetFloatValue errer: %1").arg(nRet);
		return;
	}
}

void MainInterface::on_saveBMPpbt_clicked()
{
	m_pFrame->saveCurrentFrame(MV_Image_Bmp);
}

void MainInterface::on_saveJPGpbt_clicked()
{
	m_pFrame->saveCurrentFrame(MV_Image_Jpeg);
}

void MainInterface::on_saveTiffpbt_clicked()
{
	m_pFrame->saveCurrentFrame(MV_Image_Tif);
}

void MainInterface::on_savePNGpbt_clicked()
{
	m_pFrame->saveCurrentFrame(MV_Image_Png);
}

