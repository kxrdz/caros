/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2025-2026 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "graphicsconfig.h"
#include "osggraphinfo.h"
#include "ssggraphinfo.h"
#include <glfeatures.h>
#include <graphic.h>
#include <guiscreen.h>
#include <raceman.h>
#include <tgf.h>
#include <tgfclient.h>
#include <translation.h>
#include <SDL2/SDL.h>
#include <cerrno>
#include <cstdlib>
#include <climits>

static const char *const shadowsizes[] =
{
    GR_ATT_SHADOW_512,
    GR_ATT_SHADOW_1024,
    GR_ATT_SHADOW_2048,
    GR_ATT_SHADOW_4096,
    GR_ATT_SHADOW_8192
};

void GraphicsConfig::deinit(void *args)
{
    delete static_cast<GraphicsConfig *>(args);
}

void GraphicsConfig::apply(void *args)
{
    GraphicsConfig *cfg = static_cast<GraphicsConfig *>(args);

    if (cfg->store())
        GfLogError("Failed to store graphical settings\n");

    deinit(args);
}

void GraphicsConfig::toggle(void *args)
{
    GraphicsConfig *cfg = static_cast<GraphicsConfig *>(args);

    cfg->show();
}

void GraphicsConfig::engine_changed(tComboBoxInfo *info)
{
    GraphicsConfig *cfg = static_cast<GraphicsConfig *>(info->userData);

    cfg->sel_engine = info->nPos;
    cfg->setcustom();
}

void GraphicsConfig::engine_info(void *userData)
{
    GraphicsConfig *cfg = static_cast<GraphicsConfig *>(userData);

    if (cfg->sel_engine == SSGGRAPH)
        new SsggraphInfo(cfg->hscr);
    else
        new OsggraphInfo(cfg->hscr);
}

const char *GraphicsConfig::tr(const char *s) const
{
    std::string section = std::string("translatable/") + s;

    return GfuiGetTranslatedText(param, section.c_str(), "text", "");
}

int GraphicsConfig::store_graphicsengine(void *param, bool &restart) const
{
    std::string prev = GfParmGetStr(param, RM_SECT_MODULES,
        RM_ATTR_MOD_GRAPHIC, RM_VAL_MOD_SSGRAPH);
    const char *cur = sel_engine == SSGGRAPH ?
        RM_VAL_MOD_SSGRAPH : RM_VAL_MOD_OSGGRAPH;

    if (GfParmSetStr(param, RM_SECT_MODULES, RM_ATTR_MOD_GRAPHIC, cur))
    {
        GfLogError("Failed to store graphic engine\n");
        return -1;
    }
    else if (cur != prev)
        restart = true;

    return 0;
}

int GraphicsConfig::store_multitexturing(GfglFeatures &gl) const
{
    if (!gl.isSupported(GfglFeatures::MultiTexturing))
        return 0;

    bool value = GfuiComboboxGetPosition(hscr, multitexturing);

    gl.select(GfglFeatures::MultiTexturing, value);
    return 0;
}

int GraphicsConfig::store_maxtexturesize(GfglFeatures &gl) const
{
    const char *text = GfuiComboboxGetText(hscr, maxtexturesize);
    unsigned long v;
    char *end;

    errno = 0;
    v = strtoul(text, &end, 10);

    if (errno || *end || v > INT_MAX)
    {
        GfLogError("Invalid texture size: %s\n", text);
        return -1;
    }

    gl.select(GfglFeatures::TextureMaxSize, v);
    return 0;
}

int GraphicsConfig::store_texturecompression(GfglFeatures &gl) const
{
    if (!gl.isSupported(GfglFeatures::TextureCompression))
        return 0;

    bool value = GfuiComboboxGetPosition(hscr, texturecompression);

    gl.select(GfglFeatures::TextureCompression, value);
    return 0;
}

int GraphicsConfig::store_speedunit(void *param) const
{
    const char *unit = GfuiComboboxGetPosition(hscr, speedunit) == MPH ?
        "mph" : "kph";

    return GfParmSetStr(param, "settings/speed", "unit", unit);
}

int GraphicsConfig::store_shaders(void *param) const
{
    static const char *const values[] =
    {
        GR_ATT_AGR_LITTLE,
        GR_ATT_AGR_MEDIUM,
        GR_ATT_AGR_FULL
    };

    unsigned pos = GfuiComboboxGetPosition(hscr, shaders);
    const char *value = GR_ATT_AGR_LITTLE;

    if (pos < sizeof values / sizeof *values)
        value = values[pos];

    return GfParmSetStr(param, GR_SCT_GRAPHIC, GR_ATT_SHADERS, value);
}

