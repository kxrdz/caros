/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2026 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SSGGRAPHINFO_H
#define SSGGRAPHINFO_H

#include "graphinfo.h"

class SsggraphInfo : public GraphInfo
{
public:
    SsggraphInfo(void *prevMenu);

protected:
    ~SsggraphInfo() = default;
};

#endif
