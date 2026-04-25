/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2026 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "ssggraphinfo.h"
#include "graphinfo.h"
#include <tgf.h>
#include <tgfclient.h>

SsggraphInfo::SsggraphInfo(void *prevMenu) :
    GraphInfo(prevMenu, "ssggraphinfomenu.xml")
{
}
