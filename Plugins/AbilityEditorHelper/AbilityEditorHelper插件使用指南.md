# AbilityEditorHelper 插件使用指南

## 一、引言：GAS 配置的痛点

### 1.1 GAS 配置规模的残酷真相

如今，大多数 UE 项目都采用了 Gameplay Ability System（GAS）这套强大（但也很"复杂"）的框架。以 GameplayEffect（GE）为例，一个中等规模的项目往往需要配置**成百上千个**效果——治疗、伤害、Buff、Debuff、控制效果……应有尽有。

每个 GE 都需要配置：
- 持续时间（Instant、Infinite、HasDuration）
- 堆叠策略
- 多个属性修改器
- 标签需求和控制
- 执行计算
- GameplayCue 特效
- 授予的能力

更别说 GameplayAbility（GA）了，它们同样需要配置一大堆东西。

### 1.2 传统配置方式的困境

在实践中，许多项目采用的传统解决方案是这样的：

**常见方案：** 在 Excel 中维护 GE 的数据，然后写导出工具将其转换为 JSON，再通过 SetByCaller 等运行时手段动态赋值给 GE。听起来不错，对吧？

**但问题来了：**

#### 问题 1：手工创建资产效率极低

策划或设计师需要在编辑器中**逐个**创建 GE 和 GA 蓝图资产，为每个资产手动配置几十个参数。当有成百上千个效果需要配置时，这简直是噩梦。我们开玩笑说，这得需要一个"铁血"策划来完成。

#### 问题 4：团队协作困难重重

策划人员通常不熟悉 Unreal Editor 复杂的界面和 GAS 的概念模型，难以独立完成配置。程序员则需要花费大量时间来协助配置或编写专用工具。最后的结果？两个人都累个半死，效率还是上不去。

### 1.3 传统 Excel 方案的局限性

采用 Excel + 导出代码的方式虽然比手工配置好得多，但同样存在明显的**局限性**：

1. **运行时动态赋值的局限**：通过 SetByCaller 赋值的参数只能是数值类型，无法处理复杂的结构化配置（如 Modifiers、GameplayCues 的嵌套结构）

2. **结构同步困难**：当 C++ 配置结构体变更时（添加新字段、修改类型等），Excel 工具和导出代码都需要手动更新，容易出现遗漏

3. **验证和纠错缺失**：Excel 中没有智能提示和验证，拼写错误的 Tag、Attribute 路径等直到运行时才被发现

4. **配置灵活性受限**：复杂的嵌套数据（如一个 GE 有多个 Modifiers、多个 GameplayCues）用 Excel 很难优雅地表示

这些痛点促使我们开发了 **AbilityEditorHelper** 插件，旨在通过更智能、更自动化的工作流来解决这些问题。

---

## 二、插件概述

### 2.1 什么是 AbilityEditorHelper

**AbilityEditorHelper** 是一个基于 Schema 驱动的 Unreal Engine GAS 配置自动化工具。它的核心理念很简单：**让策划和设计师能够使用他们最熟悉的 Excel 表格来配置 GameplayEffect 和 GameplayAbility，而不用进到 Unreal Editor 那个"大怪物"里面去。**

这个插件提供了一套完整的自动化工作流：
1. **从 C++ 结构体自动生成 JSON Schema**（程序员的事）
2. **基于 Schema 自动生成带智能提示的 Excel 模板**（策划填表的工具）
3. **在 Excel 中填写配置数据**（策划的主要工作）
4. **将 Excel 导出为 JSON 格式**（一键导出）
5. **自动导入 JSON 并创建/更新 UE 资产**（一键生成数千个资产）

换句话说：**程序员只需维护 C++ 结构体，策划就可以独立搞定所有的数值配置。** 两边都轻松，皆大欢喜。

### 2.2 核心价值

#### 价值 1：用 Excel 表格代替手工创建资产（拯救策划的生命）

策划和设计师可以在熟悉的 Excel 环境中**批量**配置 GE 和 GA，享受表格软件的"神级"便利功能：
- 复制粘贴快速复制相似配置（再也不用手动配一百遍了！）
- 使用公式进行数值计算（再也不用掏计算器了！）
- 批量查找替换（规模修改变成一键操作了！）
- 可视化的表格结构（清晰明了，一目了然！）

#### 价值 2：C++ 改动自动同步到配置流程（程序员的福音）

插件采用反射驱动的 Schema 生成机制。简而言之：**当程序员改 C++ 结构体时，整个工具链自动跟着变，没有任何手工同步的痛苦！**
- 重新生成 Schema（一键操作）
- Python 工具自动适配新的 Schema 结构（不用改代码！）
- Excel 模板自动更新字段和提示（不用手动修改！）
- 无需修改任何工具代码（真的！）

#### 价值 3：支持批量导入、增量更新（时间就是金钱）

- **批量导入**：一次性创建或更新数十个甚至数百个 GE/GA 资产（秒级完成！）
- **增量更新**：通过哈希机制智能识别哪些配置变了，只更新那些（大幅节省时间！）
- **自动清理**：可选择自动删除不在配置中的旧资产（告别冗余数据！）

#### 价值 4：降低配置门槛，提升团队协作效率（解决沟通成本）

- **智能提示**：Excel 中自动生成枚举值下拉列表（再也不用猜测合法的值了！）
- **格式说明**：第二行提供字段类型和格式要求的提示（清楚明白怎么填！）
- **结构化配置**：主表+子表的设计清晰表达复杂数据关系（复杂关系也能优雅表示！）
- **低学习成本**：策划无需深入理解 UE 编辑器和 GAS 实现细节（专注数值，忘掉复杂性！）

---

## 三、核心特性

### 3.1 Schema 驱动工作流（这是整个魔法的核心）

#### 什么是 Schema？

Schema 简单来说，就是 **C++ 结构体的"说明书"**——一份 JSON 格式的文档，记录了你的结构体的所有信息：
- 结构体路径和哈希值（用来识别结构体）
- 所有字段的名称、类型、枚举值（告诉工具这个字段是什么）
- 字段的元数据（ExcelHint、ExcelSeparator 等）（告诉工具怎么在 Excel 里显示）
- 特殊类型的转换规则（告诉工具怎么处理特殊类型）

**为什么需要 Schema？** 因为程序需要"理解"你的结构体，才能自动生成 Excel 模板。Schema 就是这个"理解"的中介。

#### 工作原理（一张图胜千言）

```
C++ 结构体定义（FGameplayEffectConfig）
           ↓
    UE 反射系统采集字段信息
           ↓
生成 JSON Schema（GameplayEffectConfig.schema.json）
           ↓
  Python 工具读取 Schema 生成 Excel 模板
           ↓
        Excel 模板提供给策划填写
           ↓
  Python 工具根据 Schema 解析 Excel 导出 JSON
           ↓
    C++ 代码读取 JSON 创建 UE 资产
```

#### 优势

- **单一数据源**：C++ 结构体是唯一的配置定义来源
- **自动同步**：结构体变更后重新生成 Schema 即可自动适配
- **可扩展性**：通过 UPROPERTY Meta 标签灵活控制导出行为

### 3.2 智能 Excel 模板生成（这是策划的天堂）

#### 自动生成的 Excel 特性（你会惊叹于它的聪明）

生成的 Excel 模板不只是一个普通表格，它很"聪明"：

##### 1. 下拉列表（再也不用猜了）

