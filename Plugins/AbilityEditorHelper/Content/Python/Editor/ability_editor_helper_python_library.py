import unreal

from ability_editor_excel_tool import generate_gameplay_effect_excel_template, export_gameplay_effect_excel_to_json, generate_excel_template_from_schema

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
    
    @unreal.ufunction(params=[str, str], static=True)
    def GenerateExcelTemplateFromSchema(Name, OutPath):
        generate_excel_template_from_schema("GameplayEffectConfig", "D:\Project\AbilityHelperSample\Plugins\AbilityEditorHelper\DataSample\Test.xlsx", "D:\Project\AbilityHelperSample\Plugins\AbilityEditorHelper\Content\Python\Schema")
    
    