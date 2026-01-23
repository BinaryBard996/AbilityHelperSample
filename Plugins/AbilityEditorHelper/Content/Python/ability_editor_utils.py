import os
import unreal

def normalize(p: str) -> str:
    if not p:
        return ""
    # UE: 统一成 / 并清理多余分隔符
    p = unreal.Paths.normalize_filename(p)
    # Python: 再做一次绝对化（如果你允许相对路径的话）
    p = os.path.abspath(p)
    return p

def validate_excel_file(excel_file_path: str):
    excel_file_path = normalize(excel_file_path)

    if not excel_file_path:
        return False, "Excel 路径为空"

    if not os.path.isfile(excel_file_path):
        return False, f"Excel 文件不存在：{excel_file_path}"

    ext = os.path.splitext(excel_file_path)[1].lower()
    if ext not in [".xlsx", ".xlsm", ".csv"]:
        return False, f"Excel 文件扩展名不支持：{ext}（期望 .xlsx/.xlsm/.csv）"

    return True, excel_file_path

def validate_json_output(json_file_path: str, auto_create_dir=True):
    json_file_path = normalize(json_file_path)

    if not json_file_path:
        return False, "Json 输出路径为空"

    out_dir = os.path.dirname(json_file_path)
    if not out_dir:
        return False, f"Json 输出路径不合法：{json_file_path}"

    if not os.path.isdir(out_dir):
        if auto_create_dir:
            os.makedirs(out_dir, exist_ok=True)
        else:
            return False, f"Json 输出目录不存在：{out_dir}"

    ext = os.path.splitext(json_file_path)[1].lower()
    if ext != ".json":
        return False, f"Json 输出文件扩展名应为 .json：{json_file_path}"

    return True, json_file_path

def find_schema_file(schema_dir: str, struct_type_name: str):
    schema_dir = normalize(schema_dir)

    if not schema_dir:
        return False, "Schema 路径为空"

    if not os.path.isdir(schema_dir):
        return False, f"Schema 路径不是目录或不存在：{schema_dir}"

    candidates = [
        os.path.join(schema_dir, f"{struct_type_name}.schema.json"),
        os.path.join(schema_dir, f"{struct_type_name}.json"),
        os.path.join(schema_dir, f"{struct_type_name}.schema"),
    ]
    for p in candidates:
        if os.path.isfile(p):
            return True, p

    return False, f"Schema 目录中找不到 {struct_type_name} 的 schema 文件，已尝试：{candidates}"

def validate_all(excel_dir, json_dir, schema_dir, excel_file_name, json_file_name, struct_type_name):
    excel_file = normalize(os.path.join(excel_dir, excel_file_name))
    json_file  = normalize(os.path.join(json_dir,  json_file_name))

    ok, excel_or_err = validate_excel_file(excel_file)
    if not ok:
        return False, excel_or_err

    ok, json_or_err = validate_json_output(json_file)
    if not ok:
        return False, json_or_err

    ok, schema_or_err = find_schema_file(schema_dir, struct_type_name)
    if not ok:
        return False, schema_or_err

    return True, (excel_or_err, json_or_err, schema_or_err)