对于枚举类型字段，自动生成下拉列表，彻底避免拼写错误：

```
DurationType 字段下拉选项：
- Instant
- Infinite
- HasDuration
```

**你再也不用担心输入 "Infinte" 这样的错字了。只需点击下拉箭头，选择正确的值即可。**

##### 2. 字段类型提示

第二行显示字段的格式要求和说明：

| Name | Description | DurationType | AssetTags |
|------|-------------|--------------|-----------|
| 字符串 | 效果描述 | Instant/Infinite/HasDuration | GameplayTag，逗号分隔 |

##### 3. 主表+子表设计（优雅地处理复杂数据）

对于包含数组类型的结构体，插件会自动创建多个 Sheet：
- **主表（Main Sheet）**：放简单的字段（Name、Description、Duration 等）
- **子表（如 Modifiers Sheet）**：每个数组字段 `TArray<Struct>` 都有一个独立的 Sheet

子表通过 **ParentName** 列与主表关联。这样的设计有什么好处呢？

- **结构清晰**：一个效果可以有多个修改器，而不需要在主表里放一大堆重复的行
- **易于维护**：修改某个修改器时，只需在子表里改一行，不用改多行
- **无限扩展**：想加 10 个修改器就加 10 个，不受 Excel 列数限制

**主表示例：**
| Name | Description | DurationType |
|------|-------------|--------------|
| GE_Heal | 治疗效果 | Instant |

**Modifiers 子表示例：**
| ParentName | Attribute | ModifierOp | Magnitude |
|------------|-----------|-----------|-----------|
| GE_Heal | TestAttributeSet.HP | AddBase | 100.0 |
| GE_Heal | TestAttributeSet.MP | AddBase | 50.0 |

##### 4. 保留现有数据

生成模板时可选择 `preserve_data=True`，保留已有的配置数据，只更新结构变化。

### 3.3 双向数据同步

#### 完整的数据流

```
Excel 填表 → Python 导出为 JSON → C++ 导入创建资产
```

#### 增量更新机制

系统为每个配置行计算哈希值（基于所有字段内容）：
- 首次导入：创建新资产
- 再次导入：对比哈希值
  - 哈希未变：跳过
  - 哈希改变：更新资产

这大幅减少了大型项目的导入时间。

#### 自动清理旧资产

调用导入函数时可设置 `bClearGameplayEffectFolderFirst = true`：
- 自动删除目标目录下不在当前配置中的 GE 资产
- 保持资产目录整洁，避免遗留无用文件

### 3.4 强大的扩展性（为你的特殊需求量身定制）

#### 自定义 UPROPERTY Meta 标签（程序员的黑魔法）

如果你有特殊需求，可以在 C++ 结构体中使用 Meta 标签来控制 Excel 的生成行为：

```cpp
// 自定义提示文本（在 Excel 第二行显示）
UPROPERTY(EditAnywhere, meta = (ExcelHint = "格式：ClassName.PropertyName"))
FString Attribute;

// 自定义数组分隔符（某些时候用 | 比用 , 更合适）
UPROPERTY(EditAnywhere, meta = (ExcelSeparator = "|"))
TArray<FString> Tags;

// 忽略某个字段（某些调试字段不需要导出到 Excel）
UPROPERTY(EditAnywhere, meta = (ExcelIgnore = "true"))
FString InternalDebugField;
```

**这样做的好处：** 配置工具会根据你的指示来生成 Excel，完全按照你的需求来。

#### 支持派生结构体

可以基于现有结构体派生新的配置类型，Schema 生成会自动识别继承关系。

#### 后处理委托

插件提供 `OnGameplayEffectCreatedOrUpdated` 委托，允许在资产创建/更新后执行自定义逻辑：
- 自动配置额外属性
- 触发资产后处理
- 集成项目特定的工作流

---

## 四、工作原理

### 4.1 整体架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                     AbilityEditorHelper 工作流                      │
└─────────────────────────────────────────────────────────────────┘

┌──────────────────┐
│  C++ 结构体定义   │  程序员维护
│FGameplayEffectConfig│
└────────┬─────────┘
         │
         │ ① 通过 UE 反射系统
         ↓
┌──────────────────┐
│   JSON Schema    │  自动生成
│*.schema.json 文件 │
└────────┬─────────┘
         │
         │ ② Python 读取 Schema
         ↓
┌──────────────────┐
│   Excel 模板      │  自动生成（带下拉列表和提示）
│*.xlsx 文件        │
└────────┬─────────┘
         │
         │ ③ 策划填写配置
         ↓
┌──────────────────┐
│  填写完的 Excel   │  配置数据
│*.xlsx 文件        │
└────────┬─────────┘
         │
         │ ④ Python 基于 Schema 解析
         ↓
┌──────────────────┐
│    JSON 数据      │  标准格式
│*.json 文件        │
└────────┬─────────┘
         │
         │ ⑤ C++ 反序列化并创建资产
         ↓
┌──────────────────┐
│  UE GE/GA 资产   │  最终产物
│*.uasset 文件      │
└──────────────────┘
```

### 4.2 关键技术点

#### 1. UE 反射系统采集结构体信息

插件使用 Unreal Engine 的反射机制遍历 `USTRUCT` 和 `UPROPERTY`：

```cpp
// AbilityEditorHelperLibrary.cpp
void UAbilityEditorHelperLibrary::GenerateSchemaFromStruct(const FString& StructPath)
{
    UScriptStruct* ScriptStruct = LoadObject<UScriptStruct>(nullptr, *StructPath);

    // 遍历所有属性
    for (TFieldIterator<FProperty> It(ScriptStruct); It; ++It)
    {
        FProperty* Property = *It;

        // 获取属性类型、名称、元数据
        FString PropertyName = Property->GetName();
        FString PropertyType = GetPropertyTypeName(Property);
        FString ExcelHint = Property->GetMetaData(TEXT("ExcelHint"));

        // 记录到 Schema
        // ...
    }
}
```

#### 2. Schema 作为中间描述层

生成的 Schema 是标准 JSON 格式，可被任何语言解析：

```json
{
  "structPath": "/Script/AbilityEditorHelper.GameplayEffectConfig",
  "hash": "abc123...",
  "fields": [
    {
      "name": "DurationType",
      "kind": "enum",
      "enumValues": ["Instant", "Infinite", "HasDuration"],
      "excelHint": "效果持续类型"
    },
    {
      "name": "Modifiers",
      "kind": "array",
      "innerKind": "struct",
      "innerStructPath": "/Script/AbilityEditorHelper.GEModifierConfig",
      "excelSheet": "Modifiers"
    }
  ]
}
```

#### 3. Python 脚本驱动 Excel 操作

[ability_editor_excel_tool.py](d:\Project\AbilityHelperSample\Plugins\AbilityEditorHelper\Content\Python\ability_editor_excel_tool.py) 提供核心功能：

```python
def generate_excel_template_from_schema(schema_name_or_path, out_path, preserve_data=False):
    """基于 Schema 生成 Excel 模板"""
    schema = load_schema(schema_name_or_path)

    # 创建主表 Sheet
    main_sheet = create_main_sheet(schema)

    # 为每个数组字段创建子表 Sheet
    for field in schema['fields']:
        if field['kind'] == 'array' and field['innerKind'] == 'struct':
            create_sub_sheet(field)

    # 添加下拉列表和提示行
    add_data_validation(main_sheet, schema)

    workbook.save(out_path)
