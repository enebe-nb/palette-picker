#include "menu.hpp"
#include "main.hpp"

static wchar_t* getWCharName(int charId) {
    auto name = SokuLib::union_cast<const char* (*)(int)>(0x43f3f0)(charId);
    static wchar_t wname[24]; MultiByteToWideChar(CP_ACP, 0, name, -1, wname, 24);
    return wname;
}

PaletteMenu::PaletteMenu(int charId) : paletteList(charId, getWCharName(charId)), paletteView(charId, 440, 300) {
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

    opponentInput.isOpponentMenu = true;
    playerInput.list = opponentInput.list = &paletteList;
    playerInput.reset(0, paletteList.startingPalette);
    opponentInput.reset(0, paletteList.opponentPalette);

    renderValues();
    cursor.set(&SokuLib::inputMgrs.input.verticalAxis, 5);
    cursorPalette.set(&SokuLib::inputMgrs.input.verticalAxis, 8);
}

PaletteMenu::~PaletteMenu() {
    for (int i = 0; i < 5; ++i) if (menuItens[i].dxHandle) SokuLib::textureMgr.remove(menuItens[i].dxHandle);
    for (int i = 0; i < 3; ++i) if (menuValues[i].dxHandle) SokuLib::textureMgr.remove(menuValues[i].dxHandle);
}

void PaletteMenu::renderValues() {
    int texId;
    for (int i = 0; i < 3; ++i) if (menuValues[i].dxHandle) SokuLib::textureMgr.remove(menuValues[i].dxHandle);

    std::string value = "Default 000";
    if (paletteList.startingPalette >= 0) playerInput.getName(paletteList.startingPalette, value);
    SokuLib::textureMgr.createTextTexture(&texId, value.c_str(), fontMenuValue, 200, 30, 0, 0);
    menuValues[0].setTexture2(texId, 0, 0, 200, 30);

    opponentInput.getName(paletteList.opponentPalette, value);
    SokuLib::textureMgr.createTextTexture(&texId, value.c_str(), fontMenuValue, 200, 30, 0, 0);
    menuValues[1].setTexture2(texId, 0, 0, 200, 30);

    value = paletteList.useDefaults ? "Show" : "Hide";
    SokuLib::textureMgr.createTextTexture(&texId, value.c_str(), fontMenuValue, 200, 30, 0, 0);
    menuValues[2].setTexture2(texId, 0, 0, 200, 30);
}

PaletteView::PaletteView(int charId, int x, int y) : x(x), y(y) {
    static char cicleRes[] = "data/scene/select/character/08b_circle/circle_00.bmp";
    cicleRes[46] = '0' + charId/10; cicleRes[47] = '0' + charId%10;
    int texId, w, h;
    SokuLib::textureMgr.loadTexture(&texId, cicleRes, &w, &h);
    characterBackground.setTexture(texId, 0, 0, w, h, w/2, h/2);
}

PaletteView::~PaletteView() {
    if (characterBackground.dxHandle) SokuLib::textureMgr.remove(characterBackground.dxHandle);
    manager.ClearPattern();
}

void PaletteView::resetPaletteIndex(const PaletteList& list, int paletteIndex) {
    if (paletteIndex < 0) paletteIndex = 0;

    if (list.useDefaults) {
        if (paletteIndex < 8) {
            char paletteFile[64]; sprintf(paletteFile, "data/character/%s/palette%03d.bmp", SokuLib::union_cast<const char* (*)(int)>(0x43f3f0)(list.charId), paletteIndex);
            SokuLib::union_cast<const char* (__fastcall*)(int, int, const char*, int, int)>(0x408be0)(0x8a0048, 0, paletteFile, 0x896b88, 16);
        } else loadPalette(list.customPalettes[paletteIndex - 8]);
    } else loadPalette(list.customPalettes[paletteIndex]);

    char patternFile[64]; std::sprintf(patternFile, "data/scene/select/character/%s/stand.pat", SokuLib::union_cast<const char* (*)(int)>(0x43f3f0)(list.charId));
    manager.ClearPattern();
    manager.LoadPattern(patternFile, 0);
    characterSprite = manager.CreateEffect(0, this->x, this->y+50, SokuLib::Direction::RIGHT, 0, 0);
    characterSprite->setPose(0);
}

