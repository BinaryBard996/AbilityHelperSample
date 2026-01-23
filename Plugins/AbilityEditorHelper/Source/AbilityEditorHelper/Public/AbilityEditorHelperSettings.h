// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"
#include "AbilityEditorHelperSettings.generated.h"

class UGameplayEffect;
class UDataTable;

/**
 * 
 */
UCLASS(Config=EditorPerProjectUserSettings, DefaultConfig)
class ABILITYEDITORHELPER_API UAbilityEditorHelperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAbilityEditorHelperSettings();

	/** 创建 GameplayEffect 时默认使用的类 */
	UPROPERTY(Config, EditAnywhere, Category = "GameplayEffect")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	/** 用于批量导入/更新 GameplayEffect 的配置数据表（行结构应为 FGameplayEffectConfig） */
	UPROPERTY(Config, EditAnywhere, Category = "GameplayEffect|Import")
	TSoftObjectPtr<UDataTable> GameplayEffectDataTable;

	/** 批量创建/更新 GameplayEffect 时使用的基础路径（例如：/Game/Abilities/Effects 或 Abilities/Effects） */
	UPROPERTY(Config, EditAnywhere, Category = "DataPath")
	FString GameplayEffectPath;

	/** Excel的数据存储路径 */
	UPROPERTY(Config, BlueprintReadOnly, EditAnywhere, Category = "DataPath")
	FString ExcelPath;

	/** Json的数据存储路径 */
	UPROPERTY(Config, BlueprintReadOnly, EditAnywhere, Category = "DataPath")
	FString JsonPath;

	UPROPERTY(Config, BlueprintReadOnly, EditAnywhere, Category = "DataPath")
	FString SchemaPath;
};
