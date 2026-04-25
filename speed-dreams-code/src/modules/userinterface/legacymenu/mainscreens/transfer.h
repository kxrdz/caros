/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2026 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef TRANSFER_H
#define TRANSFER_H

#include "sink.h"
#include <curl/curl.h>
#include <cstddef>
#include <string>
#include <vector>

class transfer
{
public:
    struct header
    {
        header() = default;
        header(const std::string &field, const std::string & value) :
            field(field), value(value) {}
        std::string field, value;
    };

    typedef std::vector<header> headers;
    typedef int (*cb)(CURLcode, CURL *, const sink *, std::string &, void *,
        const transfer::headers &);
    transfer(sink *s, cb f, void *args);
    int start(const char *url, long max, const headers &headers);
    int exec(CURLcode result, std::string &error) const;
    virtual ~transfer();
    CURL *handle() const;

protected:
    static size_t on_header(char *buffer, size_t size, size_t nitems,
        void *userdata);
    headers resp_headers;
    int check(CURLcode result, std::string &error) const;

private:
    static int write(char *ptr, size_t size, size_t nmemb, void *userdata);
    sink *const s;
    const cb f;
    void *const args;
    CURL *h;
    struct curl_slist *hlist;
    int get_headers(const transfer::headers &headers,
        struct curl_slist **out) const;
};

#endif
