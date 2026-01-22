# AbilityEditorExcelTools.py
# 工具功能：
# 1) 生成FGameplayEffectConfig结构对应的Excel模板（支持：子表方案 或 平铺最多5项）
# 2) 从Excel/CSV导出为可被FJsonObjectConverter反序列化为FGameplayEffectConfig的Json数组
# 3) 若未安装openpyxl，自动回退为CSV模板/导出（子表=双CSV，平铺=单CSV）
# 4) 新增：Schema驱动工作流（从 Schema 目录读取 <StructName>.schema.json 生成模板并导出）

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

from ability_editor_consts import (
    STRUCT_DISPLAY_NAME,
    MAIN_SHEET_NAME,
    SUB_SHEET_NAME,
    MAIN_FIELDS,
    SUB_FIELDS,
    MAX_FLAT_MODIFIERS,
    ENUM_HINTS,
    SAMPLE_MAIN_ROWS,
    SAMPLE_SUB_ROWS,
)

def _build_flat_fields():
    flat = list(MAIN_FIELDS)
    for i in range(1, MAX_FLAT_MODIFIERS + 1):
        flat += [
            f"Attribute{i}",
            f"ModifierOp{i}",
            f"MagnitudeCalculationType{i}",
            f"Magnitude{i}",
        ]
    return flat

def _build_sample_flat_row(sample_main: dict, sample_sub_for_this_name: list):
    # 把sample子表的前N个填入平铺列
    row = dict(sample_main)
    for i in range(1, MAX_FLAT_MODIFIERS + 1):
        row[f"Attribute{i}"] = ""
        row[f"ModifierOp{i}"] = ""
        row[f"MagnitudeCalculationType{i}"] = ""
        row[f"Magnitude{i}"] = ""
    for idx, sub in enumerate(sample_sub_for_this_name[:MAX_FLAT_MODIFIERS], start=1):
        row[f"Attribute{idx}"] = sub.get("Attribute", "")
        row[f"ModifierOp{idx}"] = sub.get("ModifierOp", "")
        row[f"MagnitudeCalculationType{idx}"] = sub.get("MagnitudeCalculationType", "")
        row[f"Magnitude{idx}"] = sub.get("Magnitude", "")
    return row

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

# =========================
# 旧版（面向固定 FGameplayEffectConfig）仍保留
# =========================

def _write_xlsx_template_sub(xlsx_path: str):
    wb = openpyxl.Workbook()
    # 主表
    ws_main = wb.active
    ws_main.title = MAIN_SHEET_NAME
    ws_main.append(MAIN_FIELDS)
    # 第二行写入枚举提示
    hint_row = []
    for f in MAIN_FIELDS:
        hint_row.append(ENUM_HINTS.get(f, ""))
    ws_main.append(hint_row)
    # 示例数据
    for row in SAMPLE_MAIN_ROWS:
        ws_main.append([row.get(k, "") for k in MAIN_FIELDS])

    # 子表
    ws_sub = wb.create_sheet(SUB_SHEET_NAME)
    ws_sub.append(SUB_FIELDS)
    hint_row = []
    for f in SUB_FIELDS:
        hint_row.append(ENUM_HINTS.get(f, ""))
    ws_sub.append(hint_row)
    for row in SAMPLE_SUB_ROWS:
        ws_sub.append([row.get(k, "") for k in SUB_FIELDS])

    _ensure_dir(xlsx_path)
    wb.save(xlsx_path)

def _write_xlsx_template_flat(xlsx_path: str):
    wb = openpyxl.Workbook()
    ws = wb.active
    ws.title = MAIN_SHEET_NAME
    flat_fields = _build_flat_fields()
    ws.append(flat_fields)
    # 第二行提示
    hints = []
    for f in flat_fields:
        if f in MAIN_FIELDS:
            hints.append(ENUM_HINTS.get(f, ""))
        elif f.startswith("ModifierOp"):
            hints.append(ENUM_HINTS.get("ModifierOp", ""))
        elif f.startswith("MagnitudeCalculationType"):
            hints.append(ENUM_HINTS.get("MagnitudeCalculationType", ""))
        else:
            hints.append("")
    ws.append(hints)
    # 示例：将Test1的两个Modifier平铺
    name_to_sub = {}
    for s in SAMPLE_SUB_ROWS:
        name_to_sub.setdefault(s["ParentName"], []).append(s)
    for main in SAMPLE_MAIN_ROWS:
        subs = name_to_sub.get(main["Name"], [])
        row = _build_sample_flat_row(main, subs)
        ws.append([row.get(k, "") for k in flat_fields])

    _ensure_dir(xlsx_path)
    wb.save(xlsx_path)