```

#### 4. 子表关联机制（ParentName 外键）

子表通过 `ParentName` 列关联主表的 `Name` 字段：

```
主表（GameplayEffectConfig）：
┌──────┬─────────────┬──────────┐
│ Name │ Description │ Duration │
├──────┼─────────────┼──────────┤
│ GE_1 │ 测试效果     │ Infinite │
└──────┴─────────────┴──────────┘

子表（Modifiers）：
┌────────────┬────────────┬──────────┬───────────┐
│ ParentName │ Attribute  │ ModifierOp│ Magnitude │
├────────────┼────────────┼──────────┼───────────┤
│ GE_1       │ HP         │ AddBase  │ 100.0     │
│ GE_1       │ MP         │ AddBase  │ 50.0      │
└────────────┴────────────┴──────────┴───────────┘

导出为 JSON 时聚合：
{
  "Name": "GE_1",
  "Description": "测试效果",
  "Duration": "Infinite",
  "Modifiers": [
    {"Attribute": "HP", "ModifierOp": "AddBase", "Magnitude": 100.0},
    {"Attribute": "MP", "ModifierOp": "AddBase", "Magnitude": 50.0}
  ]
}
```

这种设计允许：
- 一对多关系（一个 GE 可以有多个 Modifiers）
- 无限扩展（不受 Excel 列数限制）
- 结构清晰（每个子表独立管理）

---

## 五、快速开始教程

### 5.1 环境准备

#### 步骤 1：启用必需插件（激活超能力）

在 Unreal Editor 中：

1. 打开 **Edit → Plugins**
2. 搜索并启用以下插件：
   - **Python Editor Script Plugin**：让编辑器能理解 Python（必须项！）
   - **Editor Scripting Utilities**：提供编辑器操作的 API（也是必须项！）
3. 重启编辑器（这一步不能省，否则你会很伤心）

#### 步骤 2：启用 Python 开发者模式（强烈推荐！）

1. 打开 **Edit → Project Settings**
2. 搜索 **Python**
3. 在 **Plugins → Python** 部分，勾选 **Enable Python Developer Mode**

这会让 Python 报错时给你更详细的信息。相信我，当出错时你会感激这一步的。

#### 步骤 3：安装 Python 依赖（不装这个啥都用不了）

打开 **Window → Developer Tools → Output Log**，切换到 **Python** 标签，执行：

```python
import sys, subprocess
subprocess.check_call([sys.executable, "-m", "pip", "install", "openpyxl"])
```

如果成功，会看到 `Successfully installed openpyxl` 的消息。恭喜，你离成功又近了一步！

**备选方案**：如果 `openpyxl` 装不了，也别急。插件有"降级预案"，会自动改用 CSV 格式（生成 `.csv` 文件代替 `.xlsx`）。虽然不如 Excel 舒服，但起码能用。

### 5.2 第一个配置：创建 GameplayEffect（从零到一）

#### 步骤 1：告诉插件你想要什么

1. 打开 **Edit → Project Settings**
2. 搜索 **Ability Editor Helper**（找到插件的配置界面）
3. 在 **Schema** 部分找到 **Struct Type Paths To Export Schema**（这里指定你想导出的结构体）
4. 添加结构体路径，例如：
   ```
   /Script/AbilityEditorHelper.GameplayEffectConfig
   ```
   （翻译：我想为 GameplayEffectConfig 这个结构体生成 Schema）

#### 步骤 2：生成 Schema（让插件学会你的结构体）

现在让插件根据你的 C++ 结构体生成 Schema 文件（相当于一份"说明书"）。有两种方式：

**方式 A：图形界面方式（最懒的方法）**
1. 在上述设置页面，点击 **Generate All Schemas From Settings** 按钮
2. （可选）勾选 **Clear Schema Folder First** 可以清空现有 Schema（重新开始）

**方式 B：通过蓝图或 C++ 调用（给编程高手的）**

在蓝图中调用：
- 节点：**UAbilityEditorHelperLibrary::GenerateAllSchemasFromSettings**
- 参数：`bClearSchemaFolderFirst` 设置为 True

执行成功后，Schema 文件会生成到：
```
Plugins/AbilityEditorHelper/Content/Python/Schema/
```

你会看到一堆 `.schema.json` 文件，比如：
- `GameplayEffectConfig.schema.json`（GE 的说明书）
- `GEModifierConfig.schema.json`（修改器的说明书）
- `GameplayCueConfig.schema.json`（GameplayCue 的说明书）
- 等等……

#### 步骤 3：生成 Excel 模板（从说明书变成表格）

打开 **Python 控制台**（Window → Developer Tools → Output Log → Python 标签），复制粘贴以下代码：

```python
from ability_editor_excel_tool import generate_excel_template_from_schema

generate_excel_template_from_schema(
    schema_name_or_path="GameplayEffectConfig",
    out_path="/Game/Data/GameplayEffectData.xlsx",
    preserve_data=True  # 保留现有数据（如果文件已存在）
)
```

**参数说明：**
- `schema_name_or_path`：你要转换的 Schema 名称（不需要 `.schema.json` 后缀）
- `out_path`：生成的 Excel 文件要放在哪里（可以是 UE 路径 `/Game/...` 或磁盘路径）
- `preserve_data`：是否保留旧数据（True = 只更新结构，别删我的数据！）

执行完毕后，你会在对应位置看到一个**闪闪发光的** Excel 文件。恭喜！

#### 步骤 4：在 Excel 中填写配置（策划的舞台）

打开生成的 Excel 文件，你会看到一个**标准化的表格**。它的结构是这样的：

**主表（GameplayEffectConfig）：**
- **第一行**：字段名（Name、Description、DurationType……）
- **第二行**：字段说明和格式提示（告诉你怎么填）
- **第三行开始**：你的实际配置数据（现在轮到你了！）

**示例（参考这样怎么填）：**

| Name | Description | DurationType | DurationMagnitude | Period | AssetTags |
|------|-------------|--------------|-------------------|--------|-----------|
| 字符串 | 效果描述 | Instant/Infinite/HasDuration | 浮点数 | 浮点数 | GameplayTag，逗号分隔 |
| GE_Heal | 治疗效果 | Instant | 0.0 | 0.0 | Tests.GenericTag.One |
| GE_Poison | 持续中毒 | HasDuration | 10.0 | 1.0 | Tests.GenericTag.Two |

**子表（Modifiers）—— 存放一个 GE 的修改器列表：**

| ParentName | Attribute | ModifierOp | MagnitudeCalculationType | Magnitude |
|------------|-----------|-----------|------------------------|-----------|
| 主表关联 | 属性名 | 运算类型 | 计算类型 | 数值 |
| GE_Heal | TestAttributeSet.HP | AddBase | ScalableFloat | 100.0 |
| GE_Poison | TestAttributeSet.HP | AddBase | ScalableFloat | -5.0 |

**重要的填表要点（一定要记住这些！）：**
- `Name` **必须唯一**（这是资产的身份证号）
- `ParentName` **必须对应主表的某一行**（不能瞎写）
- 枚举字段用**下拉列表选择**（下拉箭头会在单元格出现）
- GameplayTag 格式：`Tag.Category.Name`，多个 Tag 用**逗号分隔**
- Attribute 格式：`AttributeSetClassName.PropertyName`（不要写错类名！）

#### 步骤 5：导出为 JSON（把 Excel 变成机器能读的格式）

在 Python 控制台执行（把你的 Excel 转成 JSON）：

```python
from ability_editor_excel_tool import export_excel_to_json_using_schema

