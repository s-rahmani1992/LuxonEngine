#pragma once  
#include <wincodec.h>  
#include <map>  

struct GUIDComparer  
{  
    bool operator()(const GUID& Left, const GUID& Right) const  
    {  
        return memcmp(&Left, &Right, sizeof(Right)) < 0;  
    }  
};  

template<typename T>  
using GuidMap = std::map<GUID, T, GUIDComparer>;

// TODO If Possible, Add ToString for various GIUDs for example WICPixelFormatGUID
