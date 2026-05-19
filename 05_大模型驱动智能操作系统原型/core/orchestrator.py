from __future__ import annotations

from pathlib import Path

from core.analyzer import Analyzer
from core.executor import ModuleExecutor
from core.guardrail import Guardrail
from core.llm_client import DeepSeekClient
from core.task_parser import TaskParser


class Orchestrator:
    def __init__(self, base_dir: Path, config: dict) -> None:
        # 三类智能能力共用同一个 DeepSeek 客户端，但职责彼此分离：
        # 1. Guardrail 负责边界判断
        # 2. TaskParser 负责结构化解析
        # 3. Analyzer 负责结果分析
        client = DeepSeekClient(
            api_key=config["deepseek_api_key"],
            base_url=config["deepseek_base_url"],
            model=config["deepseek_model"],
        )
        self.guardrail = Guardrail(client)
        self.parser = TaskParser(client)
        self.analyzer = Analyzer(client)
        self.executor = ModuleExecutor(base_dir, config["wsl_distro"])

    def handle(self, user_input: str) -> dict:
        # 第一步：先过边界代理。
        guard = self.guardrail.check(user_input)
        if not guard.get("allowed", False):
            return {
                "allowed": False,
                "guardrail": guard,
                "message": "当前输入不符合本实验原型支持范围，请重新输入操作系统课程实验相关请求。",
            }

        # 第二步：大模型把自然语言解析为结构化任务。
        structured_task = self.parser.parse(user_input)
        domain = structured_task["domain"]
        task = structured_task["task"]

        # 第三步：根据实验域调用具体基础模块。
        if domain == "scheduler":
            execution = self.executor.run_scheduler(task)
        elif domain == "memory":
            execution = self.executor.run_memory(task)
        elif domain == "sync":
            execution = self.executor.run_sync(task)
        elif domain == "filesystem":
            execution = self.executor.run_filesystem(task)
        else:
            return {
                "allowed": False,
                "guardrail": guard,
                "message": "大模型未能解析到受支持的实验域，请重新输入。",
            }

        # 第四步：让大模型基于真实运行结果给出实验结论。
        analysis = self.analyzer.analyze(user_input, structured_task, execution["raw_output"])
        return {
            "allowed": True,
            "guardrail": guard,
            "structured_task": structured_task,
            "execution": execution,
            "analysis": analysis,
        }