int GraphicsConfig::store_shadowsize(void *param) const
{
    if (GfuiComboboxGetPosition(hscr, dynamicshadows) == NONE)
        return 0;

    const char *text = GfuiComboboxGetText(hscr, shadowsize);
    char *end;
    unsigned long v;

    errno = 0;
    v = strtoul(text, &end, 10);

    if (errno || *end)
    {
        GfLogError("Invalid shadow texture size: %s\n", text);
        return -1;
    }

    return GfParmSetNum(param, GR_SCT_GRAPHIC, GR_ATT_SHADOW_SIZE, nullptr, v);
}

int GraphicsConfig::store_dynamicshadows(void *param) const
{
    static const char *const values[] =
    {
        GR_ATT_AGR_NULL,
        GR_ATT_AGR_LITTLE,
        GR_ATT_AGR_FULL
    };

    const char *value = values[CARS], *type;
    unsigned pos = GfuiComboboxGetPosition(hscr, dynamicshadows);

    if (pos == NONE)
        type = GR_ATT_SHADOW_NONE;
    else if (pos < sizeof values / sizeof *values)
    {
        type = GR_ATT_SHADOW_LSPM;
        value = values[pos];

        if (GfParmSetStr(param, GR_SCT_GRAPHIC, GR_ATT_AGR_QUALITY, value))
        {
            GfLogError("Failed to store shadow quality\n");
            return -1;
        }
    }

    return GfParmSetStr(param, GR_SCT_GRAPHIC, GR_ATT_SHADOW_TYPE, type);
}

int GraphicsConfig::store_skydome(void *param) const
{
    // Both osggraph and ssggraph set this hardcoded threshold, and it used
    // to be the minimum allowed value in the older graphics settings menu.
    // Higher values do not make a big difference and can also affect FPS,
    // so keep it to a minimum.
    const int threshold = 12000;
    unsigned pos = GfuiComboboxGetPosition(hscr, skydome);
    int value = pos ? threshold : 0;

    // Always enable skydome for osgGraph, because limited FOV is only
    // possible in ssggraph, anyway.
    if (sel_engine == OSGGRAPH)
        value = threshold;

    return GfParmSetNum(param, GR_SCT_GRAPHIC, GR_ATT_SKYDOMEDISTANCE,
        nullptr, value);
}

int GraphicsConfig::store_visibility(void *param) const
{
    // Visibility cannot be adjusted when dynamic skydome is enabled.
    if (GfuiComboboxGetPosition(hscr, skydome))
        return 0;

    static const int values[] = {25, 50, 75};
    unsigned pos = GfuiComboboxGetPosition(hscr, visibility);
    int value = values[MID];

    if (pos < sizeof values / sizeof *values)
        value = values[pos];

    return GfParmSetNum(param, GR_SCT_GRAPHIC, GR_ATT_FOVFACT, "%", value);
}

int GraphicsConfig::store_anisotropicfilter(GfglFeatures &gl) const
{
    unsigned pos = GfuiComboboxGetPosition(hscr, anisotropicfilter);

    gl.select(GfglFeatures::AnisotropicFiltering, pos);
    return 0;
}

int GraphicsConfig::store_antialiasing(GfglFeatures &gl, bool &restart) const
{
    if (!gl.isSupported(GfglFeatures::MultiSampling))
        return 0;

    bool prev_en = gl.isSelected(GfglFeatures::MultiSampling), cur_en = false;
    int prev_samples = gl.getSelected(GfglFeatures::MultiSamplingSamples),
        cur_samples = 1;
    const char *text = GfuiComboboxGetText(hscr, antialiasing);

    if (strcmp(text, tr("none")))
    {
        unsigned long v;
        char *end;

        cur_en = true;
        errno = 0;
        v = strtoul(text, &end, 10);

        if (errno || *end != 'x')
        {
            GfLogError("Invalid antialiasing value: %s\n", text);
            return -1;
        }

        cur_samples = v;
    }

    gl.select(GfglFeatures::MultiSampling, cur_en);

    if (cur_en)
        gl.select(GfglFeatures::MultiSamplingSamples, cur_samples);

    if (prev_en != cur_en || (cur_en && prev_samples != cur_samples))
        restart = true;

    return 0;
}

