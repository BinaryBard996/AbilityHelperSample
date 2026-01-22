# Excel 设计与结构体变更同步方案

本文讨论两件事：
1) Excel 建模：FGEModifierConfig 拆子表 vs 主表固定列；
2) C++ 结构体变更后，如何让 Python 导表逻辑“零或低改动”同步，是否可以用 UE 反射自动生成导表数据结构参考。

---

## 一、FGEModifierConfig：子表 vs 主表固定列（5项）

对比两种方式：

- 子表（推荐）
  - 优点
    - 自然映射一对多（Modifiers 无限扩展），无“列爆炸”和稀疏空列问题；
    - 结构清晰，易于兼容未来新增字段/嵌套（比如给 Modifier 增加曲线、条件等）；
    - 导出规则简单：按 ParentName 分组聚合为数组；
    - 与DataTable/UStruct一致性更好（数组=子表）。
  - 缺点
    - 编辑者需要切换Sheet，存在跨表关联（ParentName）的门槛；
    - 需要校验子表“孤儿行”、拼写错误等数据质量问题。

- 主表固定列（最多5项）
  - 优点
    - 单表视图直观，筛选/排序集中，非技术人员易上手；
    - 无跨表关联，减少基础数据校验项。
  - 缺点
    - 上限受限（超5需扩表），后期变更成本高；
    - 列爆炸与稀疏（Modifier1..5的每个字段都要成组出现，易出现空洞与顺序歧义）；
    - 可读性差（列过多），且导出需复杂压缩与跳过空列的逻辑。

结论：
- 若Modifiers数量上限不确定或随设计演进可能增减，强烈推荐“子表”方案；
- 若团队严格约束“最多5个”且强调单表操作，可提供“主表展开”为便民视图，但建议把“子表方案”作为标准通道，主表仅作导入前的临时视图（由工具自动在两种形态间互转）。

---

## 二、结构体变更后与Python同步：反射驱动（推荐）

痛点：
- 直接在Python里硬编码字段名/类型/枚举值，任何C++改动都需同步改Python，易错且维护成本高。

思路：反射驱动 + 中间 Schema
1) 反射收集（在Unreal Editor Python中执行）
   - 通过 UE 反射获取 UStruct（如 FGameplayEffectConfig）、其字段（名称、类型、是否数组、是否枚举、默认值、元数据等）；
   - 若字段是嵌套UStruct（如 FGEModifierConfig），递归采集；
   - 若字段是枚举，收集枚举项字符串；
   - 生成一份 JSON Schema（自定义的轻量Schema），描述结构、数组、枚举与元数据提示。
2) 基于 Schema 生成 Excel 模板
   - 规则：基础字段 -> 主表列；TArray<Struct> -> 子表（用 ParentName 外键关联）；TArray<基础类型> -> 子表或以分隔符列表示（建议子表，扩展性更好）；
   - 枚举：在模板第二行写入“可选值提示”；
   - 特殊类型（FGameplayTagContainer / FGameplayAttribute）按“约定序列化形态”生成列说明与示例。
3) 基于 Schema 导出 Json
   - 按 Schema 定义读取主表及子表，组装成和 UStruct 可反序列化的 Json 对象数组；
   - 不需要在代码里硬编码字段名/数量，结构体增删字段，只需重生成 Schema & 模板即可。

特殊类型推荐约定：
- FGameplayTagContainer：Excel里用逗号/分号分隔的一列（如 AssetTags），导出时转为
  - { "GameplayTags": [{"TagName":"A.B"}...], "ParentTags":[...自动补全...] }
- FGameplayAttribute：Excel里用完整路径“/Script/Module.Class:Property”，导出时展开为
  - { "AttributeName": "Property", "Attribute": "/Script/Module.Class:Property", "AttributeOwner": "/Script/CoreUObject.Class'/Script/Module.Class'" }

元数据可选规范（写在UPROPERTY的meta中，用于影响模板与导出）：
- ExcelName="列名别名"
- ExcelIgnore="true"（跳过导出/模板）
- ExcelHint="在模板第二行显示的提示"
- ExcelSheet="Modifiers"（强制放入指定子表）
- ExcelSeparator=","（基础类型数组如需扁平列时的分隔符）

版本与溯源：
- 在 Schema 顶层记录：生成时间、引擎版本、模块名、Struct全名、字段签名哈希（字段名+类型拼接后哈希），便于模板/导出时校验“导出时Schema是否落后于C++”。

---

## 三、反射驱动工作流（建议落地流程）

- 步骤0：编译C++，确保编辑器加载最新模块与UStruct。
- 步骤1：生成Schema
  - 在编辑器Python控制台运行：`reflect_struct_to_schema("/Script/YourModule.GameplayEffectConfig", "C:/Data/GE_Config.schema.json")`
  - 脚本基于反射输出JSON Schema（包含字段类型、数组、枚举项、嵌套结构等）
- 步骤2：生成Excel模板
  - `generate_excel_from_schema("C:/Data/GE_Config.schema.json", "C:/Data/GameplayEffectData.xlsx")`
- 步骤3：Excel填表
  - 设计/策划按模板填写数据（主表+子表）。
- 步骤4：导出Json
  - `export_from_excel_using_schema("C:/Data/GameplayEffectData.xlsx", "C:/Data/GameplayEffectData.json", "C:/Data/GE_Config.schema.json")`
- 步骤5：运行期使用
  - Json通过 FJsonObjectConverter 直接反序列化为 UStruct 数组，或作为DataTable源进行导入。

这样，当你更新 `FGameplayEffectConfig` / `FGEModifierConfig`：
- 只需重新执行“步骤1 生成Schema”，模板与导出逻辑自动适配（无需改Python代码）；
- 如有字段需要特殊序列化规则（如新增自定义类型），在“类型适配表”里新增一条即可，反射仍是主驱动。

---

## 四、实现要点（摘要）

- 反射采集
  - 通过UE Python API获取 UStruct 与其字段（名称、类型、IsArray、Enum值、Default等）；
  - 嵌套与数组：递归，使用 `declaring_struct` 与 `inner` 类型区分；
  - 枚举：读出枚举名与DisplayName（若需要）。
- Schema结构（示例）
  ```json
  {
    "struct": "/Script/YourModule.GameplayEffectConfig",
    "hash": "....",
    "fields": [
      {"name":"DurationType","kind":"enum","enum": ["Instant","Duration","Infinite","HasDuration"]},
      {"name":"Modifiers","kind":"array","inner_struct":"/Script/YourModule.GEModifierConfig"},
      ...
    ],
    "special_rules": {
      "FGameplayTagContainer": "tag_container_rule",
      "FGameplayAttribute": "attribute_rule"
    }
  }
  ```
- 模板生成
  - 主表=所有非数组基础字段；
  - 子表=每个数组字段一个Sheet，带 `ParentName` 外键；
  - 第二行写入ENUM候选与ExcelHint。
- 导出
  - 简单字段：直抄并做基本类型转换；
  - 枚举：按字符串导出（与引擎枚举的名字对齐）；
  - Tags/Attribute：应用“特殊规则”转换。

---

## 五、选择建议（TL;DR）

- 选择：子表方案更稳健、可扩展，适合长期演进；主表展开仅在强约束“最多5个”且团队偏好单表时作为便民视图（可提供工具在两种形态间自动转换）。
- 同步：采用“反射驱动+Schema”的工具链，让C++结构变更后“重生成Schema”即可同步到Excel模板与导表逻辑，Python代码无需跟着改字段清单。
