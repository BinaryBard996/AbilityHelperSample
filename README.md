# AbilityEditorHelper

[中文] | [English]

---

## 简介 / Introduction

### [中文]
这是一个基于 Schema 驱动的虚幻引擎（Unreal Engine）技能系统（GAS）配置工具。它支持通过 Excel 自动化生成和更新 GameplayEffect 资产，简化了 GAS 的数值配置流程。

**核心特性：**
*   **Schema 驱动**：从 C++ 结构体自动生成 JSON Schema，定义 Excel 格式。
*   **Excel 自动化**：一键生成带下拉菜单、类型提示和子表关联的 Excel 模板。
*   **双向同步**：支持从 Excel 增量导出 JSON 并同步到 GameplayEffect 资产。
*   **高度定制**：支持自定义提示、分隔符，并支持 `openpyxl` 缺失时的自动 CSV 降级。

### [English]
A Schema-driven configuration tool for Unreal Engine Gameplay Ability System (GAS). It supports automated generation and updates of GameplayEffect assets via Excel, streamlining the numerical configuration process.

**Core Features:**
*   **Schema Driven**: Automatically generates JSON Schemas from C++ structs to define Excel formats.
*   **Excel Automation**: One-click generation of Excel templates with dropdowns, type hints, and sheet relations.
*   **Two-way Sync**: Supports incremental export from Excel to JSON and synchronization to GameplayEffect assets.
*   **Highly Customizable**: Supports custom hints, separators, and automatic CSV fallback if `openpyxl` is missing.

---

## 1. Unreal 环境搭建 / Unreal Environment Setup

### [中文]
为了运行本插件的 Python 脚本，需要进行以下配置：

1.  **启用插件**：在 Unreal Editor 中打开 `Edit -> Plugins`，搜索并启用：
    *   **Python Editor Script Plugin**：提供 Python 脚本支持。
    *   **Editor Scripting Utilities**：提供常用的编辑器操作 API。
2.  **启用开发模式（推荐）**：在 `Project Settings -> Plugins -> Python` 中，勾选 `Enable Python Developer Mode`。这将提供更详细的日志输出。
3.  **脚本路径**：插件脚本位于 `Plugins/AbilityEditorHelper/Content/Python`，虚幻会自动将其加入 `sys.path`。

### [English]
To run the Python scripts in this plugin, please configure the following:

1.  **Enable Plugins**: Open `Edit -> Plugins` and enable:
    *   **Python Editor Script Plugin**: Provides Python scripting support.
    *   **Editor Scripting Utilities**: Provides essential editor API utilities.
2.  **Enable Developer Mode (Recommended)**: Check `Enable Python Developer Mode` in `Project Settings -> Plugins -> Python` for detailed logging.
3.  **Script Path**: Scripts are located in `Plugins/AbilityEditorHelper/Content/Python`, which is automatically added to `sys.path` by Unreal.

---

## 2. Python 依赖库 / Python Dependencies

### [中文]
本工具依赖以下库。由于虚幻内置了 Python 环境，请确保在虚幻使用的 Python 环境中安装：

1.  **openpyxl** (核心依赖)：用于读写 `.xlsx` 文件、生成下拉列表及格式化表格。
    *   安装方式：在命令行执行 `pip install openpyxl`。
2.  **ptvsd** (可选依赖)：用于在 VS Code 或 Visual Studio 中远程调试插件的 Python 脚本。
    *   安装方式：`pip install ptvsd`。
3.  **内置库**：`json`, `os`, `sys`, `csv`, `argparse`, `typing` (标准库) 以及 `unreal` (由引擎提供)。

*注意：若未安装 `openpyxl`，项目会自动降级使用 **CSV** 格式进行模板生成和数据导出。*

### [English]
The tool depends on the following libraries. Ensure they are installed in the Python environment used by Unreal:

1.  **openpyxl** (Core): Used for reading/writing `.xlsx` files, generating dropdowns, and formatting.
    *   Installation: Run `pip install openpyxl` in your terminal.
2.  **ptvsd** (Optional): Used for remote debugging of Python scripts via VS Code or Visual Studio.
    *   Installation: `pip install ptvsd`.
3.  **Built-in**: `json`, `os`, `sys`, `csv`, `argparse`, `typing` (Standard Library) and `unreal` (Engine provided).

*Note: If `openpyxl` is missing, the tool automatically falls back to **CSV** format for template generation and data export.*

---

## 3. 生成 Schema 文件 / Generating Schema Files

### [中文]
Schema 文件（`.schema.json`）是连接 C++ 结构体与 Excel 的桥梁。

1.  **配置结构体**：在 `Project Settings -> Ability Editor Helper Settings` 的 `Schema -> Struct Type Paths To Export Schema` 中添加路径（如 `/Script/AbilityEditorHelper.GameplayEffectConfig`）。
2.  **导出 Schema**：调用蓝图函数 `UAbilityEditorHelperLibrary::GenerateAllSchemasFromSettings`。
    *   参数 `bClearSchemaFolderFirst`: 若为 True，生成前将清空 `Content/Python/Schema` 目录。
