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

PaletteList* palettes[2] = {0, 0};
int loadId = 0;

namespace {
    auto orig_setPaletteSelect = reinterpret_cast<void (__fastcall*)(SokuLib::Select*, int, int, int, int, bool)>(0x41fe60);
    auto orig_copyPaletteAddr = reinterpret_cast<int (*)(char*, const char*, const char*, int)>(0x8571d8);
    auto orig_connectToGame = reinterpret_cast<int (__fastcall*)(int, int, int, int, int, int)>(0x415610);
    auto orig_selectOnEnter = reinterpret_cast<void (__fastcall*)(int, int, int)>(0x4222c0);
    auto orig_selectCLOnEnter = reinterpret_cast<void (__fastcall*)(int, int, int)>(0x428410);
    auto orig_selectSVOnEnter = reinterpret_cast<void (__fastcall*)(int, int, int)>(0x428410);
    auto orig_selectOnRender = reinterpret_cast<int (__fastcall*)(SokuLib::Select*)>(0x420c30);
    auto orig_profileOnUpdate = reinterpret_cast<bool (__fastcall*)(int)>(0x44d4a0);

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

static int __fastcall connectToGame(int a, int b, int c, int d, int e, int f) {
    if (palettes[0]) delete palettes[0];
    if (palettes[1]) delete palettes[1];
    palettes[0] = palettes[1] = 0; loadId = 0;
    return orig_connectToGame(a, b, c, d, e, f);
}

template <void(__fastcall*& ORIG)(int, int, int)>
static void __fastcall selectOnEnter(int self, int a, int b) {
    if (palettes[0]) delete palettes[0];
    if (palettes[1]) delete palettes[1];
    palettes[0] = palettes[1] = 0; loadId = 0;
    return ORIG(self, a, b);
}

static int __fastcall selectOnRender(SokuLib::Select* selectObj) {
    int ret = orig_selectOnRender(selectObj);

    for (int playerId = 0; playerId < 2; ++playerId) {
        if (!palettes[playerId]) continue;
        if (((unsigned char*)&selectObj->leftSelectionStage)[playerId] != 1) continue;
        if (playerId == 0 && selectObj->base.VTable == (void*)0x857534
            || playerId == 1 && selectObj->base.VTable == (void*)0x8574dc) continue;
        SokuLib::MenuCursor::render(guiOffset[playerId][0], guiOffset[playerId][1], 150);
        palettes[playerId]->render(palettes[playerId]->currentPalette - 2, guiOffset[playerId][0] - 12, guiOffset[playerId][1] - 36);
        palettes[playerId]->render(palettes[playerId]->currentPalette - 1, guiOffset[playerId][0] -  5, guiOffset[playerId][1] - 18);
        palettes[playerId]->render(palettes[playerId]->currentPalette    , guiOffset[playerId][0]     , guiOffset[playerId][1]     );
        palettes[playerId]->render(palettes[playerId]->currentPalette + 1, guiOffset[playerId][0] -  5, guiOffset[playerId][1] + 18);
        palettes[playerId]->render(palettes[playerId]->currentPalette + 2, guiOffset[playerId][0] - 12, guiOffset[playerId][1] + 36);
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
    // disable for opponent in netplay
    if (playerId == 0 && selectObj->base.VTable == (void*)0x857534
        || playerId == 1 && selectObj->base.VTable == (void*)0x8574dc) {
        if (isFirst) {
            auto name = SokuLib::union_cast<const char* (*)(int)>(0x43f3f0)(charId);
            wchar_t wname[24]; MultiByteToWideChar(CP_ACP, 0, name, -1, wname, 24);
            palettes[playerId] = new PaletteList(charId, wname);
            if (palettes[playerId]->opponentPalette == -1) {
                delete palettes[playerId];
                palettes[playerId] = 0;
            } else {
                palettes[playerId]->currentPalette = palettes[playerId]->opponentPalette;
            }
        }
        return orig_setPaletteSelect(selectObj, unused, playerId, charId, paletteId, isFirst);
    }

    if (isFirst) { loadId = 0;
        auto name = SokuLib::union_cast<const char* (*)(int)>(0x43f3f0)(charId);
        wchar_t wname[24]; MultiByteToWideChar(CP_ACP, 0, name, -1, wname, 24);
        if (palettes[playerId]) delete palettes[playerId];
        palettes[playerId] = new PaletteList(charId, wname);
        palettes[playerId]->reset(((SokuLib::InputHandler*)&selectObj->leftSelect)[playerId*2+1].pos);
    } else {
        palettes[playerId]->handleInput(((SokuLib::InputHandler*)&selectObj->leftSelect)[playerId*2+1].pos);
    }

    for (int i = palettes[playerId]->currentPalette - 2; i <= palettes[playerId]->currentPalette + 2; ++i) {
        palettes[playerId]->createTextTexture(i);
    }

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

    if (palettes[playerId]->useDefaults && palettes[playerId]->currentPalette < 8) {
        wsprintf(buffer, "data/character/%s/palette%03d.bmp", name, palettes[playerId]->currentPalette);
        return false;
    }

    int paletteIndex = palettes[playerId]->currentPalette;
    if (palettes[playerId]->useDefaults) paletteIndex -= 8;
    return loadPalette(palettes[playerId]->customPalettes[paletteIndex]);
}

static bool __stdcall loadPalettePattern(char* buffer, const char* name, int paletteId) {
#ifdef _DEBUG
    logging << "loadPalettePattern(0x"<<(void*)buffer<<", "<<name<<", "<<paletteId<<");"<< std::endl;
#endif
    return loadPaletteSelect(buffer, loadId++ % 2, name, paletteId);
}

static bool GetModulePath(HMODULE handle, std::filesystem::path& result) {
    // use wchar for better path handling
    std::wstring buffer;
    int len = MAX_PATH + 1;
    do {
        buffer.resize(len);
        len = GetModuleFileNameW(handle, buffer.data(), buffer.size());
    } while(len > buffer.size());

    if (len) result = std::filesystem::proximate(std::filesystem::path(buffer.begin(), buffer.begin()+len).parent_path());
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
    orig_connectToGame = SokuLib::TamperNearCall(0x446cc0, connectToGame);
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
    orig_selectOnEnter = SokuLib::TamperDword((DWORD)&SokuLib::VTable_Select.unknown2, selectOnEnter<orig_selectOnEnter>);
    orig_selectCLOnEnter = SokuLib::TamperDword((DWORD)&SokuLib::VTable_SelectClient.unknown2, selectOnEnter<orig_selectCLOnEnter>);
    orig_selectSVOnEnter = SokuLib::TamperDword((DWORD)&SokuLib::VTable_SelectServer.unknown2, selectOnEnter<orig_selectSVOnEnter>);
    orig_profileOnUpdate = SokuLib::TamperDword((DWORD)0x859878, profileOnUpdate);
    VirtualProtect((LPVOID)RDATA_SECTION_OFFSET, RDATA_SECTION_SIZE, old, &old);

    return true;
}

extern "C" __declspec(dllexport) void AtExit() {}

BOOL WINAPI DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    return true;
}