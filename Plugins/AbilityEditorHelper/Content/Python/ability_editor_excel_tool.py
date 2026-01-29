# AbilityEditorExcelTools.py
# 工具功能：
# Schema驱动工作流（从 Schema 目录读取 <StructName>.schema.json 生成模板并导出）
# 1) 基于Schema生成Excel/CSV模板（支持嵌套结构、数组、枚举等）
# 2) 基于Schema从Excel/CSV导出为可被FJsonObjectConverter反序列化的Json数组
# 3) 若未安装openpyxl，自动回退为CSV模板/导出

import json
import os
import sys
from typing import Optional

try:
    import openpyxl  # type: ignore
    OPENPYXL_AVAILABLE = True
except Exception:
    OPENPYXL_AVAILABLE = False

# Unreal 相关（在编辑器Python中运行）
try:
    import unreal  # type: ignore
    UE_AVAILABLE = True
except Exception:
    UE_AVAILABLE = False

def _ensure_dir(path: str):
    d = os.path.dirname(path)
    if d and not os.path.exists(d):
        os.makedirs(d, exist_ok=True)

# =========================
# Schema 驱动（新增）
# =========================

def _log(msg: str):
    if UE_AVAILABLE:
        unreal.log(msg)
    else:
        print(msg)

def _warn(msg: str):
    if UE_AVAILABLE:
        unreal.log_warning(msg)
    else:
        print(f"[WARN] {msg}")

def _schema_dir_default() -> str:
    # 插件 Content/Python/Schema
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(here, "Schema")

def _struct_name_from_struct_path(struct_path: str) -> str:
    # /Script/Module.StructName -> StructName
    if not struct_path:
        return ""
    if "." in struct_path:
        return struct_path.split(".")[-1].strip()
    return struct_path.strip()

def _load_schema(schema_name_or_path: str, schema_dir: Optional[str] = None) -> dict:
    if not schema_name_or_path:
        raise ValueError("schema_name_or_path 为空")

    schema_dir = schema_dir or _schema_dir_default()

    if os.path.exists(schema_name_or_path) and schema_name_or_path.lower().endswith(".json"):
        schema_path = schema_name_or_path
    else:
        # 允许传入 StructName（如 GameplayEffectConfig）
        base = schema_name_or_path
        if base.lower().endswith(".schema.json"):
            file_name = base
        else:
            file_name = f"{base}.schema.json"
        schema_path = os.path.join(schema_dir, file_name)

    if not os.path.exists(schema_path):
        raise FileNotFoundError(f"Schema 文件不存在：{schema_path}")

    with open(schema_path, "r", encoding="utf-8") as f:
        return json.load(f)

def _get_special_rule(schema: dict, type_name_or_path: str) -> str:
    """
    schema["specialRules"] 里键是短名（例如 GameplayTagContainer / GameplayAttribute）
    """
    rules = schema.get("specialRules") or {}
    key = type_name_or_path
    if "/" in key and "." in key:
        # /Script/GameplayTags.GameplayTagContainer -> GameplayTagContainer
        key = _struct_name_from_struct_path(key)
    return rules.get(key, "")

def _excel_col_name(field: dict) -> str:
    # 优先 ExcelName，否则用字段名
    n = (field.get("excelName") or "").strip()
    if n:
        return n
    return str(field.get("name") or "").strip()

def _field_hint(schema: dict, field: dict) -> str:
    # 优先 ExcelHint；枚举则提示 enumValues；特殊类型给约定提示
    if field.get("bExcelIgnore"):
        return ""

    excel_hint = (field.get("excelHint") or "").strip()
    if excel_hint:
        return excel_hint

    kind = (field.get("kind") or "").strip()
    if kind == "enum":
        vals = field.get("enumValues") or []
        if vals:
            return " | ".join([str(v) for v in vals])

    if kind == "struct":
        struct_path = field.get("structPath") or ""
        rule = _get_special_rule(schema, struct_path)
        if rule == "tag_container_rule":
            return "用逗号/分号分隔：TagA, TagB.Sub"
        if rule == "attribute_rule":
            return "格式：/Script/Module.Class:Property"

    return ""

def _schema_fields(schema: dict) -> list:
    return schema.get("fields") or []

