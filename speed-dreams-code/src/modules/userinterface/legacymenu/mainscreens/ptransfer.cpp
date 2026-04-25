/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2026 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "ptransfer.h"
#include "transfer.h"
#include <tgf.h>

int ptransfer::start(const char *url, long max, const void *data, size_t n,
    const transfer::headers &headers)
{
    CURLcode code;
    std::string form;

    if (transfer::start(url, max, headers))
    {
        GfLogError("transfer::start failed\n");
        return -1;
    }
    else if ((code = curl_easy_setopt(handle(), CURLOPT_POST, 1L)) != CURLE_OK)
    {
        GfLogError("curl_easy_setopt post: %s\n", curl_easy_strerror(code));
        return -1;
    }
    else if ((code = curl_easy_setopt(handle(), CURLOPT_POSTFIELDSIZE, n))
        != CURLE_OK)
    {
        GfLogError("curl_easy_setopt postfield size: %s\n",
            curl_easy_strerror(code));
        return -1;
    }
    else if (data
        && (code = curl_easy_setopt(handle(), CURLOPT_COPYPOSTFIELDS, data))
            != CURLE_OK)
    {
        GfLogError("curl_easy_setopt copypostfields: %s\n",
            curl_easy_strerror(code));
        return -1;
    }

    return 0;
}

int ptransfer::start(const char *url, long max, const std::vector<form> &forms,
    const transfer::headers &headers)
{
    int ret = -1;
    CURLcode code;
    std::string form;
    char *escaped = nullptr;

    if (transfer::start(url, max, headers))
    {
        GfLogError("transfer::start failed\n");
        goto end;
    }
    else if (!(escaped = curl_easy_escape(handle(), form.c_str(), 0)))
    {
        GfLogError("curl_easy_escape failed\n");
        goto end;
    }
    else if ((code = curl_easy_setopt(handle(), CURLOPT_COPYPOSTFIELDS, escaped))
        != CURLE_OK)
    {
        GfLogError("curl_easy_setopt copypostfields: %s\n",
            curl_easy_strerror(code));
        goto end;
    }

    ret = 0;

end:
    curl_free(escaped);
    return ret;
}

int ptransfer::start(const char *url, long max, const char *path,
    const transfer::headers &headers)
{
    CURLcode code;
    curl_mime *mime = nullptr;
    curl_mimepart *part;

    if (transfer::start(url, max, headers))
    {
        GfLogError("transfer::start failed\n");
        goto failure;
    }
    else if (!(mime = curl_mime_init(handle())))
    {
        GfLogError("curl_mime_init failed\n");
        goto failure;
    }
    else if (!(part = curl_mime_addpart(mime)))
    {
        GfLogError("curl_mime_addpart failed\n");
        goto failure;
    }
    else if ((code = curl_mime_name(part, "filename")) != CURLE_OK)
    {
        GfLogError("curl_mime_name: %s\n", curl_easy_strerror(code));
        goto failure;
    }
    else if ((code = curl_mime_filedata(part, path)) != CURLE_OK)
    {
        GfLogError("curl_mime_filedata: %s\n", curl_easy_strerror(code));
        goto failure;
    }
    else if ((code = curl_easy_setopt(handle(), CURLOPT_MIMEPOST, mime))
        != CURLE_OK)
    {
        GfLogError("curl_easy_setopt mimepost: %s\n",
            curl_easy_strerror(code));
        goto failure;
    }

    return 0;

failure:
    curl_mime_free(mime);
    return -1;
}

ptransfer::ptransfer(sink *s, cb f, void *args) :
    transfer(s, f, args)
{
}
