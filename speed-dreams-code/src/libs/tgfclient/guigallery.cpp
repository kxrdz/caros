/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

 #include "gui.h"
#include "tgfclient.h"
#include <portability.h>
#include <tgfclient.h>
#include <tgf.h>
#include <string>

void GfuiGallery()
{
	if (!GfuiScreen || !GfuiScreen->gallery)
		return;

	tGfuiGallery *gallery = GfuiScreen->gallery;
	const char *file = gallery->file().c_str();

	if (file != GfuiScreen->bgImagePath)
		GfuiScreenAddBgImg(GfuiScreen, file);

	gallery->update();
}

const std::string &tGfuiGallery::dir() const
{
	return dirname;
}

const std::string &tGfuiGallery::file() const
{
	return fullpath;
}

int tGfuiGallery::open()
{
	const char *cdirname = dirname.c_str();
	tFList *list = GfDirGetList(cdirname);
	const tFList *entry = list;

	if (!list)
	{
		GfLogError("%s: GfDirGetList failed\n", cdirname);
		return -1;
	}

	do
	{
		if (entry->type != FList::file)
			continue;

		std::string fullpath = dirname + "/" + entry->name;
		const char *cfullpath = fullpath.c_str();

		if (GfTexTryPNGImage(cfullpath) && GfTexTryJPEGImage(cfullpath))
			continue;

		files.push_back(entry->name);
	} while ((entry = entry->next) != list);

	GfDirFreeList(list, NULL, true, true);

	if (files.empty())
	{
		GfLogWarning("%s contains no images\n", cdirname);
		return -1;
	}

	return 0;
}

int tGfuiGallery::init_direction()
{
	unsigned direction;

	if (portability::rand(&direction, sizeof direction))
	{
		GfLogError("portability::rand failed\n");
		return -1;
	}

	direction %= tGfuiGallery::N_DIRECTIONS;

	switch (direction)
	{
		case DIR_RIGHT:
			xoffset = overscan;
			yoffset = overscan / 2.0f;
			break;

		case DIR_LEFT:
			xoffset = 0;
			yoffset = overscan / 2.0f;
			break;

		case DIR_DOWN:
			xoffset = overscan / 2.0f;
			yoffset = 0;
			break;

		case DIR_UP:
			xoffset = overscan / 2.0f;
			yoffset = overscan;
			break;
	}

	this->direction = direction;
	return 0;
}

int tGfuiGallery::init()
{
	size_t index;
	std::string newpath;

	do
	{
		if (portability::rand(&index, sizeof index))
		{
			GfLogError("portability::rand failed\n");
			return -1;
		}

		index %= files.size();
		newpath = dirname + "/" + files.at(index);
	} while (newpath == fullpath);

	fullpath = newpath;
	GfuiScreenAddBgImg(GfuiScreen, fullpath.c_str());
	gfuiColors[GFUI_BASECOLORBGIMAGE][0] = 0;
	gfuiColors[GFUI_BASECOLORBGIMAGE][1] = 0;
	gfuiColors[GFUI_BASECOLORBGIMAGE][2] = 0;
	color = 0;
	state = FADEIN;

	if (init_direction())
		return -1;

	return 0;
}

int tGfuiGallery::fadein()
{
	if ((color += 0.005f) >= 0.6f)
	{
		time = GfTimeClock();
		color = 0.6f;
		state = SHOW;
	}

	return 0;
}

int tGfuiGallery::show()
{
	const double seconds = 8.0f;

	if (GfTimeClock() - time >= seconds)
		state = FADEOUT;

	return 0;
}

int tGfuiGallery::fadeout()
{
	if ((color -= 0.005f) <= 0)
	{
		color = 0;
		state = INIT;
	}

	return 0;
}

void tGfuiGallery::move()
{
	const float speed = 0.0375f;

	switch (direction)
	{
		case DIR_LEFT:
			if ((xoffset += speed) >= overscan)
				xoffset = overscan;
			break;

		case DIR_RIGHT:
			if ((xoffset -= speed) <= 0)
				xoffset = 0;
			break;

		case DIR_UP:
			if ((yoffset -= speed) <= 0)
				yoffset = 0;
			break;

		case DIR_DOWN:
			if ((yoffset += speed) >= overscan)
				yoffset = overscan;
			break;
	}

	GfuiScreen->xoffset = xoffset;
	GfuiScreen->yoffset = yoffset;
	GfuiScreen->overscan = overscan;
}

void tGfuiGallery::setcolor() const
{
	gfuiColors[GFUI_BASECOLORBGIMAGE][0] = color;
	gfuiColors[GFUI_BASECOLORBGIMAGE][1] = color;
	gfuiColors[GFUI_BASECOLORBGIMAGE][2] = color;
}

int tGfuiGallery::update()
{
#define INVOKE(x, y)  ((*x).*(y))
	typedef int (tGfuiGallery::*fn)();

	static const fn functions[] =
	{
		&tGfuiGallery::init,
		&tGfuiGallery::fadein,
		&tGfuiGallery::show,
		&tGfuiGallery::fadeout
	};

	if (INVOKE(this, functions[state])())
		return -1;

	move();
	setcolor();
	return 0;
}

tGfuiGallery::tGfuiGallery(const char *dirname) :
	state(INIT),
	dirname(dirname),
	overscan(30.0f),
	color(0),
	xoffset(0),
	yoffset(0),
	time(0)
{
}
