/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2015 MadBad (madbad82@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <tgfclient.h>
#include <vector>
#include <string>
#include <ctime>

class TGFCLIENT_API NotificationManager {

	public:
        enum
        {
            Idle,
            Sending,
            Receiving
        } status;

		//a list of notification messages
		std::vector<std::string> msglist;

		std::clock_t animationLastExecTime; //the current time

		//constructor
		NotificationManager();

		//destructor
		~NotificationManager();

		void updateStatus();


	private:
		void startNewNotification();
		void runAnimation();
		void removeOldUi();
		void createUi();
		void updateStatusUi();

		void* screenHandle;
		void* prevScreenHandle;
		void* menuXMLDescHdle;
		int	notifyUiIdBg;//the bg image uiid
		int notifyUiIdBusyIcon; //the busy icon
		std::vector<int> notifyUiId;//the text lines uiid
		bool busy;
		int textPadding;
		std::clock_t animationStartTime; //when the animation started
		std::clock_t animationRestStartTime; //when the animation started

		float totalAnimationDuration;//how much the animation should take to fully run in one direction
		float animationRestTime; //how much we should wait when we are fully displayed
		int animationDirection;
		int propertyFinalValue;
		std::vector<std::string> messageLines;
		int propertyChangeNeeded;

};

#endif