def _write_csv_template_sub(csv_main_path: str, csv_sub_path: str):
    import csv
    _ensure_dir(csv_main_path)
    _ensure_dir(csv_sub_path)
    with open(csv_main_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(MAIN_FIELDS)
        w.writerow([ENUM_HINTS.get(x, "") for x in MAIN_FIELDS])
        for row in SAMPLE_MAIN_ROWS:
            w.writerow([row.get(k, "") for k in MAIN_FIELDS])

    with open(csv_sub_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(SUB_FIELDS)
        w.writerow([ENUM_HINTS.get(x, "") for x in SUB_FIELDS])
        for row in SAMPLE_SUB_ROWS:
            w.writerow([row.get(k, "") for k in SUB_FIELDS])

def _write_csv_template_flat(csv_flat_path: str):
    import csv
    _ensure_dir(csv_flat_path)
    flat_fields = _build_flat_fields()
    with open(csv_flat_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(flat_fields)
        hints = []
        for f in flat_fields:
            if f in MAIN_FIELDS:
                hints.append(ENUM_HINTS.get(f, ""))
            elif f.startswith("ModifierOp"):
                hints.append(ENUM_HINTS.get("ModifierOp", ""))
            elif f.startswith("MagnitudeCalculationType"):
                hints.append(ENUM_HINTS.get("MagnitudeCalculationType", ""))
            else:
                hints.append("")
        w.writerow(hints)
        name_to_sub = {}
        for s in SAMPLE_SUB_ROWS:
            name_to_sub.setdefault(s["ParentName"], []).append(s)
        for main in SAMPLE_MAIN_ROWS:
            subs = name_to_sub.get(main["Name"], [])
            row = _build_sample_flat_row(main, subs)
            w.writerow([row.get(k, "") for k in flat_fields])

def generate_gameplay_effect_excel_template(out_path: str, use_sub_sheet: bool = True):
    """
    生成FGameplayEffectConfig的Excel模板。
    - use_sub_sheet=True：子表方案（Modifiers独立Sheet或双CSV）
    - use_sub_sheet=False：平铺方案（单Sheet或单CSV，最多5项）
    out_path:
      - 若以 .xlsx 结尾且安装openpyxl，则生成一个xlsx；
      - 否则：
        - 子表方案：生成 GameplayEffectData_Main.csv 与 GameplayEffectData_Modifiers.csv；
        - 平铺方案：生成 GameplayEffectData_Flat.csv。
    """
    if out_path.lower().endswith(".xlsx") and OPENPYXL_AVAILABLE:
        if use_sub_sheet:
            _write_xlsx_template_sub(out_path)
        else:
            _write_xlsx_template_flat(out_path)
        msg = f"[ExcelTools] 模板已生成：{out_path}（{'子表' if use_sub_sheet else '平铺'}）"
    else:
        base_dir = out_path
        if out_path.lower().endswith(".xlsx"):
            base_dir = os.path.dirname(out_path)
        if use_sub_sheet:
            csv_main = os.path.join(base_dir, "GameplayEffectData_Main.csv")
            csv_sub = os.path.join(base_dir, "GameplayEffectData_Modifiers.csv")
            _write_csv_template_sub(csv_main, csv_sub)
            msg = f"[ExcelTools] 未检测到openpyxl，已生成CSV模板（子表）：\n  {csv_main}\n  {csv_sub}"
        else:
            csv_flat = os.path.join(base_dir, "GameplayEffectData_Flat.csv")
            _write_csv_template_flat(csv_flat)
            msg = f"[ExcelTools] 未检测到openpyxl，已生成CSV模板（平铺）：\n  {csv_flat}"
    if UE_AVAILABLE:
        unreal.log(msg)
    else:
        print(msg)

def _read_xlsx_both(xlsx_path: str):
    wb = openpyxl.load_workbook(xlsx_path, data_only=True)
    is_sub = SUB_SHEET_NAME in wb.sheetnames
    ws_main = wb[MAIN_SHEET_NAME]
    main_rows = _sheet_to_dict_list(ws_main, skip_second_row_hint=True)
    if is_sub:
        ws_sub = wb[SUB_SHEET_NAME]
        sub_rows = _sheet_to_dict_list(ws_sub, skip_second_row_hint=True)
        return main_rows, sub_rows, "sub"
    else:
        return main_rows, None, "flat"

def _read_csv(csv_path: str):
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

def _collect_modifiers_from_flat_row(row: dict):
    mods = []
    for i in range(1, MAX_FLAT_MODIFIERS + 1):
        attr_cell = _safe_str(row.get(f"Attribute{i}"))
        if not attr_cell:
            continue
        attr_obj = _parse_attribute_cell(attr_cell)
        mods.append({
            "Attribute": attr_obj,
            "ModifierOp": _safe_str(row.get(f"ModifierOp{i}"), "AddBase"),
            "MagnitudeCalculationType": _safe_str(row.get(f"MagnitudeCalculationType{i}"), "ScalableFloat"),
            "Magnitude": _safe_num(row.get(f"Magnitude{i}"), 0),
        })
    return mods

def export_gameplay_effect_excel_to_json(in_path: str, out_json_path: str):
    """
    从Excel/CSV导出Json数组（GameplayEffectData）。
    支持：
      - .xlsx（需安装openpyxl，子表或平铺）
      - CSV：
         - 子表方案：GameplayEffectData_Main.csv + GameplayEffectData_Modifiers.csv
         - 平铺方案：GameplayEffectData_Flat.csv
    """
    scheme = None
    if in_path.lower().endswith(".xlsx"):
        if not OPENPYXL_AVAILABLE:
            raise RuntimeError("未安装openpyxl，无法读取xlsx。请安装后重试，或使用CSV回退。")
        main_rows, sub_rows, scheme = _read_xlsx_both(in_path)
    else:
        # 作为目录或任一csv路径输入时，尝试识别
        base_dir = in_path
        if in_path.lower().endswith(".csv"):
            base_dir = os.path.dirname(in_path)
        csv_main = os.path.join(base_dir, "GameplayEffectData_Main.csv")
        csv_sub  = os.path.join(base_dir, "GameplayEffectData_Modifiers.csv")
        csv_flat = os.path.join(base_dir, "GameplayEffectData_Flat.csv")
        if os.path.exists(csv_main) and os.path.exists(csv_sub):
            main_rows = _read_csv(csv_main)
            sub_rows  = _read_csv(csv_sub)
            scheme = "sub"
        elif os.path.exists(csv_flat):
            main_rows = _read_csv(csv_flat)
            sub_rows  = None
            scheme = "flat"
        else:
            raise FileNotFoundError(f"未找到CSV（子表：{csv_main}+{csv_sub}，或平铺：{csv_flat}）")
    # 组装
    result = []
    if scheme == "sub":
        # 组装子表：ParentName -> list
        modifiers_map = {}
        for r in sub_rows or []:
            parent = _safe_str(r.get("ParentName"))
            if not parent:
                continue
            modifiers_map.setdefault(parent, []).append(r)

        for r in main_rows:
            name = _safe_str(r.get("Name"))
            if not name or name == "Name":
                continue

            asset_tags = _split_tag_string(r.get("AssetTags"))
            granted_tags = _split_tag_string(r.get("GrantedTags"))

            item = {
                "Name": name,
                "DurationType": _safe_str(r.get("DurationType"), "Instant"),
                "DurationMagnitude": _safe_num(r.get("DurationMagnitude"), 0),
                "Period": _safe_num(r.get("Period"), 0),
                "StackingType": _safe_str(r.get("StackingType"), "None"),
                "StackLimitCount": _safe_num(r.get("StackLimitCount"), 1),
                "StackDurationRefreshPolicy": _safe_str(r.get("StackDurationRefreshPolicy"),
                                                       "RefreshOnSuccessfulApplication"),
                "StackPeriodResetPolicy": _safe_str(r.get("StackPeriodResetPolicy"),
                                                    "ResetOnSuccessfulApplication"),
                "AssetTags": _to_tag_container_obj(asset_tags),
                "GrantedTags": _to_tag_container_obj(granted_tags),
                "Modifiers": []
            }
            for m in (modifiers_map.get(name, []) or []):
                attr_cell = _safe_str(m.get("Attribute"))
                if not attr_cell:
                    continue
                attr_obj = _parse_attribute_cell(attr_cell)
                modifier = {
                    "Attribute": attr_obj,
                    "ModifierOp": _safe_str(m.get("ModifierOp"), "AddBase"),
                    "MagnitudeCalculationType": _safe_str(m.get("MagnitudeCalculationType"), "ScalableFloat"),
                    "Magnitude": _safe_num(m.get("Magnitude"), 0),
                }
                item["Modifiers"].append(modifier)
            result.append(item)

    elif scheme == "flat":
        flat_fields = set(_build_flat_fields())
        for r in main_rows:
            name = _safe_str(r.get("Name"))
            if not name or name == "Name":
                continue
            asset_tags = _split_tag_string(r.get("AssetTags"))
            granted_tags = _split_tag_string(r.get("GrantedTags"))

            item = {
                "Name": name,
                "DurationType": _safe_str(r.get("DurationType"), "Instant"),
                "DurationMagnitude": _safe_num(r.get("DurationMagnitude"), 0),
                "Period": _safe_num(r.get("Period"), 0),
                "StackingType": _safe_str(r.get("StackingType"), "None"),
                "StackLimitCount": _safe_num(r.get("StackLimitCount"), 1),
                "StackDurationRefreshPolicy": _safe_str(r.get("StackDurationRefreshPolicy"),
                                                       "RefreshOnSuccessfulApplication"),
                "StackPeriodResetPolicy": _safe_str(r.get("StackPeriodResetPolicy"),
                                                    "ResetOnSuccessfulApplication"),
                "AssetTags": _to_tag_container_obj(asset_tags),
                "GrantedTags": _to_tag_container_obj(granted_tags),
                "Modifiers": _collect_modifiers_from_flat_row(r),
            }
            # 兼容：忽略未知/非模板列
            # （此处无需额外处理，_collect_modifiers_from_flat_row已使用约定列）
            result.append(item)
    else:
        raise RuntimeError("无法识别的表格方案。")

    _ensure_dir(out_json_path)
    with open(out_json_path, "w", encoding="utf-8") as f:
        json.dump(result, f, ensure_ascii=False, indent=4)

    msg = f"[ExcelTools] 导出完成：{out_json_path}（{scheme}，共{len(result)}条）"
    if UE_AVAILABLE:
        unreal.log(msg)
    else:
        print(msg)

# 便捷函数：默认路径
def quick_generate_template_to_samples(use_sub_sheet: bool = True):
    """
    在脚本同目录生成模板（演示用）
    """
    here = os.path.dirname(os.path.abspath(__file__))
    if OPENPYXL_AVAILABLE:
        out_path = os.path.join(here, "GameplayEffectData.xlsx")
    else:
        out_path = os.path.join(here, "GameplayEffectData_Flat.csv" if not use_sub_sheet else "GameplayEffectData_Main.csv")
    generate_gameplay_effect_excel_template(out_path, use_sub_sheet=use_sub_sheet)
    return out_path

def quick_export_from_template():
    """
    默认从与脚本同目录的模板导出到插件DataSample/GameplayEffectData.json（演示用）
    """
    here = os.path.dirname(os.path.abspath(__file__))
    # 输入：xlsx优先，否则csv目录
    xlsx = os.path.join(here, "GameplayEffectData.xlsx")
    in_path = xlsx if os.path.exists(xlsx) else here
    out_json = os.path.join(os.path.dirname(here), "DataSample", "GameplayEffectData.json")
    export_gameplay_effect_excel_to_json(in_path, out_json)
    return out_json

if __name__ == "__main__":
    # 命令行用法（在外部Python环境下也可调试）
    # 1) Schema 模板：python ability_editor_excel_tool.py schema_template <schema> <输出路径/目录> [--schema-dir DIR]
    # 2) Schema 导出：python ability_editor_excel_tool.py schema_export <schema> <输入xlsx|csv> <输出json> [--schema-dir DIR]
    # 3) 固定模板：python ability_editor_excel_tool.py template <输出路径> [--use-sub-sheet true|false]
    # 4) 固定导出：python ability_editor_excel_tool.py export <输入xlsx或csv目录> <输出json路径>
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

    p1 = sub.add_parser("template")
    p1.add_argument("out", help="输出.xlsx路径（无openpyxl时忽略为CSV）")
    p1.add_argument("--use-sub-sheet", default="true",
                    help="是否使用子表方案（true/false，默认true）")

    p2 = sub.add_parser("export")
    p2.add_argument("inp", help=".xlsx文件路径 或 CSV目录路径")
    p2.add_argument("out", help="输出.json路径")

    args = parser.parse_args()
    if args.cmd == "schema_template":
        sd = args.schema_dir.strip() or None
        generate_excel_template_from_schema(args.schema, args.out, schema_dir=sd)
    elif args.cmd == "schema_export":
        sd = args.schema_dir.strip() or None
        export_excel_to_json_using_schema(args.inp, args.out, args.schema, schema_dir=sd)
    elif args.cmd == "template":
        val = str(args.use_sub_sheet).strip().lower()
        use_sub = val in ("1", "true", "yes", "y", "t")
        generate_gameplay_effect_excel_template(args.out, use_sub_sheet=use_sub)
    elif args.cmd == "export":
        export_gameplay_effect_excel_to_json(args.inp, args.out)
    else:
        print("用法：\n"
              "  schema_template <schema_name|schema_path> <out.xlsx|out_dir> [--schema-dir <dir>]\n"
              "  schema_export   <schema_name|schema_path> <in.xlsx|csv_dir> <out.json> [--schema-dir <dir>]\n"
              "  template <out.xlsx> [--use-sub-sheet true|false]\n"
              "  export <in.xlsx|csv_dir> <out.json>")
