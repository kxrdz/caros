/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "ptransfer.h"
#include "transfer.h"
#include <list>
#include <string>
#include <vector>

class dispatcher
{
public:
    dispatcher();
    ~dispatcher();
    int add(const char *url, const char *path, long max, sink *s,
        transfer::cb f, void *args,
        const transfer::headers &headers = transfer::headers());
    int add(const char *url, const std::vector<ptransfer::form> &forms,
        long max, sink *s, transfer::cb f, void *args,
        const transfer::headers &headers = transfer::headers());
    int add(const char *url, const void *data, size_t n, long max, sink *s,
        transfer::cb f, void *args,
        const transfer::headers &headers = transfer::headers());
    int add(const char *url, long max, sink *s, transfer::cb f, void *args,
        const transfer::headers &headers = transfer::headers());
    int run(unsigned &ms, std::string &error);

private:
    int dispatch(const struct CURLMsg *m, std::string &error);

    CURLM *const multi;
    std::list<transfer *> transfers;
};

#endif
