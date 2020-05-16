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