def _schema_struct_name(schema: dict) -> str:
    return _struct_name_from_struct_path(schema.get("structPath") or "")

def _write_xlsx_template_from_schema(schema: dict, out_xlsx_path: str, schema_dir: Optional[str] = None):
    if not OPENPYXL_AVAILABLE:
        raise RuntimeError("未安装openpyxl，无法生成xlsx。请安装后重试，或使用CSV回退。")

    schema_dir = schema_dir or _schema_dir_default()
    struct_name = _schema_struct_name(schema)
    main_sheet_name = struct_name if struct_name else "Main"

    wb = openpyxl.Workbook()
    ws_main = wb.active
    ws_main.title = main_sheet_name

    # 主表：Name + 非数组字段
    main_fields = [{"name": "Name", "kind": "string"}]  # DataTable行名/主键
    for f in _schema_fields(schema):
        if f.get("bExcelIgnore"):
            continue
        if (f.get("kind") or "") == "array":
            continue
        main_fields.append(f)

    headers = [_excel_col_name(f) for f in main_fields]
    ws_main.append(headers)
    ws_main.append([_field_hint(schema, f) for f in main_fields])

    # 子表：每个 array<struct> 一个Sheet
    for f in _schema_fields(schema):
        if f.get("bExcelIgnore"):
            continue
        if (f.get("kind") or "") != "array":
            continue

        inner_kind = (f.get("innerKind") or "").strip()
        if inner_kind != "struct":
            _warn(f"[ExcelTools][Schema] 暂不支持 array<{inner_kind}> 字段：{f.get('name')}")
            continue

        inner_struct_path = (f.get("innerStructPath") or "").strip()
        inner_struct_name = _struct_name_from_struct_path(inner_struct_path)
        if not inner_struct_name:
            _warn(f"[ExcelTools][Schema] 子表字段缺少 innerStructPath：{f.get('name')}")
            continue

        inner_schema = _load_schema(inner_struct_name, schema_dir=schema_dir)

        sub_sheet_name = f"{main_sheet_name}.{str(f.get('name') or '').strip()}"
        ws_sub = wb.create_sheet(sub_sheet_name)

        sub_fields = [{"name": "ParentName", "kind": "string"}]
        for sf in _schema_fields(inner_schema):
            if sf.get("bExcelIgnore"):
                continue
            if (sf.get("kind") or "") == "array":
                _warn(f"[ExcelTools][Schema] 子结构体内的数组字段暂不支持：{inner_struct_name}.{sf.get('name')}")
                continue
            sub_fields.append(sf)

        ws_sub.append([_excel_col_name(x) for x in sub_fields])
        ws_sub.append([_field_hint(inner_schema, x) for x in sub_fields])

    _ensure_dir(out_xlsx_path)
    wb.save(out_xlsx_path)

def _write_csv_template_from_schema(schema: dict, out_dir_or_file: str, schema_dir: Optional[str] = None):
    """
    CSV 回退策略：
      - 主表：<StructName>.csv
      - 子表：<StructName>.<ArrayFieldName>.csv
    """
    import csv

    schema_dir = schema_dir or _schema_dir_default()
    struct_name = _schema_struct_name(schema)
    main_sheet_name = struct_name if struct_name else "Main"

    # 输出目录判定
    out_dir = out_dir_or_file
    if out_dir_or_file.lower().endswith(".xlsx") or out_dir_or_file.lower().endswith(".csv"):
        out_dir = os.path.dirname(out_dir_or_file) or "."

    os.makedirs(out_dir, exist_ok=True)

    # 主表
    main_fields = [{"name": "Name", "kind": "string"}]
    for f in _schema_fields(schema):
        if f.get("bExcelIgnore"):
            continue
        if (f.get("kind") or "") == "array":
            continue
        main_fields.append(f)

    main_csv = os.path.join(out_dir, f"{main_sheet_name}.csv")
    with open(main_csv, "w", newline="", encoding="utf-8") as fp:
        w = csv.writer(fp)
        w.writerow([_excel_col_name(f) for f in main_fields])
        w.writerow([_field_hint(schema, f) for f in main_fields])

    # 子表
    for f in _schema_fields(schema):
        if f.get("bExcelIgnore"):
            continue
        if (f.get("kind") or "") != "array":
            continue
        inner_kind = (f.get("innerKind") or "").strip()
        if inner_kind != "struct":
            continue

        inner_struct_name = _struct_name_from_struct_path((f.get("innerStructPath") or "").strip())
        if not inner_struct_name:
            continue
        inner_schema = _load_schema(inner_struct_name, schema_dir=schema_dir)

        sub_fields = [{"name": "ParentName", "kind": "string"}]
        for sf in _schema_fields(inner_schema):
            if sf.get("bExcelIgnore"):
                continue
            if (sf.get("kind") or "") == "array":
                continue
            sub_fields.append(sf)

        sub_csv = os.path.join(out_dir, f"{main_sheet_name}.{str(f.get('name') or '').strip()}.csv")
        with open(sub_csv, "w", newline="", encoding="utf-8") as fp:
            w = csv.writer(fp)
            w.writerow([_excel_col_name(x) for x in sub_fields])
            w.writerow([_field_hint(inner_schema, x) for x in sub_fields])