void PaletteView::updatePaletteIndex(const PaletteList& list, int paletteIndex) {
    short poseId = characterSprite->frameState.poseId;
    short frame = characterSprite->frameState.poseFrame;
    resetPaletteIndex(list, paletteIndex);
    characterSprite->setPose(poseId);
    characterSprite->frameState.poseFrame = frame;
}

void PaletteView::render() {
    reinterpret_cast<void (__fastcall*)(int, int, int)>(0x404b80)(0x896b4c, 0, 2);
    characterBackground.render(this->x, this->y);
    reinterpret_cast<void (__fastcall*)(int, int, int)>(0x404b80)(0x896b4c, 0, 1);
    manager.Render(0);
}

int PaletteMenu::onProcess() {
    if (state == 0) {
        if (cursor.update()) {
            SokuLib::playSEWaveBuffer(39);
        }

        if (SokuLib::inputMgrs.input.a == 1) {
            switch (cursor.pos) {
            case 0:
                SokuLib::playSEWaveBuffer(40); state = 1;
                playerInput.reset(cursorPalette.pos, paletteList.startingPalette);
                paletteView.resetPaletteIndex(paletteList, paletteList.startingPalette);
                return true;
            case 1:
                SokuLib::playSEWaveBuffer(40); state = 2;
                opponentInput.reset(cursorPalette.pos, paletteList.opponentPalette);
                paletteView.resetPaletteIndex(paletteList, paletteList.opponentPalette);
                return true;
            case 2:
                if (paletteList.customPalettes.size()) {
                    SokuLib::playSEWaveBuffer(40);
                    paletteList.setUseDefaults(!paletteList.useDefaults);
                } else paletteList.useDefaults = true;
                paletteList.saveConfig();
                renderValues();
                return true;
            case 3: {
                SokuLib::playSEWaveBuffer(40);
                auto path = modulePath / paletteList.basePath;
                auto result = ShellExecuteW(0, L"explore", path.c_str(), 0, 0, SW_SHOWNORMAL);
                path.c_str();
            } return true;
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
        paletteView.manager.Update();
        ++paletteView.characterBackground.rotation;

        if (cursorPalette.update()) {
            if (state == 1) {
                playerInput.handleInput(cursorPalette.pos);
                paletteView.updatePaletteIndex(paletteList, playerInput.currentPalette);
            } else {
                opponentInput.handleInput(cursorPalette.pos);
                paletteView.updatePaletteIndex(paletteList, opponentInput.currentPalette);
            }
        }

        if (SokuLib::inputMgrs.input.b == 1) {
            SokuLib::playSEWaveBuffer(41);
            paletteView.manager.ClearEffects();
            state = 0;
        } else if (SokuLib::inputMgrs.input.a == 1) {
            SokuLib::playSEWaveBuffer(40);
            if (state == 1) paletteList.startingPalette = playerInput.currentPalette;
            else if (state == 2) paletteList.opponentPalette = opponentInput.currentPalette;
            paletteList.saveConfig();
            renderValues();
            paletteView.manager.ClearEffects();
            state = 0;
        } else if (SokuLib::inputMgrs.input.c == 1) {
            SokuLib::playSEWaveBuffer(40);
            if (state == 1) paletteList.startingPalette = -1;
            else if (state == 2) paletteList.opponentPalette = -1;
            paletteList.saveConfig();
            renderValues();
            paletteView.manager.ClearEffects();
            state = 0;
        }
    }

    return true;
}

int PaletteMenu::onRender() {
    if (state == 0) SokuLib::MenuCursor::render(80, 112 + cursor.pos*40, 200);
    else if (state == 1) { playerInput.render(390, 150); paletteView.render(); }
    else if (state == 2) { opponentInput.render(390, 150); paletteView.render(); }

    for (int i = 0; i < 5; ++i) menuItens[i].render(80, 110 + i*40);
    for (int i = 0; i < 3; ++i) menuValues[i].render(100, 130 + i*40);
    return true;
}