export_excel_to_json_using_schema(
    in_path="/Game/Data/GameplayEffectData.xlsx",
    out_json_path="/Game/Data/GameplayEffectData.json",
    schema_name_or_path="GameplayEffectConfig"
)
```

**参数说明：**
- `in_path`：你的 Excel 文件在哪（刚才编辑的那个）
- `out_json_path`：JSON 要输出到哪里
- `schema_name_or_path`：用哪个 Schema 来解析（要和生成模板时一样！）

执行成功后，会生成一个标准的 JSON 文件。内容大概是这样的：

```json
[
  {
    "Name": "GE_Heal",
    "Description": "治疗效果",
    "DurationType": "Instant",
    "AssetTags": {
      "GameplayTags": [{"TagName": "Tests.GenericTag.One"}],
      "ParentTags": [{"TagName": "Tests"}, {"TagName": "Tests.GenericTag"}]
    },
    "Modifiers": [
      {
        "Attribute": "TestAttributeSet.HP",
        "ModifierOp": "AddBase",
        "MagnitudeCalculationType": "ScalableFloat",
        "Magnitude": 100.0
      }
    ]
  }
]
```

（看起来很复杂？这就是为什么咱们有 Excel！）

#### 步骤 6：导入到 UE 并查看资产（见证奇迹的时刻）

现在是最激动人心的时刻——将 JSON 导入成真正的 UE 资产！有两种方式：

**方式 A：图形界面方式（推荐新手）**
1. 打开 **Edit → Project Settings → Ability Editor Helper**
2. 在 **Import** 部分，设置 **Gameplay Effect JSON Path**（指向你刚生成的 JSON 文件）
3. 设置 **Gameplay Effect Folder**（GE 资产要放在哪个文件夹）
4. （可选）勾选 **Clear Gameplay Effect Folder First**（想从头开始就勾这个）
5. 点击 **Import And Update Gameplay Effects From Json** 按钮
6. **等待……** 系统会一个接一个地创建所有资产

**方式 B：蓝图/C++ 方式（给编程高手的）**

在蓝图中调用：
- 节点：**UAbilityEditorHelperLibrary::ImportAndUpdateGameplayEffectsFromJson**
- 参数：`bClearGameplayEffectFolderFirst` 设置为 True

**导入完成后，打开 Content Browser，你会看到**你新创建的 GameplayEffect 资产：
- `GE_Heal`
- `GE_Poison`
- （以及你配置的所有其他 GE……）

**双击打开任何一个资产，你会看到所有配置都完美应用了：**
- Duration Policy ✓
- Modifiers ✓
- Gameplay Tags ✓
- 所有你在 Excel 里填的内容 ✓

**完成！** 你刚才用 Excel 配置的东西现在都成了真正的 UE 资产！

### 5.3 Excel 填表说明（新手必读）

#### 主表结构说明（怎么填才对？）

主表包含结构体的所有非数组字段。不同类型的字段有不同的填写方式：

| 字段类型 | 填写说明 | 示例 |
|---------|---------|------|
| **字符串** | 直接填写文本（随便写） | `"Test Effect"` |
| **整数** | 填写数字（没有小数点） | `10` |
| **浮点数** | 填写小数（需要小数点） | `1.5` |
| **布尔值** | 填写 `true` 或 `false`（别写其他的） | `false` |
| **枚举** | 使用下拉列表选择（强烈推荐！避免手误） | `Infinite` |
| **资产路径** | 填写完整路径或相对路径（必须有 `.后缀_C`） | `/Game/GE_Base.GE_Base_C` |
| **GameplayTag** | 逗号分隔多个 Tag（格式：`Tag.A, Tag.B`） | `Tag.A, Tag.B` |
| **基本类型数组** | 逗号分隔多个值（比如多个资产路径） | `/Game/GA_1, /Game/GA_2` |

#### 子表关联方法

每个子表都有一个 `ParentName` 列：

**规则：**
1. `ParentName` 必须对应主表中某一行的 `Name` 字段
2. 一个 `ParentName` 可以对应多行（一对多关系）
3. 子表中可以为同一个 `ParentName` 配置任意多行数据

**示例：**

主表：
```
| Name    | Description |
|---------|-------------|
| Effect1 | 第一个效果   |
| Effect2 | 第二个效果   |
```

Modifiers 子表：
```
| ParentName | Attribute | ModifierOp | Magnitude |
|------------|-----------|-----------|-----------|
| Effect1    | HP        | AddBase   | 100.0     |
| Effect1    | MP        | AddBase   | 50.0      |
| Effect2    | HP        | AddBase   | -20.0     |
```

导出后：
- `Effect1` 有 2 个 Modifiers
- `Effect2` 有 1 个 Modifier

#### 特殊类型格式

##### GameplayTag

**单个 Tag：**
```
Tests.GenericTag.One
```

**多个 Tags（逗号分隔）：**
```
Tests.GenericTag.One, Tests.GenericTag.Two
```

导出时会自动生成 ParentTags（层级结构）。

##### GameplayAttribute

**格式：**
```
AttributeSetClassName.PropertyName
```

**示例：**
```
TestAttributeSet.TestPropertyOne
```

系统会自动解析为完整的 Attribute 路径。

##### TagRequirements（需求和忽略）

**快速格式（在单个字段中配置）：**
```
Require:TagA,TagB|Ignore:TagC,TagD
```

- `Require:` 后面跟需要的 Tags
- `Ignore:` 后面跟忽略的 Tags
- 用 `|` 分隔

**标准格式（分别在 RequireTags 和 IgnoreTags 字段）：**
- RequireTags：`TagA, TagB`
- IgnoreTags：`TagC, TagD`

#### 下拉菜单的使用

对于枚举类型字段，Excel 会自动生成下拉菜单：

1. 点击单元格
2. 会出现下拉箭头
3. 选择合法的枚举值

**优点：**
- 避免拼写错误
- 快速选择合法值
- 可视化所有选项

---

## 六、进阶用法（精进你的技艺）

### 6.1 配置 GameplayAbility（技能配置指南）

GameplayAbility（GA）的配置流程与 GameplayEffect 类似，但有一些**特有的字段**和概念，需要单独说明。

#### Ability 特有字段说明

| 字段 | 类型 | 说明 |
|------|------|------|
| **CostGameplayEffectClass** | 资产路径 | 技能消耗的 GE（如消耗蓝量） |
| **CooldownGameplayEffectClass** | 资产路径 | 技能冷却的 GE |
| **AbilityTags** | GameplayTag | 标识技能的 Tags |
| **CancelAbilitiesWithTag** | GameplayTag | 激活时取消带有这些 Tags 的技能 |
| **BlockAbilitiesWithTag** | GameplayTag | 激活时阻塞带有这些 Tags 的技能 |
| **ActivationOwnedTags** | GameplayTag | 激活时获得的 Tags |
| **ActivationRequiredTags** | GameplayTag | 激活所需的 Tags |
| **ActivationBlockedTags** | GameplayTag | 激活时阻止的 Tags |
| **NetExecutionPolicy** | 枚举 | 网络执行策略（LocalPredicted、LocalOnly、ServerInitiated、ServerOnly） |
| **InstancingPolicy** | 枚举 | 实例化策略（NonInstanced、InstancedPerActor、InstancedPerExecution） |

#### Cost 和 Cooldown 配置

**Cost GE（消耗）：**
创建一个 Instant 类型的 GE，包含负值 Modifier：

```
主表：
| Name       | DurationType | Description |
|------------|--------------|-------------|
| GE_ManaCost | Instant     | 消耗蓝量     |

