# AbilityEditorHelper Excel 工作流（FGameplayEffectConfig）

目标：
- 基于UE结构体（以 `FGameplayEffectConfig` 为例）自动生成Excel模板；
- 在UE编辑器内通过Unreal Python将Excel数据导出为可反序列化为 `FGameplayEffectConfig` 的Json数组（GameplayEffectData）。

本工具脚本：`AbilityEditorExcelTools.py`  
位置：`Plugins/AbilityEditorHelper/Content/Python/`

## 依赖
- 推荐安装 `openpyxl` 到UE自带的Python环境以生成/读取 .xlsx。
  - 打开 编辑器 > Python控制台，执行：
    ```
    import sys, subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "openpyxl"])
    ```
- 若未安装 `openpyxl`，工具会自动回退为CSV模板/导出（生成 `GameplayEffectData_Main.csv` 与 `GameplayEffectData_Modifiers.csv`）。

## 生成Excel模板
在编辑器 Python 控制台执行：
