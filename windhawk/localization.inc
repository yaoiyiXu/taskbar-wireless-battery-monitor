// English is the fallback. Device protocol strings and keys are never translated.
namespace ui {
inline bool IsChinese() {
    return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_CHINESE;
}

inline const wchar_t* Text(const wchar_t* english, const wchar_t* chinese) {
    return IsChinese() ? chinese : english;
}
}  // namespace ui
