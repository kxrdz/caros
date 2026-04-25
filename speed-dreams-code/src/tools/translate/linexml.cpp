/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "linexml.h"
#include <tgf.h>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static std::string escape(const std::string &s)
{
    std::string ret;
    static const struct esc
    {
        char c;
        const char *str;
    } esc[] =
    {
        {.c = '>', .str = "&gt;"},
        {.c = '<', .str = "&lt;"},
        {.c = '&', .str = "&amp;"},
        {.c = '\"', .str = "&quot;"},
        {.c = '\'', .str = "&apos;"},
        {.c = '\n', .str = "<br>"}
    };

    for (const auto c : s)
    {
        bool found = false;

        for (const auto &e : esc)
        {
            if (c == e.c)
            {
                ret += e.str;
                found = true;
                break;
            }
        }

        if (!found)
            ret += c;
    }

    return ret;
}

void lxml::write(std::ofstream &f, int n, const std::string &indent,
    const std::string &code, const std::string &lang)
{
    char c = 0;

    if (!indent.empty())
        c = indent.at(0);

    f << indent << "<section name=\"" + std::to_string(n) + "\">\n";
    f << indent << c << "<attstr name=\"code\" val=\"" + code +"\"/>\n";
    f << indent << c << "<attstr name=\"name\" val=\"" + lang +"\"/>\n";
    f << indent << "</section>\n";
}

void lxml::write(std::ofstream &f, std::string &indent, const std::string &key,
    const std::string &value)
{
    std::string evalue = escape(value);

    f << indent << "<attstr name=\"" << key << "\" val=\"" << evalue << "\"/>\n";
}

void lxml::write(std::ofstream &f, const struct params &params,
    bool bump_version)
{
    unsigned major = params.major , minor = params.minor;

    if (bump_version)
        params.has_minor ? minor++ : major++;

    f << params.indent << "<params name=\"" << params.name << '\"';

    if (!params.type.empty())
        f << " type=\"" << params.type << '\"';

    if (!params.mode.empty())
        f << " mode=\"" << params.mode << '\"';

    if (params.has_major)
    {
        f << " version=\"" << std::to_string(major);

        if (params.has_minor)
            f << "." << std::to_string(minor);

        f << '\"';
    }

    f << ">\n";
}

static const char *skip(const char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;

    return s;
}

static const char *parse(const char *s, std::string &key, std::string &value)
{
    const char *sep = strchr(s, '='), *end;

    if (!sep)
    {
        GfLogError("Missing = in attribute\n");
        return nullptr;
    }

    key = std::string(s, sep - s);

    if (*++sep != '\"')
    {
        GfLogError("Missing start \"\n");
        return nullptr;
    }
    else if (!(end = strchr(++sep, '\"')))
    {
        GfLogError("Missing end \"\n");
        return nullptr;
    }

    value = std::string(sep, end - sep);
    return ++end;
}

static int parse(const std::string &value, unsigned &major, unsigned &minor,
    bool &has_major, bool &has_minor)
{
    const char *s = value.c_str();
    char *end;
    unsigned long v;

    minor = 0;
    v = strtoul(s, &end, 10);

    if (*end && *end != '.')
        return -1;

    has_major = true;
    major = v;

    if (*end++ == '.')
    {
        s = end;
        v = strtoul(s, &end, 10);

        if (*end)
            return -1;

        minor = v;
        has_minor = true;
    }

    return 0;
}

static int parse(const std::string &line, lxml::params &params)
{
    std::string key, value;
    const char *s = line.c_str();

    params.major = params.minor = 0;
    params.has_major = params.has_minor = false;
    s += strlen("<params");

    if (*s != ' ' && *s != '\t')
        return -1;

    while (*(s = skip(s)))
    {
        if (*s == '>')
            break;
        else if (!(s = parse(s, key, value)))
            return -1;
        else if (key == "name")
            params.name = value;
        else if (key == "type")
            params.type = value;
        else if (key == "mode")
            params.mode = value;
        else if (key == "version"
            && parse(value, params.major, params.minor, params.has_major,
                params.has_minor))
            return -1;
    }

    return 0;
}