Modifiers 子表：
| ParentName   | Attribute | ModifierOp | Magnitude |
|--------------|-----------|-----------|-----------|
| GE_ManaCost  | MP        | AddBase   | -50.0     |
```

**Cooldown GE（冷却）：**
创建一个 HasDuration 类型的 GE，带有 Cooldown Tag：

```
主表：
| Name         | DurationType | DurationMagnitude | GrantedTags |
|--------------|--------------|-------------------|-------------|
| GE_SkillCD   | HasDuration  | 5.0               | Cooldown.Skill.FireBall |

无需 Modifiers，只需 GrantedTags 即可标识冷却状态。
```

**在 Ability 中引用：**

```
| Name       | CostGameplayEffectClass | CooldownGameplayEffectClass |
|------------|------------------------|----------------------------|
| GA_FireBall | /Game/GE_ManaCost     | /Game/GE_SkillCD           |
```

#### Trigger 子表的使用

AbilityTriggers 允许配置技能的自动触发条件。

**子表结构：**

| ParentName | TriggerTag | TriggerSource |
|------------|-----------|---------------|
| 主表关联 | 触发的 Tag | OwnedTagAdded/OwnedTagPresent |
| GA_FireBall | Event.Fire | OwnedTagAdded |

**说明：**
- `TriggerTag`：当该 Tag 被添加或存在时触发技能
- `TriggerSource`：触发源类型（OwnedTagAdded、OwnedTagPresent 等）

### 6.2 自定义 Schema 元数据（为 C++ 程序员准备的黑科技）

如果默认行为不能满足你的需求，可以在 C++ 结构体定义中使用 Meta 标签来精细控制导出行为。

#### ExcelHint：自定义提示文本

为字段添加友好的说明文本：

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config",
    meta = (ExcelHint = "格式：AttributeSetClassName.PropertyName，例如 TestAttributeSet.HP"))
FString Attribute;
```

生成的 Excel 第二行会显示这段提示。

#### ExcelSeparator：自定义数组分隔符

修改数组元素的分隔符（默认为逗号）：

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config",
    meta = (ExcelSeparator = "|"))
TArray<FString> GrantedAbilityClasses;
```

填表时可以使用：
```
/Game/GA_1 | /Game/GA_2 | /Game/GA_3
```

#### ExcelIgnore：忽略某些字段

某些调试或内部字段不需要导出到 Excel：

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug",
    meta = (ExcelIgnore = "true"))
FString InternalDebugInfo;
```

生成 Excel 模板时会跳过该字段。

#### ExcelSheet：指定子表名称

为数组字段指定自定义的 Sheet 名称：

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config",
    meta = (ExcelSheet = "AttributeModifiers"))
