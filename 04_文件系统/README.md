# 04 文件系统

本模块使用 `C` 语言在 `Linux/WSL` 环境下实现一个简化的文件系统模拟器，支持目录管理、文件创建读写删除、空闲块分配与回收，并通过脚本化命令输入完成测试。

## 已实现内容

- 目录创建与切换
- 文件创建、写入、读取、删除
- 当前目录列出
- 空闲块分配与释放
- 文件占用块和目录结构展示

## 功能说明

- 使用内存中的目录树和文件元数据模拟文件系统
- 使用固定大小磁盘块模拟存储空间
- 写文件时自动按块分配空闲空间
- 删除文件时自动回收块
- 支持通过命令脚本批量演示文件系统操作
- 提供 `Makefile` 和 Linux 下的一键测试脚本

## 目录结构

- `src/main.c`：文件系统模拟器主程序
- `tests/fs_commands.txt`：示例命令脚本
- `Makefile`：编译脚本
- `run_tests.sh`：Linux 下统一测试与结果输出脚本
- `output`：运行输出目录

## 输入格式

命令文件示例：

```text
mkdir docs
cd docs
create report
write report OperatingSystemCourseDesign
read report
ls
cd ..
mkdir temp
cd temp
create log
write log hello_kernel_world
status
cd ..
delete docs/report
status
```

## 支持命令

- `mkdir <dir>`：创建目录
- `cd <dir>`：进入子目录
- `cd ..`：返回父目录
- `create <file>`：创建空文件
- `write <file> <content>`：写入文件内容
- `read <file>`：读取文件内容
- `delete <path>`：删除文件
- `ls`：列出当前目录内容
- `status`：显示文件系统分配状态

## 编译方式

在 WSL / Linux 终端进入当前目录后执行：

```bash
make
```

编译后可执行文件位于：

```text
./bin/fs_simulator
```

## 运行方式

```bash
./bin/fs_simulator -i ./tests/fs_commands.txt
```

参数说明：

- `-i`：命令脚本路径

## 一键测试

```bash
./run_tests.sh
```

脚本会自动：

- 执行 `make clean && make`
- 运行文件系统命令测试
- 将结果保存到 `output` 目录

## 输出内容

- 屏幕显示每条命令的执行结果
- `output` 目录下生成带时间戳的结果文件
