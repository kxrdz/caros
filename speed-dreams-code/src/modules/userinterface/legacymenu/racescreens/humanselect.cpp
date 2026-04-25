/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2026 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "humanselect.h"
#include <drivers.h>
#include <garagemenu.h>
#include <legacymenu.h>
#include <playerconfig.h>
#include <playerpref.h>
#include <race.h>
#include <raceman.h>
#include <racescreens.h>
#include <robot.h>
#include <tgfclient.h>
#include <translation.h>
#include <algorithm>

void HumanSelect::deinit(void *args)
{
    delete static_cast<HumanSelect *>(args);
}

const char *HumanSelect::tr(const char *key) const
{
    std::string s = std::string("translatable/") + key;

    return GfuiGetTranslatedText(param, s.c_str(), "text", "");
}

void HumanSelect::store(void *args)
{
    HumanSelect *h = static_cast<HumanSelect *>(args);
    tRmInfo *reInfo = LmRaceEngine().inData();
    void *params = reInfo->params;

    h->race->store();
    GfParmWriteFile(NULL, params, reInfo->_reName);
    delete h;
}

void HumanSelect::run_garage(void *args)
{
    HumanSelect *h = static_cast<HumanSelect *>(args);
    GfRace *r = LmRaceEngine().race();

    if (h->sel_player != h->staging.cend())
    {
        const char *name = h->sel_player->c_str();
        GfDriver *d = GfDrivers::self()->getDriverWithName(name, ROB_VAL_HUMAN);

        if (d)
        {
            RmGarageMenu &g = h->garage;

            if (!g.getMenuHandle())
                g.initialize();

            g.setSkin(d);
            g.runMenu(r, d);
            g.setPreviousMenuHandle(h->hscr);
            g.setNextMenuHandle(h->next_hook);
            h->sel_player++;
        }
        else
            GfLogError("Could not find human player with name %s\n", name);
    }
    else
    {
        tRmDriverSelect &ds = h->ds;

        ds.pRace = r;
        ds.prevScreen = h->hscr;
        ds.nextScreen = h->store_hook;
        RmDriversSelect(&ds);
    }
}

void HumanSelect::apply(void *args)
{
    HumanSelect *h = static_cast<HumanSelect *>(args);
    GfRace *r = h->race;
    const std::vector<GfDriver*> &comps = r->getCompetitors();

    for (GfDriver *d : comps)
    {
        const auto &s = h->staging;

        if (d->isHuman()
            && std::find(s.cbegin(), s.cend(), d->getName()) == s.cend())
            r->removeCompetitor(d);
    }

    for (const auto &name : h->staging)
    {
        GfDriver *d = GfDrivers::self()->getDriverWithName(name, ROB_VAL_HUMAN);

        if (std::find(comps.cbegin(), comps.cend(), d) == comps.cend())
        {
            while (!r->acceptsMoreCompetitors() && !comps.empty())
                r->removeCompetitor(comps.at(0));

            r->appendCompetitor(d);
        }
    }

    h->sel_player = h->staging.cbegin();
    run_garage(h);
}

void HumanSelect::on_list(void *args)
{
    const HumanSelect *h = static_cast<const HumanSelect *>(args);
    void *userData;

    if (GfuiScrollListGetSelectedElement(h->hscr, h->available, &userData))
        GfuiEnable(h->hscr, h->add, GFUI_ENABLE);
    else
        GfuiEnable(h->hscr, h->add, GFUI_DISABLE);

    if (GfuiScrollListGetSelectedElement(h->hscr, h->active, &userData))
        GfuiEnable(h->hscr, h->rm, GFUI_ENABLE);
    else
        GfuiEnable(h->hscr, h->rm, GFUI_DISABLE);
}

void HumanSelect::cfg_exit(void *args)
{
    HumanSelect *h = static_cast<HumanSelect *>(args);

    for (auto it = h->staging.begin(); it != h->staging.end();)
    {
        if (!GfDrivers::self()->getDriverWithName(*it, ROB_VAL_HUMAN))
            it = h->staging.erase(it);
        else
            it++;
    }

    GfRace *r = h->race;

    // Since GfRace keeps a vector of GfDriver *, those might have changed
    // if human players have been added and/or removed, so the vector must
    // be reloaded.
    r->load(r->getManager());
    h->update_ui();
    GfuiScreenActivate(h->hscr);
}

