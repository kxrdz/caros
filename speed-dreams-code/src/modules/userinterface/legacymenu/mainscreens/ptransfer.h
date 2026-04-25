/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2026 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef PTRANSFER_H
#define PTRANSFER_H

#include "transfer.h"
#include "sink.h"
#include <curl/curl.h>
#include <string>
#include <vector>

class ptransfer : public transfer
{
public:
    struct form
    {
        std::string key, value;
    };

    ptransfer(sink *s, cb f, void *args);

    int start(const char *url, long max, const char *path,
        const transfer::headers &headers = transfer::headers());
    int start(const char *url, long max, const std::vector<form> &forms,
        const transfer::headers &headers = transfer::headers());
    int start(const char *url, long max, const void *data, size_t n,
        const transfer::headers &headers = transfer::headers());
    int exec(CURLcode result, std::string &error) const;
};

#endif
