#include "main.hpp"
#include <SokuLib.hpp>
#include <fstream>

static bool loadGamePalette(std::istream& input) {
    auto& palette = SokuLib::Palette::currentPalette;
    uint8_t bpp = input.get();
    if (bpp != palette.bitsPerPixel) {
        if (palette.data) SokuLib::DeleteFct(palette.data);
        palette.data = SokuLib::NewFct(bpp == 16 ? 512 : 1024);
        palette.bitsPerPixel = bpp;
    } else if (!palette.data) palette.data = SokuLib::NewFct(bpp == 16 ? 512 : 1024);

    input.read((char*)palette.data, palette.bitsPerPixel == 16 ? 512 : 1024);
    return true;
}

static bool loadPEPalette(std::istream& input) {
    auto& palette = SokuLib::Palette::currentPalette;
    if (GetModuleHandleA("true-color-palettes.dll")) {
        if (palette.bitsPerPixel == 16) {
            if (palette.data) SokuLib::DeleteFct(palette.data);
            palette.data = SokuLib::NewFct(1024);
        } else if (!palette.data) palette.data = SokuLib::NewFct(1024);
        palette.bitsPerPixel = 32;
        input.read((char*)palette.data, 1024);
    } else {
        palette.bitsPerPixel = 16;
        if (!palette.data) palette.data = SokuLib::NewFct(512);
        uint32_t buffer[256]; input.read((char*)buffer, 1024);
        for (int i = 0; i < 256; ++i) {
            uint16_t pcolor = i != 0;
            pcolor = (pcolor << 5) + ((buffer[i] >> 19) & 0x1F);
            pcolor = (pcolor << 5) + ((buffer[i] >> 11) & 0x1F);
            pcolor = (pcolor << 5) + ((buffer[i] >> 3) & 0x1F);
            ((uint16_t*)palette.data)[i] = pcolor;
        }
    }
    return true;
}

static bool loadActPalette(std::istream& input) {
    auto& palette = SokuLib::Palette::currentPalette;
    uint8_t buffer[256 * 3]; input.read((char*)buffer, 256 * 3);
    if (GetModuleHandleA("true-color-palettes.dll")) {
        if (palette.bitsPerPixel == 16) {
            if (palette.data) SokuLib::DeleteFct(palette.data);
            palette.data = SokuLib::NewFct(1024);
        } else if (!palette.data) palette.data = SokuLib::NewFct(1024);

        palette.bitsPerPixel = 32;
        for (int i = 0; i < 256; ++i) {
            ((int32_t*)palette.data)[i] =
                (i == 0 ? 0 : 0xff000000)
                | ((uint32_t)buffer[i*3] << 16)
                | ((uint32_t)buffer[i*3+1] << 8)
                | (uint32_t)buffer[i*3+2];
        }
    } else {
        palette.bitsPerPixel = 16;
        if (!palette.data) palette.data = SokuLib::NewFct(512);

        for (int i = 0; i < 256; ++i) {
            uint16_t pcolor = i != 0;
            pcolor = (pcolor << 5) + ((buffer[i*3] >> 3) & 0x1F);
            pcolor = (pcolor << 5) + ((buffer[i*3+1] >> 3) & 0x1F);
            pcolor = (pcolor << 5) + ((buffer[i*3+2] >> 3) & 0x1F);
            ((uint16_t*)palette.data)[i] = pcolor;
        }
    }
    return true;
}