3.  **输出**：系统将为每个结构体在 Schema 目录下生成对应的 JSON 文件。

### [English]
Schema files (`.schema.json`) bridge C++ structs and Excel tables.

1.  **Configure Structs**: In `Project Settings -> Ability Editor Helper Settings`, add struct paths (e.g., `/Script/AbilityEditorHelper.GameplayEffectConfig`) to `Struct Type Paths To Export Schema`.
2.  **Export Schemas**: Call the Blueprint function `UAbilityEditorHelperLibrary::GenerateAllSchemasFromSettings`.
    *   Parameter `bClearSchemaFolderFirst`: If True, it clears the `Content/Python/Schema` directory before generation.
3.  **Output**: Generates corresponding JSON files for each configured struct in the Schema directory.

---

## 4. 创建 Excel 模板 / Creating Excel Templates

### [中文]
1.  **生成方式**：调用 Python 函数 `AbilityEditorHelperPythonLibrary::GenerateExcelTemplateFromSchema`。
    *   参数 `preserve_data`: **重要特性**。若设为 True（默认），在更新模板时会保留现有 Excel 中已有的数据行，仅更新列名、下拉菜单和提示。
2.  **特性**：
    *   **自动下拉菜单**：枚举类型字段会自动生成数据验证下拉列表。
    *   **类型提示**：表格第二行显示字段类型要求（如 `GameplayTag`、`Attribute` 格式）。
    *   **子表关联**：`TArray<Struct>` 类型的数组将自动创建独立的 Sheet，通过 `Name` 与 `ParentName` 建立关联。

### [English]
1.  **Generation**: Call the Python function `AbilityEditorHelperPythonLibrary::GenerateExcelTemplateFromSchema`.
    *   Parameter `preserve_data`: **Key Feature**. If set to True (default), it preserves existing data rows when updating a template, only refreshing column names, dropdowns, and hints.
2.  **Features**:
    *   **Auto Dropdowns**: Enum fields automatically generate Excel data validation lists.
    *   **Type Hints**: The second row displays formatting requirements (e.g., `GameplayTag`, `Attribute`).
    *   **Sheet Relations**: `TArray<Struct>` fields automatically create separate Sheets, linked via `Name` and `ParentName`.

---

## 5. 数据导出与资产同步 / Data Export & Asset Sync

### [中文]
这是一个完整的自动化工作流：

1.  **导出 JSON**：调用 `AbilityEditorHelperPythonLibrary::ExportExcelToJsonUsingSchema`。脚本将解析 Excel 主表及其关联的所有子表。
2.  **创建/更新资产**：调用 `UAbilityEditorHelperLibrary::ImportAndUpdateGameplayEffectsFromJson`。
    *   **增量更新**：工具会计算数据哈希值，仅对发生变更的配置行执行资产更新，大幅提升大型项目的同步速度。
    *   **自动清理**：若勾选 `bClearGameplayEffectFolderFirst`，将自动移除目标路径下不再出现在配置中的旧 GE 资产。
    *   **组件自动化**：自动配置 Modifiers, Tags, GameplayCues, Abilities, Executions, Immunity 等所有 GAS 核心组件。

### [English]
A complete automated workflow:

1.  **Export JSON**: Call `AbilityEditorHelperPythonLibrary::ExportExcelToJsonUsingSchema`. The script parses the main Excel sheet and all related sub-sheets.
2.  **Create/Update Assets**: Call `UAbilityEditorHelperLibrary::ImportAndUpdateGameplayEffectsFromJson`.
3.  **Incremental Update**: The tool calculates data hashes and only updates assets for changed rows, significantly speeding up sync for large projects.
4.  **Auto Cleanup**: If `bClearGameplayEffectFolderFirst` is checked, old GE assets in the target folder that are no longer in the configuration will be removed.
5.  **Component Automation**: Automatically configures Modifiers, Tags, GameplayCues, Abilities, Executions, Immunity, and other GAS core components.

---

## 高级特性与元数据 / Advanced Features & Metadata

### [中文]
您可以在 C++ 结构体中使用 `meta` 标签进一步控制 Excel 生成：
*   `ExcelHint`: 在 Excel 提示行显示自定义说明文字。
*   `ExcelSeparator`: 指定数组字段在单元格内的分隔符（如 `|` 或 `,`）。
*   **Attribute 格式**：支持 `AttributeSetName.PropertyName` 简写，也支持完整的 `/Script/Module.Class:Property` 路径。
*   **Tag Requirements**: 使用 `Require:TagA,TagB|Ignore:TagC` 格式在单个单元格内配置复杂的标签需求。

### [English]
Control Excel generation using `meta` tags in C++ structs:
*   `ExcelHint`: Displays custom instruction text in the Excel hint row.
*   `ExcelSeparator`: Specifies the inline separator for array fields (e.g., `|` or `,`).
*   **Attribute Format**: Supports `AttributeSetName.PropertyName` shorthand or full `/Script/Module.Class:Property` paths.
*   **Tag Requirements**: Use `Require:TagA,TagB|Ignore:TagC` to configure complex requirements in a single cell.
