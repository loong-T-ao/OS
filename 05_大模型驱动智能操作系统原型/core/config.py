from __future__ import annotations

import json
from pathlib import Path


def load_config(path: Path) -> dict:
    # 配置文件只在启动时读取一次。
    # 如果字段不完整，直接抛错，能让问题更早暴露。
    if not path.exists():
        raise FileNotFoundError(f"配置文件不存在: {path}")

    data = json.loads(path.read_text(encoding="utf-8"))
    required = [
        "server_host",
        "server_port",
        "deepseek_api_key",
        "deepseek_base_url",
        "deepseek_model",
        "wsl_distro",
    ]
    missing = [key for key in required if key not in data]
    if missing:
        raise ValueError(f"配置文件缺少字段: {', '.join(missing)}")
    return data
