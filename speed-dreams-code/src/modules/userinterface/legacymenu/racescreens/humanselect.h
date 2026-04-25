/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2026 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef HUMANSELECT_H
#define HUMANSELECT_H

#include "garagemenu.h"
#include <race.h>
#include <racescreens.h>
#include <string>
#include <vector>

class HumanSelect
{
public:
    HumanSelect(void *prevMenu, GfRace *race, bool allow_empty = false);
    ~HumanSelect();

private:
    static void deinit(void *args);
    static void on_cfg(void *args);
    static void on_add(void *args);
    static void on_rm(void *args);
    static void on_list(void *args);
    static void apply(void *args);
    static void cfg_exit(void *args);
    static void run_garage(void *args);
    static void store(void *args);
    const char *tr(const char *key) const;
    void update_ui() const;
    void *const prevMenu, *const hscr, *const param, *const cfg_hook,
        *const next_hook, *const store_hook, *phscr;
    GfRace *const race;
    const bool allow_empty;
    std::vector<std::string> staging;
    int available, active, apply_btn, add, back, rm, error, errorl2, bg, cfg;
    bool is_osggraph;
    std::vector<std::string>::const_iterator sel_player;
    RmGarageMenu garage;
    tRmDriverSelect ds;
};

#endif
