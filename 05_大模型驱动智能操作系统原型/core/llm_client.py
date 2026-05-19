from __future__ import annotations

import json
from urllib import request


class DeepSeekClient:
    def __init__(self, api_key: str, base_url: str, model: str) -> None:
        # 使用 urllib 而不是第三方库，降低环境依赖。
        self.api_key = api_key
        self.base_url = base_url.rstrip("/")
        self.model = model

    def chat_json(self, system_prompt: str, user_prompt: str) -> dict:
        # 整个扩展原型都要求大模型返回 JSON。
        # 这样我们可以稳定做后续路由，而不是靠脆弱的字符串解析。
        payload = {
            "model": self.model,
            "messages": [
                {"role": "system", "content": system_prompt},
                {"role": "user", "content": user_prompt},
            ],
            "temperature": 0.2,
            "response_format": {"type": "json_object"},
        }
        req = request.Request(
            url=f"{self.base_url}/chat/completions",
            data=json.dumps(payload).encode("utf-8"),
            headers={
                "Content-Type": "application/json",
                "Authorization": f"Bearer {self.api_key}",
            },
            method="POST",
        )
        with request.urlopen(req, timeout=120) as response:
            raw = json.loads(response.read().decode("utf-8"))
        content = raw["choices"][0]["message"]["content"]
        return json.loads(content)
