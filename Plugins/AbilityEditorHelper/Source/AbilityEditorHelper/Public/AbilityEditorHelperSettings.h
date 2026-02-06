// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"
#include "AbilityEditorHelperSettings.generated.h"

class UGameplayEffect;
class UGameplayAbility;
class UDataTable;
class UEditorUtilityWidgetBlueprint;

/**
 *
 */
UCLASS(Config=EditorPerProjectUserSettings, DefaultConfig)
class ABILITYEDITORHELPER_API UAbilityEditorHelperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAbilityEditorHelperSettings();

	// === GameplayEffect 配置 ===

	/** 创建 GameplayEffect 时默认使用的类 */
	UPROPERTY(Config, EditAnywhere, Category = "GameplayEffect")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	/** 用于批量导入/更新 GameplayEffect 的配置数据表（行结构应为 FGameplayEffectConfig） */
	UPROPERTY(Config, EditAnywhere, Category = "GameplayEffect|Import")
	TSoftObjectPtr<UDataTable> GameplayEffectDataTable;

	/** 批量创建/更新 GameplayEffect 时使用的基础路径（例如：/Game/Abilities/Effects） */
	UPROPERTY(Config, EditAnywhere, Category = "GameplayEffect")
	FString GameplayEffectPath;

	// === GameplayAbility 配置 ===

	/** 创建 GameplayAbility 时默认使用的类 */
	UPROPERTY(Config, EditAnywhere, Category = "GameplayAbility")
	TSubclassOf<UGameplayAbility> GameplayAbilityClass;

	/** 用于批量导入/更新 GameplayAbility 的配置数据表（行结构应为 FGameplayAbilityConfig） */
	UPROPERTY(Config, EditAnywhere, Category = "GameplayAbility|Import")
	TSoftObjectPtr<UDataTable> GameplayAbilityDataTable;

	/** 批量创建/更新 GameplayAbility 时使用的基础路径（例如：/Game/Abilities/Abilities） */
	UPROPERTY(Config, EditAnywhere, Category = "GameplayAbility")
	FString GameplayAbilityPath;

	// === Excel/数据类型 配置 ===

	/** GameplayEffect 对应的 Excel 文件名（可不带 .xlsx 后缀） */
	UPROPERTY(Config, BlueprintReadWrite, EditAnywhere, Category = "GameplayEffect|Import")
	FString GameplayEffectExcelName;

	/** GameplayAbility 对应的 Excel 文件名（可不带 .xlsx 后缀） */
	UPROPERTY(Config, BlueprintReadWrite, EditAnywhere, Category = "GameplayAbility|Import")
	FString GameplayAbilityExcelName;

	/** GameplayEffect 配置结构体的类型路径（如 /Script/AbilityEditorHelper.GameplayEffectConfig） */
	UPROPERTY(Config, BlueprintReadWrite, EditAnywhere, Category = "GameplayEffect|Import")
	FString GameplayEffectDataType;

	/** GameplayAbility 配置结构体的类型路径（如 /Script/AbilityEditorHelper.GameplayAbilityConfig） */
	UPROPERTY(Config, BlueprintReadWrite, EditAnywhere, Category = "GameplayAbility|Import")
	FString GameplayAbilityDataType;

	// === 数据路径配置 ===

	/** Excel的数据存储路径 */
	UPROPERTY(Config, BlueprintReadOnly, EditAnywhere, Category = "DataPath")
	FString ExcelPath;

	/** Json的数据存储路径 */
	UPROPERTY(Config, BlueprintReadOnly, EditAnywhere, Category = "DataPath")
	FString JsonPath;

	/** Schema 导出路径 */
	UPROPERTY(Config, BlueprintReadOnly, EditAnywhere, Category = "DataPath")
	FString SchemaPath;

	// === Schema 配置 ===

	// === EditorWidget 配置 ===

	/** 编辑器工具窗口对应的 EditorUtilityWidget 蓝图资产，可通过 Window 菜单中的 Action 打开 */
	UPROPERTY(Config, EditAnywhere, Category = "EditorWidget")
	TSoftObjectPtr<UEditorUtilityWidgetBlueprint> EditorUtilityWidgetBlueprint;

	/**
	 * 需要自动导出 Schema 的结构体类型路径列表
	 * 格式：/Script/ModuleName.StructName
	 * 例如：/Script/AbilityEditorHelper.GameplayEffectConfig
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Schema")
	TArray<FString> StructTypePathsToExportSchema;
};
