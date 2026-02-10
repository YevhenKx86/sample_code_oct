#pragma once
#include "oct_vars.h"

//In pack this is the first data, starting at offset 0 
typedef struct 
{
    uint8_t         Magic[4];                       //Incorporates pack format version into itself
    uint32_t        Crc;
    uint64_t        Guid;
    uint32_t        Version;
    uint32_t        Size;
    uint8_t         Title[OCT_ASSET_NAME_MAXLEN];
    uint32_t        Colors;
    uint32_t        AssetsCount;
    uint32_t        AssetsOffset;
    uint32_t        CodeOffset;
} octPackHeader_t;

//Description of single asset; array of these follow the pack header
typedef struct 
{
    char            Name[OCT_ASSET_NAME_MAXLEN];
    uint32_t        Encoding;
    uint32_t        Offset;                         //Raw data of this asset
    uint32_t        Size;
} octAssetDesc_t;

uint8_t                     OctPackBuf[OCT_PACK_CAP];                          //Reserved place in memory to load entire pack into it
const uint8_t*              OctPack;

octPackHeader_t*            OctPackHeader;                                  //Points to the pack description when the pack is loaded
octAssetDesc_t*             OctPackAssets;                                  //Points to the desc of the first asset when the pack is loaded
const char                  OctPackAssetNotFoundName[] = "ASSET_NOT_FOUND";



const char* OCT_PACK_getNameBySpriteId(uint32_t id)                  
    { 
        if (OctPackHeader == NULL || OctPackAssets == NULL || id >= OctPackHeader->AssetsCount) return OctPackAssetNotFoundName;
        return OctPackAssets[id].Name;
    }


void* OCT_PACK_getSprite(uint32_t id)
    {
        //[TODO]: return placeholder asset
        if (OctPackHeader == NULL || OctPackAssets == NULL || id >= OctPackHeader->AssetsCount) { OCT_terminate("getSprite OOB"); return NULL; }

        //First byte of raw asset
        return (void*)(OctPack + OctPackAssets[id].Offset);
    }


int OCT_PACK_getSpriteIdByName(const char* name)
    {
        //No pack loaded
        if (OctPackHeader == NULL || OctPackAssets == NULL) return -1;

        for (uint32_t i = 0; i < OctPackHeader->AssetsCount; i++)
            if (!strncmp(OctPackAssets[i].Name, name, OCT_ASSET_NAME_MAXLEN)) return i;
        return -1;
    }


//Get bitmap description
const octBmp_t* OCT_BMP_get(uint32_t bmp_idx)
    {
        const octBmp_t* bmp = (octBmp_t*)OCT_PACK_getSprite(bmp_idx);
        return bmp;
    }


//Put bitmap description into given container
void OCT_BMP_info(uint32_t bmp_idx, octBmpInfo_t* info)
    {
        //Extract data
        memset(info, 0, sizeof(octBmpInfo_t));
        const octBmp_t* bmp = OCT_BMP_get(bmp_idx);
        if (bmp == NULL) return;

        //Screen size multiplier
        uint8_t s = (bmp->Flags & OCT_FLAG_FULLSIZE) ? 1 : 2;

        //Fill structure
        OCT_strcpy(info->Name, OCT_ASSET_NAME_MAXLEN, OCT_PACK_getNameBySpriteId(bmp_idx));
        info->PivotX = bmp->PivotX,  info->PivotY = bmp->PivotY,  info->Tags = bmp->Tags,  info->W = s * bmp->W,  info->H = s * bmp->H,  info->Bx = bmp->Bx,  info->By = bmp->By,  info->Bw = bmp->Bw,  info->Bh = bmp->Bh,  info->NumPixels = bmp->NumPixels;
        info->Number = bmp->Number,  info->Group = bmp->Group,  info->Type = bmp->Type;
    }


int OCT_PACK_getAssetId(const char* name)
    {
        (void)name;
#ifdef OCTSIM
        ///for (int i = 0; i < soundsCount; i++)
        ///    if (!strncmp(cacheSound[i].Alias, name, OCT_ASSET_NAME_MAXLEN)) return i;
        ///if (name != NULL) OCT_terminate("Bad sound name");
#endif
        return -1;
    }


int OCT_PACK_play(int id, int volume)
    {
        //Allow single sound source
        //if (!OCT_is_leader()) return 0;


        //Assert
        ///if (id < 0 || id >= soundsCount) OCT_terminate("Bad sound id");
        /*
        //mciSendString "setaudio " + MP3FileName + " volume to" + NewVolume

        //char mci[256];
        sprintf(msiCommands[msiCount].Alias, "%s", cacheSound[id].Alias);
        InterlockedAdd(&msiCount, 1);
        //mciSendStringA(mci, NULL, 0, 0);
        */
        (void)id, (void)volume;
        return 0;
    }


int SND_cacheSounds(int* ids, int num)
    {
        (void)ids; (void)num;
        return num;
    }

///int GFX_getAssetId(const char* name)        { return OCT_PACK_getAssetId(name); }
int SND_getAssetId(const char* name)        { (void)name; return 0; /*OCT_PACK_getAssetId(name); */}

int SND_play(int id, int volume)
    {
        (void)id; (void)volume;

#ifdef OCTSIM
/*
        //Allow single sound source
        if (OctCubeId != 0) return 0;

        //[TODO]: Do this optionally
        if (id <= 0) return 0;

        //Assert
        if (id < 0 || id >= soundsCount) OCT_terminate("Bad sound id");

        //mciSendString "setaudio " + MP3FileName + " volume to" + NewVolume

        //char mci[256];
        sprintf(msiCommands[msiCount].Alias, "%s", cacheSound[id].Alias);
        InterlockedAdd(&msiCount, 1);
        //mciSendStringA(mci, NULL, 0, 0);
*/
#endif
        return 0;
    }



    //octAsset_t

void MakePack(wchar_t* bin_path)
    {
        //


        //Fill desc 4kB
        //Table FileInfo[]: Offset, Size
        //Bin
        //Glue all assets together (prepared\*.png)
        
        //Read this pack
        (void)bin_path;
    }