#include "menu.hpp"
#include "main.hpp"

static wchar_t* getWCharName(int charId) {
    auto name = SokuLib::union_cast<const char* (*)(int)>(0x43f3f0)(charId);
    static wchar_t wname[24]; MultiByteToWideChar(CP_ACP, 0, name, -1, wname, 24);
    return wname;
}

PaletteMenu::PaletteMenu(int charId) : paletteList(charId, getWCharName(charId)) {
    fontMenuItem.create();
    fontMenuItem.setIndirect(SokuLib::FontDescription {
        "Tahoma",
        255,255,255,
        255,255,255,
        16, 600,
        false, true, false,
        0, 0, 0, 0, 0
    });

    fontMenuValue.create();
    fontMenuValue.setIndirect(SokuLib::FontDescription {
        "Tahoma",
        255,255,255,
        255,250,120,
        16, 400,
        false, true, false,
        0, 0, 0, 0, 0
    });

    int texId;
    SokuLib::textureMgr.createTextTexture(&texId, "Starting Palette", fontMenuItem, 200, 30, 0, 0);
    menuItens[0].setTexture2(texId, 0, 0, 200, 30);
    SokuLib::textureMgr.createTextTexture(&texId, "Opponent Palette", fontMenuItem, 200, 30, 0, 0);
    menuItens[1].setTexture2(texId, 0, 0, 200, 30);
    SokuLib::textureMgr.createTextTexture(&texId, "Hide Defaults", fontMenuItem, 200, 30, 0, 0);
    menuItens[2].setTexture2(texId, 0, 0, 200, 30);
    SokuLib::textureMgr.createTextTexture(&texId, "Show Palette Folder", fontMenuItem, 200, 30, 0, 0);
    menuItens[3].setTexture2(texId, 0, 0, 200, 30);
    SokuLib::textureMgr.createTextTexture(&texId, "Exit", fontMenuItem, 200, 30, 0, 0);
    menuItens[4].setTexture2(texId, 0, 0, 200, 30);

    renderValues();
    cursor.set(&SokuLib::inputMgrs.input.verticalAxis, 5);
    cursorPalette.set(&SokuLib::inputMgrs.input.verticalAxis, 8);
}

PaletteMenu::~PaletteMenu() {
    for (int i = 0; i < 5; ++i) if (menuItens[i].dxHandle) SokuLib::textureMgr.remove(menuItens[i].dxHandle);
    for (int i = 0; i < 3; ++i) if (menuValues[i].dxHandle) SokuLib::textureMgr.remove(menuValues[i].dxHandle);
    manager.ClearPattern();
}

void PaletteMenu::renderValues() {
    int texId;
    for (int i = 0; i < 3; ++i) if (menuValues[i].dxHandle) SokuLib::textureMgr.remove(menuValues[i].dxHandle);

    std::string value = "Default 000";
    if (paletteList.startingPalette >= 0) paletteList.getName(paletteList.startingPalette, value);
    SokuLib::textureMgr.createTextTexture(&texId, value.c_str(), fontMenuValue, 200, 30, 0, 0);
    menuValues[0].setTexture2(texId, 0, 0, 200, 30);
    value = "Don't Force";
    if (paletteList.opponentPalette >= 0) paletteList.getName(paletteList.opponentPalette, value);
    SokuLib::textureMgr.createTextTexture(&texId, value.c_str(), fontMenuValue, 200, 30, 0, 0);
    menuValues[1].setTexture2(texId, 0, 0, 200, 30);
    value = paletteList.useDefaults ? "Show" : "Hide";
    SokuLib::textureMgr.createTextTexture(&texId, value.c_str(), fontMenuValue, 200, 30, 0, 0);
    menuValues[2].setTexture2(texId, 0, 0, 200, 30);
}

void PaletteMenu::loadCurPalette() {
    for (int i = paletteList.currentPalette - 2; i <= paletteList.currentPalette + 2; ++i) {
        paletteList.createTextTexture(i);
    }

    int paletteIndex = paletteList.currentPalette;
    if (paletteList.useDefaults) {
        if (paletteIndex < 8) {
            char paletteFile[64]; sprintf(paletteFile, "data/character/%s/palette%03d.bmp", SokuLib::union_cast<const char* (*)(int)>(0x43f3f0)(paletteList.charId), paletteIndex);
            SokuLib::union_cast<const char* (__fastcall*)(int, int, const char*, int, int)>(0x408be0)(0x8a0048, 0, paletteFile, 0x896b88, 16);
        } else loadPalette(paletteList.customPalettes[paletteIndex - 8]);
    } else  loadPalette(paletteList.customPalettes[paletteIndex]);

    char patternFile[64]; std::sprintf(patternFile, "data/scene/select/character/%s/stand.pat", SokuLib::union_cast<const char* (*)(int)>(0x43f3f0)(paletteList.charId));
    manager.ClearPattern();
    manager.LoadPattern(patternFile, 0);
}

