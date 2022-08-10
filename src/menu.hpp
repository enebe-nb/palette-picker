#pragma once

#include <SokuLib.hpp>
#include <IEffectManager.hpp>
#include "palette-list.hpp"

class PaletteMenu : SokuLib::IMenu {
public:
    PaletteList paletteList;
    SokuLib::SWRFont fontMenuItem;
    SokuLib::SWRFont fontMenuValue;
    SokuLib::MenuCursor cursor;
    SokuLib::MenuCursor cursorPalette;
    SokuLib::v2::EffectManager_Select manager;
    SokuLib::v2::EffectObjectBase* characterSprite = 0;
    int state = 0;

    SokuLib::Sprite menuItens[5];
    SokuLib::Sprite menuValues[3];

    PaletteMenu(int charId);
    virtual ~PaletteMenu();
    virtual void _() {}
    virtual int onProcess();
    virtual int onRender();

    void renderValues();
    void loadCurPalette();
};
