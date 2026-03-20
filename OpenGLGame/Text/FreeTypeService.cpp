#include "pch.h"
#include "FreeTypeService.h"

FreeTypeService::~FreeTypeService()
{
    shutdown();
}

bool FreeTypeService::initialize()
{
    shutdown();

    if (FT_Init_FreeType(&_library) != 0)
    {
        cerr << "Failed to initialize FreeType." << endl;
        return false;
    }

    return true;
}

void FreeTypeService::shutdown()
{
    if (_library != nullptr)
    {
        FT_Done_FreeType(_library);
        _library = nullptr;
    }
}

FT_Library FreeTypeService::getLibrary() const
{
    return _library;
}

bool FreeTypeService::isReady() const
{
    return _library != nullptr;
}
