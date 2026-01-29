// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilityEditorHelperSettings.h"
#include "GameplayEffect.h"
#include "AbilityEditorTypes.h"

UAbilityEditorHelperSettings::UAbilityEditorHelperSettings()
{
	GameplayEffectClass = UGameplayEffect::StaticClass();

	// 初始化需要导出 Schema 的结构体类型路径列表
	// 使用字符串路径，便于配置文件持久化和用户自定义
	if (StructTypePathsToExportSchema.Num() == 0)
	{
		// 核心配置结构体
		StructTypePathsToExportSchema.Add(FGameplayEffectConfig::StaticStruct()->GetPathName());
		StructTypePathsToExportSchema.Add(FGEModifierConfig::StaticStruct()->GetPathName());

		// 辅助配置结构体
		StructTypePathsToExportSchema.Add(FTagRequirementsConfig::StaticStruct()->GetPathName());
		StructTypePathsToExportSchema.Add(FAttributeBasedModifierConfig::StaticStruct()->GetPathName());
		StructTypePathsToExportSchema.Add(FSetByCallerModifierConfig::StaticStruct()->GetPathName());
		StructTypePathsToExportSchema.Add(FGameplayCueConfig::StaticStruct()->GetPathName());
		StructTypePathsToExportSchema.Add(FEffectQueryConfig::StaticStruct()->GetPathName());
		StructTypePathsToExportSchema.Add(FExecutionConfig::StaticStruct()->GetPathName());

		// Schema 元数据结构体（可选，用于调试）
		StructTypePathsToExportSchema.Add(FExcelSchemaField::StaticStruct()->GetPathName());
		StructTypePathsToExportSchema.Add(FExcelSchema::StaticStruct()->GetPathName());
	}
}