static int parse(const std::string &line, const std::string &section,
    std::string &outvalue)
{
    std::string key, value;
    const char *s = line.c_str();

    s += strlen("<section");

    if (*s != ' ' && *s != '\t')
        return -1;

    while (*(s = skip(s)))
    {
        if (*s == '>')
            break;
        else if (!(s = parse(s, key, value)))
            return -1;
        else if (key == "name")
        {
            outvalue = value;

            if (value == section)
                return 0;
        }
    }

    return -1;
}

static int parse(const std::string &line, const std::string &name)
{
    std::string key, value;
    const char *s = line.c_str();

    s += strlen("<attstr");

    if (*s != ' ' && *s != '\t')
        return -1;

    while (*(s = skip(s)))
    {
        if (*s == '>' || !strcmp(s, "/>"))
            break;
        else if (!(s = parse(s, key, value)))
            return -1;
        else if (key == "name" && value == name)
            return 0;
    }

    return -1;
}

static std::vector<std::string> split(const std::string &section)
{
    std::vector<std::string> ret;
    std::istringstream stream(section);

    for (std::string line; std::getline(stream, line, '/');)
        ret.push_back(line);

    return ret;
}

static int parse(const std::string &line, int lineno,
    const std::string &section, const std::string &key, std::string &indent,
    lxml::params &params, std::string &last_section, size_t &section_i)
{
    const char *s = line.c_str(), *start = s;
    std::vector<std::string> sections = split(section);

    indent = "";

    while (*s == ' ' || *s == '\t')
        indent += *s++;

    if (!strncmp(s, "<params", strlen("<params")))
    {
        std::string pline = line.substr(s - start);
        lxml::params p;

        if (!parse(pline, p))
        {
            p.indent = indent;
            p.line = lineno;
            params = p;
        }
        else
        {
            GfLogError("Invalid params line:\n%s\n", s);
            return -1;
        }
    }
    else if (!strncmp(s, "<section", strlen("<section")))
    {
        std::string sline = line.substr(s - start);
        lxml::params p;

        if (!parse(sline, sections.at(section_i), last_section)
            && section_i < sections.size())
            section_i++;
    }
    else if (!strcmp(s, "</section>"))
    {
        if (section_i && last_section == sections.at(section_i - 1))
            section_i--;
    }
    else if (!strncmp(s, "<attstr", strlen("<attstr")))
    {
        if (section_i == sections.size())
        {
            std::string aline = line.substr(s - start);

            if (!parse(aline, key))
                return lineno;
        }
    }

    return 0;
}

int lxml::key(const std::string &path, const std::string &section,
    const std::string &key, params &params, std::string &indent)
{
    std::ifstream f(path, std::ios::binary);
    std::string line, last_section;
    size_t section_i = 0;

    if (!f.is_open())
    {
        GfLogError("Failed to open %s for reading\n", path.c_str());
        return -1;
    }

    for (int lineno = 1; std::getline(f, line, '\n'); lineno++)
    {
        int result = parse(line, lineno, section, key, indent, params,
            last_section, section_i);

        if (result < 0)
            return -1;
        else if (result)
            return result;
    }

    return -1;
}

static int parse(const std::string &line, int lineno,
    const std::string &section, std::string &indent,
    std::string &last_section, size_t &section_i)
{
    const char *s = line.c_str(), *start = s;
    std::vector<std::string> sections = split(section);

    indent = "";

    while (*s == ' ' || *s == '\t')
        indent += *s++;

    if (!strncmp(s, "<section", strlen("<section")))
    {
        std::string sline = line.substr(s - start);
        lxml::params p;

        if (!parse(sline, sections.at(section_i), last_section)
            && section_i < sections.size())
            section_i++;
    }
    else if (!strcmp(s, "</section>"))
    {
        if (section_i && last_section == sections.at(section_i - 1))
            return lineno;
    }

    return 0;
}

int lxml::section(const std::string &path, const std::string &section,
    std::string &indent)
{
    std::ifstream f(path, std::ios::binary);
    std::string line, last_section;
    size_t section_i = 0;

    if (!f.is_open())
    {
        GfLogError("Failed to open %s for reading\n", path.c_str());
        return -1;
    }

    for (int lineno = 1; std::getline(f, line, '\n'); lineno++)
    {
        int result = parse(line, lineno, section, indent, last_section,
            section_i);

        if (result < 0)
            return -1;
        else if (result)
            return result;
    }

    return -1;
}
