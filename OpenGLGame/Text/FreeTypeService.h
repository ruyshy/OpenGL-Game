#pragma once

#ifndef FREETYPESERVICE_H_
#define FREETYPESERVICE_H_

#include "pch.h"

class FreeTypeService
{
public:
    FreeTypeService() = default;
    ~FreeTypeService();

    FreeTypeService(const FreeTypeService&) = delete;
    FreeTypeService& operator=(const FreeTypeService&) = delete;

    bool initialize();
    void shutdown();

    FT_Library getLibrary() const;
    bool isReady() const;

private:
    FT_Library _library = nullptr;
};

#endif // !FREETYPESERVICE_H_
