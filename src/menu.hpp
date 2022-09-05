#pragma once

#include <SokuLib.hpp>
#include <IEffectManager.hpp>
#include "palette-list.hpp"

class PaletteView {
    int x, y;
public:
    SokuLib::v2::EffectManager_Select manager;
    SokuLib::v2::EffectObjectBase* characterSprite = 0;
    SokuLib::Sprite characterBackground;

    PaletteView(int charId, int x, int y);
    ~PaletteView();

    void resetPaletteIndex(const PaletteList& list, int paletteIndex);
    void updatePaletteIndex(const PaletteList& list, int paletteIndex);
    void render();
};

class PaletteMenu : SokuLib::IMenu {
public:
    PaletteList paletteList;
    PaletteInput playerInput;
    PaletteInput opponentInput;
    PaletteView paletteView;
    SokuLib::SWRFont fontMenuItem;
    SokuLib::SWRFont fontMenuValue;
    SokuLib::MenuCursor cursor;
    SokuLib::MenuCursor cursorPalette;
    int state = 0;

    SokuLib::Sprite menuItens[5];
    SokuLib::Sprite menuValues[3];

    PaletteMenu(int charId);
    virtual ~PaletteMenu();
    virtual void _() {}
    virtual int onProcess();
    virtual int onRender();

    void renderValues();
};
