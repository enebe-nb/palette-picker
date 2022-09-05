#pragma once

#include <filesystem>
#include <SokuLib.hpp>

class PaletteList {
public:
    const int charId;
    const std::wstring charName;
    int opponentPalette;
    int startingPalette;
    bool useDefaults;

    std::filesystem::path basePath;
    std::vector<std::filesystem::path> customPalettes;

    PaletteList(int charId, const std::wstring& charName);

    void loadConfig();
    void saveConfig();
    inline int maxPalettes() const { return customPalettes.size() + (useDefaults ? 8 : 0); }
    void setUseDefaults(bool value);
};

class PaletteInput {
public:
    PaletteList* list = 0;
    int currentPalette = 0;
    int currentInput = 0;
    SokuLib::Sprite* defaultsPalettesText[8] = {0,0,0,0,0,0,0,0};
    std::vector<SokuLib::Sprite*> customPalettesText;
    bool isOpponentMenu = false;
    SokuLib::Sprite* spDontForce = 0;
    SokuLib::Sprite* spAllowCustoms = 0;

    PaletteInput();
    ~PaletteInput();

    void reset(int pos, int starting);
    void handleInput(int pos);
    void getName(int pos, std::string& output);
    void render(int x, int y);

private:
    void _render(int pos, int x, int y);
};
