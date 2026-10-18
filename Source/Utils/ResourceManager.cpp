#include "ResourceManager.h"
#include "../IO/File.h"
#include "../IO/Log.h"
#include "../Item/ItemInfoManager.h"
#include "../Proton/ProtonUtils.h"
#include "../Utils/ZLibUtils.h"
#include "Texture.h"

ResourceManager::ResourceManager() {}

ResourceManager::~ResourceManager()
{
    Kill();
}

BLImage* ResourceManager::LoadTileSheet(const string& path)
{
    if (path.empty())
        return nullptr;

    string fullPath = m_path + "/" + path;

    BLImage* pExistImg = IsTileSheetExists(path);
    if (pExistImg)
        return pExistImg;

    if (GetFileExtension(path) == "rttex")
    {
        File file;
        if (!file.Open(fullPath))
            return nullptr;

        uint32 fileSize = file.GetSize();
        uint8* pData = new uint8[fileSize];

        if (file.Read(pData, fileSize) != fileSize)
        {
            SAFE_DELETE_ARRAY(pData);
            file.Close();
            return nullptr;
        }

        BLImage* pImg = DecodeRTTex(pData, fileSize);

        SAFE_DELETE_ARRAY(pData);
        file.Close();

        if (!pImg)
            return nullptr;

        m_tileSheets[path] = pImg;
        return pImg;
    }

    BLImage* pImg = new BLImage();
    if (pImg->read_from_file(fullPath.c_str()) != BL_SUCCESS)
    {
        SAFE_DELETE(pImg);
        return nullptr;
    }

    m_tileSheets[path] = pImg;
    return pImg;
}

BLImage* ResourceManager::DecodeRTTex(uint8* pSrc, uint32 size)
{
    if (!pSrc || size < sizeof(Proton::rtpack_header))
        return nullptr;

    MemoryBuffer memBuffer(pSrc, size);
    uint8* pDecomp = nullptr;

    if (std::memcmp(pSrc, C_RTFILE_PACKAGE_HEADER, C_RTFILE_PACKAGE_HEADER_BYTE_SIZE) == 0)
    {
        Proton::rtpack_header rtPackHeader;
        memBuffer.Read(rtPackHeader);

        if (rtPackHeader.compressionType == 1)
        {
            pDecomp = zLibInflateToMemory(pSrc + memBuffer.GetOffset(), rtPackHeader.compressedSize,
                                          rtPackHeader.decompressedSize);
            if (!pDecomp)
                return nullptr;

            memBuffer = MemoryBuffer(pDecomp, rtPackHeader.decompressedSize);
        }
    }
    else if (std::memcmp(pSrc, C_RTFILE_TEXTURE_HEADER, C_RTFILE_PACKAGE_HEADER_BYTE_SIZE) != 0)
        return nullptr;

    Proton::rttex_header rtTexHeader;
    memBuffer.Read(rtTexHeader);

    Proton::rttex_mip_header texMip;
    memBuffer.Read(texMip);

    uint32 pixelBytes = rtTexHeader.width * rtTexHeader.height * 4;
    if (rtTexHeader.width <= 0 || rtTexHeader.height <= 0 || texMip.dataSize < pixelBytes)
    {
        SAFE_DELETE_ARRAY(pDecomp);
        return nullptr;
    }

    BLImage* pImg = new BLImage(rtTexHeader.width, rtTexHeader.height, BL_FORMAT_PRGB32);
    BLImageData imgData;
    pImg->get_data(&imgData);

    uint8* pTextureData = new uint8[texMip.dataSize];
    memBuffer.ReadRaw(pTextureData, texMip.dataSize);

    FlipVerticalWithPremultiply(pTextureData, rtTexHeader.width, rtTexHeader.height, rtTexHeader.bUsesAlpha);
    memcpy(imgData.pixel_data, pTextureData, pixelBytes);

    SAFE_DELETE_ARRAY(pTextureData);
    SAFE_DELETE_ARRAY(pDecomp);

    return pImg;
}

BLImage* ResourceManager::IsTileSheetExists(const string& path)
{
    auto it = m_tileSheets.find(path);
    if (it != m_tileSheets.end())
        return it->second;

    return nullptr;
}

void ResourceManager::Kill()
{
    for (auto& [_, pTex] : m_textures)
    {
        SAFE_DELETE(pTex);
    }

    for (auto& [_, pImg] : m_tileSheets)
    {
        SAFE_DELETE(pImg);
    }

    for (auto& [_, pFont] : m_fonts)
    {
        SAFE_DELETE(pFont);
    }

    m_tileSheets.clear();
    m_textures.clear();
    m_itemTextures.clear();
    m_fonts.clear();
}

