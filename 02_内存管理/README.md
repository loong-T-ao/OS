# 02 内存管理

本模块使用 `C` 语言在 `Linux/WSL` 环境下实现典型内存管理算法，包括页面置换和动态分区管理两部分，支持统一编译、批量测试和结果输出。

## 已实现内容

### 页面置换算法

- `FIFO`：先进先出页面置换
- `LRU`：最近最久未使用页面置换

### 动态分区管理算法

- `FF`：首次适应
- `BF`：最佳适应

## 功能说明

- 支持从文本文件读取页面访问序列和内存容量
- 支持从文本文件读取动态分区初始状态和操作序列
- 输出页面装入过程、缺页次数、缺页率
- 输出内存分配、回收、分区合并过程
- 提供 `Makefile` 和 Linux 下的一键测试脚本

## 目录结构

- `src/main.c`：内存管理主程序
- `tests/page_input.txt`：页面置换测试数据
- `tests/partition_input.txt`：动态分区测试数据
- `Makefile`：编译脚本
- `run_tests.sh`：Linux 下统一测试与结果输出脚本
- `output`：运行输出目录

## 输入格式

### 1. 页面置换输入

文件示例：

```text
3
7 0 1 2 0 3 0 4 2 3 0 3 2
```

含义：

- 第一行：页框数量
- 第二行：页面访问序列

### 2. 动态分区输入

文件示例：

```text
100
alloc A 20
alloc B 35
alloc C 15
free B
alloc D 18
free A
alloc E 10
free C
alloc F 30
```

含义：

- 第一行：总内存大小
- 后续每行一条操作
- `alloc 进程名 大小`：申请内存
- `free 进程名`：释放内存

## 编译方式

在 WSL / Linux 终端进入当前目录后执行：

```bash
make
```

编译后可执行文件位于：

```text
./bin/memory_manager
```

## 运行方式

运行全部内容：

```bash
./bin/memory_manager -m all -p ./tests/page_input.txt -d ./tests/partition_input.txt
```

只运行页面置换：

```bash
./bin/memory_manager -m paging -p ./tests/page_input.txt
```

只运行动态分区：

```bash
./bin/memory_manager -m partition -d ./tests/partition_input.txt
```

参数说明：

- `-m`：模式，可选 `all`、`paging`、`partition`
- `-p`：页面置换输入文件
- `-d`：动态分区输入文件

## 一键测试

```bash
./run_tests.sh
```

脚本会自动：

- 执行 `make clean && make`
- 运行页面置换和动态分区全部测试
- 将输出结果保存到 `output` 目录

## 输出内容

- 屏幕显示算法执行过程和统计结果
- `output` 目录下生成带时间戳的结果文件