def generate_excel_template_from_schema(schema_name_or_path: str, out_path: str, schema_dir: Optional[str] = None):
    """
    新入口：基于 Schema 生成 Excel/CSV 模板
    - schema_name_or_path：
        - 结构体名（如 GameplayEffectConfig），会在 Schema 目录下查找 GameplayEffectConfig.schema.json
        - 或直接传入 Schema 的绝对/相对路径
    - out_path：
        - .xlsx 且 openpyxl 可用：输出 xlsx
        - 否则：输出 CSV 到 out_path 所在目录（或 out_path 本身作为目录）
    """
    schema = _load_schema(schema_name_or_path, schema_dir=schema_dir)

    if out_path.lower().endswith(".xlsx") and OPENPYXL_AVAILABLE:
        _write_xlsx_template_from_schema(schema, out_path, schema_dir=schema_dir)
        _log(f"[ExcelTools][Schema] 模板已生成：{out_path}")
    else:
        _write_csv_template_from_schema(schema, out_path, schema_dir=schema_dir)
        _log(f"[ExcelTools][Schema] 未检测到openpyxl或非xlsx输出，已生成CSV模板到目录：{os.path.dirname(out_path) or out_path}")

def _safe_bool(v, default=False):
    if v is None or v == "":
        return default
    if isinstance(v, bool):
        return v
    s = str(v).strip().lower()
    if s in ("1", "true", "yes", "y", "t", "on"):
        return True
    if s in ("0", "false", "no", "n", "f", "off"):
        return False
    return default

def _to_scalar_from_cell(schema: dict, field: dict, value):
    kind = (field.get("kind") or "").strip()

    if kind == "bool":
        return _safe_bool(value, False)
    if kind == "int":
        return _safe_num(value, 0)
    if kind in ("float", "double"):
        return float(_safe_num(value, 0))
    if kind in ("string", "name", "text"):
        return _safe_str(value, "")
    if kind == "enum":
        return _safe_str(value, "")
    if kind == "struct":
        struct_path = (field.get("structPath") or "").strip()
        rule = _get_special_rule(schema, struct_path)
        if rule == "tag_container_rule":
            return _to_tag_container_obj(_split_tag_string(value))
        if rule == "attribute_rule":
            s = _safe_str(value, "")
            return _parse_attribute_cell(s) if s else {}
        if rule == "tag_requirements_rule":
            return _parse_tag_requirements(value)
        # 通用struct：允许用户直接填一个JSON对象字符串
        s = _safe_str(value, "")
        if s.startswith("{") and s.endswith("}"):
            try:
                return json.loads(s)
            except Exception:
                return {}
        return {}
    return value

def _sheet_to_dict_list(ws, skip_second_row_hint: bool = True):
    rows = list(ws.rows)
    if not rows:
        return []
    headers = [str(c.value).strip() if c.value is not None else "" for c in rows[0]]
    data = []
    start_idx = 1
    # 跳过第二行枚举提示
    if skip_second_row_hint and len(rows) >= 2:
        start_idx = 2
    for r in rows[start_idx:]:
        # 若整行为空则跳过
        if all((c.value is None or str(c.value).strip() == "") for c in r):
            continue
        row_map = {}
        for i, cell in enumerate(r):
            key = headers[i] if i < len(headers) else f"COL_{i}"
            row_map[key] = cell.value
        data.append(row_map)
    return data

