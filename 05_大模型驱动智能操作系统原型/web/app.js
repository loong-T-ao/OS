const runButton = document.getElementById("runButton");
const messageInput = document.getElementById("message");
const guardrailEl = document.getElementById("guardrail");
const structuredEl = document.getElementById("structured");
const rawOutputEl = document.getElementById("rawOutput");
const analysisEl = document.getElementById("analysis");

function pretty(value) {
  if (typeof value === "string") {
    return value;
  }
  return JSON.stringify(value, null, 2);
}

async function runTask() {
  const message = messageInput.value.trim();
  if (!message) {
    alert("请输入实验请求。");
    return;
  }

  runButton.disabled = true;
  runButton.textContent = "执行中...";
  guardrailEl.textContent = "边界判断中...";
  structuredEl.textContent = "结构化解析中...";
  rawOutputEl.textContent = "等待基础模块执行...";
  analysisEl.textContent = "等待 AI 分析...";

  try {
    const response = await fetch("/api/run", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ message }),
    });
    const payload = await response.json();
    if (!payload.ok) {
      throw new Error(payload.error || "请求失败");
    }

    const data = payload.data;
    guardrailEl.textContent = pretty(data.guardrail);

    if (!data.allowed) {
      structuredEl.textContent = "当前请求未通过边界校验。";
      rawOutputEl.textContent = "未执行基础模块。";
      analysisEl.textContent = data.message || "请重新输入。";
      return;
    }

    structuredEl.textContent = pretty(data.structured_task);
    rawOutputEl.textContent = data.execution.raw_output || "";
    analysisEl.textContent = pretty(data.analysis);
  } catch (error) {
    guardrailEl.textContent = "请求失败";
    structuredEl.textContent = "";
    rawOutputEl.textContent = "";
    analysisEl.textContent = String(error);
  } finally {
    runButton.disabled = false;
    runButton.textContent = "开始执行";
  }
}

runButton.addEventListener("click", runTask);
