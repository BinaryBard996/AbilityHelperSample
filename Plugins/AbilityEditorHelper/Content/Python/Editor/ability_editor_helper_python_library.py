import os
import unreal
import ability_editor_utils

from ability_editor_excel_tool import (
    generate_excel_template_from_schema,
    export_excel_to_json_using_schema)

@unreal.uclass()
class AbilityEditorHelperPythonLibrary(unreal.BlueprintFunctionLibrary):

    # 根据结构体类型名称从schema生成Excel模板文件
    @unreal.ufunction(params=[str, str, bool], static=True)
    def GenerateExcelTemplateFromSchema(struct_type_name, excel_file_name, preserve_data=True):
        ability_editor_helper_settings = unreal.AbilityEditorHelperLibrary.get_ability_editor_helper_settings()
        excel_path = ability_editor_helper_settings.excel_path
        schema_path = ability_editor_helper_settings.schema_path
        
        excel_file_path = ability_editor_utils.normalize(os.path.join(excel_path, excel_file_name))
        bool_valid, schema_file_path_or_err = ability_editor_utils.find_schema_file(schema_path, struct_type_name)
        if not bool_valid:
            # schema文件不存在
            unreal.log_error(schema_file_path_or_err)
            return
        
        generate_excel_template_from_schema(schema_file_path_or_err, excel_file_path, preserve_data=preserve_data)

        # 检验excel文件是否被成功创建了
        bool_valid, excel_file_path_or_err = ability_editor_utils.validate_excel_file(excel_file_path)
        if not bool_valid:
            unreal.log_error(excel_file_path_or_err)
            return
        
    # 将Excel文件导出为JSON格式，并使用指定的结构类型进行验证和转换
    @unreal.ufunction(params=[str, str, str], static=True)
    def ExportExcelToJsonUsingSchema(excel_file_name, json_file_name, struct_type_name):
        ability_editor_helper_settings = unreal.AbilityEditorHelperLibrary.get_ability_editor_helper_settings()
        excel_path = ability_editor_helper_settings.excel_path
        json_path = ability_editor_helper_settings.json_path
        schema_path = ability_editor_helper_settings.schema_path
        
        bool_valid, result = ability_editor_utils.validate_all(excel_dir=excel_path, json_dir=json_path, schema_dir=schema_path, excel_file_name=excel_file_name, json_file_name=json_file_name, struct_type_name=struct_type_name)
        if not bool_valid:
            unreal.log_error(result)
            return
        
        export_excel_to_json_using_schema(result[0], result[1], result[2])
