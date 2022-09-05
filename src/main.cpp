#include <windows.h>
#include <SokuLib.hpp>
#include "main.hpp"
#include "palette-list.hpp"
#include "menu.hpp"

// global variable initialization
std::filesystem::path modulePath;
std::filesystem::path configPath;
#ifdef _DEBUG
std::ofstream logging("palette-picker.log");
#endif

PaletteInput* palettes[2] = {0, 0};
int loadId = 0;

namespace {
    auto orig_setPaletteSelect = reinterpret_cast<void (__fastcall*)(SokuLib::Select*, int, int, int, int, bool)>(0x41fe60);
    auto orig_copyPaletteAddr = reinterpret_cast<int (*)(char*, const char*, const char*, int)>(0x8571d8);
    auto orig_selectOnRender = reinterpret_cast<int (__fastcall*)(SokuLib::Select*)>(0x420c30);
    auto orig_profileOnUpdate = reinterpret_cast<bool (__fastcall*)(int)>(0x44d4a0);
    auto orig_loadMainMenu = reinterpret_cast<void* (__fastcall*)(int, int, int)>(0x41e420);

    template<int N> static inline void TamperCode(int addr, uint8_t (&code)[N]) {
        for (int i = 0; i < N; ++i) {
            uint8_t swap = *(uint8_t*)(addr+i);
            *(uint8_t*)(addr+i) = code[i];
            code[i] = swap;
        }
    }

    template <int S> static inline void TamperCode(int addr, const uint8_t (&code)[S]) {
        memcpy((void*)addr, code, S);
    }

    constexpr int guiOffset[2][2] = {
        {155, 90},
        {155, 370},
    };
}

static void* __fastcall loadMainMenu(int self, int a, int b) {
#ifdef _DEBUG
    logging << "loadMainMenu("<<b<<");" << std::endl;
#endif
    if (b == SokuLib::SCENE_TITLE) {
        if (palettes[0]) { palettes[0]->list; delete palettes[0]; }
        if (palettes[1]) { palettes[1]->list; delete palettes[1]; }
        palettes[0] = palettes[1] = 0; loadId = 0;
    } return orig_loadMainMenu(self, a, b);
}

static int __fastcall selectOnRender(SokuLib::Select* selectObj) {
    int ret = orig_selectOnRender(selectObj);

    for (int playerId = 0; playerId < 2; ++playerId) {
        if (!palettes[playerId]) continue;
        if (((unsigned char*)&selectObj->leftSelectionStage)[playerId] != 1) continue;
        if (playerId == 0 && selectObj->base.VTable == (void*)0x857534
            || playerId == 1 && selectObj->base.VTable == (void*)0x8574dc) continue;
        SokuLib::MenuCursor::render(guiOffset[playerId][0], guiOffset[playerId][1], 150);
        palettes[playerId]->render(guiOffset[playerId][0], guiOffset[playerId][1]);
    }

    return ret;
}

static bool __fastcall profileOnUpdate(int menu) {
    int currentChar = *(int*)(menu+0xa0);
    if (SokuLib::inputMgrs.input.c == 1 && currentChar >= 0) {
        SokuLib::playSEWaveBuffer(40);
        SokuLib::activateMenu(new PaletteMenu(currentChar));
        return true;
    }

    return orig_profileOnUpdate(menu);
}

static void __fastcall setPaletteSelect(SokuLib::Select* selectObj, int unused, int playerId, int charId, int paletteId, bool isFirst) {
#ifdef _DEBUG
    logging << "setPaletteSelect(0x"<<(void*)selectObj<<", "<<playerId<<", "<<charId<<", "<<paletteId<<", "<<isFirst<<");"<< std::endl;
#endif
    if (isFirst) {
        loadId = 0;
        if (palettes[playerId] && palettes[playerId]->list->charId != charId) {
            delete palettes[playerId]->list;
            delete palettes[playerId];
            palettes[playerId] = 0;
        }
        if (!palettes[playerId]) {
            auto name = SokuLib::union_cast<const char* (*)(int)>(0x43f3f0)(charId);
            wchar_t wname[24]; MultiByteToWideChar(CP_ACP, 0, name, -1, wname, 24);
            palettes[playerId] = new PaletteInput();
            palettes[playerId]->list = new PaletteList(charId, wname);
            palettes[playerId]->reset(((SokuLib::InputHandler*)&selectObj->leftSelect)[playerId*2+1].pos, palettes[playerId]->list->startingPalette);
        }
    }

    if (playerId == 0 && SokuLib::mainMode == SokuLib::BATTLE_MODE_VSSERVER
        || playerId == 1 && SokuLib::mainMode == SokuLib::BATTLE_MODE_VSCLIENT) {
        if (isFirst) {
            if (palettes[playerId]->list->opponentPalette == -1) {
                delete palettes[playerId];
                palettes[playerId] = 0;
            } else {
                palettes[playerId]->currentPalette = palettes[playerId]->list->opponentPalette;
                if (palettes[playerId]->currentPalette < 0) palettes[playerId]->currentPalette = 0;
            }
        } else if (palettes[playerId] && palettes[playerId]->list->opponentPalette == -2)
            palettes[playerId]->handleInput(((SokuLib::InputHandler*)&selectObj->leftSelect)[playerId*2+1].pos);
    } else palettes[playerId]->handleInput(((SokuLib::InputHandler*)&selectObj->leftSelect)[playerId*2+1].pos);

    return orig_setPaletteSelect(selectObj, unused, playerId, charId, paletteId, isFirst);
}

