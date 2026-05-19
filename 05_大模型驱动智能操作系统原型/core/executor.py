from __future__ import annotations

import subprocess
from pathlib import Path


class ModuleExecutor:
    def __init__(self, base_dir: Path, wsl_distro: str) -> None:
        self.base_dir = base_dir
        self.repo_root = base_dir.parent
        self.runtime_dir = base_dir / "runtime"
        self.runtime_dir.mkdir(parents=True, exist_ok=True)
        self.wsl_distro = wsl_distro

    def _find_module_dir(self, prefix: str) -> Path:
        for item in self.repo_root.iterdir():
            if item.is_dir() and item.name.startswith(prefix):
                return item
        raise FileNotFoundError(f"Module directory not found: {prefix}*")

    def _to_wsl_path(self, path: Path) -> str:
        resolved = path.resolve()
        drive = resolved.drive.rstrip(":").lower()
        tail = resolved.as_posix().split(":/", 1)[1]
        return f"/mnt/{drive}/{tail}"

    def _run_wsl(self, command: str) -> str:
        completed = subprocess.run(
            ["wsl", "-d", self.wsl_distro, "bash", "-lc", command],
            capture_output=True,
            text=False,
            check=False,
        )
        stdout_text = self._decode_output(completed.stdout)
        stderr_text = self._decode_output(completed.stderr)

        if completed.returncode != 0:
            error_text = "\n".join(
                part for part in [stdout_text.strip(), stderr_text.strip()] if part
            )
            raise RuntimeError(error_text or "WSL command failed")

        return stdout_text.strip()

    def _ensure_built(self, module_dir: Path) -> None:
        self._run_wsl(f"cd '{self._to_wsl_path(module_dir)}' && make")

    def run_scheduler(self, task: dict) -> dict:
        module_dir = self._find_module_dir("01_")
        self._ensure_built(module_dir)

        input_path = self.runtime_dir / "scheduler_input.txt"
        lines = []
        for item in task["processes"]:
            lines.append(
                f"{item['name']} {item['arrival_time']} {item['burst_time']} {item['priority']}"
            )
        input_path.write_text("\n".join(lines), encoding="utf-8")

        command = (
            f"cd '{self._to_wsl_path(module_dir)}' && "
            f"./bin/scheduler -i '{self._to_wsl_path(input_path)}' "
            f"-a {task.get('algorithm', 'all')} -q {int(task.get('time_slice', 2))}"
        )
        raw_output = self._run_wsl(command)
        return {"input_file": str(input_path), "raw_output": raw_output}

    def run_memory(self, task: dict) -> dict:
        module_dir = self._find_module_dir("02_")
        self._ensure_built(module_dir)

        mode = task.get("mode", "all")
        page_path = self.runtime_dir / "memory_page_input.txt"
        partition_path = self.runtime_dir / "memory_partition_input.txt"

        page_replacement = task.get("page_replacement", {})
        frame_count = int(page_replacement.get("frame_count", 3))
        pages = page_replacement.get("pages", [7, 0, 1, 2, 0, 3, 0, 4])
        page_path.write_text(
            f"{frame_count}\n{' '.join(str(x) for x in pages)}\n",
            encoding="utf-8",
        )

        partition = task.get("partition", {})
        total_size = int(partition.get("total_size", 100))
        operations = partition.get("operations", ["alloc A 20", "free A"])
        partition_path.write_text(
            f"{total_size}\n" + "\n".join(operations) + "\n",
            encoding="utf-8",
        )

        command = (
            f"cd '{self._to_wsl_path(module_dir)}' && "
            f"./bin/memory_manager -m {mode} "
            f"-p '{self._to_wsl_path(page_path)}' "
            f"-d '{self._to_wsl_path(partition_path)}'"
        )
        raw_output = self._run_wsl(command)
        return {
            "page_input_file": str(page_path),
            "partition_input_file": str(partition_path),
            "raw_output": raw_output,
        }

    def run_sync(self, task: dict) -> dict:
        module_dir = self._find_module_dir("03_")
        self._ensure_built(module_dir)

        config_path = self.runtime_dir / "sync_config.txt"
        config = task.get("config", {})
        keys = [
            "buffer_size",
            "producer_count",
            "consumer_count",
            "items_per_producer",
            "reader_count",
            "writer_count",
            "reads_per_reader",
            "writes_per_writer",
            "philosopher_count",
            "meals_per_philosopher",
        ]
        lines = [f"{key}={int(config.get(key, self._default_sync_value(key)))}" for key in keys]
        config_path.write_text("\n".join(lines) + "\n", encoding="utf-8")

        command = (
            f"cd '{self._to_wsl_path(module_dir)}' && "
            f"./bin/sync_demo -m {task.get('mode', 'all')} -c '{self._to_wsl_path(config_path)}'"
        )
        raw_output = self._run_wsl(command)
        return {"config_file": str(config_path), "raw_output": raw_output}

    def run_filesystem(self, task: dict) -> dict:
        module_dir = self._find_module_dir("04_")
        self._ensure_built(module_dir)

        command_path = self.runtime_dir / "fs_commands.txt"
        commands = task.get("commands", ["mkdir docs", "status"])
        command_path.write_text("\n".join(commands) + "\n", encoding="utf-8")

        command = (
            f"cd '{self._to_wsl_path(module_dir)}' && "
            f"./bin/fs_simulator -i '{self._to_wsl_path(command_path)}'"
        )
        raw_output = self._run_wsl(command)
        return {"command_file": str(command_path), "raw_output": raw_output}

    @staticmethod
    def _default_sync_value(key: str) -> int:
        defaults = {
            "buffer_size": 5,
            "producer_count": 2,
            "consumer_count": 2,
            "items_per_producer": 4,
            "reader_count": 3,
            "writer_count": 2,
            "reads_per_reader": 3,
            "writes_per_writer": 2,
            "philosopher_count": 5,
            "meals_per_philosopher": 2,
        }
        return defaults[key]

    @staticmethod
    def _decode_output(data: bytes | None) -> str:
        if not data:
            return ""
        for encoding in ("utf-8", "utf-16le", "gbk"):
            try:
                return data.decode(encoding)
            except UnicodeDecodeError:
                continue
        return data.decode("utf-8", errors="replace")
