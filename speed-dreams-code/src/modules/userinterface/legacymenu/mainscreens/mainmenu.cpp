/***************************************************************************

    file                 : mainmenu.cpp
    created              : Sat Mar 18 23:42:38 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <tgfclient.h>
#include <musicplayer.h>
#include <notification.h>
#include <translation.h>

#include <racescreens.h>
#include <playerconfig.h>

#include "downloadsmenu.h"
#include "mainmenu.h"
#include "exitmenu.h"
#include "optionsmenu.h"
#include "creditsmenu.h"

#define TRACK_METADATA_SECT "track metadata"

static void *MenuHandle = 0;
static NotificationManager notifications;


static void
onPlayerConfigMenuActivate(void * /* dummy */)
{
    /* Here, we need to call OptionOptionInit each time the firing button
       is pressed, and not only once at the Main menu initialization,
       because the previous menu has to be saved (ESC, Back) and because it can be this menu,
       as well as the Raceman menu */
    GfuiScreenActivate(PlayerConfigMenuInit(MenuHandle));
}

static void
onRaceSelectMenuActivate(void * /* dummy */)
{
    GfuiScreenActivate(RmRaceSelectInit(MenuHandle));
}

//static void
//onRaceWESelectMenuActivate(void * /* dummy */)
/*{
	GfuiScreenActivate(RmRaceWESelectInit(MenuHandle));
}*/

static void
onOptionsMenuActivate(void * /* dummy */)
{
    GfuiScreenActivate(OptionsMenuInit(MenuHandle));
}

static void
onCreditsMenuActivate(void * /* dummy */)
{
    CreditsMenuActivate(MenuHandle);
}

static void
onDownloadsMenuActivate(void *)
{
    new DownloadsMenu(MenuHandle);
}

static void
onExitMenuActivate(void * /*dummy*/)
{
    GfuiScreenActivate(ExitMenuInit(MenuHandle));
}

static void recompute(unsigned ms, void *args)
{
    notifications.updateStatus();
}

static void
onMainMenuActivate(void * /* dummy */)
{
    // This must be done after GfuiScreenActivate because this function
    // overwrites the recompute callback to a null pointer.
    GfuiApp().eventLoop().setRecomputeCB(::recompute, nullptr);
}

/*
 * Function
 *	MainMenuInit
 *
 * Description
 *	init the main menu
 *
 * Parameters
 *	none
 *
 * Return
 *	0 ok -1 nok
 *
 * Remarks
 *
 */

void *
MainMenuInit(bool SupportsHumanDrivers)
{
    // Initialize only once.
    if (MenuHandle)
        return MenuHandle;

    MenuHandle = GfuiScreenCreate((float*)NULL,
				    NULL, onMainMenuActivate,
				    NULL, (tfuiCallback)NULL,
				    1);

    void *menuDescHdle = GfuiMenuLoad("mainmenu.xml");

    GfuiMenuCreateStaticControls(MenuHandle, menuDescHdle);

    //Add buttons and create based on xml
    GfuiMenuCreateButtonControl(MenuHandle, menuDescHdle, "race", NULL, onRaceSelectMenuActivate);
    //GfuiMenuCreateButtonControl(MenuHandle, menuDescHdle, "weekend", NULL, onRaceWESelectMenuActivate);
	if (SupportsHumanDrivers)
		GfuiMenuCreateButtonControl(MenuHandle, menuDescHdle, "configure", NULL, onPlayerConfigMenuActivate);
    GfuiMenuCreateButtonControl(MenuHandle, menuDescHdle, "options", NULL, onOptionsMenuActivate);
    GfuiMenuCreateButtonControl(MenuHandle, menuDescHdle, "credits", NULL, onCreditsMenuActivate);
    GfuiMenuCreateButtonControl(MenuHandle, menuDescHdle, "downloads", NULL, onDownloadsMenuActivate);
    GfuiMenuCreateButtonControl(MenuHandle, menuDescHdle, "quit", NULL, onExitMenuActivate);

    GfuiMenuDefaultKeysAdd(MenuHandle);
    GfuiAddKey(MenuHandle, GFUIK_ESCAPE, "Quit the game", NULL, onExitMenuActivate, NULL);

    const musicTrack &track = getMusicTrack();
    std::string artist = GfuiGetTranslatedText(menuDescHdle,
            TRACK_METADATA_SECT "/artist", "text", ""),
        title = GfuiGetTranslatedText(menuDescHdle,
            TRACK_METADATA_SECT "/title", "text", ""),
        license = GfuiGetTranslatedText(menuDescHdle,
            TRACK_METADATA_SECT "/license", "text", ""),
        trackinfo = "\n";

    trackinfo += artist + ": " + track.author + '\n';
    trackinfo += title + ": " + track.name + '\n';
    trackinfo += license + ": " + track.license;
    notifications.msglist.push_back(trackinfo);

    int version = GfuiMenuCreateLabelControl(MenuHandle, menuDescHdle, "version");

    if (version >= 0)
        GfuiLabelSetText(MenuHandle, version, GfuiApp().version().c_str());

    GfParmReleaseHandle(menuDescHdle);
    return MenuHandle;
}

/*
 * Function
 *	MainMenuRun
 *
 * Description
 *	Activate the main menu
 *
 * Parameters
 *	none
 *
 * Return
 *	0 ok -1 nok
 *
 * Remarks
 *
 */
int
MainMenuRun(void)
{
    GfuiScreenActivate(MenuHandle);

    return 0;
}