TArray<FGEModifierConfig> Modifiers;
```

生成的子表 Sheet 名称会是 "AttributeModifiers" 而不是默认的 "Modifiers"。

### 6.3 增量更新与版本管理（大型项目的秘密武器）

#### 哈希机制原理（为什么导入这么快？）

插件为每个配置行计算哈希值：

```cpp
// 伪代码
FString CalculateRowHash(const FGameplayEffectConfig& Config)
{
    FString CombinedData = Config.Name + Config.Description +
                           Config.DurationType + /* 所有字段 */;
    return FMD5::HashAnsiString(*CombinedData);
}
```

导入时对比哈希值：
- 新配置（哈希不存在）：创建资产
- 配置未变（哈希相同）：跳过
- 配置已变（哈希不同）：更新资产

#### 如何进行增量导入

**场景：** 已有 100 个 GE，本次只修改了其中 5 个。

**步骤：**
1. 在 Excel 中修改 5 个 GE 的配置
2. 导出为 JSON（包含全部 100 个 GE）
3. 调用导入函数（`bClearGameplayEffectFolderFirst = false`）
4. 系统只更新哈希值变化的 5 个 GE

**优点：**
- 大幅减少导入时间
- 避免不必要的资产重新编译
- 保护未修改的资产

#### 团队协作最佳实践（如何让团队和谐相处）

##### 实践 1：版本控制管理 Excel 文件（Git 也要管好）

将 Excel 文件纳入版本控制（Git/SVN）是最佳实践：
- **追踪历史**：谁什么时候改了什么一目了然
- **多人协作**：多个人可以在不同的时间修改配置，Git 会帮你合并
- **回滚**：出错了？一条命令回到之前的版本

**建议：**
- 使用 `.xlsx` 格式（而非 `.xls`），这样 Git diff 会更清楚
- 定期提交（别等到最后一天突然提交 100 个修改）
- 提交信息要有意义（比如 "调整火球技能伤害从 100 改为 150"，别写 "update"）

##### 实践 2：程序和策划的分工（各司其职）

| 角色 | 职责 |
|------|------|
| **程序员** | 维护 C++ 结构体 / 添加新字段 / 生成 Schema / 通知策划更新模板 |
| **策划** | 填 Excel 表格 / 导出 JSON / 导入到 UE / 测试效果 |

简单来说：**程序员管结构，策划管数据。各自安心做自己的事儿。**

##### 实践 3：定期清理和验证（保持 Excel 的"整洁"）

定期执行：
```python
# 重新生成 Excel 模板（保留数据）
generate_excel_template_from_schema(
    schema_name_or_path="GameplayEffectConfig",
    out_path="/Game/Data/GameplayEffectData.xlsx",
    preserve_data=True
)
```

这会更新 Excel 结构，但保留现有配置数据，确保与最新 Schema 同步。

---

## 七、示例文件解析（通过真实例子学习）

插件贴心地提供了**完整的示例文件**，位于：
```
Plugins/AbilityEditorHelper/DataSample/
```

我们来详细解析这些示例，学习如何正确配置 GE 和 GA。相信我，看完这些例子，你就知道该怎么填表了。

### 7.1 TestEffect.json 和 TestEffect.xlsx 解析

#### Test_1 配置说明（Infinite 类型的 GE）

[TestEffect.json](d:\Project\AbilityHelperSample\Plugins\AbilityEditorHelper\DataSample\TestEffect.json) 中的第一个配置：

```json
{
  "Name": "Test_1",
  "Description": "Test",
  "ParentClass": "",
  "DurationType": "Infinite",
  "DurationMagnitude": 0.0,
  "Period": 0.0,
  "StackingType": "None",
  "AssetTags": {
    "GameplayTags": [
      {"TagName": "Tests.GenericTag.One"},
      {"TagName": "Tests.GenericTag.Two"}
    ]
  },
  "ApplicationTagRequirements": {
    "RequireTags": {
      "GameplayTags": [{"TagName": "Tests.GenericTag.One"}]
    }
  },
  "OngoingTagRequirements": {
    "IgnoreTags": {
      "GameplayTags": [{"TagName": "Tests.GenericTag.One"}]
    }
  },
  "Modifiers": [
    {
      "Attribute": "TestAttributeSet.TestPropertyOne",
      "ModifierOp": "AddBase",
      "MagnitudeCalculationType": "ScalableFloat",
      "Magnitude": 100.0
    }
  ],
  "Executions": [
    {
      "CalculationClass": "/Game/DevTest/GEE_Test.GEE_Test_C"
    }
  ]
}
```

**配置要点解析：**

##### 1. 基础配置字段

| 字段 | 值 | 说明 |
|------|-----|------|
| **Name** | `"Test_1"` | 资产名称，也是主键 |
| **Description** | `"Test"` | 效果描述（可选） |
| **ParentClass** | `""` | 空表示从基类 UGameplayEffect 创建 |
| **DurationType** | `"Infinite"` | 永久效果（不会自动移除） |
| **DurationMagnitude** | `0.0` | Duration 为 Infinite 时忽略 |
| **Period** | `0.0` | 不执行周期性效果 |
| **StackingType** | `"None"` | 不允许堆叠 |

##### 2. Modifiers 子表的使用

配置了一个 Modifier：
- **Attribute**: `TestAttributeSet.TestPropertyOne` - 目标属性
- **ModifierOp**: `AddBase` - 运算类型（基础加法）
- **MagnitudeCalculationType**: `ScalableFloat` - 使用简单的浮点数值
- **Magnitude**: `100.0` - 加 100 点

**效果：** 应用此 GE 后，目标的 `TestPropertyOne` 属性会增加 100。

##### 3. Executions 配置

配置了一个 Execution：
- **CalculationClass**: `/Game/DevTest/GEE_Test.GEE_Test_C` - 自定义计算类

**说明：** Execution 允许执行复杂的自定义逻辑，比 Modifier 更灵活。

##### 4. Tag 配置示例

**AssetTags（资产标签）：**
```json
"AssetTags": {
  "GameplayTags": [
    {"TagName": "Tests.GenericTag.One"},
    {"TagName": "Tests.GenericTag.Two"}
  ]
}
```
这些 Tags 标识该 GE 的类型/类别，可用于查询和过滤。

**ApplicationTagRequirements（应用需求）：**
```json
"ApplicationTagRequirements": {
  "RequireTags": {
    "GameplayTags": [{"TagName": "Tests.GenericTag.One"}]
  }
}
```
目标必须拥有 `Tests.GenericTag.One` Tag 才能应用此 GE。

**OngoingTagRequirements（持续需求）：**
```json
"OngoingTagRequirements": {
  "IgnoreTags": {
    "GameplayTags": [{"TagName": "Tests.GenericTag.One"}]
  }
}
```
如果目标拥有 `Tests.GenericTag.One` Tag，此 GE 会被移除（持续条件不满足）。

#### Test_2 配置说明（展示不同的配置组合）

第二个配置更简单：

```json
{
  "Name": "Test_2",
  "Description": "Test",
  "ParentClass": "/Game/GE_Base.GE_Base_C",
  "DurationType": "Infinite",
  "Modifiers": [],
  "GameplayCues": [],
  "Executions": []
}
```

**配置要点：**

##### 使用 ParentClass

```json
"ParentClass": "/Game/GE_Base.GE_Base_C"
```

指定了父类蓝图，新创建的 GE 会继承父类的所有配置。这在以下场景很有用：
- 多个 GE 共享基础配置
- 在蓝图中自定义额外逻辑
- 实现 GE 的模板模式

##### 空数组的处理

```json
"Modifiers": [],
"GameplayCues": [],
"Executions": []
```

空数组表示该 GE 没有 Modifiers、GameplayCues 和 Executions。这种 GE 可能只用于：
- 授予 Tags（通过 GrantedTags）
- 阻塞其他技能（通过 BlockAbilitiesWithTags）
- 作为 Cost 或 Cooldown GE

### 7.2 TestAbility.json 和 TestAbility.xlsx 解析

[TestAbility.json](d:\Project\AbilityHelperSample\Plugins\AbilityEditorHelper\DataSample\TestAbility.json) 中包含两个 GameplayAbility 配置。

#### TestAbility_1 配置说明

```json
{
  "Name": "TestAbility_1",
  "Description": "",
  "ParentClass": "",
  "CostGameplayEffectClass": "",
  "CooldownGameplayEffectClass": "",
  "AbilityTags": {
    "GameplayTags": [{"TagName": "Tests.GenericTag.One"}]
  },
  "bServerRespectsRemoteAbilityCancellation": false,
  "bReplicateInputDirectly": false,
  "NetExecutionPolicy": "LocalPredicted",
  "NetSecurityPolicy": "ClientOrServer",
  "InstancingPolicy": "InstancedPerActor",
  "ReplicationPolicy": "ReplicateNo",
  "bRetriggerInstancedAbility": false,
  "AbilityTriggers": []
}
```

**配置要点：**

##### 1. 基础 Ability 字段

| 字段 | 值 | 说明 |
|------|-----|------|
| **Name** | `"TestAbility_1"` | 技能资产名称 |
| **ParentClass** | `""` | 从基类 UGameplayAbility 创建 |
| **CostGameplayEffectClass** | `""` | 无消耗（不消耗资源） |
| **CooldownGameplayEffectClass** | `""` | 无冷却 |

##### 2. Tag 配置

**AbilityTags（技能标签）：**
```json
"AbilityTags": {
  "GameplayTags": [{"TagName": "Tests.GenericTag.One"}]
}
```

用于标识该技能，其他系统可以通过 Tag 查询或过滤技能。

**其他 Tag 字段（均为空）：**
- `CancelAbilitiesWithTag`：激活时取消的技能 Tags
- `BlockAbilitiesWithTag`：激活时阻塞的技能 Tags
- `ActivationOwnedTags`：激活时临时获得的 Tags
- `ActivationRequiredTags`：激活所需的 Tags
- `ActivationBlockedTags`：拥有这些 Tags 时无法激活

##### 3. 网络和实例化策略

| 字段 | 值 | 说明 |
|------|-----|------|
| **NetExecutionPolicy** | `"LocalPredicted"` | 本地预测执行（先本地执行，后服务器确认） |
| **NetSecurityPolicy** | `"ClientOrServer"` | 客户端或服务器都可以激活 |
| **InstancingPolicy** | `"InstancedPerActor"` | 每个 Actor 一个实例 |
| **ReplicationPolicy** | `"ReplicateNo"` | 不复制技能实例 |

这些配置决定了技能在多人游戏中的行为。

#### TestAbility_2 配置说明

```json
{
  "Name": "TestAbility_2",
  "ParentClass": "/Game/DevTest/GA_Test.GA_Test_C",
  "AbilityTags": {
    "GameplayTags": [
      {"TagName": "Tests.GenericTag.One"},
      {"TagName": "Tests.GenericTag.Two"}
    ]
  },
  "TestFloatValue": 1.0,
  "TestIntValue": 2,
  "bTestBoolValue": true,
  "AbilityTriggers": []
}
```

**配置要点：**

##### 使用 ParentClass

```json
"ParentClass": "/Game/DevTest/GA_Test.GA_Test_C"
```

从自定义蓝图类继承，可以：
- 复用蓝图中的逻辑实现
- 在蓝图中定义激活行为（ActivateAbility）
- 添加自定义事件和变量

##### 自定义字段

```json
"TestFloatValue": 1.0,
"TestIntValue": 2,
"bTestBoolValue": true
```

这些是在派生结构体中添加的自定义字段。如果你在 C++ 中定义了：

```cpp
USTRUCT(BlueprintType)
struct FMyGameplayAbilityConfig : public FGameplayAbilityConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TestFloatValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TestIntValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bTestBoolValue;
};
```

这些字段会出现在 Schema 和 Excel 模板中，可以在导入时自动应用到蓝图。

### 7.3 从示例学习

#### 如何仿照示例创建自己的配置

**步骤 1：确定配置类型**
- 需要创建 GE 还是 GA？
- 参考相似的示例配置

**步骤 2：复制示例行**
- 在 Excel 中复制一行示例数据
- 修改 Name 为新的名称

**步骤 3：修改关键字段**
- 调整 Description、DurationType 等基础字段
- 根据需要修改 Tags 配置
- 在子表中添加 Modifiers、Executions 等

**步骤 4：导出并导入**
- 执行 Python 导出命令生成 JSON
- 执行导入命令创建 UE 资产
- 在编辑器中打开资产验证配置

#### 常用配置模式总结

##### 模式 1：即时治疗效果

```
主表：
| Name    | DurationType | Description |
|---------|--------------|-------------|
| GE_Heal | Instant      | 治疗效果    |

