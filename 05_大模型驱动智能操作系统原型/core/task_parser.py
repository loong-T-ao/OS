from __future__ import annotations

from core.llm_client import DeepSeekClient


PARSER_PROMPT = """
你是一个操作系统实验任务解析器，需要将用户的自然语言请求解析为结构化 JSON。

仅支持四类 domain：
- scheduler
- memory
- sync
- filesystem

必须只输出 JSON。

顶层格式如下：
{
  "domain": "scheduler|memory|sync|filesystem",
  "title": "简短任务名称",
  "need_analysis": true,
  "task": {}
}

一、如果 domain = scheduler
task 格式：
{
  "algorithm": "all|fcfs|sjf|rr|priority",
  "time_slice": 2,
  "processes": [
    {"name": "P1", "arrival_time": 0, "burst_time": 4, "priority": 2}
  ]
}

二、如果 domain = memory
task 格式：
{
  "mode": "all|paging|partition",
  "page_replacement": {
    "frame_count": 3,
    "pages": [7,0,1,2,0,3]
  },
  "partition": {
    "total_size": 100,
    "operations": [
      "alloc A 20",
      "free A"
    ]
  }
}

三、如果 domain = sync
task 格式：
{
  "mode": "all|pc|rw|dp",
  "config": {
    "buffer_size": 5,
    "producer_count": 2,
    "consumer_count": 2,
    "items_per_producer": 4,
    "reader_count": 3,
    "writer_count": 2,
    "reads_per_reader": 3,
    "writes_per_writer": 2,
    "philosopher_count": 5,
    "meals_per_philosopher": 2
  }
}

四、如果 domain = filesystem
task 格式：
{
  "commands": [
    "mkdir docs",
    "cd docs",
    "create report",
    "write report hello"
  ]
}

要求：
1. 尽量从用户输入中提取明确参数
2. 如果用户没给全，就补合理默认值
3. scheduler 和 memory 优先提取具体数据，不能只给空壳
4. 只输出合法 JSON
"""


class TaskParser:
    def __init__(self, client: DeepSeekClient) -> None:
        self.client = client

    def parse(self, user_input: str) -> dict:
        # 这一层只负责把自然语言变成结构化任务，不参与实际执行。
        return self.client.chat_json(PARSER_PROMPT, user_input)