int GraphicsConfig::store() const
{
    int ret = -1;
    bool restart = false;
    GfglFeatures &gl = GfglFeatures::self();
    static const char path[] = "config/osghudconfig.xml";
    void *re = GfParmReadFileLocal(RACE_ENG_CFG, GFPARM_RMODE_STD),
        *graph = GfParmReadFileLocal(GR_PARAM_FILE, GFPARM_RMODE_STD),
        *osghud = GfParmReadFileLocal(path, GFPARM_RMODE_STD);

    if (!re)
    {
        GfLogError("GfParmReadFileLocal %s failed\n", RACE_ENG_CFG);
        goto end;
    }
    else if (!graph)
    {
        GfLogError("GfParmReadFileLocal %s failed\n", GR_PARAM_FILE);
        goto end;
    }
    else if (!osghud)
    {
        GfLogError("GfParmReadFileLocal %s failed\n", path);
        goto end;
    }
    else if (store_visibility(graph)
        || store_skydome(graph)
        || store_dynamicshadows(graph)
        || store_shadowsize(graph)
        || store_shaders(graph)
        || store_speedunit(osghud)
        || store_texturecompression(gl)
        || store_maxtexturesize(gl)
        || store_multitexturing(gl)
        || store_antialiasing(gl, restart)
        || store_anisotropicfilter(gl)
        || store_graphicsengine(re, restart))
        goto end;
    else if (GfParmWriteFileLocal(GR_PARAM_FILE, graph, "Graph"))
    {
        GfLogError("GfParmWriteFileLocal %s failed\n", GR_PARAM_FILE);
        goto end;
    }
    else if (GfParmWriteFileLocal(RACE_ENG_CFG, re, "raceengine"))
    {
        GfLogError("GfParmWriteFileLocal %s failed\n", RACE_ENG_CFG);
        goto end;
    }
    else if (GfParmWriteFileLocal(path, osghud, "osghudconfig"))
    {
        GfLogError("GfParmWriteFileLocal %s failed\n", path);
        goto end;
    }

    gl.storeSelection();

    if (restart)
        GfuiApp().restart();

    ret = 0;

end:
    if (graph)
        GfParmReleaseHandle(graph);

    if (re)
        GfParmReleaseHandle(re);

    if (osghud)
        GfParmReleaseHandle(osghud);

    return ret;
}

void GraphicsConfig::show() const
{
    for (const auto &id : idtype)
    {
        int visible = id.second == sel_engine ? GFUI_VISIBLE : GFUI_INVISIBLE;

        GfuiVisibilitySet(hscr, id.first, visible);
    }

    int enable = GfuiComboboxGetPosition(hscr, dynamicshadows) == NONE ?
        GFUI_DISABLE : GFUI_ENABLE;

    GfuiEnable(hscr, shadowsize, enable);

    enable = GfuiComboboxGetPosition(hscr, skydome) ?
        GFUI_DISABLE : GFUI_ENABLE;

    GfuiEnable(hscr, visibility, enable);
}

void GraphicsConfig::setcustom() const
{
    GfuiComboboxSetSelectedIndex(hscr, preset, PRESET_CUSTOM);
    show();
}

void GraphicsConfig::setcustom(tComboBoxInfo *info)
{
    static_cast<GraphicsConfig *>(info->userData)->setcustom();
}

void GraphicsConfig::setcustom(void *userData)
{
    static_cast<GraphicsConfig *>(userData)->setcustom();
}

void GraphicsConfig::setpreset(int id, const char *preset) const
{
    const std::vector<std::string> *choices = GfuiComboboxGetChoices(hscr, id);

    if (!choices)
    {
        GfLogError("id not a combobox: %d\n", id);
        return;
    }

    for (unsigned i = 0; i < choices->size(); i++)
    {
        const std::string &choice = choices->at(i);

        if (choice == preset)
        {
            GfuiComboboxSetSelectedIndex(hscr, id, i);
            break;
        }
    }
}

void GraphicsConfig::setpreset(const struct preset_ssggraph &p) const
{
    setpreset(visibility, p.visibility);
    setpreset(texturecompression, p.texturecompression);
    setpreset(skydome, p.skydome);
    setpreset(multitexturing, p.multitexturing);
}

void GraphicsConfig::setpreset(const struct preset_osggraph &p) const
{
    setpreset(dynamicshadows, p.dynamicshadows);
    setpreset(shadowsize, p.shadowsize);
    setpreset(shaders, p.shaders);
}

