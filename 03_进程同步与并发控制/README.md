# 03 进程同步与并发控制

本模块使用 `C` 语言在 `Linux/WSL` 环境下结合 `pthread` 实现三个经典同步问题，突出互斥、同步、并发执行和死锁避免。

## 已实现内容

- `生产者-消费者`
- `读者-写者`
- `哲学家进餐`

## 功能说明

- 使用多线程模拟并发执行
- 使用互斥锁、条件变量和信号量实现同步控制
- 输出关键执行过程，便于展示线程调度和资源竞争
- 哲学家进餐问题采用有序拿叉策略，避免死锁
- 提供 `Makefile` 和 Linux 下的一键测试脚本

## 目录结构

- `src/main.c`：同步与并发控制主程序
- `tests/sync_config.txt`：测试参数配置文件
- `Makefile`：编译脚本
- `run_tests.sh`：Linux 下统一测试与结果输出脚本
- `output`：运行输出目录

## 输入格式

配置文件示例：

```text
buffer_size=5
producer_count=2
consumer_count=2
items_per_producer=4
reader_count=3
writer_count=2
reads_per_reader=3
writes_per_writer=2
philosopher_count=5
meals_per_philosopher=2
```

## 编译方式

在 WSL / Linux 终端进入当前目录后执行：

```bash
make
```

编译后可执行文件位于：

```text
./bin/sync_demo
```

## 运行方式

运行全部问题：

```bash
./bin/sync_demo -m all -c ./tests/sync_config.txt
```

只运行某一个问题：

```bash
./bin/sync_demo -m pc -c ./tests/sync_config.txt
./bin/sync_demo -m rw -c ./tests/sync_config.txt
./bin/sync_demo -m dp -c ./tests/sync_config.txt
```

参数说明：

- `-m`：模式，可选 `all`、`pc`、`rw`、`dp`
- `-c`：配置文件路径

## 一键测试

```bash
./run_tests.sh
```

脚本会自动：

- 执行 `make clean && make`
- 运行全部同步问题测试
- 将结果保存到 `output` 目录

## 输出内容

- 屏幕显示线程执行过程
- `output` 目录下生成带时间戳的结果文件
