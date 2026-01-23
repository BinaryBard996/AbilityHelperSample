import os
import unreal
import ability_editor_utils

from ability_editor_excel_tool import (
    generate_gameplay_effect_excel_template, 
    export_gameplay_effect_excel_to_json, 
    generate_excel_template_from_schema,
    export_excel_to_json_using_schema)

@unreal.uclass()
class AbilityEditorHelperPythonLibrary(unreal.BlueprintFunctionLibrary):
     
    # 依据FGameplayEffectConfig去创建对应的Excel模板    
    @unreal.ufunction(params=[str, bool], static=True)
    def GenerateGameplayEffectExcelTemplate(excel_file_path: str, use_sub_sheet: bool = True):
        return generate_gameplay_effect_excel_template(excel_file_path, use_sub_sheet)
    
    # 将excel的数据，导出成为json
    @unreal.ufunction(params=[str, str], static=True)
    def ExportGameplayEffectExcelToJson(excel_file_path: str, json_file_path: str):
        return export_gameplay_effect_excel_to_json(excel_file_path, json_file_path)
    
    # 根据结构体类型名称从schema生成Excel模板文件
    @unreal.ufunction(params=[str, str], static=True)
    def GenerateExcelTemplateFromSchema(struct_type_name, excel_file_name):
        ability_editor_helper_settings = unreal.AbilityEditorHelperLibrary.get_ability_editor_helper_settings()
        excel_path = ability_editor_helper_settings.excel_path
        schema_path = ability_editor_helper_settings.schema_path
        
        excel_file_path = ability_editor_utils.normalize(os.path.join(excel_path, excel_file_name))
        bool_valid, schema_file_path_or_err = ability_editor_utils.find_schema_file(schema_path, struct_type_name)
        if not bool_valid:
            # schema文件不存在
            unreal.log_error(schema_file_path_or_err)
            return
        
        generate_excel_template_from_schema(schema_file_path_or_err, excel_file_path)

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
