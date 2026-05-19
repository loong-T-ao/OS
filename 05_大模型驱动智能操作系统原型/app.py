from __future__ import annotations

import json
import traceback
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

from core.config import load_config
from core.orchestrator import Orchestrator


# 扩展项目根目录。
BASE_DIR = Path(__file__).resolve().parent
# 前端静态资源目录。
WEB_DIR = BASE_DIR / "web"

# 启动时加载配置，并初始化总控器。
# 这样请求到来时不需要重复创建模型客户端和执行器。
CONFIG = load_config(BASE_DIR / "config.json")
ORCHESTRATOR = Orchestrator(BASE_DIR, CONFIG)


class AppHandler(BaseHTTPRequestHandler):
    # 所有 JSON 接口统一通过这个函数返回，方便前端稳定解析。
    def _send_json(self, payload: dict, status: int = HTTPStatus.OK) -> None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    # 静态文件分发函数，服务前端页面和脚本。
    def _send_file(self, path: Path, content_type: str) -> None:
        body = path.read_bytes()
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self) -> None:
        # 前端页面和健康检查都走 GET。
        parsed = urlparse(self.path)
        if parsed.path == "/":
            return self._send_file(WEB_DIR / "index.html", "text/html; charset=utf-8")
        if parsed.path == "/app.js":
            return self._send_file(WEB_DIR / "app.js", "application/javascript; charset=utf-8")
        if parsed.path == "/styles.css":
            return self._send_file(WEB_DIR / "styles.css", "text/css; charset=utf-8")
        if parsed.path == "/api/health":
            return self._send_json({"ok": True, "message": "service running"})
        self._send_json({"ok": False, "error": "Not found"}, HTTPStatus.NOT_FOUND)

    def do_POST(self) -> None:
        # 所有实验执行请求统一走这个入口。
        parsed = urlparse(self.path)
        if parsed.path != "/api/run":
            return self._send_json({"ok": False, "error": "Not found"}, HTTPStatus.NOT_FOUND)

        try:
            content_length = int(self.headers.get("Content-Length", "0"))
            raw_body = self.rfile.read(content_length)
            payload = json.loads(raw_body.decode("utf-8"))
            user_input = str(payload.get("message", "")).strip()

            if not user_input:
                return self._send_json(
                    {"ok": False, "error": "请输入实验请求。"},
                    HTTPStatus.BAD_REQUEST,
                )

            # 总控器负责整个智能链路：
            # 边界判断 -> 结构化解析 -> 基础模块执行 -> AI 分析。
            result = ORCHESTRATOR.handle(user_input)
            return self._send_json({"ok": True, "data": result})
        except Exception as exc:  # pragma: no cover
            # 出错时把 traceback 一并返回，便于本地调试。
            return self._send_json(
                {
                    "ok": False,
                    "error": str(exc),
                    "traceback": traceback.format_exc(),
                },
                HTTPStatus.INTERNAL_SERVER_ERROR,
            )

    def log_message(self, format: str, *args) -> None:
        # 关闭默认 HTTP 访问日志，避免终端刷屏影响演示。
        return


def main() -> None:
    server = ThreadingHTTPServer((CONFIG["server_host"], int(CONFIG["server_port"])), AppHandler)
    print(f"智能操作系统原型已启动: http://{CONFIG['server_host']}:{CONFIG['server_port']}")
    server.serve_forever()


if __name__ == "__main__":
    main()
