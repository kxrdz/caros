/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2026 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef WELCOMEMENU_H
#define WELCOMEMENU_H

#include <tgfclient.h>
#include "languageconfig.h"
#include <vector>

class WelcomeMenu
{
public:
    WelcomeMenu(bool (*next)());
    static bool shouldRun();

private:
    ~WelcomeMenu();
    static void apply(void *args);
    static void update(tComboBoxInfo *info);
    void update();
    int set(const LanguageConfig::lang &lang) const;
    const char *tr(const char *key) const;

    static const char *const path;
    void *const hscr, *const param;
    bool (*const next)();
    int welcome, apply_btn, combo, lines[5];
    std::vector<LanguageConfig::lang> languages;
};

#endif