void HumanSelect::on_cfg(void *args)
{
    HumanSelect *h = static_cast<HumanSelect *>(args);

    if (!(h->phscr = PlayerConfigMenuInit(h->cfg_hook)))
        GfuiLabelSetText(h->hscr, h->error, h->tr("error/playercfg"));
    else
        GfuiScreenActivate(h->phscr);
}

void HumanSelect::on_add(void *args)
{
    HumanSelect *h = static_cast<HumanSelect *>(args);
    int index = GfuiScrollListGetSelectedElementIndex(h->hscr, h->available);
    void *userData;

    if (GfuiScrollListGetSelectedElement(h->hscr, h->available, &userData))
    {
        h->staging.push_back(static_cast<const char *>(userData));
        h->update_ui();

        int n = GfuiScrollListGetNumberOfElements(h->hscr, h->available);

        if (n && index >= n)
            index = n - 1;

        GfuiScrollListSetSelectedElement(h->hscr, h->available, index);
    }
}

void HumanSelect::on_rm(void *args)
{
    HumanSelect *h = static_cast<HumanSelect *>(args);
    int index = GfuiScrollListGetSelectedElementIndex(h->hscr, h->active);
    void *userData;

    if (GfuiScrollListGetSelectedElement(h->hscr, h->active, &userData))
    {
        auto it = std::find(h->staging.cbegin(), h->staging.cend(),
            static_cast<const char *>(userData));

        if (it != h->staging.end())
            h->staging.erase(it);

        h->update_ui();

        int n = GfuiScrollListGetNumberOfElements(h->hscr, h->active);

        if (n && index >= n)
            index = n - 1;

        GfuiScrollListSetSelectedElement(h->hscr, h->active, index);
    }
}

void HumanSelect::update_ui() const
{
    int i = 0;

    GfuiScrollListClear(hscr, available);
    GfuiScrollListClear(hscr, active);

    for (const auto &player : staging)
    {
        const char *name = player.c_str();

        GfuiScrollListInsertElement(hscr, active, name, i++, (void *)name);
    }

    std::vector<GfDriver *> drivers =
        GfDrivers::self()->getDriversWithTypeAndCategory(ROB_VAL_HUMAN);

    i = 0;

    for (auto driver : drivers)
    {
        const char *name = driver->getName().c_str();

        if (std::find(staging.cbegin(), staging.cend(), name) == staging.end())
            GfuiScrollListInsertElement(hscr, available, name, i++, (void *)name);
    }

    unsigned n = GfuiScrollListGetNumberOfElements(hscr, active);

    on_list((void *)this);
    GfuiEnable(hscr, apply_btn, GFUI_ENABLE);
    GfuiEnable(hscr, back, GFUI_ENABLE);
    GfuiVisibilitySet(hscr, bg, GFUI_INVISIBLE);
    GfuiLabelSetText(hscr, error, "");
    GfuiLabelSetText(hscr, errorl2, "");

    if (!race->acceptsDriverType(ROB_VAL_HUMAN))
    {
        if (n)
            GfuiEnable(hscr, apply_btn, GFUI_DISABLE);

        GfuiLabelSetText(hscr, error, tr("info/onlybots/1"));
        GfuiLabelSetText(hscr, errorl2, tr("info/onlybots/2"));
        GfuiVisibilitySet(hscr, bg, GFUI_VISIBLE);
    }
    else if (allow_empty)
    {
        if (!n)
        {
            GfuiLabelSetText(hscr, error, tr("info/nohumans/1"));
            GfuiLabelSetText(hscr, errorl2, tr("info/nohumans/2"));
            GfuiVisibilitySet(hscr, bg, GFUI_VISIBLE);
        }
        else if (n > race->getMaxCompetitors())
        {
            const char *fmt = tr("error/toomanyplayers/2");
            char buf[128];

            snprintf(buf, sizeof buf, fmt, race->getMaxCompetitors());
            GfuiEnable(hscr, apply_btn, GFUI_DISABLE);
            GfuiLabelSetText(hscr, error, tr("error/toomanyplayers/1"));
            GfuiLabelSetText(hscr, errorl2, buf);
            GfuiVisibilitySet(hscr, bg, GFUI_VISIBLE);
        }
    }
    else
    {
        GfuiEnable(hscr, apply_btn, n ? GFUI_ENABLE : GFUI_DISABLE);

        if (!n && !GfuiScrollListGetNumberOfElements(hscr, available))
        {
            GfuiEnable(hscr, back, GFUI_DISABLE);
            GfuiLabelSetText(hscr, error, tr("error/nohumans/1"));
            GfuiLabelSetText(hscr, errorl2, tr("error/nohumans/2"));
            GfuiVisibilitySet(hscr, bg, GFUI_VISIBLE);
        }
    }

    if (n > 1 && is_osggraph)
    {
        GfuiLabelSetText(hscr, error, tr("error/unsupported/1"));
        GfuiLabelSetText(hscr, errorl2, tr("error/unsupported/2"));
        GfuiEnable(hscr, apply_btn, GFUI_DISABLE);
        GfuiVisibilitySet(hscr, bg, GFUI_VISIBLE);
    }
}

