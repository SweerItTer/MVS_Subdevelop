# MVS_Subdevelop 工业相机控制套件

## 项目概述
🔍 基于MVS SDK开发的工业相机控制框架，提供设备管理、实时图像采集和基础分析功能。

### 核心特性
- 🖥️ 多厂商相机统一接入（GigE/USB3.0）
- 🎥 实时图像采集与帧率控制
- 📊 相机参数可视化配置
- 🖼️ OpenCV图像显示集成

## 📦 系统要求
- CMake 3.21+
- Qt 5/6
- MV_GenICam驱动

## 🛠️ 编译指南
```bash
mkdir build
cd build
cmake .. -DMVCAM_COMMON_RUNENV="[您的SDK安装路径]"
cmake --build . --config Release
```

## 项目结构
```
├── src/
│   ├── core/        # 相机控制核心（设备发现、参数配置、图像采集）
│   ├── gui/         # Qt可视化界面（设备管理、实时预览）
│   └── model/       # 数据模型（相机配置持久化、图像元数据）
```

## 现有功能
✅ 实时图像采集与显示  
✅ 相机参数动态调整  
✅ SDK异常处理机制

## 🚀 未来计划
### AI识别模块（开发中）
1. **智能检测框架**  
   - 基于OpenCV的ROI自动识别
   - 深度学习模型集成（YOLO/ResNet）
2. **分析功能**  
   - 实时缺陷检测
   - 产品计数统计
   - 尺寸测量算法
3. **标注工具**  
   - 支持创建训练数据集
   - PASCAL VOC/COCO格式导出