Texture* ResourceManager::GetTexture(const string& path, int32 frameW, int32 frameH)
{
    Texture* pTex = nullptr;

    auto it = m_textures.find(path);
    if (it != m_textures.end())
    {
        pTex = it->second;
    }
    else
    {
        BLImage* pImg = GetTileSheet(path);
        if (!pImg)
            return nullptr;

        pTex = new Texture(pImg);
        m_textures[path] = pTex;
    }

    if (frameW > 0 || frameH > 0)
    {
        pTex->SetFrameSize(frameW, frameH);
    }

    return pTex;
}

Texture* ResourceManager::GetItemTexture(int32 itemID)
{
    auto it = m_itemTextures.find(itemID);
    if (it != m_itemTextures.end())
        return it->second;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    string path = pItem ? ("game/" + pItem->textureFile) : "game/tiles_page1.rttex";

    Texture* pTex = GetTexture(path);
    if (pTex)
    {
        m_itemTextures[itemID] = pTex;
    }

    if (pItem)
    {
        switch (pItem->storage)
        {
            case STORAGE_SINGLE_FRAME:
            case STORAGE_SMART_EDGE:
            case STORAGE_SMART_EDGE_HORIZ:
            case STORAGE_SMART_CLING:
            case STORAGE_SMART_EDGE_OUTER:
            case STORAGE_RANDOM:
            case STORAGE_SMART_EDGE_VERT:
            case STORAGE_SMART_EDGE_HORIZ_CAVE:
            case STORAGE_SMART_CLING2:
            case STORAGE_SMART_EDGE_DIAGON:
            {
                if (pItem->id == ITEM_ID_PRISMATIC_AURA || pItem->id == ITEM_ID_LEGENDARY_COMET_HAIR)
                    pTex->SetFrameSize(32, 32);
                else if (pItem->HasFlag2(ITEM_FLAG_LONG_HAND_ITEM64x32))
                    pTex->SetFrameSize(64, 32);
                else
                    pTex->SetFrameSize(32, 32);
                break;
            }
        }
    }

    return pTex;
}

BLImage* ResourceManager::GetTileSheet(const string& tileSheet)
{
    BLImage* pImg = IsTileSheetExists(tileSheet);
    if (pImg)
        return pImg;

    pImg = LoadTileSheet(tileSheet);
    if (!pImg && tileSheet != "game/tiles_page1.rttex")
        return GetTileSheet("game/tiles_page1.rttex");

    return pImg;
}

BLFont* ResourceManager::LoadFont(uint16 fontID, const string& path)
{
    if (path.empty())
        return nullptr;

    string fullPath = m_path + "/" + path;

    BLFontFace fontFace;
    if (fontFace.create_from_file(fullPath.c_str()) != BL_SUCCESS)
        return nullptr;

    BLFont* pFont = new BLFont();
    pFont->create_from_face(fontFace, 16.0f);

    m_fonts[fontID] = pFont;
    return pFont;
}

BLFont* ResourceManager::GetFont(uint16 fontID)
{
    auto it = m_fonts.find(fontID);
    if (it != m_fonts.end())
        return it->second;

    return nullptr;
}

void ResourceManager::FlipVerticalWithPremultiply(uint8* pPixelData, int32 width, int32 height, bool premultiply)
{
    uint32 rowBytes = width * 4;

    for (int32 y = 0; y < height / 2; ++y)
    {
        uint8* top = pPixelData + y * rowBytes;
        uint8* bottom = pPixelData + (height - 1 - y) * rowBytes;

        for (int32 x = 0; x < width; ++x)
        {
            uint8* t = top + x * 4;
            uint8* b = bottom + x * 4;

            for (int32 i = 0; i < 4; ++i)
            {
                std::swap(t[i], b[i]);
            }
        }
    }

    for (int32 y = 0; y < height; ++y)
    {
        uint8* row = pPixelData + y * rowBytes;
        for (int32 x = 0; x < width; ++x)
        {
            uint8* pixel = row + x * 4;

            std::swap(pixel[0], pixel[2]);

            if (premultiply)
            {
                uint8 alpha = pixel[3];

                if (alpha == 0)
                {
                    pixel[0] = 0;
                    pixel[1] = 0;
                    pixel[2] = 0;
                }
                else if (alpha < 255)
                {
                    pixel[0] = (pixel[0] * alpha + 127) / 255;
                    pixel[1] = (pixel[1] * alpha + 127) / 255;
                    pixel[2] = (pixel[2] * alpha + 127) / 255;
                }
            }
        }
    }
}

ResourceManager* GetResourceManager()
{
    return ResourceManager::GetInstance();
}