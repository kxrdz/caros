/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2026 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "graphinfo.h"
#include <tgf.h>
#include <tgfclient.h>

void GraphInfo::deinit(void *args)
{
    delete static_cast<GraphInfo *>(args);
}

GraphInfo::~GraphInfo()
{
    GfuiScreenRelease(hscr);
    GfuiScreenActivate(prev);
}

GraphInfo::GraphInfo(void *prevMenu, const char *path) :
    hscr(GfuiScreenCreate()),
    prev(prevMenu)
{
    void *param = GfuiMenuLoad(path);

    if (!hscr)
        throw std::runtime_error("GfuiScreenCreate failed");
    else if (!param)
        throw std::runtime_error("GfuiMenuLoad failed");
    else if (!GfuiMenuCreateStaticControls(hscr, param))
        throw std::runtime_error("GfuiMenuCreateStaticControls failed");
    else if (GfuiMenuCreateButtonControl(hscr, param, "back", this, deinit) < 0)
        throw std::runtime_error("create cancel failed");

    GfuiMenuDefaultKeysAdd(hscr);
    GfuiAddKey(hscr, GFUIK_ESCAPE, "Back to previous menu", this, deinit, NULL);
    GfParmReleaseHandle(param);
    GfuiScreenActivate(hscr);
}