void GraphicsConfig::setpreset(tComboBoxInfo *info)
{
    GraphicsConfig *gfx = static_cast<GraphicsConfig *>(info->userData);
    const char *text = GfuiComboboxGetText(gfx->hscr, gfx->preset);

    if (!strcmp(text, gfx->tr("low")))
    {
        const struct preset &p = gfx->presets.at(PRESET_LOW);

        gfx->sel_engine = SSGGRAPH;
        gfx->setpreset(gfx->antialiasing, p.antialiasing);
        gfx->setpreset(gfx->anisotropicfilter, p.anisotropicfilter);
        gfx->setpreset(p.u.ssggraph);
    }
    else if (!strcmp(text, gfx->tr("balanced")))
    {
        const struct preset &p = gfx->presets.at(PRESET_BALANCED);

        gfx->sel_engine = OSGGRAPH;
        gfx->setpreset(gfx->antialiasing, p.antialiasing);
        gfx->setpreset(gfx->anisotropicfilter, p.anisotropicfilter);
        gfx->setpreset(p.u.osggraph);
    }
    else if (!strcmp(text, gfx->tr("high")))
    {
        const struct preset &p = gfx->presets.at(PRESET_HIGH);

        gfx->sel_engine = OSGGRAPH;
        gfx->setpreset(gfx->antialiasing, p.antialiasing);
        gfx->setpreset(gfx->anisotropicfilter, p.anisotropicfilter);
        gfx->setpreset(p.u.osggraph);
    }

    GfuiComboboxSetSelectedIndex(gfx->hscr, gfx->graphicsengine, gfx->sel_engine);
    gfx->show();
}

bool GraphicsConfig::ispreset(const struct preset_ssggraph &p) const
{
    return !strcmp(GfuiComboboxGetText(hscr, multitexturing), p.multitexturing)
        && !strcmp(GfuiComboboxGetText(hscr, texturecompression),
            p.texturecompression)
        && !strcmp(GfuiComboboxGetText(hscr, skydome), p.skydome)
        && !strcmp(GfuiComboboxGetText(hscr, visibility), p.visibility);
}

bool GraphicsConfig::ispreset(const struct preset_osggraph &p) const
{
    return !strcmp(GfuiComboboxGetText(hscr, dynamicshadows), p.dynamicshadows)
        && !strcmp(GfuiComboboxGetText(hscr, shadowsize), p.shadowsize)
        && !strcmp(GfuiComboboxGetText(hscr, shaders), p.shaders);
}

bool GraphicsConfig::ispreset(const struct preset &p) const
{
    if (sel_engine != p.engine
        || strcmp(GfuiComboboxGetText(hscr, antialiasing), p.antialiasing)
        || strcmp(GfuiComboboxGetText(hscr, anisotropicfilter),
            p.anisotropicfilter))
        return false;

    switch (sel_engine)
    {
        case SSGGRAPH:
            return ispreset(p.u.ssggraph);

        case OSGGRAPH:
            return ispreset(p.u.osggraph);
    }

    return false;
}

void GraphicsConfig::load_preset() const
{
    int preset = PRESET_CUSTOM;

    for (unsigned i = 0; i < presets.size(); i++)
    {
        const struct preset &p = presets.at(i);

        if (ispreset(p))
        {
            preset = i;
            break;
        }
    }

    GfuiComboboxSetSelectedIndex(hscr, this->preset, preset);
}

void GraphicsConfig::load_anisotropicfilter(const GfglFeatures &gl) const
{
    int max = gl.getSupported(GfglFeatures::AnisotropicFiltering),
        sel = gl.getSelected(GfglFeatures::AnisotropicFiltering);
    unsigned n = GfuiComboboxGetNumberOfChoices(hscr, anisotropicfilter);

    if (sel < 0 || sel > max)
        sel = max;

    if ((unsigned)sel >= n)
        sel = n - 1;

    GfuiComboboxSetSelectedIndex(hscr, anisotropicfilter, sel);
}

void GraphicsConfig::load_antialiasing(const GfglFeatures &gl) const
{
    if (gl.isSupported(GfglFeatures::MultiSampling)
        && gl.isSelected(GfglFeatures::MultiSampling))
    {
        int max = gl.getSupported(GfglFeatures::MultiSamplingSamples),
            sel = gl.getSelected(GfglFeatures::MultiSamplingSamples);

        for (int i = 1, j = 1; i <= max; i <<= 1, j++)
        {
            if (i == sel)
            {
                GfuiComboboxSetSelectedIndex(hscr, antialiasing, j);
                break;
            }
        }
    }
}

void GraphicsConfig::load_multitexturing(const GfglFeatures &gl) const
{
    if (!gl.isSupported(GfglFeatures::MultiTexturing))
        return;

    bool value = gl.isSelected(GfglFeatures::MultiTexturing);

    GfuiComboboxSetSelectedIndex(hscr, multitexturing, value);

}

