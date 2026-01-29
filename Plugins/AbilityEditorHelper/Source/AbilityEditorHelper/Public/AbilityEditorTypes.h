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
 * Tag 需求配置（用于表示 MustHaveTags/MustNotHaveTags）
 */
USTRUCT(BlueprintType)
struct FTagRequirementsConfig
{
	GENERATED_BODY()

	// 必须拥有的 Tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TagRequirements")
	FGameplayTagContainer RequireTags;

	// 必须不拥有的 Tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TagRequirements")
	FGameplayTagContainer IgnoreTags;
};

/**
 * AttributeBased 修改器配置
 */
USTRUCT(BlueprintType)
struct FAttributeBasedModifierConfig
{
	GENERATED_BODY()

	// 后备属性（Backing Attribute）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttributeBased")
	FGameplayAttribute BackingAttribute;

	// 属性计算类型
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttributeBased")
	EAttributeBasedFloatCalculationType AttributeCalculationType = EAttributeBasedFloatCalculationType::AttributeMagnitude;

	// 系数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttributeBased")
	float Coefficient = 1.0f;

	// 乘法前的加法值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttributeBased")
	float PreMultiplyAdditiveValue = 0.0f;

	// 乘法后的加法值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttributeBased")
	float PostMultiplyAdditiveValue = 0.0f;
};

/**
 * SetByCaller 修改器配置
 */
USTRUCT(BlueprintType)
struct FSetByCallerModifierConfig
{
	GENERATED_BODY()

	// 数据 Tag（用于运行时查找）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SetByCaller")
	FGameplayTag DataTag;

	// 数据名称（用于运行时查找）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SetByCaller")
	FName DataName;
};

/**
 * GameplayCue 配置
 */
USTRUCT(BlueprintType)
struct FGameplayCueConfig
{
	GENERATED_BODY()

	// GameplayCue Tag
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayCue")
	FGameplayTag GameplayCueTag;

	// 最小等级
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayCue")
	float MinLevel = 0.0f;

	// 最大等级
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayCue")
	float MaxLevel = 0.0f;
};

/**
 * 效果查询配置（用于 Immunity 和 Removal）
 */
USTRUCT(BlueprintType)
struct FEffectQueryConfig
{
	GENERATED_BODY()

	// 匹配任意拥有的 Tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FGameplayTagContainer MatchAnyOwningTags;

	// 匹配所有拥有的 Tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FGameplayTagContainer MatchAllOwningTags;

	// 不匹配拥有的 Tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FGameplayTagContainer MatchNoOwningTags;

	// 匹配任意来源 Tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FGameplayTagContainer MatchAnySourceTags;

	// 匹配所有来源 Tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FGameplayTagContainer MatchAllSourceTags;

	// 不匹配来源 Tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FGameplayTagContainer MatchNoSourceTags;
};

/**
 * Execution 配置（最小支持）
 */
USTRUCT(BlueprintType)
struct FExecutionConfig
{
	GENERATED_BODY()

	// Execution 计算类 (Asset path)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Execution",
		meta = (ExcelHint = "Asset path to execution calculation class"))
	FString CalculationClass;

	// 传递的 Tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Execution")
	FGameplayTagContainer PassedInTags;
};

/**
 * GE 修改器配置（用于修改Attribute）
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

	// === 增强功能：AttributeBased 计算配置 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FAttributeBasedModifierConfig AttributeBasedConfig;

	// === 增强功能：SetByCaller 配置 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FSetByCallerModifierConfig SetByCallerConfig;

	// === 增强功能：自定义计算类 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier",
		meta = (ExcelHint = "Asset path to calculation class"))
	FString CustomCalculationClass;

	// === 增强功能：Source Tag 需求 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FTagRequirementsConfig SourceTagRequirements;

	// === 增强功能：Target Tag 需求 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FTagRequirementsConfig TargetTagRequirements;
};

/**
 * 完整的 GE 配置数据（DataTable 行结构）
 */
USTRUCT(BlueprintType)
struct FGameplayEffectConfig : public FTableRowBase
{
	GENERATED_BODY()

	// === 基础配置 ===

	// 父类（可选，用于继承默认值）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic",
		meta = (ExcelHint = "Asset path: /Game/Effects/GE_Base"))
	FString ParentClass;

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

	// === Tag 需求配置 ===

	// 应用时的 Tag 需求（RequireTagsToApply）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Application")
	FTagRequirementsConfig ApplicationTagRequirements;

	// 持续期间的 Tag 需求（RequireTagsToContinue）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Application")
	FTagRequirementsConfig OngoingTagRequirements;

	// 移除时的 Tag 需求
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Application")
	FTagRequirementsConfig RemovalTagRequirements;

	// === 阻断/取消配置 ===

	// 取消拥有这些 Tags 的 Abilities
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FGameplayTagContainer CancelAbilitiesWithTags;

	// 阻断拥有这些 Tags 的 Abilities
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FGameplayTagContainer BlockAbilitiesWithTags;

	// === 授予的 Abilities ===

	// 授予的 Ability 类列表（逗号分隔的资产路径）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities",
		meta = (ExcelHint = "Comma-separated asset paths", ExcelSeparator = ","))
	TArray<FString> GrantedAbilityClasses;

	// === 修改器配置 ===

	// Attribute 修改器列表
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers")
	TArray<FGEModifierConfig> Modifiers;

	// === GameplayCues 配置 ===

	// GameplayCue 列表
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayCues")
	TArray<FGameplayCueConfig> GameplayCues;

	// === 效果查询配置 ===

	// 免疫查询列表
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Immunity")
	TArray<FEffectQueryConfig> ImmunityQueries;

	// 移除效果查询列表
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Removal")
	TArray<FEffectQueryConfig> RemovalQueries;

	// === Executions 配置 ===

	// Execution 列表
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Executions")
	TArray<FExecutionConfig> Executions;

};
