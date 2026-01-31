// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "TestAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 
 */
UCLASS()
class ABILITYEDITORHELPER_API UTestAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Test")
	FGameplayAttributeData TestPropertyOne;
	ATTRIBUTE_ACCESSORS(UTestAttributeSet, TestPropertyOne)

	UPROPERTY(BlueprintReadOnly, Category = "Test")
	FGameplayAttributeData TestPropertyTwo;
	ATTRIBUTE_ACCESSORS(UTestAttributeSet, TestPropertyTwo)
};
