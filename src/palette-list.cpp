#include "palette-list.hpp"
#include "main.hpp"
#include <Windows.h>
#include <set>

namespace {
    const std::set<std::filesystem::path> allowedExtensions = { L".pal", L".act", L".bmp" };
    SokuLib::SWRFont* font = 0;
}

PaletteList::PaletteList(int charId, const std::wstring& charName)
    : charId(charId), charName(charName) {
    loadConfig();
    if (!font) {
        font = new SokuLib::SWRFont();
        font->create();
        font->setIndirect(SokuLib::FontDescription{
            "Tahoma",
            255,255,255,
            255,250,120,
            16, 400,
            false, true, false,
            0, 0, 0, 0, 0
        });
    }
}
PaletteList::~PaletteList() {
    for (int i = 0; i < 8; ++i) {
        auto& sprite = defaultsPalettesText[i];
        if (!sprite) continue;
        SokuLib::textureMgr.remove(sprite->dxHandle);
        delete sprite;
        sprite = 0;
    }

    for (auto& sprite : customPalettesText) {
        if (!sprite) continue;
        SokuLib::textureMgr.remove(sprite->dxHandle);
        delete sprite;
        sprite = 0;
    }
}

void PaletteList::loadConfig() {
    wchar_t buffer[512];
    useDefaults = GetPrivateProfileIntW(charName.c_str(), L"useDefaults", true, configPath.c_str());
    const std::wstring defaultPath = L"palettes/" + charName;
    GetPrivateProfileStringW(charName.c_str(), L"basePath", defaultPath.c_str(), buffer, 512, configPath.c_str());
    basePath = buffer;

    std::error_code err;
    const std::filesystem::path absPath = modulePath / basePath;
    if (!std::filesystem::exists(absPath)) std::filesystem::create_directories(absPath);
    for (auto const& entry : std::filesystem::recursive_directory_iterator(absPath, err)) {
        if (err) break;
        if (!entry.is_regular_file()) continue;
        if (allowedExtensions.contains(entry.path().extension())) {
            customPalettes.push_back(entry.path());
        }
    }

    GetPrivateProfileStringW(charName.c_str(), L"startingPalette", L"\\-1", buffer, 512, configPath.c_str());
    if (buffer[0] == L'\\') startingPalette = _wtoi(buffer+1);
    else {
        startingPalette = -1;
        for (int i = 0; i < customPalettes.size(); ++i) {
            if (customPalettes[i].stem() == buffer) {
                startingPalette = useDefaults ? i + 8 : i;
                break;
            }
        }
    }

    GetPrivateProfileStringW(charName.c_str(), L"opponentPalette", L"\\-1", buffer, 512, configPath.c_str());
    if (buffer[0] == L'\\') opponentPalette = _wtoi(buffer+1);
    else {
        opponentPalette = -1;
        for (int i = 0; i < customPalettes.size(); ++i) {
            if (customPalettes[i].stem() == buffer) {
                opponentPalette = useDefaults ? i + 8 : i;
                break;
            }
        }
    }
}

void PaletteList::saveConfig() {
    wchar_t buffer[32];
    WritePrivateProfileStringW(charName.c_str(), L"useDefaults", _itow(useDefaults, buffer, 10), configPath.c_str());
    WritePrivateProfileStringW(charName.c_str(), L"basePath", basePath.c_str(), configPath.c_str());

    std::wstring value;
    if (useDefaults) {
        if (startingPalette < 8) value = L'\\' + std::to_wstring(startingPalette);
        else value = customPalettes[startingPalette - 8].stem();
    } else {
        if (startingPalette < 0) value = L'\\' + std::to_wstring(startingPalette);
        else value = customPalettes[startingPalette].stem();
    } WritePrivateProfileStringW(charName.c_str(), L"startingPalette", value.c_str(), configPath.c_str());

    if (useDefaults) {
        if (opponentPalette < 8) value = L'\\' + std::to_wstring(opponentPalette);
        else value = customPalettes[opponentPalette - 8].stem();
    } else {
        if (opponentPalette < 0) value = L'\\' + std::to_wstring(opponentPalette);
        else value = customPalettes[opponentPalette].stem();
    } WritePrivateProfileStringW(charName.c_str(), L"opponentPalette", value.c_str(), configPath.c_str());
}

void PaletteList::reset(int pos) {
    currentInput = pos;
    currentPalette = startingPalette < 0 || startingPalette > maxPalettes() ? 0 : startingPalette;
}

void PaletteList::handleInput(int nextInput) {
    if (maxPalettes()) {
        if (nextInput == (currentInput+1)%8
            || nextInput == (currentInput+2)%8) currentPalette++;
        if (nextInput == (currentInput-1+8)%8
            || nextInput == (currentInput-2+8)%8) currentPalette--;
        if (currentPalette < 0) currentPalette += maxPalettes();
        else currentPalette = currentPalette % maxPalettes();
    }
    currentInput = nextInput;
}

void PaletteList::createTextTexture(int pos) {
    int max = maxPalettes(); if (!max) return;
    while (pos < 0) pos = (pos+max)%max;
    pos = pos%max;

    if (useDefaults)
        if (pos >= 8) pos -= 8;
        else if (defaultsPalettesText[pos]) return;
        else {
            char buffer[] = "Default 000"; buffer[10] = '0' + pos;
            defaultsPalettesText[pos] = new SokuLib::Sprite();
            int texId; SokuLib::textureMgr.createTextTexture(&texId, buffer, *font, 150, 20, 0, 0);
            defaultsPalettesText[pos]->setTexture2(texId, 0, 0, 150, 20);
            return;
        }

    if (customPalettesText.size() <= pos) customPalettesText.resize(pos+1, 0);
    if (customPalettesText[pos]) return;

    const std::string name = customPalettes[pos].stem().string();
    customPalettesText[pos] = new SokuLib::Sprite();
    int texId; SokuLib::textureMgr.createTextTexture(&texId, name.c_str(), *font, 150, 20, 0, 0);
    customPalettesText[pos]->setTexture2(texId, 0, 0, 150, 20);
}

void PaletteList::render(int pos, int x, int y) {
    int max = maxPalettes(); if (!max) return;
    while (pos < 0) pos = (pos+max)%max;
    pos = pos%max;

    if (useDefaults)
        if (pos >= 8) pos -= 8;
        else return defaultsPalettesText[pos]->render(x, y);

    customPalettesText[pos]->render(x, y);
}

void PaletteList::getName(int pos, std::string& output) {
    int max = maxPalettes(); if (!max) return;
    while (pos < 0) pos = (pos+max)%max;
    pos = pos%max;

    if (useDefaults)
        if (pos >= 8) pos -= 8;
        else {
            output = "Default 00"; output += '0' + pos;
            return;
        }

    output = customPalettes[pos].stem().string();
}
