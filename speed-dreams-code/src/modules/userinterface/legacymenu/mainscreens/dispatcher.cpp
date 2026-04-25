/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "dispatcher.h"
#include <curl/curl.h>
#include <tgf.h>
#include <stdexcept>
#include <string>

int dispatcher::dispatch(const struct CURLMsg *m, std::string &error)
{
    for (auto it = transfers.begin(); it != transfers.end(); it++)
    {
        CURL *h = m->easy_handle;
        transfer *t = *it;

        if (t->handle() == h)
        {
            CURLcode result = m->data.result;
            CURLMcode c;
            int ret;

            ret = t->exec(result, error);

            if ((c = curl_multi_remove_handle(multi, h)) != CURLM_OK)
                GfLogError("curl_multi_remove_handle failed with %s\n",
                    curl_multi_strerror(c));

            delete t;
            transfers.erase(it);
            return ret;
        }
    }

    GfLogError("no suitable easy handle found\n");
    return -1;
}

int dispatcher::run(unsigned &ms, std::string &error)
{
    int ret = 0, running;
    double msd = ms;

    do
    {
        double now = GfTimeClock();
        CURLMcode code = curl_multi_perform(multi, &running);

        if (code != CURLM_OK)
        {
            GfLogError("curl_multi_perform: %s\n", curl_multi_strerror(code));
            return -1;
        }
        else if ((code = curl_multi_poll(multi, NULL, 0, msd, 0)) != CURLM_OK)
        {
            GfLogError("curl_multi_poll: %s\n", curl_multi_strerror(code));
            return -1;
        }

        double elapsed = (GfTimeClock() - now) * 1000.0;

        if (elapsed < msd)
            msd -= elapsed;
        else
            msd = 0;

        int pending = 0;

        do
        {
            const CURLMsg *m = curl_multi_info_read(multi, &pending);

            if (m)
            {
                if (m->msg != CURLMSG_DONE)
                    GfLogError("unexpected msg %d\n", m->msg);
                else if (dispatch(m, error))
                    ret = -1;
            }
        } while (pending);
    } while (running && (unsigned)msd);

    ms = msd;
    return ret;
}

int dispatcher::add(const char *url, const void *data, size_t n, long max,
    sink *s, ptransfer::cb f, void *args, const transfer::headers &headers)
{
    ptransfer *t = new ptransfer(s, f, args);

    if (t->start(url, max, data, n, headers))
    {
        GfLogError("Failed to initialize ptransfer\n");
        goto failure;
    }
    else if (curl_multi_add_handle(multi, t->handle()) != CURLM_OK)
    {
        GfLogError("curl_multi_add_handle failed\n");
        goto failure;
    }

    transfers.push_back(t);
    return 0;

failure:
    delete t;
    return -1;
}

int dispatcher::add(const char *url, const std::vector<ptransfer::form> &forms,
    long max, sink *s, ptransfer::cb f, void *args,
    const transfer::headers &headers)
{
    ptransfer *t = new ptransfer(s, f, args);

    if (t->start(url, max, forms, headers))
    {
        GfLogError("Failed to initialize ptransfer\n");
        goto failure;
    }
    else if (curl_multi_add_handle(multi, t->handle()) != CURLM_OK)
    {
        GfLogError("curl_multi_add_handle failed\n");
        goto failure;
    }

    transfers.push_back(t);
    return 0;

failure:
    delete t;
    return -1;
}

int dispatcher::add(const char *url, const char *path, long max, sink *s,
    ptransfer::cb f, void *args, const transfer::headers &headers)
{
    ptransfer *t = new ptransfer(s, f, args);

    if (t->start(url, max, path, headers))
    {
        GfLogError("Failed to initialize ptransfer\n");
        goto failure;
    }
    else if (curl_multi_add_handle(multi, t->handle()) != CURLM_OK)
    {
        GfLogError("curl_multi_add_handle failed\n");
        goto failure;
    }

    transfers.push_back(t);
    return 0;

failure:
    delete t;
    return -1;
}

int dispatcher::add(const char *url, long max, sink *s, transfer::cb f,
    void *args, const transfer::headers &headers)
{
    transfer *t = new transfer(s, f, args);

    if (t->start(url, max, headers))
    {
        GfLogError("Failed to initialize transfer\n");
        goto failure;
    }
    else if (curl_multi_add_handle(multi, t->handle()) != CURLM_OK)
    {
        GfLogError("curl_multi_add_handle failed\n");
        goto failure;
    }

    transfers.push_back(t);
    return 0;

failure:
    delete t;
    return -1;
}

dispatcher::~dispatcher()
{
    for (const auto &t : transfers)
    {
        curl_multi_remove_handle(multi, t->handle());
        delete t;
    }

    curl_multi_cleanup(multi);
}

dispatcher::dispatcher() :
    multi(curl_multi_init())
{
    if (!multi)
        std::runtime_error("curl_multi_init failed");
}
