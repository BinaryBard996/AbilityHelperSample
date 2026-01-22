# -*- coding: utf-8 -*-
# ability_editor__consts.py
# 说明：集中存放基于固定结构（如 FGameplayEffectConfig）使用到的常量与示例数据。
# 若后续完全切换为 Schema 驱动并不再需要这些常量，可统一从此处移除。

# 显示名称与默认 Sheet 名称
STRUCT_DISPLAY_NAME = "FGameplayEffectConfig"
MAIN_SHEET_NAME = "FGameplayEffectConfig"
SUB_SHEET_NAME = "FGameplayEffectConfig.Modifiers"

# 主表字段（与UStruct中同名，Name为DataTable行名）
MAIN_FIELDS = [
    "Name",
    "DurationType",
    "DurationMagnitude",
    "Period",
    "StackingType",
    "StackLimitCount",
    "StackDurationRefreshPolicy",
    "StackPeriodResetPolicy",
    "AssetTags",    # 逗号分隔的标签：如 GameplayCue.Test, GameplayCue.X
    "GrantedTags",  # 逗号分隔的标签
]

# 子表字段（Modifiers）
SUB_FIELDS = [
    "ParentName",  # 关联主表的Name
    "Attribute",   # 形如 /Script/GameplayAbilities.AbilitySystemComponent:OutgoingDuration
    "ModifierOp",  # EGameplayModOp::Type 的名字，如 AddBase, Multiply, Override 等
    "MagnitudeCalculationType",  # EGameplayEffectMagnitudeCalculation 名称，如 ScalableFloat
    "Magnitude",   # 数值
]

# 平铺最大数量
MAX_FLAT_MODIFIERS = 5

# 一些枚举提示，写入模板第二行，便于填表
ENUM_HINTS = {
    "DurationType": "Instant | Duration | Infinite | HasDuration",
    "StackingType": "None | AggregateBySource | AggregateByTarget",
    "StackDurationRefreshPolicy": "RefreshOnSuccessfulApplication | NeverRefresh",
    "StackPeriodResetPolicy": "ResetOnSuccessfulApplication | NeverReset",
    "ModifierOp": "AddBase | Multiply | Override | Additive | Division | ...",
    "MagnitudeCalculationType": "ScalableFloat | AttributeBased | ...",
}

# 示例数据（演示两条，与样例基本一致）
SAMPLE_MAIN_ROWS = [
    {
        "Name": "Test1",
        "DurationType": "Instant",
        "DurationMagnitude": 0,
        "Period": 0,
        "StackingType": "None",
        "StackLimitCount": 1,
        "StackDurationRefreshPolicy": "RefreshOnSuccessfulApplication",
        "StackPeriodResetPolicy": "ResetOnSuccessfulApplication",
        "AssetTags": "GameplayCue.Test",
        "GrantedTags": "",
    },
    {
        "Name": "Test2",
        "DurationType": "Infinite",
        "DurationMagnitude": 0,
        "Period": 0,
        "StackingType": "None",
        "StackLimitCount": 1,
        "StackDurationRefreshPolicy": "RefreshOnSuccessfulApplication",
        "StackPeriodResetPolicy": "ResetOnSuccessfulApplication",
        "AssetTags": "",
        "GrantedTags": "",
    },
]

SAMPLE_SUB_ROWS = [
    {
        "ParentName": "Test1",
        "Attribute": "/Script/GameplayAbilities.AbilitySystemComponent:OutgoingDuration",
        "ModifierOp": "AddBase",
        "MagnitudeCalculationType": "ScalableFloat",
        "Magnitude": 100,
    },
    {
        "ParentName": "Test1",
        "Attribute": "/Script/GameplayAbilities.AbilitySystemComponent:IncomingDuration",
        "ModifierOp": "AddBase",
        "MagnitudeCalculationType": "ScalableFloat",
        "Magnitude": 0,
    },
]