void GraphicsConfig::load_maxtexturesize(const GfglFeatures &gl) const
{
    int value = gl.getSelected(GfglFeatures::TextureMaxSize),
        max = gl.getSupported(GfglFeatures::TextureMaxSize);
    unsigned n = GfuiComboboxGetNumberOfChoices(hscr, maxtexturesize);
    bool found = false;

    for (int i = 8, j = 0; i < max; i <<= 1, j++)
    {
        if (i == value)
        {
            GfuiComboboxSetSelectedIndex(hscr, maxtexturesize, j);
            found = true;
            break;
        }
    }

    if (!found)
        GfuiComboboxSetSelectedIndex(hscr, maxtexturesize, n - 1);
}

void GraphicsConfig::load_texturecompression(const GfglFeatures &gl) const
{
    if (!gl.isSupported(GfglFeatures::TextureCompression))
        return;

    bool value = gl.isSelected(GfglFeatures::TextureCompression);

    GfuiComboboxSetSelectedIndex(hscr, texturecompression, value);
}

void GraphicsConfig::load_shadowsize(const GfglFeatures &gl, void *param) const
{
    // Old advancedgraphconfig.cpp incorrectly loaded/stored this key as a
    // string, even if config/graph.xml originally defined it as a number.
    // Attempt to read these broken configs to keep backwards compatibility.
    const char *svalue = GfParmGetStr(param, GR_SCT_GRAPHIC,
        GR_ATT_SHADOW_SIZE, nullptr);
    int max = gl.getSupported(GfglFeatures::TextureMaxSize),
        value = GfParmGetNum(param, GR_SCT_GRAPHIC, GR_ATT_SHADOW_SIZE,
            nullptr, max);

    if (svalue)
    {
        unsigned long v;
        char *end;

        errno = 0;
        v = strtoul(svalue, &end, 10);

        if (!errno && !*end && v <= INT_MAX)
            value = v;
    }

    if (value > max)
        value = max;

    for (size_t i = 0; i < sizeof shadowsizes / sizeof *shadowsizes; i++)
    {
        unsigned long v = strtoul(shadowsizes[i], nullptr, 0);

        if (v == (unsigned)value)
        {
            GfuiComboboxSetSelectedIndex(hscr, shadowsize, i);
            break;
        }
    }
}

void GraphicsConfig::load_shaders(void *param) const
{
    const char *value = GfParmGetStr(param, GR_SCT_GRAPHIC, GR_ATT_SHADERS,
        GR_ATT_AGR_LITTLE);
    enum lowmidhigh lmh = LOW;

    if (!strcmp(value, GR_ATT_AGR_MEDIUM))
        lmh = MID;
    else if (!strcmp(value, GR_ATT_AGR_FULL))
        lmh = HIGH;

    GfuiComboboxSetSelectedIndex(hscr, shaders, lmh);
}

void GraphicsConfig::load_speedunit(void *param) const
{
    const char *unit= GfParmGetStr(param, "settings/speed", "unit", "kph");
    enum speedunit su = !strcmp(unit, "mph") ? MPH : KPH;

    GfuiComboboxSetSelectedIndex(hscr, speedunit, su);
}

void GraphicsConfig::load_dynamicshadows(void *param) const
{
    const char *type = GfParmGetStr(param, GR_SCT_GRAPHIC, GR_ATT_SHADOW_TYPE,
        GR_ATT_SHADOW_LSPM);
    enum dynamicshadows ds = NONE;

    if (!strcmp(type, GR_ATT_SHADOW_LSPM))
    {
        const char *value = GfParmGetStr(param, GR_SCT_GRAPHIC,
            GR_ATT_AGR_QUALITY, GR_ATT_AGR_LITTLE);

        if (!strcmp(value, GR_ATT_AGR_NULL))
            ds = NONE;
        else if (!strcmp(value, GR_ATT_AGR_FULL))
            ds = CARSTRACK;
        else
            ds = CARS;
    }

    GfuiComboboxSetSelectedIndex(hscr, dynamicshadows, ds);
}

void GraphicsConfig::load_skydome(void *param) const
{
    int value = GfParmGetNum(param, GR_SCT_GRAPHIC, GR_ATT_SKYDOMEDISTANCE,
        nullptr, 0);

    GfuiComboboxSetSelectedIndex(hscr, skydome, !!value);
}

