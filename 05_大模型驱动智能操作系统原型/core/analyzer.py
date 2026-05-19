from __future__ import annotations

from core.llm_client import DeepSeekClient


ANALYZER_PROMPT = """
你是一个操作系统课程实验分析助手。
你会收到：
1. 用户原始需求
2. 结构化任务
3. 基础模块原始执行结果

请输出 JSON：
{
  "summary": "一段简明总结",
  "observations": [
    "观察1",
    "观察2"
  ],
  "conclusion": "最终结论"
}

要求：
1. 分析必须结合原始结果
2. 语言简洁，适合课程实验展示
3. 如果是算法对比，要指出性能差异和特点
"""


class Analyzer:
    def __init__(self, client: DeepSeekClient) -> None:
        self.client = client

    def analyze(self, user_input: str, task: dict, raw_output: str) -> dict:
        # 分析模块只对“已经执行完成的真实结果”做解释。
        # 这样可以避免大模型凭空编造实验结论。
        prompt = (
            f"用户需求:\n{user_input}\n\n"
            f"结构化任务:\n{task}\n\n"
            f"基础模块原始输出:\n{raw_output}"
        )
        return self.client.chat_json(ANALYZER_PROMPT, prompt)
