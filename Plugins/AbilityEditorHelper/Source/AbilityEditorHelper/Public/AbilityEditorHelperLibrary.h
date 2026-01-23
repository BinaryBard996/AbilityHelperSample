// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AbilityEditorTypes.h"
#include "AbilityEditorHelperLibrary.generated.h"

class UBlueprint;
class UGameplayEffect;
class UAbilityEditorHelperSettings;

/**
 * 
 */
UCLASS()
class ABILITYEDITORHELPER_API UAbilityEditorHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper")
	static const UAbilityEditorHelperSettings* GetAbilityEditorHelperSettings();

	/**
	 * 在指定路径和父类的基础上创建一个Blueprint资产并返回。
	 * 支持路径格式：
	 *   - /Game/Folder/AssetName
	 *   - /Game/Folder/AssetName.AssetName
	 * 非编辑器环境下返回nullptr并置bOutSuccess=false。
	 */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|Blueprint", meta=(DisplayName="Create Blueprint Asset", Keywords="Create Blueprint Asset"))
	static UBlueprint* CreateBlueprintAsset(const FString& BlueprintPath, TSubclassOf<UObject> ParentClass, bool& bOutSuccess);

	/**
	 * 获取或创建指定路径的Blueprint资产：
	 *  - 若资产已存在则直接加载并返回；
	 *  - 若不存在则创建并返回。
	 * 非编辑器环境仅尝试加载，无法创建时返回nullptr且bOutSuccess=false。
	 */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|Blueprint", meta=(DisplayName="Get Or Create Blueprint Asset", Keywords="Get Or Create Blueprint Asset"))
	static UBlueprint* GetOrCreateBlueprintAsset(const FString& BlueprintPath, TSubclassOf<UObject> ParentClass, bool& bOutSuccess);

	/**
	 * 根据 FGameplayEffectConfig 在指定路径创建或更新 GameplayEffect 资产，并写入配置数据。
	 * - 若资产已存在则覆盖关键字段；
	 * - 若资产不存在则在编辑器下创建新资产并导入配置。
	 * 非编辑器环境下仅尝试加载但不会创建，失败则返回nullptr且bOutSuccess=false。
	 */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|GameplayEffect", meta=(DisplayName="Create Or Import GameplayEffect From Config", Keywords="GameplayEffect Create Import From Config"))
	static UGameplayEffect* CreateOrImportGameplayEffect(const FString& GameplayEffectPath, const FGameplayEffectConfig& Config, bool& bOutSuccess);

	/**
	 * 基于 UAbilityEditorHelperSettings 中的 DataTable 与 GameplayEffectPath，批量创建/更新 GameplayEffect。
	 * 不返回创建的对象或数量，处理结果通过日志输出。
	 */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|GameplayEffect", meta=(DisplayName="Create Or Update GameplayEffects From Settings", Keywords="GameplayEffect Import Update From Settings"))
	static void CreateOrUpdateGameplayEffectsFromSettings();

	/** 
	 * 将任意 UStruct 的描述导出为 Schema(JSON)。输入 StructPath 完整路径（如 /Script/Module.StructName）。
	 */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|Schema")
	static bool WriteStructSchemaToJson(UScriptStruct* StructType, const FString& OutJsonFilePath, FString& OutError);

	/**
	 * 便捷函数：生成 Schema 到插件 Content/Python/Schema 目录，文件名 <StructName>.schema.json
	 */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|Schema")
	static bool GenerateStructSchemaToPythonFolder(UScriptStruct* StructType, FString& OutError);
	
	/**
	 * 从指定 JSON 文件导入数据到目标 DataTable。
	 * @param TargetDataTable      目标数据表
	 * @param JsonFilePath         JSON 文件的绝对路径
	 * @param bClearBeforeImport   导入前是否清空现有行
	 * @param OutImportedRowCount  成功导入的行数（由引擎返回）
	 * @param OutError             失败时的错误信息
	 * @return                     是否导入成功（依据引擎返回值>=0判定）
	 */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|DataTable", meta=(DisplayName="Import DataTable From JSON File", Keywords="DataTable Import JSON"))
	static bool ImportDataTableFromJsonFile(UDataTable* TargetDataTable, const FString& JsonFilePath, bool bClearBeforeImport, int32& OutImportedRowCount, FString& OutError);

};
