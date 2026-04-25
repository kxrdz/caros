/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2025-2026 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef GRAPHICSCONFIG_H
#define GRAPHICSCONFIG_H

#include <tgfclient.h>
#include <glfeatures.h>
#include <map>
#include <vector>

class GraphicsConfig
{
public:
    GraphicsConfig(void *prevMenu);

private:
    enum epreset {PRESET_LOW, PRESET_BALANCED, PRESET_HIGH, PRESET_CUSTOM};
    enum lowmidhigh {LOW, MID, HIGH};
    enum dynamicshadows {NONE, CARS, CARSTRACK};
    enum speedunit {KPH, MPH};
    enum anisotropicfilter {AF_NONE, AF_BILINEAR, AF_TRILINEAR};
    enum engine {SSGGRAPH, OSGGRAPH};

    struct preset_ssggraph
    {
        const char *texturecompression, *skydome,
            *multitexturing, *visibility;
    };

    struct preset_osggraph
    {
        const char *shadowsize, *dynamicshadows, *shaders;
    };

    struct preset
    {
        int engine;
        const char *antialiasing, *anisotropicfilter;

        union
        {
            struct preset_ssggraph ssggraph;
            struct preset_osggraph osggraph;
        } u;
    };

    ~GraphicsConfig();
    static void apply(void *args);
    static void deinit(void *args);
    static void toggle(void *args);
    static void engine_changed(tComboBoxInfo *info);
    static void engine_info(void *userData);
    static void setcustom(void *userData);
    static void setcustom(tComboBoxInfo *info);
    static void setdynamicshadows(tComboBoxInfo *info);
    static void setpreset(tComboBoxInfo *info);
    const char *tr(const char *s) const;
    void setcustom() const;
    void show() const;
    int load();
    void init_presets();
    void load_graphicsengine(void *param);
    void load_visibility(void *param) const;
    void load_skydome(void *param) const;
    void load_dynamicshadows(void *param) const;
    void load_shadowsize(const GfglFeatures &gl, void *param) const;
    void load_shaders(void *param) const;
    void load_speedunit(void *param) const;
    void load_texturecompression(const GfglFeatures &gl) const;
    void load_maxtexturesize(const GfglFeatures &gl) const;
    void load_multitexturing(const GfglFeatures &gl) const;
    void load_antialiasing(const GfglFeatures &gl) const;
    void load_anisotropicfilter(const GfglFeatures &gl) const;
    void load_preset() const;
    int store() const;
    int store_graphicsengine(void *param, bool &restart) const;
    int store_visibility(void *param) const;
    int store_skydome(void *param) const;
    int store_dynamicshadows(void *param) const;
    int store_shadowsize(void *param) const;
    int store_shaders(void *param) const;
    int store_speedunit(void *param) const;
    int store_texturecompression(GfglFeatures &gl) const;
    int store_maxtexturesize(GfglFeatures &gl) const;
    int store_multitexturing(GfglFeatures &gl) const;
    int store_antialiasing(GfglFeatures &gl, bool &restart) const;
    int store_anisotropicfilter(GfglFeatures &gl) const;
    bool ispreset(const struct preset_ssggraph &p) const;
    bool ispreset(const struct preset_osggraph &p) const;
    bool ispreset(const struct preset &p) const;
    void setpreset(const struct preset_ssggraph &p) const;
    void setpreset(const struct preset_osggraph &p) const;
    void setpreset(int id, const char *preset) const;

    void *const hscr, *const prev, *const param;
    int sel_engine;
    int preset, graphicsengine, graphicsengine_info, visibility,
        visibility_lbl, skydome, skydome_lbl, dynamicshadows,
        dynamicshadows_lbl, shaders, shaders_lbl, antialiasing,
        anisotropicfilter,
        texturecompression, texturecompression_lbl,
        maxtexturesize, maxtexturesize_lbl, multitexturing, multitexturing_lbl,
        multitexturing_bg, shadowsize, shadowsize_lbl, speedunit, speedunit_lbl;
    std::vector<struct preset> presets;
    std::map<int, engine> idtype;
};

#endif