void GraphicsConfig::load_visibility(void *param) const
{
    int value = GfParmGetNum(param, GR_SCT_GRAPHIC, GR_ATT_FOVFACT, "%", 50);
    enum lowmidhigh lmh = LOW;

    if (value >= 66)
        lmh = HIGH;
    else if (value >= 33)
        lmh = MID;

    GfuiComboboxSetSelectedIndex(hscr, visibility, lmh);
}

void GraphicsConfig::load_graphicsengine(void *param)
{
    const char *value = GfParmGetStr(param, RM_SECT_MODULES,
        RM_ATTR_MOD_GRAPHIC, RM_VAL_MOD_SSGRAPH);

    sel_engine = !strcmp(value, RM_VAL_MOD_SSGRAPH) ? SSGGRAPH : OSGGRAPH;
    GfuiComboboxSetSelectedIndex(hscr, graphicsengine, sel_engine);
}

int GraphicsConfig::load()
{
    int ret = -1;
    static const char path[] = "config/osghudconfig.xml";
    void *re = GfParmReadFileLocal(RACE_ENG_CFG, GFPARM_RMODE_STD),
        *graph = GfParmReadFileLocal(GR_PARAM_FILE, GFPARM_RMODE_STD),
        *osghud = GfParmReadFileLocal(path, GFPARM_RMODE_STD);
    const GfglFeatures &gl = GfglFeatures::self();

    if (!re)
    {
        GfLogError("GfParmReadFileLocal %s failed\n", RACE_ENG_CFG);
        goto end;
    }
    else if (!graph)
    {
        GfLogError("GfParmReadFileLocal %s failed\n", GR_PARAM_FILE);
        goto end;
    }
    else if (!osghud)
    {
        GfLogError("GfParmReadFileLocal %s failed\n", path);
        goto end;
    }

    load_graphicsengine(re);
    load_visibility(graph);
    load_skydome(graph);
    load_dynamicshadows(graph);
    load_shadowsize(gl, graph);
    load_shaders(graph);
    load_speedunit(osghud);
    load_texturecompression(gl);
    load_maxtexturesize(gl);
    load_multitexturing(gl);
    load_antialiasing(gl);
    load_anisotropicfilter(gl);
    load_preset();
    ret = 0;

end:
    if (graph)
        GfParmReleaseHandle(graph);

    if (re)
        GfParmReleaseHandle(re);

    if (osghud)
        GfParmReleaseHandle(osghud);

    return ret;
}

void GraphicsConfig::init_presets()
{
    struct preset low, balanced, high;
    const auto gl = GfglFeatures::self();

    low.engine = SSGGRAPH;
    low.antialiasing = tr("none");
    low.u.ssggraph.skydome = tr("off");
    low.u.ssggraph.visibility = tr("mid");

    if (gl.isSupported(GfglFeatures::TextureCompression))
        low.u.ssggraph.texturecompression = tr("on");
    else
        low.u.ssggraph.texturecompression = tr("unsupported");

    if (gl.isSupported(GfglFeatures::MultiTexturing))
        low.u.ssggraph.multitexturing = tr("on");
    else
        low.u.ssggraph.multitexturing = tr("unsupported");

    if (gl.getSupported(GfglFeatures::AnisotropicFiltering))
    {
        low.anisotropicfilter = tr("bilinear");
        balanced.anisotropicfilter = tr("bilinear");
        high.anisotropicfilter = tr("trilinear");
    }
    else
        low.anisotropicfilter = tr("unsupported");

    balanced.engine = OSGGRAPH;

    if (gl.isSupported(GfglFeatures::MultiSampling))
        balanced.antialiasing = "1x";
    else
        balanced.antialiasing = tr("unsupported");

    balanced.u.osggraph.dynamicshadows = tr("carsonly");
    balanced.u.osggraph.shaders = tr("low");
    balanced.u.osggraph.shadowsize = GR_ATT_SHADOW_1024;

    high.engine = OSGGRAPH;

    if (gl.isSupported(GfglFeatures::MultiSampling))
        high.antialiasing = "8x";
    else
        high.antialiasing = tr("unsupported");

    high.u.osggraph.dynamicshadows = tr("carstrack");
    high.u.osggraph.shaders = tr("high");
    high.u.osggraph.shadowsize = GR_ATT_SHADOW_2048;

    presets.push_back(low);
    presets.push_back(balanced);
    presets.push_back(high);
}

GraphicsConfig::~GraphicsConfig()
{
    GfParmReleaseHandle(param);
    GfuiScreenRelease(hscr);
    GfuiScreenActivate(prev);
}