def _read_xlsx_sheet_map(xlsx_path: str) -> dict:
    if not OPENPYXL_AVAILABLE:
        raise RuntimeError("未安装openpyxl，无法读取xlsx。请安装后重试，或使用CSV回退。")
    wb = openpyxl.load_workbook(xlsx_path, data_only=True)
    out = {}
    for name in wb.sheetnames:
        ws = wb[name]
        out[name] = _sheet_to_dict_list(ws, skip_second_row_hint=True)
    return out

def _read_csv_table(csv_path: str) -> list:
    import csv
    rows = []
    with open(csv_path, "r", encoding="utf-8") as f:
        reader = csv.reader(f)
        headers = None
        for i, line in enumerate(reader):
            if i == 0:
                headers = [x.strip() for x in line]
                continue
            # 跳过第二行枚举提示
            if i == 1:
                continue
            if not any((x or "").strip() for x in line):
                continue
            row = {}
            for idx, v in enumerate(line):
                key = headers[idx] if idx < len(headers) else f"COL_{idx}"
                row[key] = v
            rows.append(row)
    return rows

def _read_csv_sheet_map(base_dir_or_csv: str, main_sheet_name: str, array_field_names: list) -> dict:
    base_dir = base_dir_or_csv
    if base_dir_or_csv.lower().endswith(".csv"):
        base_dir = os.path.dirname(base_dir_or_csv) or "."

    out = {}

    main_csv = os.path.join(base_dir, f"{main_sheet_name}.csv")
    if not os.path.exists(main_csv):
        raise FileNotFoundError(f"主表CSV不存在：{main_csv}")
    out[main_sheet_name] = _read_csv_table(main_csv)

    for af in array_field_names:
        sub_name = f"{main_sheet_name}.{af}"
        sub_csv = os.path.join(base_dir, f"{sub_name}.csv")
        if os.path.exists(sub_csv):
            out[sub_name] = _read_csv_table(sub_csv)
        else:
            out[sub_name] = []

    return out

