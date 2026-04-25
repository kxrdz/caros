/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "app.h"
#include "linexml.h"
#include <tgf.h>
#include <tgfclient.h>
#include <portability.h>
#include <osspec.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

int Application::parse_param(void *h, const std::string &section,
    const std::string &param)
{
    std::string suffix = "-" + ref;
    auto pos = param.find(suffix);
    const char *refvalue, *csection = section.c_str();

    // Ignore attributes not ending with "-<ref>".
    if (pos == std::string::npos
        || pos + suffix.size() != param.size())
        return 0;
    // Ignore non-text entries.
    else if (!(refvalue = GfParmGetStr(h, csection, param.c_str(), nullptr)))
        return 0;

    std::string key = param.substr(0, pos);
    const char *value = GfParmGetStr(h, csection, key.c_str(), nullptr);

    // Reference key (i.e., "<param>-<ref>") not found. Skip.
    if (!value)
        return 0;

    std::string tgtkey = key + "-" + target;
    const char *tgtvalue = GfParmGetStr(h, csection, tgtkey.c_str(), "");
    entry entry;

    entry.file = GfParmGetFileName(h);
    entry.section = section;
    entry.key = key;
    entry.value = value;
    entry.refvalue = refvalue;
    entry.tgtvalue = tgtvalue;
    entries.push_back(entry);
    return 0;
}

int Application::parse_section(void *h, const std::string &section)
{
    const char *csection = section.c_str();
    std::vector<std::string> params = GfParmListGetParamsNamesList(h, csection);

    for (const auto &param : params)
        if (parse_param(h, section, param))
            return -1;

    for (int result = GfParmListSeekFirst(h, csection);
        !result;
        result = GfParmListSeekNext(h, csection))
    {
        const char *name = GfParmListGetCurEltName(h, csection);;

        if (name)
        {
            std::string subsection = section + "/" + name;

            if (parse_section(h, subsection))
                return -1;
        }
    }

    return 0;
}

int Application::parse_file(const std::string &file)
{
    void *h = GfParmReadFile(file, GFPARM_RMODE_STD);
    std::vector<std::string> sections;

    if (!h)
        // Not a valid XML file. Skip.
        return 0;

    sections = GfParmListGetSectionNamesList(h);

    GfLogInfo("%s: version %d.%d\n", file.c_str(),
        GfParmGetMajorVersion(h), GfParmGetMinorVersion(h));

    for (const auto &section : sections)
        if (parse_section(h, section))
            return -1;

    GfParmReleaseHandle(h);
    return 0;
}

int Application::parse_dir(const std::string &dir)
{
    const char *cdirname = dir.c_str();
    tFList *list = GfDirGetList(cdirname);
    const tFList *entry = list;

    if (!list)
    {
        GfLogError("%s: GfDirGetList failed\n", cdirname);
        return -1;
    }

    do
    {
        std::string fullpath = dir + "/" + entry->name;

        if (entry->type == FList::dir)
        {
            if (parse_dir(fullpath))
                return -1;
        }
        else if (entry->type == FList::file)
        {
            const char *ext = strstr(entry->name, PARAMEXT);

            if (!ext || *(ext + strlen(PARAMEXT)))
                continue;
            else if (parse_file(fullpath))
                return -1;
        }
    } while ((entry = entry->next) != list);

    GfDirFreeList(list, NULL, true, true);
    return 0;
}

int Application::setup_uniq()
{
    for (const auto &entry : entries)
    {
        bool found = false;

        for (const auto &uniq : unique)
            if (uniq.value == entry.value)
            {
                found = true;
                break;
            }

        if (found)
            continue;

        unique.push_back(entry);
    }

    return 0;
}

size_t Application::untranslated() const
{
    size_t ret = 0;

    for (const auto &entry : unique)
        if (entry.tgtvalue.empty())
            ret++;

    return ret;
}

