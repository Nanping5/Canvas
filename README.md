# Qt Canvas Drawing Application / Qt画布绘图应用

## 项目简介 / Project Introduction
本项目是一个基于Qt的绘图工具，支持多种绘图算法和动画效果。  
This project is a drawing tool based on Qt, supporting various drawing algorithms and animation effects.

## 功能特点 / Features
### 绘图工具 / Drawing Tools
- **自由绘制** / Freehand Drawing
- **直线** / Lines
  - Bresenham算法 / Bresenham Algorithm
  - 中点算法 / Midpoint Algorithm
- **圆** / Circles
- **多边形** / Polygons
- **贝塞尔曲线** / Bezier Curves
- **圆弧** / Arcs
- **填充工具** / Fill Tool
- **橡皮擦** / Eraser

### 线型 / Line Styles
- 实线 / Solid
- 虚线 / Dashed
- 点线 / Dotted

### 高级功能 / Advanced Features
- **裁剪** / Clipping
  - Cohen-Sutherland算法 / Cohen-Sutherland Algorithm
  - 中点分割算法 / Midpoint Subdivision Algorithm
- **变换** / Transformations
  - 旋转 / Rotation
  - 缩放 / Scaling
- **选择与移动** / Selection and Movement
- **动画窗口** / Animation Window
  - 烟花效果 / Fireworks Effect
  - 粒子系统 / Particle System
  - 颜色模式切换 / Color Mode Switching

### 界面功能 / UI Features
- 可定制工具栏 / Customizable Toolbar
- 实时预览 / Real-time Preview
- 缩放与平移 / Zoom and Pan
- 保存与加载 / Save and Load

## 使用说明 / Usage
1. **编译与运行** / Build and Run
   - 本项目基于Qt框架开发，需要安装Qt 5或Qt 6及相关的Qt Creator。  
     This project is developed based on the Qt framework. Qt 5 or Qt 6 and Qt Creator are required.
   - 克隆或下载本项目代码后，使用Qt Creator打开CmakeLists.txt工程文件。  
     After cloning or downloading the project code, open the CmakeLists.txt project file with Qt Creator.
   - 编译并运行项目。  
     Compile and run the project.

2. **绘图方式** / Drawing Methods
   - **绘制直线** / Draw Lines: 按下鼠标左键，拖动鼠标并释放，完成直线绘制。  
     Press the left mouse button, drag the mouse and release to complete the line drawing.
   - **绘制空心圆** / Draw Hollow Circles: 按下鼠标左键，选定圆心，拖动鼠标至合适半径后释放，即可绘制完整空心圆。  
     Press the left mouse button to select the center, drag the mouse to the appropriate radius and release to draw a complete hollow circle.
   - **绘制圆弧** / Draw Arcs (Optional): 如果需要部分圆弧，可以调整 `startAngle` 和 `endAngle`。  
     If you need a partial arc, you can adjust `startAngle` and `endAngle`.

3. **调整画笔样式** / Adjust Pen Style
   - 可修改 `penColor` 变量以改变绘制颜色。  
     Modify the `penColor` variable to change the drawing color.
   - 通过 `penWidth` 调整线条粗细。  
     Adjust the line thickness with `penWidth`.
   - 通过 `lineStyle` 选择不同的线条风格，如实线、虚线等。  
     Choose different line styles, such as solid, dashed, etc., with `lineStyle`.

## 许可证 / License
本项目采用MIT许可证，欢迎自由使用和修改。  
This project is licensed under the MIT License, and you are welcome to use and modify it freely.

---
如有任何问题或建议，请提交issue或fork本项目进行改进！  
If you have any questions or suggestions, please submit an issue or fork this project to improve it!
