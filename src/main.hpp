#pragma once
#include <filesystem>
#include <windows.h>

extern std::filesystem::path modulePath;
extern std::filesystem::path configPath;
#ifdef _DEBUG
#include <fstream>
extern std::ofstream logging;
#endif

bool loadPalette(const std::filesystem::path& path);

inline std::string ws2s(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(GetACP(), 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(GetACP(), 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
