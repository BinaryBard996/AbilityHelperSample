// SampleGameplayAbility.h
// DevTest: GameplayAbility 的子类示例，用于测试 GA 扩展机制

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SampleGameplayAbility.generated.h"

/**
 * GameplayAbility 的示例子类
 * 包含与 FGameplayAbilitySampleConfig 对应的扩展属性
 * 这些属性将通过 OnPostProcessGameplayAbility 委托从配置写入
 */
UCLASS(Blueprintable)
class ABILITYHELPERSAMPLE_API USampleGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USampleGameplayAbility();

	/** 测试用浮点属性 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SampleExtension")
	float TestFloatValue;

	/** 测试用整数属性 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SampleExtension")
	int32 TestIntValue;

	/** 测试用布尔属性 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SampleExtension")
	bool bTestBoolValue;
};
