import unreal


def open_ability_editor_helper_widget():
    """
    打开在 Settings 中配置的 AbilityEditorHelper EditorUtilityWidget。
    等同于 C++ 侧的 UAbilityEditorHelperLibrary::OpenEditorUtilityWidget()。
    """
    return unreal.AbilityEditorHelperLibrary.open_editor_utility_widget()