Modifiers 子表：
| ParentName | Attribute | ModifierOp | Magnitude |
|------------|-----------|-----------|-----------|
| GE_Heal    | HP        | AddBase   | 100.0     |
```

##### 模式 2：持续伤害（DOT）

```
主表：
| Name       | DurationType | DurationMagnitude | Period |
|------------|--------------|-------------------|--------|
| GE_Poison  | HasDuration  | 10.0              | 1.0    |

Modifiers 子表：
| ParentName | Attribute | ModifierOp | Magnitude |
|------------|-----------|-----------|-----------|
| GE_Poison  | HP        | AddBase   | -5.0      |
```

每秒扣除 5 点 HP，持续 10 秒。

##### 模式 3：属性增益 Buff

```
主表：
| Name         | DurationType | DurationMagnitude | GrantedTags |
|--------------|--------------|-------------------|-------------|
| GE_StrengthBuff | HasDuration | 30.0            | Buff.Strength |

Modifiers 子表：
| ParentName      | Attribute | ModifierOp | Magnitude |
|-----------------|-----------|-----------|-----------|
| GE_StrengthBuff | Strength  | MultiplyAdditive | 0.5 |
```

增加 50% 力量，持续 30 秒。

##### 模式 4：技能 Cost（消耗）

```
主表：
| Name        | DurationType |
|-------------|--------------|
| GE_ManaCost | Instant      |

Modifiers 子表：
| ParentName  | Attribute | ModifierOp | Magnitude |
|-------------|-----------|-----------|-----------|
| GE_ManaCost | MP        | AddBase   | -50.0     |
```

瞬间消耗 50 点蓝量。

##### 模式 5：技能 Cooldown（冷却）

```
主表：
| Name       | DurationType | DurationMagnitude | GrantedTags |
|------------|--------------|-------------------|-------------|
| GE_SkillCD | HasDuration  | 5.0               | Cooldown.Skill |
```

授予冷却 Tag，持续 5 秒。技能检查该 Tag 来判断是否在冷却中。

---

## 八、总结：为什么这个插件能救你一命

### 8.1 插件的核心优势回顾

**AbilityEditorHelper** 用以下几点硬核优势，让你的 UE GAS 项目效率起飞：

#### 优势 1：自动化工作流（再也不用手工配置了）

- **零手工维护**：Schema 自动生成，Excel 模板自动生成，连导表代码都不用写！
- **批量操作**：一次性创建/更新**成百个** GE/GA 资产（秒级完成）
- **增量更新**：智能识别哪些配置变了，只更新那些（省时省力）

#### 优势 2：低耦合架构（程序员和策划终于不用互相折磨了）

- **C++ 为单一数据源**：所有配置定义来自 C++ 结构体（一处修改，处处生效）
- **Schema 作为中间层**：完全解耦 C++ 和配置工具（改一边不用改另一边）
- **结构变更自动适配**：C++ 改动后重新生成 Schema，一切都自动更新（没有手工同步的痛苦）

#### 优势 3：降低配置门槛（策划终于可以自主工作了）

- **Excel 友好**：策划用熟悉的表格工具，不用进 Unreal Editor（感谢上帝！）
- **智能提示**：下拉列表、格式说明、类型提示样样俱全（再也不用猜怎么填）
- **清晰的数据结构**：主表+子表的设计很直观（一眼就能看懂）

#### 优势 4：团队协作优化（最后，大家都开心了）

- **明确的角色分工**：程序维护结构，策划填写数据（井水不犯河水）
- **版本控制友好**：Excel 文件易于追踪变更（Git 里可以看到谁改了什么）
- **减少沟通成本**：策划可以独立完成配置，程序员再也不用陪着调参（效率提升 N 倍）

### 8.2 对 GAS 工作流的改进效果（用数字说话）

#### 效率提升（时间就是金钱）

- **配置时间**：从手工创建资产的"分钟级"（有时候几个小时）缩短到批量导入的"秒级"
  - 以前：配置 100 个 GE 需要几天……
  - 现在：Excel 一填，导出导入，搞定！
- **迭代速度**：修改数值后一键导出导入，快速验证效果（设计师的梦想成真了！）
- **错误率**：下拉列表和格式验证大幅减少拼写错误（几乎不可能手误了）

#### 可维护性提升（代码的整洁度）

- **结构同步**：C++ 结构体变更自动反映到配置流程（没有代沟了）
- **历史追溯**：版本控制记录所有配置变更历史（想看谁什么时候改了什么？一条 Git 命令！）
- **规范统一**：所有配置遵循相同的结构和格式（没有五花八门的不规范配置了）

#### 团队协作提升（人与人之间的和谐）

- **策划自主性**：无需程序员协助即可完成数值配置（策划终于可以自主工作了！）
- **程序员解放**：从繁琐的配置工作中解放出来，可以专注核心功能开发（程序员也终于开心了！）
- **并行开发**：策划和程序可以并行工作，互不阻塞（效率翻倍！）

### 8.3 未来可能的扩展方向（想象一下……）

#### 方向 1：可视化编辑器（谁说必须用 Excel？）

开发 UE 编辑器插件，提供专用 GUI 界面：
- **可视化配置 GE/GA**：拖拖拽拽就能配置
- **实时预览效果**：改个数值立即看到效果（设计师会哭着感谢你）
- **拖拽式配置复杂关系**：再复杂的嵌套关系也能直观操作

#### 方向 2：数值验证和测试（自动帮你找 bug）

- **配置合法性检查**：在导出前就告诉你哪些数据有问题（Magnitude 超范围了？立即提醒）
- **自动化测试框架**：自动验证 GE 效果是否符合预期（不用手工测了）
- **数值平衡分析工具**：帮你找出数值不平衡的地方（游戏平衡师的福音）

#### 方向 3：更多数据源支持（满足各种需求）

- **Google Sheets 集成**：在云端协作，再也不用传 Excel 文件了
- **数据库直连**：大型项目可以直接连数据库
- **自定义格式支持**：JSON、YAML、CSV……你想要啥格式就支持啥

#### 方向 4：模板库和最佳实践（站在巨人的肩膀上）

- **内置常用 GE/GA 模板**：治疗、伤害、Buff、Debuff 等常用效果（复制粘贴即用）
- **配置模式库**：DOT、HOT、AOE、Cooldown 等常见模式（不用从零开始）
- **项目示例和教程**：看别人怎么用这个插件（学习效率最高）

---

## 附录

### A. 完整的配置字段参考表

#### FGameplayEffectConfig 字段列表

| 字段名 | 类型 | 说明 |
|--------|------|------|
| Name | FString | 资产名称（必填，唯一） |
| Description | FString | 效果描述 |
| ParentClass | FString | 父类蓝图路径 |
| DurationType | Enum | 持续类型：Instant/Infinite/HasDuration |
| DurationMagnitude | float | 持续时间（DurationType=HasDuration 时有效） |
| Period | float | 周期执行间隔（0 表示不周期执行） |
| StackingType | Enum | 堆叠类型：None/AggregateBySource/AggregateByTarget |
| StackLimitCount | int32 | 堆叠上限 |
| StackDurationRefreshPolicy | Enum | 堆叠时刷新持续时间策略 |
| StackPeriodResetPolicy | Enum | 堆叠时重置周期策略 |
| AssetTags | GameplayTagContainer | 资产标签 |
| GrantedTags | GameplayTagContainer | 授予目标的标签 |
| ApplicationTagRequirements | TagRequirements | 应用时的标签需求 |
| OngoingTagRequirements | TagRequirements | 持续时的标签需求 |
| RemovalTagRequirements | TagRequirements | 移除时的标签需求 |
| CancelAbilitiesWithTags | GameplayTagContainer | 取消带有这些标签的技能 |
| BlockAbilitiesWithTags | GameplayTagContainer | 阻塞带有这些标签的技能 |
| GrantedAbilityClasses | TArray&lt;FString&gt; | 授予的技能类路径 |
| Modifiers | TArray&lt;FGEModifierConfig&gt; | 属性修改器数组（子表） |
| GameplayCues | TArray&lt;FGameplayCueConfig&gt; | GameplayCue 配置数组（子表） |
| ImmunityQueries | TArray&lt;FEffectQueryConfig&gt; | 免疫查询数组（子表） |
| RemovalQueries | TArray&lt;FEffectQueryConfig&gt; | 移除查询数组（子表） |
| Executions | TArray&lt;FExecutionConfig&gt; | 执行计算数组（子表） |

#### FGameplayAbilityConfig 字段列表

| 字段名 | 类型 | 说明 |
|--------|------|------|
| Name | FString | 技能资产名称（必填，唯一） |
| Description | FString | 技能描述 |
| ParentClass | FString | 父类蓝图路径 |
| CostGameplayEffectClass | FString | Cost GE 类路径 |
| CooldownGameplayEffectClass | FString | Cooldown GE 类路径 |
| AbilityTags | GameplayTagContainer | 技能标签 |
| CancelAbilitiesWithTag | GameplayTagContainer | 取消技能标签 |
| BlockAbilitiesWithTag | GameplayTagContainer | 阻塞技能标签 |
| ActivationOwnedTags | GameplayTagContainer | 激活时拥有的标签 |
| ActivationRequiredTags | GameplayTagContainer | 激活所需标签 |
| ActivationBlockedTags | GameplayTagContainer | 激活时阻止的标签 |
| SourceRequiredTags | GameplayTagContainer | 源所需标签 |
| SourceBlockedTags | GameplayTagContainer | 源阻止标签 |
| TargetRequiredTags | GameplayTagContainer | 目标所需标签 |
| TargetBlockedTags | GameplayTagContainer | 目标阻止标签 |
| NetExecutionPolicy | Enum | 网络执行策略 |
| NetSecurityPolicy | Enum | 网络安全策略 |
| InstancingPolicy | Enum | 实例化策略 |
| ReplicationPolicy | Enum | 复制策略 |
| bServerRespectsRemoteAbilityCancellation | bool | 服务器是否响应远程取消 |
| bReplicateInputDirectly | bool | 是否直接复制输入 |
| bRetriggerInstancedAbility | bool | 是否重新触发已实例化技能 |
| AbilityTriggers | TArray&lt;FAbilityTriggerConfig&gt; | 触发器数组（子表） |

### B. Excel 模板结构示例

#### 主表结构（GameplayEffectConfig）

```
行1（字段名）：
| Name | Description | DurationType | DurationMagnitude | Period | ... |