int Application::translate(const entry &entry)
{
    size_t untr = untranslated();
    GfLogInfo("%lu entries, %lu of them unique, %lu translated (%.1f%%)\n",
        entries.size(), unique.size(), unique.size() - untr,
        100.0f - (100.0f * untr / unique.size()));

    std::cout <<
        "\nFile: " << entry.file << '\n'
        << "Section: " << entry.section << '\n'
        << "Key: " << entry.key << '\n'
        << "Original text: \"" << entry.value << "\"\n"
        << "Reference language text (" << ref << "): \"" << entry.refvalue
        << "\"\n";

    std::string line;

    if (std::getline(std::cin, line, '\n'))
    {
        for (auto &u : unique)
            if (u.key == entry.key
                && u.value == entry.value
                && u.tgtvalue.empty())
                u.tgtvalue = line;

        for (auto &e : entries)
            if (e.key == entry.key
                && e.value == entry.value
                && e.tgtvalue.empty())
            {
                e.tgtvalue = line;
                translated.push_back(e);
            }
    }
    else
        return 1;

    return 0;
}

int Application::translate()
{
    GfLogInfo("Started translation session.\n");
    GfLogInfo("Press Ctrl+D on Unix-like systems "
        "(or Enter + Ctrl+Z on Windows systems) to end the session "
        "and save your changes.\n\n");

    size_t untr = untranslated();

    for (const auto &entry : unique)
        if (entry.tgtvalue.empty() && translate(entry))
            break;

    GfLogInfo("Session finished. Translated %lu strings", translated.size());

    if (untr)
        GfLogInfo(" (%.1f%%)", 100.0f - (100.0f * untranslated() / untr));

    GfLogInfo(".\n");
    return 0;
}

int Application::translate(std::ifstream &f)
{
    std::string untr;
    size_t n_untr = untranslated();

    while (std::getline(f, untr, '\n'))
    {
        std::string tr;

        if (!std::getline(std::cin, tr, '\n'))
            break;

        for (auto &entry : unique)
            if (entry.tgtvalue.empty() && entry.value == untr)
                entry.tgtvalue = tr;

        for (auto &entry : entries)
            if (entry.tgtvalue.empty() && entry.value == untr)
            {
                entry.tgtvalue = tr;
                translated.push_back(entry);
            }
    }

    GfLogInfo("Translated %lu strings", translated.size());

    if (n_untr)
        GfLogInfo(" (%.1f%%)", 100.0f - (100.0f * untranslated() / n_untr));

    GfLogInfo(".\n");
    return 0;
}

bool Application::ispatched(const std::vector<std::string> &files,
    const std::string &file) const
{
    for (const auto &f : files)
        if (f == file)
            return true;

    return false;
}

int Application::save(const entry &entry, std::vector<std::string> &patched) const
{
    std::string path = entry.file + ".tmp", indent, line;
    std::ifstream inf(entry.file, std::ios::binary);
    std::ofstream outf(path, std::ios::binary);
    lxml::params params;
    int lineno = lxml::key(entry.file, entry.section, entry.key, params, indent);
    const char *tmppath = path.c_str(), *newpath = entry.file.c_str();

    if (!outf.is_open())
    {
        GfLogError("Could not open %s for writing\n", path.c_str());
        return -1;
    }
    else if (lineno < 0)
    {
        GfLogError("%s: failed to locate \"%s\" in section \"%s\"\n",
            entry.file.c_str(), entry.value.c_str(), entry.section.c_str());
        goto failure;
    }
    else if (!inf.is_open())
    {
        GfLogError("Could not open %s for reading\n", entry.file.c_str());
        goto failure;
    }

    for (int i = 1; std::getline(inf, line, '\n'); i++)
    {
        if (i == params.line)
        {
            bool bump_version = !ispatched(patched, entry.file);

            lxml::write(outf, params, bump_version);

            if (bump_version)
                patched.push_back(entry.file);
        }
        else
        {
            outf << line << '\n';

            if (i == lineno)
            {
                std::string key = entry.key + "-" + target;

                lxml::write(outf, indent, key, entry.tgtvalue);
            }
        }
    }

    inf.close();
    outf.close();

    if (rename(tmppath, newpath))
    {
        GfLogError("Failed to rename %s to %s: %s\n", tmppath, newpath,
            strerror(errno));
        goto failure;
    }

    return 0;

failure:
    outf.close();

    if (remove(tmppath))
        GfLogError("Failed to remove %s: %s\n", tmppath, strerror(errno));

    return -1;
}

