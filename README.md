# Direct3D 11 龙书习题解答

# 说明
* 本案例运行环境为windows 10，在windows 7环境下无法运行，后序会开发windows 7运行的分支。

# Chapter 6
* 01 Chapter 6 BoxApp: 在学习过DX12的基础之上，DX11的逻辑反而更加简单，在这里无需赘述。在这里对原先框架进行了大量修改，添加了一些常用的方法，具体可以参考d3dApp.h/.cpp和d3dUtil.h/.cpp文件。在DX11中已经支持HLSL，并且编译器也会警告，未来可能不再支持FX相关的编译，在这里也全部使用了HLSL新的处理方式。
* 02 Chapter 6 ShapesApp: 在这里对框架再次进行了修改，在d3dApp.h中添加了绘制几何体的辅助结构，在绘制物体时可以方便地堆顶点缓冲区和索引缓冲区进行偏移。同时修复了d3dApp中OnResize()方法中的bug，避免Comptr的引用错误。
* 03 Chapter 6 WavesApp: 动态顶点缓冲区和常量缓冲区类似，都需要每一帧都更新数据，实现起来并不复杂。相比DX12，DX11不能操纵GPU围栏，因此不能将常量资源封装，这样对于静态物体就不用每一帧更新了，这是DX12十分优越的一点。
* 04 Chapter 6 Exersice_2: 两个输入槽分别处理位置和顶点数据，在这里需要创建两个顶点缓冲区，在绘制时也需要根据输入槽不同指定对应的顶点缓冲区。

# Chapter 7
* 05 Chapter 7 WavesLightingApp: 在这里将框架再次重构，添加了光照信息、材质信息，优化了时间处理。
* 06 Chapter 7 ShapesLightingApp: 有了光照，场景相对的有了一定细节，后序还会更加完善。

# Chapter 8
* 07 Chapter 8 CrateApp: 纹理使用细节很多，在这里只是最基础的使用。
* 08 Chapter 8 WavesApp: 原书基础上还原，添加了纹理，程序逐渐开始有趣，陆地海洋也开始漂亮了。
* 09 Chapter 8 Exercise_3: 此题使用了两张纹理，这里初学可能会有一个误区，就是一个物体只能绑定一个材质，一个材质只有一张纹理，实质上在这里材质只是有一个纹理在描述符堆中索引，并没有绑定材质，最终着色器接收纹理还是在IA阶段。此题还有一个难点就是纹理旋转，二维旋转矩阵容易求出，就是在考虑旋转中心点时需要多下一点心思，具体可以参考此题的Shader/Default.hlsl，在顶点着色器中实现纹理旋转。
* 10 Chapter 8 Exercise_5: 此题首先需要使用[texconv](https://github.com/microsoft/DirectXTex/wiki/Texconv)工具将bmp文件转为dds文件，程序运行时每一帧重新指定纹理即可。
* 11 Chapter 8 Exercise_7: 此题没有难度，主要是对纹理的基础使用，完善后场景也比较丰富。