int PaletteMenu::onProcess() {
    if (state == 0) {
        if (cursor.update()) {
            SokuLib::playSEWaveBuffer(39);
        }

        if (SokuLib::inputMgrs.input.a == 1) {
            switch (cursor.pos) {
            case 0: case 1:
                SokuLib::playSEWaveBuffer(40); state = cursor.pos + 1;
                paletteList.reset(cursorPalette.pos);
                loadCurPalette();
                characterSprite = manager.CreateEffect(0, 440, 320, SokuLib::Direction::RIGHT, 0, 0);
                characterSprite->setPose(0);
                return true;
            case 2:
                if (paletteList.customPalettes.size()) {
                    SokuLib::playSEWaveBuffer(40);
                    paletteList.useDefaults = !paletteList.useDefaults;
                } else paletteList.useDefaults = true;
                paletteList.saveConfig();
                renderValues();
                return true;
            case 3:{
                SokuLib::playSEWaveBuffer(40);
                auto path = modulePath / paletteList.basePath;
                auto result = ShellExecuteW(0, L"explore", path.c_str(), 0, 0, SW_SHOWNORMAL);
                path.c_str();
            }return true;
            case 4:
                SokuLib::playSEWaveBuffer(41);
                return false;
            }
        } else if (SokuLib::inputMgrs.input.b == 1) {
            SokuLib::playSEWaveBuffer(41);
            return false;
        } else if (SokuLib::inputMgrs.input.c == 1) {
            if (cursor.pos < 2) {
                SokuLib::playSEWaveBuffer(40);
                if (cursor.pos == 0) paletteList.startingPalette = -1;
                else if (cursor.pos == 1) paletteList.opponentPalette = -1;
                paletteList.saveConfig();
                renderValues();
            }
        }
    } else {
        manager.Update();

        if (cursorPalette.update()) {
            paletteList.handleInput(cursorPalette.pos);
            short poseId = characterSprite->frameState.poseId;
            short frame = characterSprite->frameState.poseFrame;
            loadCurPalette();
            characterSprite = manager.CreateEffect(0, 440, 320, SokuLib::Direction::RIGHT, 0, 0);
            characterSprite->setPose(poseId);
            characterSprite->frameState.poseFrame = frame;
        }

        if (SokuLib::inputMgrs.input.b == 1) {
            SokuLib::playSEWaveBuffer(41);
            manager.ClearEffects();
            state = 0;
        } else if (SokuLib::inputMgrs.input.a == 1) {
            SokuLib::playSEWaveBuffer(40);
            if (state == 1) paletteList.startingPalette = paletteList.currentPalette;
            else if (state == 2) paletteList.opponentPalette = paletteList.currentPalette;
            paletteList.saveConfig();
            renderValues();
            manager.ClearEffects();
            state = 0;
        } else if (SokuLib::inputMgrs.input.c == 1) {
            SokuLib::playSEWaveBuffer(40);
            if (state == 1) paletteList.startingPalette = -1;
            else if (state == 2) paletteList.opponentPalette = -1;
            paletteList.saveConfig();
            renderValues();
            manager.ClearEffects();
            state = 0;
        }
    }

    return true;
}

int PaletteMenu::onRender() {
    if (state == 0) SokuLib::MenuCursor::render(80, 112 + cursor.pos*40, 200);
    else {
        SokuLib::MenuCursor::render(390, 150, 150);
        paletteList.render(paletteList.currentPalette - 2, 368, 114);
        paletteList.render(paletteList.currentPalette - 1, 385, 132);
        paletteList.render(paletteList.currentPalette    , 390, 150);
        paletteList.render(paletteList.currentPalette + 1, 385, 168);
        paletteList.render(paletteList.currentPalette + 2, 368, 186);
        manager.Render(0);
    }
    for (int i = 0; i < 5; ++i) menuItens[i].render(80, 110 + i*40);
    for (int i = 0; i < 3; ++i) menuValues[i].render(100, 130 + i*40);
    return true;
}