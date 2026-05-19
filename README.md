# 操作系统课程设计

本项目为操作系统课程设计代码仓库，包含 4 个基础实验模块和 1 个“大模型驱动智能操作系统原型”扩展模块。

代码仓库地址：

- 仓库页面：`https://github.com/loong-T-ao/OS`
- Git 克隆地址：`https://github.com/loong-T-ao/OS.git`

访问方式说明：

```bash
git clone https://github.com/loong-T-ao/OS.git
cd OS
```

也可以直接使用浏览器打开仓库页面查看源码、目录结构和 README 说明。

## 项目结构

- `01_处理机调度`：实现 FCFS、SJF、RR、Priority 等调度算法
- `02_内存管理`：实现页面置换与动态分区管理
- `03_进程同步与并发控制`：实现生产者消费者、读者写者、哲学家进餐
- `04_文件系统`：实现简化文件系统和目录管理模拟
- `05_大模型驱动智能操作系统原型`：使用 Python + DeepSeek API + 本地前端，将自然语言请求自动转换为实验输入并执行基础模块
- `scripts`：统一检查和辅助脚本
- `output`：运行输出与汇总结果

## 基础模块说明

### 1. 处理机调度

目录：`01_处理机调度`

功能：

- 支持 `FCFS`
- 支持 `SJF`
- 支持 `RR`
- 支持 `Priority`

输入文件格式示例：

```text
P1 0 2 1
P2 0 4 1
P3 0 6 1
P4 0 8 1
```

运行示例：

```bash
./bin/scheduler -i ./tests/sample_processes.txt -a all -q 2
```

### 2. 内存管理

目录：`02_内存管理`

功能：

- 页面置换：`FIFO`、`LRU`
- 动态分区：`FF`、`BF`

运行示例：

```bash
./bin/memory_manager -m all -p ./tests/page_input.txt -d ./tests/partition_input.txt
```

### 3. 进程同步与并发控制

目录：`03_进程同步与并发控制`

功能：

- 生产者消费者
- 读者写者
- 哲学家进餐

运行示例：

```bash
./bin/sync_demo -m all -c ./tests/sync_config.txt
```

### 4. 文件系统

目录：`04_文件系统`

功能：

- 目录创建与切换
- 文件创建、写入、读取、删除
- 空闲块分配与回收
- 文件系统状态查看

运行示例：

```bash
./bin/fs_simulator -i ./tests/fs_commands.txt
```

## 大模型驱动智能操作系统原型

目录：`05_大模型驱动智能操作系统原型`

该模块在 4 个基础实验模块之上增加了一层自然语言交互。用户输入实验需求后，系统会自动完成：

1. 边界判断
2. 结构化任务解析
3. 动态生成基础模块输入文件
4. 调用对应实验程序执行
5. 展示原始实验输出
6. 生成 AI 分析结论

### 功能流程

整体流程如下：

```text
自然语言输入
-> Guardrail 边界判断
-> TaskParser 解析为 JSON
-> ModuleExecutor 生成输入文件并调用基础模块
-> 返回原始输出
-> Analyzer 生成实验分析
```

### 环境要求

运行该模块前需要准备：

1. Windows
2. Python 3.10 及以上
3. 已安装 WSL
4. WSL 中有可用 Linux 发行版
5. 可用的 DeepSeek API Key
6. 4 个基础实验模块可在 WSL 中通过 `make` 编译

### 配置方法

进入 `05_大模型驱动智能操作系统原型` 目录后，准备配置文件：

```text
config.example.json -> config.json
```

需要填写的关键字段：

- `deepseek_api_key`
- `deepseek_base_url`
- `deepseek_model`
- `wsl_distro`

示例：

```json
{
  "server_host": "127.0.0.1",
  "server_port": 8899,
  "deepseek_api_key": "你的 DeepSeek API Key",
  "deepseek_base_url": "https://api.deepseek.com",
  "deepseek_model": "deepseek-v4-flash",
  "wsl_distro": "Ubuntu"
}
```

注意：

- `wsl_distro` 必须与本机 `wsl -l -v` 中显示的发行版名称完全一致
- 如果名称写错，系统会调用 WSL 失败

### 启动方式

方式一：双击启动

```text
05_大模型驱动智能操作系统原型/start.bat
```

方式二：命令行启动

```powershell
cd .\05_大模型驱动智能操作系统原型
.\start.bat
```

如果本机没有配置 `py` 或 `python` 命令，需要先安装 Python 并加入 PATH。

启动成功后，在浏览器访问：

```text
http://127.0.0.1:8899
```

### 使用方法

在网页输入框中直接输入自然语言实验需求，例如：

```text
请创建 4 个进程，全部到达时间为 0，运行时间分别为 2、4、6、8，使用时间片轮转算法，时间片为 2，并给出调度结果分析。
```

系统会展示 4 个结果区：

- 边界代理结果：判断请求是否属于本课程实验范围
- 结构化任务：大模型将自然语言转换后的 JSON 任务
- 基础模块原始输出：底层 C 程序的真实运行结果
- AI 分析结论：基于原始结果生成的总结与分析

### 原理说明

大模型不会直接执行 C 程序，而是先把自然语言转成结构化 JSON，再由执行器转换为各基础模块原本支持的输入文件格式：

- 调度模块：生成 `scheduler_input.txt`
- 内存模块：生成 `memory_page_input.txt` 和 `memory_partition_input.txt`
- 并发模块：生成 `sync_config.txt`
- 文件系统模块：生成 `fs_commands.txt`

然后再通过 WSL 调用对应可执行程序执行。

## 统一脚本

根目录提供统一辅助脚本：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_all.ps1
```

该脚本主要用于：

- 检查项目结构
- 探测编译器
- 汇总输出目录

## 提交说明

本仓库适合作为课程设计报告中的“代码托管地址”填写项。报告中可写为：

```text
代码托管地址：https://github.com/loong-T-ao/OS
访问方式：浏览器直接访问，或使用 git clone https://github.com/loong-T-ao/OS.git 获取源码
```