def export_excel_to_json_using_schema(in_path: str, out_json_path: str, schema_name_or_path: str, schema_dir: Optional[str] = None):
    """
    新入口：用 Schema 驱动导出
    - in_path：
        - xlsx 文件
        - 或 CSV 目录/任一csv文件路径
    - out_json_path：输出 json（对象数组）
    - schema_name_or_path：同 generate_excel_template_from_schema
    """
    schema_dir = schema_dir or _schema_dir_default()
    schema = _load_schema(schema_name_or_path, schema_dir=schema_dir)

    struct_name = _schema_struct_name(schema)
    main_sheet_name = struct_name if struct_name else "Main"

    # 收集 array<struct> 字段
    array_fields = []
    for f in _schema_fields(schema):
        if f.get("bExcelIgnore"):
            continue
        if (f.get("kind") or "") != "array":
            continue
        if (f.get("innerKind") or "").strip() != "struct":
            continue
        array_fields.append(f)

    if in_path.lower().endswith(".xlsx"):
        sheet_map = _read_xlsx_sheet_map(in_path)
    else:
        sheet_map = _read_csv_sheet_map(
            in_path,
            main_sheet_name=main_sheet_name,
            array_field_names=[str(f.get("name") or "").strip() for f in array_fields],
        )

    main_rows = sheet_map.get(main_sheet_name) or []

    # 子表按 ParentName 聚合
    child_rows_map = {}  # arrayFieldName -> { ParentName -> [rows...] }
    for af in array_fields:
        af_name = str(af.get("name") or "").strip()
        sub_sheet = f"{main_sheet_name}.{af_name}"
        sub_rows = sheet_map.get(sub_sheet) or []
        by_parent = {}
        for r in sub_rows:
            p = _safe_str(r.get("ParentName"), "")
            if not p:
                continue
            by_parent.setdefault(p, []).append(r)
        child_rows_map[af_name] = by_parent

    # 主表字段：Name + 非数组字段
    main_fields = [{"name": "Name", "kind": "string"}]
    for f in _schema_fields(schema):
        if f.get("bExcelIgnore"):
            continue
        if (f.get("kind") or "") == "array":
            continue
        main_fields.append(f)

    result = []
    for r in main_rows:
        name = _safe_str(r.get("Name"), "")
        if not name or name == "Name":
            continue

        item = {"Name": name}

        # 写入非数组字段
        for f in main_fields:
            fn = str(f.get("name") or "").strip()
            if fn == "Name":
                continue
            col = _excel_col_name(f)
            cell_val = r.get(col)
            item[fn] = _to_scalar_from_cell(schema, f, cell_val)

        # 写入 array<struct> 子表
        for af in array_fields:
            af_name = str(af.get("name") or "").strip()
            inner_struct_name = _struct_name_from_struct_path((af.get("innerStructPath") or "").strip())
            if not inner_struct_name:
                item[af_name] = []
                continue

            inner_schema = _load_schema(inner_struct_name, schema_dir=schema_dir)
            rows_for_this = (child_rows_map.get(af_name) or {}).get(name) or []
            arr = []
            for cr in rows_for_this:
                child_obj = {}
                for sf in _schema_fields(inner_schema):
                    if sf.get("bExcelIgnore"):
                        continue
                    if (sf.get("kind") or "") == "array":
                        continue
                    sf_name = str(sf.get("name") or "").strip()
                    col = _excel_col_name(sf)
                    child_obj[sf_name] = _to_scalar_from_cell(inner_schema, sf, cr.get(col))
                arr.append(child_obj)
            item[af_name] = arr

        result.append(item)

    _ensure_dir(out_json_path)
    with open(out_json_path, "w", encoding="utf-8") as f:
        json.dump(result, f, ensure_ascii=False, indent=4)

    _log(f"[ExcelTools][Schema] 导出完成：{out_json_path}（共{len(result)}条）")

def _split_tag_string(s) -> list:
    if not s:
        return []
    if isinstance(s, str):
        raw = s
    else:
        raw = str(s)
    tags = [t.strip() for t in raw.replace(";", ",").split(",") if t.strip()]
    # 去重保持顺序
    seen = set()
    result = []
    for t in tags:
        if t not in seen:
            seen.add(t)
            result.append(t)
    return result

def _compute_parent_tags(tag: str) -> list:
    # GameplayCue.Test.A -> ["GameplayCue", "GameplayCue.Test"]
    parts = tag.split(".")
    if len(parts) <= 1:
        return []
    parents = []
    for i in range(1, len(parts)):
        parents.append(".".join(parts[:i]))
    return parents

def _to_tag_container_obj(tag_list: list) -> dict:
    gameplay_tags = [{"TagName": t} for t in tag_list]
    parent_set = set()
    for t in tag_list:
        for p in _compute_parent_tags(t):
            parent_set.add(p)
    parent_tags = [{"TagName": p} for p in sorted(parent_set)]
    return {"GameplayTags": gameplay_tags, "ParentTags": parent_tags}

def _parse_attribute_cell(cell_value: str) -> dict:
    """
    期望格式：
      - 完整：/Script/GameplayAbilities.AbilitySystemComponent:OutgoingDuration
      - 或 /Script/Module.Class:Property
    输出：
      {
        "AttributeName": "OutgoingDuration",
        "Attribute": "/Script/GameplayAbilities.AbilitySystemComponent:OutgoingDuration",
        "AttributeOwner": "/Script/CoreUObject.Class'/Script/GameplayAbilities.AbilitySystemComponent'"
      }
    """
    if not cell_value:
        return {}
    s = str(cell_value).strip()
    if ":" not in s:
        raise ValueError(f"Attribute列需使用 /Script/Module.Class:Property 格式，当前：{s}")
    class_path, prop_name = s.split(":", 1)
    class_path = class_path.strip()
    prop_name = prop_name.strip()
    # AttributeOwner 形如：/Script/CoreUObject.Class'/Script/GameplayAbilities.AbilitySystemComponent'
    owner_obj = f"/Script/CoreUObject.Class'{class_path}'"
    return {
        "AttributeName": prop_name,
        "Attribute": f"{class_path}:{prop_name}",
        "AttributeOwner": owner_obj
    }