行2（提示）：
| 字符串，唯一标识 | 效果描述 | Instant/Infinite/HasDuration | 浮点数 | 浮点数 | ... |

行3+（数据）：
| GE_Heal | 治疗效果 | Instant | 0.0 | 0.0 | ... |
```

#### 子表结构（Modifiers）

```
行1（字段名）：
| ParentName | Attribute | ModifierOp | MagnitudeCalculationType | Magnitude | ... |

行2（提示）：
| 主表Name | AttributeSet.Property | AddBase/MultiplyAdditive/... | ScalableFloat/AttributeBased/... | 浮点数 | ... |

行3+（数据）：
| GE_Heal | TestAttributeSet.HP | AddBase | ScalableFloat | 100.0 | ... |
```

### C. Python 脚本 API 参考

#### 生成 Excel 模板

```python
from ability_editor_excel_tool import generate_excel_template_from_schema

generate_excel_template_from_schema(
    schema_name_or_path: str,  # Schema 名称或路径
    out_path: str,             # 输出 Excel 文件路径
    preserve_data: bool = False # 是否保留现有数据
)
```

#### 导出 Excel 为 JSON

```python
from ability_editor_excel_tool import export_excel_to_json_using_schema

export_excel_to_json_using_schema(
    in_path: str,              # 输入 Excel 文件路径
    out_json_path: str,        # 输出 JSON 文件路径
    schema_name_or_path: str   # 使用的 Schema 名称或路径
)
```

### D. 常用 Meta 标签列表

| Meta 标签 | 用途 | 示例 |
|-----------|------|------|
| **ExcelHint** | 字段提示文本 | `meta = (ExcelHint = "格式：ClassName.PropertyName")` |
| **ExcelSeparator** | 数组分隔符 | `meta = (ExcelSeparator = "\|")` |
| **ExcelIgnore** | 忽略该字段 | `meta = (ExcelIgnore = "true")` |
| **ExcelSheet** | 指定子表名称 | `meta = (ExcelSheet = "AttributeModifiers")` |

---

本文档完整介绍了 **AbilityEditorHelper** 插件的功能、使用方法和最佳实践。希望这个插件能像一股"清风"吹过你的项目，让 GAS 配置从"噩梦"变成"小菜一碟"。

**再次总结一下这个插件的秘密武器：**
- 程序员只需维护 C++ 结构体（其他的自动搞定）
- 策划可以独立填 Excel 表格（再也不用怕 Unreal Editor）
- 导出导入一键完成（成百个资产瞬间生成）
- 增量更新智能识别（不用怕浪费时间）
- 结构变更自动适配（没有代沟）

**最后的最后：** 如果这个文档对你有帮助，或者你有什么建议和问题，欢迎反馈！你的反馈会让这个插件变得更好。

**现在，去享受 GAS 配置的快乐吧！** 🎮

---

*本文档最后更新于：2026 年初。插件还在不断演进中，敬请期待更多功能！*
