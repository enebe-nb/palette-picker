#pragma once
#include <filesystem>

extern std::filesystem::path modulePath;
extern std::filesystem::path configPath;
#ifdef _DEBUG
#include <fstream>
extern std::ofstream logging;
#endif

bool loadPalette(const std::filesystem::path& path);
