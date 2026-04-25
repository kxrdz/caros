/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef LINEXML_H
#define LINEXML_H

#include <fstream>
#include <string>

namespace lxml
{

struct params
{
    int line;
    bool has_major, has_minor;
    unsigned major, minor;
    std::string indent, name, type, mode;
};

int key(const std::string &path, const std::string &section,
    const std::string &key, params &params, std::string &indent);
int section(const std::string &path, const std::string &section,
    std::string &indent);
void write(std::ofstream &f, std::string &indent, const std::string &key,
    const std::string &value);
void write(std::ofstream &f, const struct params &params, bool bump_version);
void write(std::ofstream &f, int n, const std::string &indent,
    const std::string &code, const std::string &lang);

}

#endif
