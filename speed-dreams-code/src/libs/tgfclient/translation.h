#ifndef TRANSLATION_H
#define TRANSLATION_H

#include "tgfclient_api.h"

TGFCLIENT_API const char *GfuiGetTranslatedText(void *hparm,
                                                const char *pszPath,
                                                const char *pszKey,
                                                const char *pszDefault);

TGFCLIENT_API void gfuiInitLanguage();
TGFCLIENT_API void gfuiShutdownLanguage();

#endif