static bool loadBitmapPalette(std::istream& input) {
    uint16_t magicNumber; input.read((char*)&magicNumber, 2);
    if (magicNumber != 0x4d42) return false;

    input.ignore(8);
    uint32_t dataOffset; input.read((char*)&dataOffset, 4);
    uint32_t headerSize; input.read((char*)&headerSize, 4);
    if (headerSize < 16) return false;

    uint16_t bitsPerPixel; uint32_t width, height;
    input.read((char*)&width, 4);
    input.read((char*)&height, 4);
    input.ignore(2);
    input.read((char*)&bitsPerPixel, 2);
    //resource.paddedWidth = (resource.width + 3) / 4 * 4;

    uint32_t compressionMethod, dataSize = 0;
    size_t rowSize = (bitsPerPixel * width + 31) / 32 * 4;
    if (headerSize > 16) {
        input.read((char*)&compressionMethod, 4);
        input.read((char*)&dataSize, 4);
        input.seekg(headerSize - 24, std::ios::cur);
    } if (!dataSize) {
        compressionMethod = 0;
        dataSize = rowSize * height;
    }

    if (bitsPerPixel <= 8) {
        // load from the palette table
        auto& palette = SokuLib::Palette::currentPalette;
        palette.bitsPerPixel = 16;
        if (!palette.data) palette.data = SokuLib::NewFct(512);

        const int maxColors = bitsPerPixel == 8 ? 256 : bitsPerPixel == 4 ? 16 : bitsPerPixel == 2 ? 4 : 1;
        for (int i = 0; i < maxColors; ++i) {
            uint32_t color; input.read((char*)&color, 4);
            uint16_t pcolor = i != 0;
            pcolor = (pcolor << 5) + ((color >> 19) & 0x1F);
            pcolor = (pcolor << 5) + ((color >> 11) & 0x1F);
            pcolor = (pcolor << 5) + ((color >> 3) & 0x1F);
            ((uint16_t*)palette.data)[i] = pcolor;
        } for (int i = maxColors; i < 256; ++i) ((uint16_t*)palette.data)[i] = 0x0000;
        return true;
    } else {
        // load from the image colors
        input.seekg(dataOffset);
        uint8_t* buffer = new uint8_t[dataSize];
        input.read((char*)buffer, dataSize);

        auto& palette = SokuLib::Palette::currentPalette;
        palette.bitsPerPixel = 16;
        if (bitsPerPixel == 16) {
            for (int i = 0; i < 256; ++i) {
                const int line = height - i/width - 1;
                if (line < 0) break;
                ((uint16_t*)palette.data)[i] = ((uint16_t*)buffer)[line*width + i%width];
            }
        } else if (bitsPerPixel == 32) {
            for (int i = 0; i < 256; ++i) {
                const int line = height - i/width - 1;
                if (line < 0) break;
                uint32_t color = ((uint32_t*)buffer)[line*width + i%width];
                uint16_t pcolor = i != 0;
                pcolor = (pcolor << 5) + ((color >> 19) & 0x1F);
                pcolor = (pcolor << 5) + ((color >> 11) & 0x1F);
                pcolor = (pcolor << 5) + ((color >> 3) & 0x1F);
                ((uint16_t*)palette.data)[i] = pcolor;
            }
        } else if (bitsPerPixel == 24) {
            for (int i = 0; i < 256; ++i) {
                const int line = height - i/width - 1;
                if (line < 0) break;
                uint8_t* color = buffer + (line*width + i%width)*3;
                uint16_t pcolor = i != 0;
                pcolor = (pcolor << 5) + ((color[0] >> 3) & 0x1F);
                pcolor = (pcolor << 5) + ((color[1] >> 3) & 0x1F);
                pcolor = (pcolor << 5) + ((color[2] >> 3) & 0x1F);
                ((uint16_t*)palette.data)[i] = pcolor;
            }
        } else { delete[] buffer; return false; }
        delete[] buffer;
        return true;
    }
    return false;
}

bool loadPalette(const std::filesystem::path& path) {
#ifdef _DEBUG
    logging << "loadPalette(\""<<path<<"\");"<< std::endl;
#endif
    if (path.extension() == L".pal") {
        std::ifstream input(path, std::ios::binary);
        uint8_t bpp = input.peek();
        if (bpp == 16 || bpp == 32) {
            input.seekg(0, std::ios::end);
            size_t size = input.tellg();
            input.seekg(0, std::ios::beg);
            if (bpp == 16 && size == 513 || bpp == 32 && size == 1025) {
                return loadGamePalette(input);
            }
        }

        return loadPEPalette(input);
    } else if (path.extension() == L".act") {
        std::ifstream input(path, std::ios::binary);
        return loadActPalette(input);
    } else if (path.extension() == L".bmp") {
        std::ifstream input(path, std::ios::binary);
        return loadBitmapPalette(input);
    }
}