def _parse_tag_requirements(cell_value) -> dict:
    """
    解析 Tag Requirements 的行内格式
    格式：  "Require:Tag.A,Tag.B|Ignore:Tag.C,Tag.D"
    输出：
      {
        "RequireTags": {"GameplayTags": [...], "ParentTags": [...]},
        "IgnoreTags": {"GameplayTags": [...], "ParentTags": [...]}
      }
    """
    s = _safe_str(cell_value, "")
    if not s:
        return {
            "RequireTags": _to_tag_container_obj([]),
            "IgnoreTags": _to_tag_container_obj([])
        }

    require = []
    ignore = []

    if "|" in s:
        parts = s.split("|")
        for part in parts:
            part = part.strip()
            if part.startswith("Require:"):
                require = _split_tag_string(part[8:])
            elif part.startswith("Ignore:"):
                ignore = _split_tag_string(part[7:])
    else:
        # 回退：将整个字符串视为 RequireTags
        require = _split_tag_string(s)

    return {
        "RequireTags": _to_tag_container_obj(require),
        "IgnoreTags": _to_tag_container_obj(ignore)
    }

def _parse_asset_path(cell_value) -> str:
    """
    规范化 UE 资产路径
    接受：
      - 完整路径：/Game/Effects/GE_Base.GE_Base
      - 包路径：/Game/Effects/GE_Base
      - 相对路径：Effects/GE_Base
    返回：规范化的资产路径字符串
    """
    s = _safe_str(cell_value, "")
    if not s:
        return ""

    # 如果不以 / 开头，规范化为 /Game/
    if not s.startswith("/"):
        s = "/Game/" + s

    # 如果只是包路径（没有 .），添加资产名称
    if "." not in s:
        asset_name = s.split("/")[-1]
        s = f"{s}.{asset_name}"

    return s

def _safe_num(v, default=0):
    if v is None or v == "":
        return default
    try:
        if isinstance(v, (int, float)):
            return v
        s = str(v).strip()
        if "." in s:
            return float(s)
        return int(s)
    except Exception:
        return default

def _safe_str(v, default=""):
    if v is None:
        return default
    return str(v).strip()

if __name__ == "__main__":
    # 命令行用法（在外部Python环境下也可调试）
    # 1) Schema 模板：python ability_editor_excel_tool.py schema_template <schema> <输出路径/目录> [--schema-dir DIR]
    # 2) Schema 导出：python ability_editor_excel_tool.py schema_export <schema> <输入xlsx|csv> <输出json> [--schema-dir DIR]
    import argparse
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="cmd")

    p0 = sub.add_parser("schema_template")
    p0.add_argument("schema", help="Schema名称(如 GameplayEffectConfig) 或 .schema.json 路径")
    p0.add_argument("out", help="输出.xlsx路径（无openpyxl时输出CSV到目录）")
    p0.add_argument("--schema-dir", default="", help="Schema目录（默认插件Content/Python/Schema）")

    p00 = sub.add_parser("schema_export")
    p00.add_argument("schema", help="Schema名称(如 GameplayEffectConfig) 或 .schema.json 路径")
    p00.add_argument("inp", help=".xlsx文件路径 或 CSV目录/任一csv路径")
    p00.add_argument("out", help="输出.json路径")
    p00.add_argument("--schema-dir", default="", help="Schema目录（默认插件Content/Python/Schema）")

    args = parser.parse_args()
    if args.cmd == "schema_template":
        sd = args.schema_dir.strip() or None
        generate_excel_template_from_schema(args.schema, args.out, schema_dir=sd)
    elif args.cmd == "schema_export":
        sd = args.schema_dir.strip() or None
        export_excel_to_json_using_schema(args.inp, args.out, args.schema, schema_dir=sd)
    else:
        print("用法：\n"
              "  schema_template <schema_name|schema_path> <out.xlsx|out_dir> [--schema-dir <dir>]\n"
              "  schema_export   <schema_name|schema_path> <in.xlsx|csv_dir> <out.json> [--schema-dir <dir>]")
