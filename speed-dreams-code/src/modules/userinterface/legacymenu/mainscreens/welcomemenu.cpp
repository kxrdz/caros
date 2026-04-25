/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2026 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "welcomemenu.h"
#include "languageconfig.h"
#include <translation.h>
#include <tgf.h>
#include <tgfclient.h>
#include <cstddef>
#include <stdexcept>
#include <string>

const char *const WelcomeMenu::path = "config/language" PARAMEXT;

bool WelcomeMenu::shouldRun()
{
    bool ret = true;
    void *h = GfParmReadFileLocal(path, GFPARM_RMODE_STD);

    if (!h)
    {
        GfLogError("GfParmReadFileLocal %s failed\n", path);
        goto end;
    }
    else if (GfParmGetStr(h, "Language", "language", nullptr))
        ret = false;

end:
    if (h)
        GfParmReleaseHandle(h);

    return ret;
}

void WelcomeMenu::apply(void *args)
{
    WelcomeMenu *w = static_cast<WelcomeMenu *>(args);

    delete w;
}

void WelcomeMenu::update(tComboBoxInfo *info)
{
    WelcomeMenu *w = static_cast<WelcomeMenu *>(info->userData);

    w->update();
}

const char *WelcomeMenu::tr(const char *key) const
{
    std::string s = std::string("translatable/") + key;

    return GfuiGetTranslatedText(param, s.c_str(), "text", "");
}

int WelcomeMenu::set(const LanguageConfig::lang &lang) const
{
    int ret = -1;
    void *h;

    if (!(h = GfParmReadFileLocal(path, GFPARM_RMODE_STD)))
    {
        GfLogError("GfParmReadFileLocal %s failed\n", path);
        goto end;
    }
    else if (GfParmSetStr(h, "Language", "language", lang.code.c_str()))
    {
        GfLogError("GfParmSetStr failed\n");
        goto end;
    }
    else if (GfParmWriteFileLocal(path, h, "language"))
    {
        GfLogError("GfParmWriteFile failed\n");
        goto end;
    }

    ret = 0;

end:
    if (h)
        GfParmReleaseHandle(h);

    return ret;
}

void WelcomeMenu::update()
{
    const char *selected = GfuiComboboxGetText(hscr, combo);

    for (const auto &lang : languages)
    {
        if (selected == lang.name)
        {
            if (set(lang))
                throw std::runtime_error("Failed to set language");

            break;
        }
    }

    // Reload language settings.
    gfuiShutdownLanguage();
    gfuiInitLanguage();

    for (size_t i = 0; i < sizeof lines / sizeof *lines; i++)
    {
        std::string key = "line" + std::to_string(i + 1);

        GfuiLabelSetText(hscr, lines[i], tr(key.c_str()));
    }

    GfuiLabelSetText(hscr, welcome, tr("welcome"));
    GfuiButtonSetText(hscr, apply_btn, tr("apply"));
}

WelcomeMenu::WelcomeMenu(bool (*next)()) :
    hscr(GfuiScreenCreate()),
    param(GfuiMenuLoad("welcomemenu.xml")),
    next(next)
{
    if (!hscr)
        throw std::runtime_error("GfuiScreenCreate failed");
    else if (!param)
        throw std::runtime_error("Could not open welcomemenu.xml");
    else if (!GfuiMenuCreateStaticControls(hscr, param))
        throw std::runtime_error("GfuiMenuCreateStaticControls failed");
    else if ((apply_btn = GfuiMenuCreateButtonControl(hscr, param, "apply",
        this, apply)) < 0)
        throw std::runtime_error("create next failed");
    else if ((combo = GfuiMenuCreateComboboxControl(hscr, param, "language",
        this, update)) < 0)
        throw std::runtime_error("GfuiMenuCreateComboboxControl failed");
    else if ((welcome = GfuiMenuCreateLabelControl(hscr, param, "welcome")) < 0)
        throw std::runtime_error("GfuiMenuCreateLabelControl failed");

    for (size_t i = 0; i < sizeof lines / sizeof *lines; i++)
    {
        std::string key = "line" + std::to_string(i + 1);
        int &line = lines[i];

        if ((line = GfuiMenuCreateLabelControl(hscr, param, key.c_str())) < 0)
            throw std::runtime_error("GfuiMenuCreateLabelControl failed");
    }

    void *lang;

    if (!(lang = GfParmReadFileLocal(path, GFPARM_RMODE_STD)))
        throw std::runtime_error("Could not open welcomemenu.xml");

    if (LanguageConfig::get_languages(languages))
        throw std::runtime_error("LanguageConfig::get_languages failed");

    for (const auto &lang : languages)
        GfuiComboboxAddText(hscr, combo, lang.name.c_str());

    GfuiMenuDefaultKeysAdd(hscr);
    GfuiAddKey(hscr, GFUIK_ESCAPE, "Go to next menu", this, apply, NULL);
    GfuiAddKey(hscr, GFUIK_RETURN, "Go to next menu", this, apply, NULL);
    GfuiScreenActivate(hscr);
    GfParmReleaseHandle(lang);
    update();
}

WelcomeMenu::~WelcomeMenu()
{
    GfParmReleaseHandle(param);
    GfuiScreenRelease(hscr);
    next();
}
