// Upstream driver baseline: bbb5c75f4aaca0adc85fd56fcf377b78ea680f4f, plus PR #43 (MCHOSE).
// Safety backports: 99c55b211bc9ab1d40e9c863c74f59480e0039b6 (digitizers/ignore),
// 1e07289997d91cbb67d00a879e39f2a26130de49 (Razer/Synapse).
// This is a selective backport, NOT a full update to upstream main.
namespace ModSafety {
std::mutex ignoredMutex;
std::set<std::pair<USHORT, USHORT>> ignored;

void ConfigureIgnored(const std::wstring& text) {
    std::set<std::pair<USHORT, USHORT>> parsed;
    size_t start = 0;
    while (start < text.size()) {
        size_t end = text.find_first_of(L",; \t\r\n", start);
        std::wstring token = text.substr(start, end == std::wstring::npos ? end : end - start);
        if (!token.empty()) {
            if (token.size() != 9 || token[4] != L':' ||
                token.find_first_not_of(L"0123456789abcdefABCDEF", 0) != 4 ||
                token.substr(5).find_first_not_of(L"0123456789abcdefABCDEF") != std::wstring::npos) {
                Wh_Log(L"Invalid ignored-device entry; polling paused until the setting is corrected.");
                // Fail closed rather than accidentally querying a mistyped exclusion.
                parsed.insert({0, 0});
            } else {
                parsed.insert({(USHORT)wcstoul(token.substr(0, 4).c_str(), nullptr, 16),
                               (USHORT)wcstoul(token.substr(5).c_str(), nullptr, 16)});
            }
        }
        if (end == std::wstring::npos) break;
        start = end + 1;
    }
    std::lock_guard<std::mutex> lock(ignoredMutex);
    ignored = std::move(parsed);
}

bool IsIgnored(USHORT vid, USHORT pid) {
    std::lock_guard<std::mutex> lock(ignoredMutex);
    return ignored.count({0, 0}) || ignored.count({vid, pid});
}

void Filter(std::vector<HidColl>& colls) {
    std::set<std::pair<USHORT, USHORT>> digitizers;
    for (const auto& c : colls)
        if (c.page() == 0x0D) digitizers.insert({c.vid, c.pid});
    colls.erase(std::remove_if(colls.begin(), colls.end(), [&](const HidColl& c) {
        return IsIgnored(c.vid, c.pid) || digitizers.count({c.vid, c.pid});
    }), colls.end());
}

bool KysonaKnown(USHORT vid, USHORT pid) {
    // IDs explicitly documented in upstream kysona.cpp at 666b7def119066d098c105805d68f9314af2fc64.
    return (vid == 0x3554 && (pid == 0xF55D || pid == 0xF5D5 || pid == 0xF55E ||
            pid == 0xF58A || pid == 0xF58C || pid == 0xF58E || pid == 0xF58F || pid == 0xF505 || pid == 0xF506)) ||
           (vid == 0x3770 && pid == 0x0200) || (vid == 0x3710 && pid == 0x5406);
}
} // namespace ModSafety