int Application::save() const
{
    std::vector<std::string> patched;

    for (const auto &entry : translated)
        if (save(entry, patched))
            return -1;

    return 0;
}

int Application::dump() const
{
    if (chdir(cwd.c_str()))
    {
        GfLogError("chdir failed\n");
        return EXIT_FAILURE;
    }

    std::ofstream f(outfile, std::ios::binary);

    if (chdir(GfDataDir()))
    {
        GfLogError("chdir failed\n");
        return EXIT_FAILURE;
    }
    else if (!f.is_open())
    {
        GfLogError("Could not open %s for writing\n", outfile.c_str());
        return -1;
    }

    for (const auto &entry : unique)
        if (entry.tgtvalue.empty())
        {
            const std::string &value = entry.value;

            f.write(value.c_str(), value.size());
            f.put('\n');
        }

    f.flush();
    return 0;
}

bool Application::interactive() const
{
    return infile.empty() && outfile.empty();
}

const std::string Application::menulang = "data/menu/languagemenu.xml";

int Application::insert_target(int n) const
{
    const char *cmenu = menulang.c_str();
    std::string path = menulang + ".tmp", indent, line;
    std::ifstream inf(menulang, std::ios::binary);
    std::ofstream outf(path, std::ios::binary);
    std::string section = "Languages/" + std::to_string(n++);
    const char *tmppath = path.c_str();
    int lineno = lxml::section(menulang, section, indent);

    if (!outf.is_open())
    {
        GfLogError("Could not open %s for writing\n", path.c_str());
        return -1;
    }
    else if (lineno < 0)
    {
        GfLogError("%s: failed to locate section \"%s\"\n", cmenu,
            section.c_str());
        goto failure;
    }
    else if (!inf.is_open())
    {
        GfLogError("Could not open %s for reading\n", cmenu);
        goto failure;
    }

    for (int i = 1; std::getline(inf, line, '\n'); i++)
    {
        outf << line << '\n';

        if (i == lineno)
            lxml::write(outf, n, indent, target, language);
    }

    inf.close();
    outf.close();

    if (rename(tmppath, cmenu))
    {
        GfLogError("Failed to rename %s to %s: %s\n", tmppath, cmenu,
            strerror(errno));
        goto failure;
    }

    return 0;

failure:
    outf.close();

    if (remove(tmppath))
        GfLogError("Failed to remove %s: %s\n", tmppath, strerror(errno));

    return -1;
}

int Application::ensure_target() const
{
    bool found = false;
    void *h = GfParmReadFile(menulang, GFPARM_RMODE_STD);
    int n;

    if (!h)
    {
        GfLogError("GfParmReadFile %s failed\n", menulang.c_str());
        return -1;
    }

    n = GfParmGetEltNb(h, "Languages");

    for (int i = 1; i <= n; i++)
    {
        std::string section = "Languages/" + std::to_string(i);
        const char *code = GfParmGetStr(h, section.c_str(), "code", "");

        if (code == target)
        {
            found = true;
            break;
        }
    }

    GfParmReleaseHandle(h);

    if (!found)
    {
        GfLogInfo("Inserting language %s (code %s) into %s\n",
            language.c_str(), target.c_str(), menulang.c_str());
        return insert_target(n);
    }

    return 0;
}