HumanSelect::~HumanSelect()
{
    GfuiHookRelease(store_hook);
    GfuiHookRelease(next_hook);
    GfuiHookRelease(cfg_hook);
    GfParmReleaseHandle(param);
    GfuiScreenRelease(hscr);
    GfuiScreenActivate(prevMenu);
}

HumanSelect::HumanSelect(void *prevMenu, GfRace *race, bool allow_empty) :
    prevMenu(prevMenu),
    hscr(GfuiScreenCreate()),
    param(GfuiMenuLoad("humanselectmenu.xml")),
    cfg_hook(GfuiHookCreate(this, cfg_exit)),
    next_hook(GfuiHookCreate(this, run_garage)),
    store_hook(GfuiHookCreate(this, store)),
    race(race),
    allow_empty(allow_empty)
{
    void *re = GfParmReadFileLocal(RACE_ENG_CFG, GFPARM_RMODE_STD);

    if (!hscr)
        throw std::runtime_error("GfuiScreenCreate failed");
    else if (!param)
        throw std::runtime_error("GfuiMenuLoad failed");
    else if (!cfg_hook)
        throw std::runtime_error("GfuiHookCreate failed");
    else if (!re)
        throw std::runtime_error("GfParmReadFileLocal failed");
    else if (!GfuiMenuCreateStaticControls(hscr, param))
        throw std::runtime_error("GfuiMenuCreateStaticControls failed");

    cfg = GfuiMenuCreateButtonControl(hscr, param, "cfg", this, on_cfg);
    add = GfuiMenuCreateButtonControl(hscr, param, "add", this, on_add);
    rm = GfuiMenuCreateButtonControl(hscr, param, "rm", this, on_rm);
    back = GfuiMenuCreateButtonControl(hscr, param, "back", this, deinit);
    apply_btn = GfuiMenuCreateButtonControl(hscr, param, "apply", this, apply);
    available = GfuiMenuCreateScrollListControl(hscr, param, "available", this, on_list);
    active = GfuiMenuCreateScrollListControl(hscr, param, "active", this, on_list);
    bg = GfuiMenuCreateStaticImageControl(hscr, param, "bg");
    error = GfuiMenuCreateLabelControl(hscr, param, "error");
    errorl2 = GfuiMenuCreateLabelControl(hscr, param, "errorl2");

    if (cfg < 0
        || back < 0
        || add < 0
        || rm < 0
        || apply_btn < 0
        || available < 0
        || active < 0
        || error < 0
        || errorl2 < 0
        || bg < 0)
        throw std::runtime_error("Failed to setup GUI elements");

    if (race->acceptsDriverType(ROB_VAL_HUMAN))
    {
        const std::vector<GfDriver*> &comps = race->getCompetitors();

        for (const GfDriver *drv : comps)
            if (drv->isHuman())
                staging.push_back(drv->getName());
    }

    const char *engine = GfParmGetStr(re, RM_SECT_MODULES, RM_ATTR_MOD_GRAPHIC,
        RM_VAL_MOD_SSGRAPH);

    is_osggraph = !strcmp(engine, RM_VAL_MOD_OSGGRAPH);
    GfParmReleaseHandle(re);
    GfuiMenuDefaultKeysAdd(hscr);
    GfuiAddKey(hscr, GFUIK_ESCAPE, "Back to previous menu", this, deinit, NULL);
    update_ui();
    GfuiScreenActivate(hscr);
}