static bool __stdcall loadPaletteSelect(char* buffer, int playerId, const char* name, int paletteId) {
#ifdef _DEBUG
    logging << "loadPaletteSelect(0x"<<(void*)buffer<<", "<<playerId<<", "<<name<<", "<<paletteId<<");"<< std::endl;
#endif
    if (!palettes[playerId]) {
        wsprintf(buffer, "data/character/%s/palette%03d.bmp", name, paletteId);
        return false;
    }

    if (palettes[playerId]->list->useDefaults && palettes[playerId]->currentPalette < 8) {
        wsprintf(buffer, "data/character/%s/palette%03d.bmp", name, palettes[playerId]->currentPalette);
        return false;
    }

    int paletteIndex = palettes[playerId]->currentPalette;
    if (palettes[playerId]->list->useDefaults) paletteIndex -= 8;
    return loadPalette(palettes[playerId]->list->customPalettes[paletteIndex]);
}

static bool __stdcall loadPalettePattern(char* buffer, const char* name, int paletteId) {
#ifdef _DEBUG
    logging << "loadPalettePattern(0x"<<(void*)buffer<<", "<<name<<", "<<paletteId<<");"<< std::endl;
#endif
    int id = loadId++ % 2;
    if (id && palettes[0] && palettes[1] && palettes[0]->currentPalette == palettes[1]->currentPalette)
        palettes[1]->currentPalette = palettes[1]->currentPalette == 0 && palettes[1]->list->maxPalettes() ? 1 : 0;

    return loadPaletteSelect(buffer, id, name, paletteId);
}

static bool GetModulePath(HMODULE handle, std::filesystem::path& result) {
    // use wchar for better path handling
    std::wstring buffer;
    int len = MAX_PATH + 1;
    do {
        buffer.resize(len);
        len = GetModuleFileNameW(handle, buffer.data(), buffer.size());
    } while(len > buffer.size());

    if (len) result = std::filesystem::path(buffer.begin(), buffer.begin()+len).parent_path();
    return len;
}

const BYTE TARGET_HASH[16] = {0xdf, 0x35, 0xd1, 0xfb, 0xc7, 0xb5, 0x83, 0x31, 0x7a, 0xda, 0xbe, 0x8c, 0xd9, 0xf5, 0x3b, 0x2e};
extern "C" __declspec(dllexport) bool CheckVersion(const BYTE hash[16]) {
    return ::memcmp(TARGET_HASH, hash, sizeof TARGET_HASH) == 0;
}

extern "C" __declspec(dllexport) bool Initialize(HMODULE hMyModule, HMODULE hParentModule) {
    GetModulePath(hMyModule, modulePath);
    configPath = modulePath / L"palette-picker.ini";
#ifdef _DEBUG
    logging << "modulePath: " << modulePath << std::endl;
#endif

    DWORD old;
    VirtualProtect((LPVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, PAGE_WRITECOPY, &old);
    orig_setPaletteSelect = SokuLib::TamperNearCall(0x42087d, setPaletteSelect);
    orig_selectOnRender = SokuLib::TamperNearCall(0x421561, selectOnRender);
    TamperCode(0x41ff97, {
        0x57,                           // push EDI
        0x52,                           // push EDX
        0xE8, 0x00, 0x00, 0x00, 0x00,   // call xxx
        0x85, 0xC0,                     // test eax, eax
        0x75, 0x1a,                     // jnz skipload
        0x90, 0x90, 0x90, 0x90          // nop align
    }); SokuLib::TamperNearCall(0x41ff99, loadPaletteSelect);

    TamperCode(0x467aeb, {
        0x52,                               // push edx
        0xE8, 0x06, 0x00, 0x00, 0x00,       // call xxx
        0x85, 0xC0,                         // test eax, eax
        0x0f, 0x85, 0x89, 0x00, 0x00, 0x00, // jnz skipload
        0xB8, 0x00, 0x00, 0x00, 0x00,       // lea eax, "utsuho"
        0x8B, 0xCD,                         // mov ecx, ebp
    }); SokuLib::TamperNearCall(0x467aec, loadPalettePattern);
    SokuLib::TamperDword(0x467afa, "utsuho");
    VirtualProtect((LPVOID)TEXT_SECTION_OFFSET, TEXT_SECTION_SIZE, old, &old);

    VirtualProtect((LPVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, PAGE_WRITECOPY, &old);
    orig_profileOnUpdate = SokuLib::TamperDword((DWORD)0x859878, profileOnUpdate);
    orig_loadMainMenu = SokuLib::TamperDword((DWORD)0x861af0, loadMainMenu);
    VirtualProtect((LPVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, old, &old);

    return true;
}

extern "C" __declspec(dllexport) void AtExit() {}

BOOL WINAPI DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    return true;
}