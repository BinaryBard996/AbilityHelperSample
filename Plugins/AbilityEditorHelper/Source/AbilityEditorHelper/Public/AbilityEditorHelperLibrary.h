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
	 * @param bClearGameplayEffectFolderFirst  在导入前是否先清理 GameplayEffectPath 路径下（含子目录）中不在 DataTable 的 GE 资产
	 */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|GameplayEffect", meta=(DisplayName="Create Or Update GameplayEffects From Settings", Keywords="GameplayEffect Import Update From Settings", CPP_Default_bClearGameplayEffectFolderFirst="false"))
	static void CreateOrUpdateGameplayEffectsFromSettings(bool bClearGameplayEffectFolderFirst = false);

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
	 * 批量生成 Schema：根据 UAbilityEditorHelperSettings 中配置的结构体列表，批量导出所有 Schema 到 Python/Schema 目录
	 * @param bClearSchemaFolderFirst  生成前是否先清空 Schema 文件夹
	 * @param OutSuccessCount  成功导出的结构体数量
	 * @param OutFailureCount  导出失败的结构体数量
	 * @param OutErrors        所有失败的错误信息（每行一个错误）
	 * @return                 是否全部成功
	 */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|Schema")
	static bool GenerateAllSchemasFromSettings(bool bClearSchemaFolderFirst, int32& OutSuccessCount, int32& OutFailureCount, FString& OutErrors);

	/**
	 * 从结构体路径字符串加载 UScriptStruct
	 * @param StructPath  结构体路径（格式：/Script/ModuleName.StructName）
	 * @return            加载的结构体指针，失败返回 nullptr
	 */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|Schema")
	static UScriptStruct* LoadStructFromPath(const FString& StructPath);

	/**
	 * 从指定 JSON 文件导入数据到目标 DataTable。
	 * @param TargetDataTable      目标数据表
	 * @param JsonFileName         JSON 文件名（将与 UAbilityEditorHelperSettings::JsonPath 拼接成完整路径）
	 * @param bClearBeforeImport   导入前是否清空现有行
	 * @param OutImportedRowCount  成功导入的行数（由引擎返回）
	 * @param OutError             失败时的错误信息
	 * @return                     是否导入成功（依据引擎返回值>=0判定）
	 */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|DataTable", meta=(DisplayName="Import DataTable From JSON File", Keywords="DataTable Import JSON"))
	static bool ImportDataTableFromJsonFile(UDataTable* TargetDataTable, const FString& JsonFileName, bool bClearBeforeImport, int32& OutImportedRowCount, FString& OutError);

	/**
	 * 将简化的属性字符串解析为 FGameplayAttribute
	 * @param AttributeString  简化格式字符串（如 "TestAttributeSet.TestPropertyOne"）
	 * @param OutAttribute     输出的 FGameplayAttribute
	 * @return                 是否解析成功
	 */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|Attribute", meta=(DisplayName="Parse Attribute String"))
	static bool ParseAttributeString(const FString& AttributeString, FGameplayAttribute& OutAttribute);

	/**
	 * 从 JSON 文件导入数据并更新 GameplayEffects（增量更新）
	 * 该函数会：
	 * 1. 读取 JSON 文件
	 * 2. 与现有 DataTable 数据比较，找出新增或变化的行
	 * 3. 只对变化的行更新 DataTable
	 * 4. 只对变化的行创建/更新 GameplayEffect 资产
	 *
	 * @param JsonFileName             JSON 文件名（相对于 Settings::JsonPath）
	 * @param bClearGameplayEffectFolderFirst  是否先清理不在 DataTable 中的 GE 资产
	 * @param OutUpdatedRowNames       被更新的行名列表
	 * @param OutError                 错误信息
	 * @return                         是否成功
	 */
	UFUNCTION(BlueprintCallable, Category="AbilityEditorHelper|GameplayEffect", meta=(DisplayName="Import And Update GameplayEffects From JSON"))
	static bool ImportAndUpdateGameplayEffectsFromJson(const FString& JsonFileName, bool bClearGameplayEffectFolderFirst, TArray<FName>& OutUpdatedRowNames);

};
