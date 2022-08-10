#pragma once

#include <filesystem>
#include <SokuLib.hpp>

class PaletteList {
public:
    const int charId;
    const std::wstring charName;
    int opponentPalette;
    int startingPalette;
    int currentPalette = 0;
    int currentInput = 0;
    bool useDefaults;

    std::filesystem::path basePath;
    std::vector<std::filesystem::path> customPalettes;
    SokuLib::Sprite* defaultsPalettesText[8] = {0,0,0,0,0,0,0,0};
    std::vector<SokuLib::Sprite*> customPalettesText;

    void loadConfig();
    void saveConfig();
    inline int maxPalettes() const { return customPalettes.size() + (useDefaults ? 8 : 0); }

    void reset(int pos);
    void handleInput(int pos);
    void createTextTexture(int pos);
    void render(int pos, int x, int y);
    void getName(int pos, std::string& output);

    PaletteList(int charId, const std::wstring& charName);
    ~PaletteList();
};