GraphicsConfig::GraphicsConfig(void *prevMenu) :
    hscr(GfuiScreenCreate()),
    prev(prevMenu),
    param(GfuiMenuLoad("graphicsconfigmenu.xml")),
    sel_engine(SSGGRAPH)
{
    static const char *const presets[PRESET_CUSTOM + 1] =
        {"low", "balanced", "high", "custom"},
        *const lmh[HIGH + 1] = {"low", "mid", "high"},
        *const offon[] = {"off", "on"},
        *const shadows[CARSTRACK + 1] = {"none", "carsonly", "carstrack"};

    if (!hscr)
        throw std::runtime_error("GfuiScreenCreate failed");
    else if (!param)
        throw std::runtime_error("GfuiMenuLoad failed");
    else if (!GfuiMenuCreateStaticControls(hscr, param))
        throw std::runtime_error("GfuiMenuCreateStaticControls failed");

    preset = GfuiMenuCreateComboboxControl(hscr, param, "preset", this,
        setpreset);
    graphicsengine = GfuiMenuCreateComboboxControl(hscr, param,
        "graphicsengine", this, engine_changed);
    graphicsengine_info = GfuiMenuCreateButtonControl(hscr, param,
        "graphicsengine_info", this, engine_info);
    antialiasing = GfuiMenuCreateComboboxControl(hscr, param,
        "antialiasing", this, setcustom);
    visibility = GfuiMenuCreateComboboxControl(hscr, param, "visibility",
        this, setcustom);
    visibility_lbl = GfuiMenuCreateLabelControl(hscr, param, "visibility_lbl");
    skydome = GfuiMenuCreateComboboxControl(hscr, param, "skydome", this,
        setcustom);
    skydome_lbl = GfuiMenuCreateLabelControl(hscr, param, "skydome_lbl");
    dynamicshadows = GfuiMenuCreateComboboxControl(hscr, param,
        "dynamicshadows", this, setcustom);
    dynamicshadows_lbl = GfuiMenuCreateLabelControl(hscr, param, "dynamicshadows_lbl");
    shaders = GfuiMenuCreateComboboxControl(hscr, param, "shaders", this,
        setcustom);
    shaders_lbl = GfuiMenuCreateLabelControl(hscr, param, "shaders_lbl");
    anisotropicfilter = GfuiMenuCreateComboboxControl(hscr, param,
        "anisotropicfilter", this, setcustom);
    texturecompression = GfuiMenuCreateComboboxControl(hscr, param,
        "texturecompression", this, setcustom);
    texturecompression_lbl = GfuiMenuCreateLabelControl(hscr, param,
        "texturecompression_lbl");
    maxtexturesize = GfuiMenuCreateComboboxControl(hscr, param,
        "maxtexturesize", this, setcustom);
    maxtexturesize_lbl = GfuiMenuCreateLabelControl(hscr, param,
        "maxtexturesize_lbl");
    multitexturing = GfuiMenuCreateComboboxControl(hscr, param,
        "multitexturing", this, setcustom);
    multitexturing_bg = GfuiMenuCreateStaticImageControl(hscr, param,
        "multitexturing_bg");
    multitexturing_lbl = GfuiMenuCreateLabelControl(hscr, param,
        "multitexturing_lbl");
    shadowsize = GfuiMenuCreateComboboxControl(hscr, param,
        "shadowsize", this, setcustom);
    shadowsize_lbl = GfuiMenuCreateLabelControl(hscr, param,
        "shadowsize_lbl");
    speedunit = GfuiMenuCreateComboboxControl(hscr, param, "speedunit");
    speedunit_lbl = GfuiMenuCreateLabelControl(hscr, param, "speedunit_lbl");

    if (preset < 0
        || graphicsengine < 0
        || graphicsengine_info < 0
        || visibility < 0
        || visibility_lbl < 0
        || skydome < 0
        || skydome_lbl < 0
        || dynamicshadows < 0
        || dynamicshadows_lbl < 0
        || shaders < 0
        || shaders_lbl < 0
        || anisotropicfilter < 0
        || texturecompression < 0
        || texturecompression_lbl < 0
        || maxtexturesize < 0
        || maxtexturesize_lbl < 0
        || multitexturing < 0
        || multitexturing_lbl < 0
        || multitexturing_bg < 0
        || speedunit < 0
        || speedunit_lbl < 0)
        throw std::runtime_error("failed to create dynamic GUI elements");
    else if (GfuiMenuCreateButtonControl(hscr, param, "cancel", this, deinit) < 0)
        throw std::runtime_error("create cancel failed");
    else if (GfuiMenuCreateButtonControl(hscr, param, "apply", this, apply) < 0)
        throw std::runtime_error("create next failed");

    GfuiComboboxAddText(hscr, graphicsengine, tr("ssggraph"));
    GfuiComboboxAddText(hscr, graphicsengine, tr("osggraph"));
    GfuiMenuDefaultKeysAdd(hscr);
    GfuiAddKey(hscr, GFUIK_RETURN, "Apply changes", this, apply, NULL);
    GfuiAddKey(hscr, GFUIK_ESCAPE, "Back to previous menu", this, deinit, NULL);

    for (const auto pr : presets)
        GfuiComboboxAddText(hscr, preset, tr(pr));

    for (const auto s : lmh)
    {
        GfuiComboboxAddText(hscr, visibility, tr(s));
        GfuiComboboxAddText(hscr, shaders, tr(s));
    }

    for (const auto s : offon)
    {
        GfuiComboboxAddText(hscr, skydome, tr(s));
        GfuiComboboxAddText(hscr, texturecompression, tr(s));
        GfuiComboboxAddText(hscr, multitexturing, tr(s));
    }

    for (const auto s : shadows)
        GfuiComboboxAddText(hscr, dynamicshadows, tr(s));

    const auto gl = GfglFeatures::self();

    if (GfglFeatures::self().getSupported(GfglFeatures::AnisotropicFiltering))
    {
        static const char *const values[] = {"bilinear", "trilinear"};

        GfuiComboboxAddText(hscr, anisotropicfilter, tr("none"));

        for (const auto s : values)
            GfuiComboboxAddText(hscr, anisotropicfilter, tr(s));
    }
    else
    {
        GfuiComboboxAddText(hscr, anisotropicfilter, tr("unsupported"));
        GfuiEnable(hscr, antialiasing, GFUI_DISABLE);
    }

    int max = gl.getSupported(GfglFeatures::TextureMaxSize);

    for (int i = 8; i <= max; i <<= 1)
        GfuiComboboxAddText(hscr, maxtexturesize, std::to_string(i).c_str());

    for (size_t i = 0; i < sizeof shadowsizes / sizeof *shadowsizes; i++)
        GfuiComboboxAddText(hscr, shadowsize, shadowsizes[i]);

    int samples;

    if (gl.isSupported(GfglFeatures::MultiSampling)
        && (samples = gl.getSupported(GfglFeatures::MultiSamplingSamples)) > 0)
    {
        GfuiComboboxAddText(hscr, antialiasing, tr("none"));

        for (int i = 1; i <= samples; i <<= 1)
        {
            std::string value = std::to_string(i) + 'x';

            GfuiComboboxAddText(hscr, antialiasing, value.c_str());
        }
    }
    else
    {
        GfuiComboboxAddText(hscr, antialiasing, tr("unsupported"));
        GfuiEnable(hscr, antialiasing, GFUI_DISABLE);
    }

    if (!GfglFeatures::self().isSupported(GfglFeatures::MultiTexturing))
        GfuiEnable(hscr, multitexturing, GFUI_DISABLE);

    if (!SDL_GL_ExtensionSupported("GL_ARB_texture_compression"))
        GfuiEnable(hscr, texturecompression, GFUI_DISABLE);

    GfuiComboboxAddText(hscr, speedunit, "kph");
    GfuiComboboxAddText(hscr, speedunit, "mph");
    init_presets();

    if (load())
        throw std::runtime_error("Failed to load graphical settings");

    idtype[visibility] = SSGGRAPH;
    idtype[visibility_lbl] = SSGGRAPH;
    idtype[texturecompression] = SSGGRAPH;
    idtype[texturecompression_lbl] = SSGGRAPH;
    idtype[maxtexturesize] = SSGGRAPH;
    idtype[maxtexturesize_lbl] = SSGGRAPH;
    idtype[skydome] = SSGGRAPH;
    idtype[skydome_lbl] = SSGGRAPH;
    idtype[multitexturing] = SSGGRAPH;
    idtype[multitexturing_lbl] = SSGGRAPH;
    idtype[multitexturing_bg] = SSGGRAPH;
    idtype[dynamicshadows] = OSGGRAPH;
    idtype[dynamicshadows_lbl] = OSGGRAPH;
    idtype[shaders] = OSGGRAPH;
    idtype[shaders_lbl] = OSGGRAPH;
    idtype[shadowsize] = OSGGRAPH;
    idtype[shadowsize_lbl] = OSGGRAPH;
    idtype[speedunit] = OSGGRAPH;
    idtype[speedunit_lbl] = OSGGRAPH;
    GfuiScreenActivate(hscr);
    show();
}
