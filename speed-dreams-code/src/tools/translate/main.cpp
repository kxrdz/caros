/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define SDL_MAIN_HANDLED

#include "app.h"
#include <tgf.h>
#include <tgfclient.h>
#include <portability.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>

int main(int argc, char *argv[])
{
    Application app;

    app.initialize(true, argc, argv);

    if (!app.parseOptions())
        return EXIT_FAILURE;

    if (chdir(GfDataDir()))
    {
        GfLogError("Could not start %s : failed to cd to the datadir '%s' (%s)\n",
                   app.name().c_str(), GfDataDir(), strerror(errno));
        return EXIT_FAILURE;
    }

    return app.run();
}
