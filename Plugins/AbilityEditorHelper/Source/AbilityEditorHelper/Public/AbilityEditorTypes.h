// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "UObject/Object.h"
#include "AbilityEditorTypes.generated.h"

/**
 * 用于导出到 JSON 的字段 Schema 描述（驱动 Excel 模板与导出）
 */
USTRUCT(BlueprintType)
struct FExcelSchemaField
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	FString Category;

	// 基础类型分类：bool/int/float/double/string/name/text/enum/struct/array/unknown
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	FString Kind;

	// 若 Kind == enum
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	FString EnumPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	TArray<FString> EnumValues;

	// 若 Kind == struct
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	FString StructPath;

	// 若 Kind == array：数组元素信息
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	FString InnerKind;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	FString InnerStructPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	FString InnerEnumPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	TArray<FString> InnerEnumValues;

	// Excel 相关可选元数据（从 UPROPERTY meta 读取）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	bool bExcelIgnore = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	FString ExcelName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	FString ExcelHint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	FString ExcelSheet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	FString ExcelSeparator;
};

/**
 * 整体 Schema：包含结构体路径、字段签名哈希、字段列表与特殊规则提示
 */
USTRUCT(BlueprintType)
struct FExcelSchema
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	FString StructPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	FString Hash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	TArray<FExcelSchemaField> Fields;

	// 特殊类型规则（供 Python 侧识别特殊序列化）：
	// 例如 "GameplayTagContainer"->"tag_container_rule"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AbilityEditorHelper|Schema")
	TMap<FString, FString> SpecialRules;
};

/**
 * GE 修改器配置（用于修改Attribute）
 * TODO. 目前只实现最简单的一版
 */
USTRUCT(BlueprintType)
struct FGEModifierConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FGameplayAttribute Attribute;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	TEnumAsByte<EGameplayModOp::Type> ModifierOp = EGameplayModOp::Additive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	EGameplayEffectMagnitudeCalculation MagnitudeCalculationType = EGameplayEffectMagnitudeCalculation::ScalableFloat;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	float Magnitude;
};

/**
 * 完整的 GE 配置数据（DataTable 行结构）
 * TODO. 目前只实现最简单的一版
 */
USTRUCT(BlueprintType)
struct FGameplayEffectConfig : public FTableRowBase
{
	GENERATED_BODY()

	// === 基础配置 ===
	
	// GE 的持续类型（Instant, Duration, Infinite, HasDuration）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	EGameplayEffectDurationType DurationType = EGameplayEffectDurationType::Instant;

	// 持续时间（仅当 DurationType 为 Duration 或 HasDuration 时有效）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic", meta = (EditCondition = "DurationType == EGameplayEffectDurationType::Duration || DurationType == EGameplayEffectDurationType::HasDuration"))
	float DurationMagnitude = 0.f;

	// 执行周期（Period, 0 表示不周期执行）

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	float Period = 0.f;

	// === 堆叠配置 ===
	
	// 堆叠类型（None, AggregateBySource, AggregateByTarget）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stacking")
	EGameplayEffectStackingType StackingType = EGameplayEffectStackingType::None;

	// 最大堆叠层数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stacking")
	int32 StackLimitCount = 1;

	// 堆叠持续时间刷新策略
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stacking")
	EGameplayEffectStackingDurationPolicy StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

	// 堆叠周期刷新策略
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stacking")
	EGameplayEffectStackingPeriodPolicy StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::ResetOnSuccessfulApplication;

	// === Tag 配置 ===

	// GE 自身的 Tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FGameplayTagContainer AssetTags;
	
	// GE 在激活期间赋予目标 Tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FGameplayTagContainer GrantedTags;

	// === 修改器配置 ===
	
	// Attribute 修改器列表
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers")
	TArray<FGEModifierConfig> Modifiers;
	
};
