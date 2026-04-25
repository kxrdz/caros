/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2026 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "transfer.h"
#include <tgf.h>
#include <curl/curl.h>
#include <string>

size_t transfer::on_header(char *buffer, size_t size, size_t nitems,
    void *userdata)
{
    transfer *tb = static_cast<transfer *>(userdata);

    if (tb->resp_headers.size() > 100)
    {
        GfLogError("Exceeded maximum number of HTTP headers\n");
        return 0;
    }

    size_t ret = size * nitems;
    std::string header(buffer, ret);
    size_t sep = header.find(':');

    if (sep == std::string::npos)
        return ret;

    std::string field = header.substr(0, sep++);

    while (sep < header.size()
        && (header.at(sep) == ' ' || header.at(sep) == '\t'))
        sep++;

    if (sep >= header.size())
        return ret;

    struct header h(field, header.substr(sep));

    tb->resp_headers.push_back(h);
    return ret;
}

int transfer::get_headers(const transfer::headers &headers,
    struct curl_slist **out) const
{
    struct curl_slist *l = nullptr;

    for (const auto &header : headers)
    {
        std::string s = header.field + ": " + header.value;
        struct curl_slist *sl = curl_slist_append(l, s.c_str());

        if (!sl)
        {
            GfLogError("curl_slist_append failed\n");
            goto failure;
        }

        l = sl;
    }

    *out = l;
    return 0;

failure:
    curl_slist_free_all(l);
    return -1;
}

int transfer::check(CURLcode result, std::string &error) const
{
    int ret = -1;
    long response;
    CURLcode code;
    char *url;

    if ((code = curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE,
        &response)) != CURLE_OK)
    {
        const char *e = curl_easy_strerror(code);

        error = "Failed to retrieve response code: ";
        error += e;
    }
    else if ((code = curl_easy_getinfo(h, CURLINFO_EFFECTIVE_URL, &url))
        != CURLE_OK)
    {
        const char *e = curl_easy_strerror(code);

        error = "Failed to retrieve effective URL: ";
        error += e;
    }
    else if (result != CURLE_OK && result != CURLE_HTTP_RETURNED_ERROR)
    {
        const char *e = curl_easy_strerror(result);

        error = url;
        error += ": ";
        error += e;
    }
    else if (response != 200)
    {
        error = url;
        error += ": unexpected HTTP status ";
        error += std::to_string(response);
    }
    else
        ret = 0;

    if (ret)
        GfLogError("%s\n", error.c_str());

    return ret;
}

int transfer::write(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    sink *s = static_cast<sink *>(userdata);
    size_t total = size * nmemb;

    if (size != 0 && total / size != nmemb)
    {
        GfLogError("size calculation wrapped around\n");
        return !size;
    }
    else if (s->append(ptr, total))
    {
        GfLogError("append failed\n");
        return !size;
    }

    return total;
}

int transfer::start(const char *url, long max, const headers &headers)
{
    CURLcode code;
    CURL *h = curl_easy_init();
    struct curl_slist *list = nullptr;

    if (!h)
    {
        GfLogError("curl_easy_init failed\n");
        goto failure;
    }
    else if (get_headers(headers, &list))
    {
        GfLogError("get_headers failed\n");
        goto failure;
    }
    else if ((code = curl_easy_setopt(h, CURLOPT_URL, url)) != CURLE_OK)
    {
        GfLogError("curl_easy_setopt url: %s\n", curl_easy_strerror(code));
        goto failure;
    }
#if LIBCURL_VERSION_MAJOR > 7 || \
    (LIBCURL_VERSION_MAJOR == 7 && LIBCURL_VERSION_MINOR >= 85)
    else if ((code = curl_easy_setopt(h, CURLOPT_PROTOCOLS_STR,
        "http,https")) != CURLE_OK)
    {
        GfLogError("curl_easy_setopt protocols: %s\n",
            curl_easy_strerror(code));
        goto failure;
    }
#else
    else if ((code = curl_easy_setopt(h, CURLOPT_PROTOCOLS,
        CURLPROTO_HTTP | CURLPROTO_HTTPS)) != CURLE_OK)
    {
        GfLogError("curl_easy_setopt protocols: %s\n",
            curl_easy_strerror(code));
        goto failure;
    }
#endif
    else if ((code = curl_easy_setopt(h, CURLOPT_HTTPHEADER, list)) != CURLE_OK)
    {
        GfLogError("curl_easy_setopt http header: %s\n",
            curl_easy_strerror(code));
        goto failure;
    }
    else if ((code = curl_easy_setopt(h, CURLOPT_HEADERFUNCTION, on_header))
        != CURLE_OK)
    {
        GfLogError("curl_easy_setopt header function: %s\n",
            curl_easy_strerror(code));
        goto failure;
    }
    else if ((code = curl_easy_setopt(h, CURLOPT_HEADERDATA, this)) != CURLE_OK)
    {
        GfLogError("curl_easy_setopt header data: %s\n",
            curl_easy_strerror(code));
        goto failure;
    }
    else if ((code = curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L))
        != CURLE_OK)
    {
        GfLogError("curl_easy_setopt follow location: %s\n",
            curl_easy_strerror(code));
        goto failure;
    }
    /* This value is the documented default since cURL 8.3.0.
     * Make it explicit to follow the same behaviour on older versions. */
    else if ((code = curl_easy_setopt(h, CURLOPT_MAXREDIRS, 30L))
        != CURLE_OK)
    {
        GfLogError("curl_easy_setopt max redirs: %s\n",
            curl_easy_strerror(code));
        goto failure;
    }
    else if (s)
    {
        if ((code = curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, write))
            != CURLE_OK)
        {
            GfLogError("curl_easy_setopt writefunction: %s\n",
                curl_easy_strerror(code));
            return -1;
        }
        else if ((code = curl_easy_setopt(h, CURLOPT_MAXFILESIZE, max)))
        {
            GfLogError("curl_easy_setopt maxfilesize: %s\n",
                curl_easy_strerror(code));
            return -1;
        }
        else if ((code = curl_easy_setopt(h, CURLOPT_FAILONERROR, 1L)))
        {
            GfLogError("curl_easy_setopt failonerror: %s\n",
                curl_easy_strerror(code));
            return -1;
        }
        else if ((code = curl_easy_setopt(h, CURLOPT_WRITEDATA,
            static_cast<void *>(s))) != CURLE_OK)
        {
            GfLogError("curl_easy_setopt writedata: %s\n",
                curl_easy_strerror(code));
            return -1;
        }
    }

    this->hlist = list;
    this->h = h;
    return 0;

failure:
    curl_slist_free_all(list);
    curl_easy_cleanup(h);
    return -1;
}

CURL *transfer::handle() const
{
    return h;
}

transfer::~transfer()
{
    if (s)
        delete s;

    curl_slist_free_all(hlist);
    curl_easy_cleanup(h);
}

int transfer::exec(CURLcode result, std::string &error) const
{
    if (s)
        s->flush();

    return check(result, error) || f(result, h, s, error, args, resp_headers);
}

transfer::transfer(sink *s, cb f, void *args) :
    s(s),
    f(f),
    args(args),
    h(nullptr),
    hlist(nullptr)
{
}
