from __future__ import annotations

from core.llm_client import DeepSeekClient


GUARDRAIL_PROMPT = """
你是一个操作系统课程实验原型的边界代理。
你的职责是先判断用户输入是否属于以下允许范围：
1. 处理机调度实验
2. 内存管理实验
3. 进程同步与并发控制实验
4. 简易文件系统实验
5. 对上述实验结果的解释、比较、分析

如果用户请求不属于上述范围，必须拒绝并要求重新输入。

你必须只输出 JSON，格式如下：
{
  "allowed": true,
  "reason": "简短说明",
  "suggested_domain": "scheduler|memory|sync|filesystem|unknown"
}
"""


class Guardrail:
    def __init__(self, client: DeepSeekClient) -> None:
        self.client = client

    def check(self, user_input: str) -> dict:
        # 这是“外部框架边界”。
        # 任何请求必须先通过边界代理，才能进入后续结构化解析和代码执行。
        return self.client.chat_json(GUARDRAIL_PROMPT, user_input)