int Application::run()
{
    static const char *const dirs[] =
    {
        "config",
        "data/menu",
        "data/tracks"
    };

    GfLogDefault.setLevelThreshold(GfLogger::eError);

    for (const auto dir : dirs)
    {
        std::string fulldir = GfDataDir() + std::string(dir);

        if (parse_dir(fulldir.c_str()))
            return EXIT_FAILURE;
    }

    GfLogDefault.setLevelThreshold(GfLogger::eInfo);

    if (setup_uniq())
        return EXIT_FAILURE;
    else if (interactive())
    {
        if (translate() || save() || ensure_target())
            return EXIT_FAILURE;
    }
    else if (!outfile.empty())
    {
        if (dump())
            return EXIT_FAILURE;
    }
    else if (!infile.empty())
    {
        if (chdir(cwd.c_str()))
        {
            GfLogError("chdir failed\n");
            return EXIT_FAILURE;
        }

        std::ifstream f(infile, std::ios::binary);

        if (chdir(GfDataDir()))
        {
            GfLogError("chdir failed\n");
            return EXIT_FAILURE;
        }
        else if (!f.is_open())
        {
            GfLogError("Could not open %s for reading\n", infile.c_str());
            return EXIT_FAILURE;
        }
        else if (translate(f) || save() || ensure_target())
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void Application::initialize(bool bLoggingEnabled, int argc, char **argv)
{
    GfApplication::initialize(bLoggingEnabled, argc, argv);

    registerOption("t", "target", true);
    registerOption("l", "language", true);
    registerOption("r", "ref", true);
    registerOption("i", "input", true);
    registerOption("o", "output", true);

    addOptionsHelpSyntaxLine("-t|--target <target>");
    addOptionsHelpSyntaxLine("[-l|--language <language>]");
    addOptionsHelpSyntaxLine("[-r|--ref <ref>]");
    addOptionsHelpSyntaxLine("[-i|--input <infile>]");
    addOptionsHelpSyntaxLine("[-o|--output <outfile>]");

    addOptionsHelpSyntaxLine("<target>  : Set target language");
    addOptionsHelpSyntaxLine("<language>: Common name for the target language "
        "(e.g.: Esperanto, English, Russian, etc.). Required unless "
        "-o|--output is used.");
    addOptionsHelpSyntaxLine("<ref>     : Set reference language (default=cat)");
    addOptionsHelpSyntaxLine("<infile>  : Set input file with a line-separated "
        "list of untranslated strings, as returned by -o|--output. "
        "Another file with the translated strings must be provided from "
        "standard input.");
    addOptionsHelpSyntaxLine("<outfile> : Output file with a line-separated "
        "list of untranslated strings");
}

bool Application::parseOptions()
{
    if (!GfApplication::parseOptions())
        return false;

    for (const auto &opt : _lstOptions)
    {
        if (!opt.bFound)
            continue;
        else if (opt.strLongName == "target")
            target = opt.strValue;
        else if (opt.strLongName == "language")
            language = opt.strValue;
        else if (opt.strLongName == "ref")
            ref = opt.strValue;
        else if (opt.strLongName == "input")
            infile = opt.strValue;
        else if (opt.strLongName == "output")
            outfile = opt.strValue;
    }

    if (target.empty())
    {
        printUsage("Missing target language\n");
        return false;
    }
    else if (outfile.empty() && language.empty())
    {
        printUsage("Missing target language common name\n");
        return false;
    }
    else if (!infile.empty() && !outfile.empty())
    {
        printUsage("-i|--infile and -o|--outfile cannot be used together\n");
        return false;
    }

    return true;
}

int Application::getcwd(std::string &out) const
{
    int ret = -1;
    size_t n = 1;
    char *s = static_cast<char *>(malloc(n));

    if (!s)
        goto end;

    while (!::getcwd(s, n))
    {
        if (errno != ERANGE)
        {
            GfLogError("getcwd: %s\n", strerror(errno));
            goto end;
        }

        char *ns = static_cast<char *>(realloc(s, ++n));

        if (!ns)
            goto end;

        s = ns;
    }

    out = s;
    out += '/';
    ret = 0;
end:
    free(s);
    return ret;
}

Application::Application() :
    GfApplication("Translate", "0.1.0", "Translation tool for Speed Dreams"),
    ref("cat")
{
    if (getcwd(cwd))
        throw std::runtime_error("Application::getcwd failed");
}
