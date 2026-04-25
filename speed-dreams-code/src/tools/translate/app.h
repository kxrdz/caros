/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef TRANSLATE_APP_H
#define TRANSLATE_APP_H

#include <tgfclient.h>
#include <fstream>
#include <string>
#include <vector>

class Application : public GfApplication
{
public:
    Application();
    bool parseOptions();
    void initialize(bool bLoggingEnabled, int argc = 0, char **argv = nullptr);
    int run();

private:
    struct entry
    {
        std::string file, section, key, value, refvalue, tgtvalue;
    };

    int getcwd(std::string &s) const;
    int parse_dir(const std::string &dir);
    int parse_file(const std::string &dir);
    int parse_section(void *h, const std::string &section);
    int parse_param(void *h, const std::string &section,
        const std::string &param);
    int setup_uniq();
    size_t untranslated() const;
    bool interactive() const;
    int translate();
    int translate(const entry &entry);
    int translate(std::ifstream &f);
    int dump() const;
    int save() const;
    int save(const entry &entry, std::vector<std::string> &patched) const;
    int ensure_target() const;
    int insert_target(int n) const;
    bool ispatched(const std::vector<std::string> &files,
        const std::string &file) const;
    std::vector<entry> entries, unique, translated;
    std::string cwd, target, ref, language, infile, outfile;
    static const std::string menulang;
};

#endif
