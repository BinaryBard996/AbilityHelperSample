// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilityEditorHelperWidget.h"
#include "AbilityEditorHelperSettings.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

FString UAbilityEditorHelperWidget::GetGameplayEffectExcelName() const
{
	FString Name = GetDefault<UAbilityEditorHelperSettings>()->GameplayEffectExcelName;
	if (!Name.IsEmpty() && !Name.EndsWith(TEXT(".xlsx")))
	{
		Name += TEXT(".xlsx");
	}
	return Name;
}

FString UAbilityEditorHelperWidget::GetGameplayAbilityExcelName() const
{
	FString Name = GetDefault<UAbilityEditorHelperSettings>()->GameplayAbilityExcelName;
	if (!Name.IsEmpty() && !Name.EndsWith(TEXT(".xlsx")))
	{
		Name += TEXT(".xlsx");
	}
	return Name;
}

FString UAbilityEditorHelperWidget::GetGameplayEffectDataType() const
{
	return GetDefault<UAbilityEditorHelperSettings>()->GameplayEffectDataType;
}

FString UAbilityEditorHelperWidget::GetGameplayAbilityDataType() const
{
	return GetDefault<UAbilityEditorHelperSettings>()->GameplayAbilityDataType;
}

void UAbilityEditorHelperWidget::SaveGameplayEffectExcelName(const FString& InName)
{
	UAbilityEditorHelperSettings* Settings = GetMutableDefault<UAbilityEditorHelperSettings>();
	Settings->GameplayEffectExcelName = InName;
	Settings->SaveConfig();
}

void UAbilityEditorHelperWidget::SaveGameplayAbilityExcelName(const FString& InName)
{
	UAbilityEditorHelperSettings* Settings = GetMutableDefault<UAbilityEditorHelperSettings>();
	Settings->GameplayAbilityExcelName = InName;
	Settings->SaveConfig();
}

void UAbilityEditorHelperWidget::ShowEditorNotification(const FText& Message, float Duration)
{
	FNotificationInfo Info(Message);
	if (Duration > 0.f)
	{
		Info.bFireAndForget = true;
		Info.ExpireDuration = Duration;
	}
	else
	{
		Info.bFireAndForget = false;
		Info.bUseSuccessFailIcons = false;
	}
	FSlateNotificationManager::Get().AddNotification(Info);
}
