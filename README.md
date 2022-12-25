# oset-platform
基于oset底软开发的模块化平台

<img width="478" alt="捕获" src="https://user-images.githubusercontent.com/59110419/209470103-3978be6f-78d7-47e7-a5eb-d083bdaac37c.PNG">

<img width="485" alt="捕获" src="https://user-images.githubusercontent.com/59110419/209470154-36433170-fdcf-4114-9daf-ebcbdcbe265d.PNG">

为了实现各种不同体制协议的仿真和模拟，实现协议栈的模块化加载和迭代，减少操作系统对上层业务的影响，提高软件开发的统一性和可重用性，开发了OSET通用平台。整个通用平台由Os-SET底软平台和Open-SET开放运行平台两部分构成。Os-SET底层平台提供各种系统底层接口，实现底层系统和上层业务的隔离。Open-SET平台是依托于Os-SET底软平台开发的一款开放运行平台，整个平台全部采用模块化架构，支持模块的实时加载卸载，支持运行业务的实时弹缩，提供远程调用库，支持python的跨语言直接调用，尽量避免使用中间件。此外整个平台都围绕开放原则开发，定义了简单通用的函数接口和模板，方便第三方开发和迭代。

1、Os-SET底软平台

![image](https://user-images.githubusercontent.com/59110419/209470393-fada7561-d157-44ab-95d6-f78b4f901f1d.png)
Core目录为核心目录，核心功能均在此目录开发，全部采用动态库提供给第三方。
平台目录结构如下
├─cmake                      Cmake编译引用库
├─core                        OSET底软目录
│  ├─base					   OSET base库
│  ├─esl         			   Esl远程调用库
│  ├─mods					  API模块库
├─doc                  说明文档
├─tests                 单元测试
├─tools                 常用工具
├─CMakeLists.txt        cmake编译

2、open-SET平台

Open-SET平台由三大部分构成：
1）Core核心：core核心负责整个平台的running，是整个平台最重要的运行框架，负责Mod和App的管理和调度。
2）Mod：Module是平台的中间层，分为common mod和algorithm mod两大类，common模块提供各种常用的API接口，供给上层业务调用，是Os-SET的实例化，如日志模块、CLI模块、Redis模块等；algorithm模块是常用的算法模块，提供各种算法API接口。
3）App：APP是平台最上层的业务，由业务逻辑和Mod API组合而成。
整个平台都是模块化设计，MOD和APP两大核心可以任意加载、卸载、重载，非常灵活。平台的运行依托于Os-SET底软平台，否则无法运行。
