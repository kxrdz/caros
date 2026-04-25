/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024-2026 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SHA256_H
#define SHA256_H

#include "hash.h"
#include <cstddef>
#include <string>

class sha256 : public hash
{
public:
    static sha256 &self();
    int run(const std::string &path, std::string &hash) const;
    int run(const void *in, size_t n, std::string &hash) const;

private:
    sha256();
    ~sha256() = default;
    int print(const unsigned char *in, size_t n, std::string &out) const;
    static bool init;
};

#endif
