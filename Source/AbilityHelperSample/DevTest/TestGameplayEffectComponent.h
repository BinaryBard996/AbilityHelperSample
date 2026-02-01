// TestGameplayEffectComponent.h
// DevTest: 测试用 GE Component

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "TestGameplayEffectComponent.generated.h"

/**
 * 测试用 GameplayEffect Component
 * 用于验证方案三的扩展机制
 */
UCLASS()
class ABILITYHELPERSAMPLE_API UTestGameplayEffectComponent : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	/** 测试用整数属性 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
	int32 TestIntProperty = 0;

	/** 测试用布尔属性 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
	bool bTestBoolProperty = false;
};
