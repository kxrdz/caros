/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2015 MadBad (madbad82@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "tgfclient.h"
#include "notification.h"
#include <sstream>

//string splitting utils
static std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

static std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

NotificationManager::NotificationManager()
: screenHandle(NULL),
  prevScreenHandle(NULL),
  menuXMLDescHdle(NULL)
{

    this->status = Idle;
    this->busy = false;
    this->notifyUiIdBg = -1;//the bg image ui id
    this->notifyUiIdBusyIcon = -1;//the bg image ui id

    this->animationRestTime = 4 ; //how much wes should wait when we a re fully displayed
    this->totalAnimationDuration = 0.3 ;//how much the animation should take to fully run in one direction
    this->animationLastExecTime = std::clock(); //the current time

}

NotificationManager::~NotificationManager()
{
    if (menuXMLDescHdle)
        GfParmReleaseHandle(menuXMLDescHdle);
}

void NotificationManager::updateStatus(){

    //get the current screen
    this->screenHandle = GfuiGetScreen();

    //get the ui descriptor
    if (!this->menuXMLDescHdle) {
        //do not reuse the descriptor because two or more instances can co-exist
        this->menuXMLDescHdle = GfParmReadFile("data/menu/notifications.xml",
            GFPARM_RMODE_STD, true, true, false);
    }

    //if we are doing nothing and we have some message to display: let's do it
    if(this->busy==false && !this->msglist.empty()){

        this->startNewNotification();

    }

    //if we are running an animation
    if(this->busy==true){

        this->runAnimation();

    }

    //update status icon
    this->updateStatusUi();

    //remember the current screen for the next run
    this->prevScreenHandle = this->screenHandle;

}

void NotificationManager::startNewNotification(){

    //we are running an animation
    this->busy=true;

    //set the animation direction
    this->animationDirection=1;

    //retrieve the message to display
    std::string newText = this->msglist.front();

    //divide the current message in lines
    this->messageLines = split(this->msglist.front(), '\n');

    //reset the start time(s)
    this->animationStartTime = this->animationLastExecTime = std::clock();
    this->animationRestStartTime = 0;

    //reset the start property
    int propertyCurrentValue=(int)GfParmGetNum(this->menuXMLDescHdle, "dynamic controls/slide", "x", "null", 0);
    this->propertyChangeNeeded=(int)GfParmGetNum(this->menuXMLDescHdle, "dynamic controls/slide", "width", "null", 0);
    this->propertyFinalValue = propertyCurrentValue + this->propertyChangeNeeded;

    //padding between the text and the bg image
    this->textPadding = propertyCurrentValue - (int)GfParmGetNum(this->menuXMLDescHdle, "dynamic controls/slidebg", "x", "null", 0);

    //start
    this->animationDirection = 1;
    this->runAnimation();

}
void NotificationManager::runAnimation(){

    //read the initial state of the UI
    int propertyCurrentValue = (int)GfParmGetNum(this->menuXMLDescHdle, "dynamic controls/slide", "x", "null", 0);
    // change needed from current status of the animation to the end
    int remainingChangeNeeded = (this->propertyFinalValue - propertyCurrentValue);

    //log current time
    std::clock_t currentTime = std::clock();

    //CASE 1
    //we still need to apply some change to reach the final value
    if(remainingChangeNeeded != 0){

        //how much time is we are running the animation
        // float animationRunningTime = (currentTime - this->animationStartTime) / (float) CLOCKS_PER_SEC;

        //how much time is passed from the last run
        float animationTimeFromLastStep = (currentTime - this->animationLastExecTime) / (float) CLOCKS_PER_SEC;
        //time remaining for the animation
        // float animationTimeRemaining = this->totalAnimationDuration - animationRunningTime;

        //
        //int propertyStepChange = remainingChangeNeeded / animationTimeRemaining * animationTimeFromLastStep;
        //int propertyStepChange = remainingChangeNeeded / animationTimeRemaining;
        //if we have not arhieving 30fps slow down the animation
        if(animationTimeFromLastStep > 0.033333333){

            animationTimeFromLastStep = animationTimeFromLastStep;

        }
        int propertyStepChange = this->propertyChangeNeeded / this->totalAnimationDuration * this->animationDirection * animationTimeFromLastStep;

        // if the change is too little we round it up to 1 unit at least
        if((propertyStepChange * this->animationDirection) < 1 ){

            propertyStepChange = 1 * this->animationDirection;

        }

        //new value for the property
        int propertyNewValue = propertyCurrentValue + propertyStepChange;

        //it he new value with the change applied is greater that the final result we want we correct it to be equal to the final result
        if (propertyNewValue * this->animationDirection  > propertyFinalValue * this->animationDirection ){

            propertyNewValue = propertyFinalValue;

        }

        //apply the new values
        GfParmSetNum(this->menuXMLDescHdle, "dynamic controls/slide", "x", "null", propertyNewValue);
        GfParmSetNum(this->menuXMLDescHdle, "dynamic controls/slidebg", "x", "null", propertyNewValue - this->textPadding);

        //remember the time we ran the last(this) animation frame
        this->animationLastExecTime = currentTime;

        /*
        GfLogInfo("###############################\n");
        GfLogInfo("StartTime: %d \n",this->animationStartTime);
        GfLogInfo("CurrentTime: %d \n",currentTime);
        GfLogInfo("RunningTime: %f \n ",(currentTime - this->animationStartTime) / (float) CLOCKS_PER_SEC);
        GfLogInfo("RunningTime: %f \n ",(currentTime - this->animationStartTime));
        GfLogInfo("RunningTime: %f \n ",(float) CLOCKS_PER_SEC);

        GfLogInfo("\n ");
        GfLogInfo("AnimationDuration: %f \n ",this->totalAnimationDuration);
        GfLogInfo("TimeRemaining: %f \n ",animationTimeRemaining);
        GfLogInfo("\n ");
        GfLogInfo("FinalValue: %i \n ",this->propertyFinalValue);
        GfLogInfo("CurrentValue: %i \n ",propertyCurrentValue);
        GfLogInfo("Change Needed: %i \n ",remainingChangeNeeded);
        GfLogInfo("StepChange: %i \n ",propertyStepChange);
        GfLogInfo("\n ");
        GfLogInfo("Direction: %i \n ",this->animationDirection);
        */

        this->removeOldUi();
        this->createUi();

    }



    //CASE 2
    // no change needed while running the runOutAnimation
    if(remainingChangeNeeded == 0 && this->animationDirection == -1){

        //delette this message from the queque
        this->msglist.erase (this->msglist.begin());

        //we are no longer busy
        this->busy=false;

    }


    //CASE 3
    // no change needed while running the runInAnimation: we have ended the runInAnimation
    if(remainingChangeNeeded == 0 && this->animationDirection == 1){
        if(this->animationRestStartTime==0){

            //we are just done runnig the runInAnimation
            //log the time we start waiting while fully displayed
            this->animationRestStartTime = std::clock();

        }else{

            //if rest time has expired: start the runOutAnimation
            if(((currentTime - this->animationRestStartTime) / (float) CLOCKS_PER_SEC) > this->animationRestTime){

                //change the animation direction
                this->animationDirection = -1;

                //reset the animation start time
                this->animationStartTime = this->animationLastExecTime = std::clock(); //when the animation started

                //read property info
                this->propertyChangeNeeded= (int)GfParmGetNum(this->menuXMLDescHdle, "dynamic controls/slide", "width", "null", 0);
                this->propertyFinalValue = propertyCurrentValue - this->propertyChangeNeeded;

            }

        }

    }

}

void NotificationManager::removeOldUi(){

    //if there is a prev screen
    if( GfuiScreenIsActive(this->prevScreenHandle) )
    {
        //if there is some prev ui around hide it
        if(this->notifyUiIdBg > 0)
        {
            GfuiVisibilitySet(this->prevScreenHandle, this->notifyUiIdBg, GFUI_INVISIBLE);
        }

        //iterate trougth ui and set them invisible
        for (size_t i = 0; i < notifyUiId.size(); i++)
        {
            GfuiVisibilitySet(this->prevScreenHandle, this->notifyUiId[i], GFUI_INVISIBLE);
        }
    }

    //delete the prev ui's
    this->notifyUiId.clear();
    this->notifyUiIdBg=-1;
}

void NotificationManager::createUi()
{
    //create the new UI
    this->notifyUiIdBg = GfuiMenuCreateStaticImageControl(this->screenHandle, this->menuXMLDescHdle, "slidebg");
    GfuiVisibilitySet(this->screenHandle, this->notifyUiIdBg, GFUI_VISIBLE);

    //get first line vertical position
    int ypos=(int)GfParmGetNum(this->menuXMLDescHdle, "dynamic controls/slide", "y", "null", 0);
    // int yposmod= ypos;

    //iterate trougth lines
    for (size_t i = 0; i < this->messageLines.size(); i++)
    {
        int uiId;
        uiId= GfuiMenuCreateLabelControl(this->screenHandle, this->menuXMLDescHdle, "slide");

        //change the vertical position
        int yposmod = ypos - (i+1)*(10);
        GfParmSetNum(this->menuXMLDescHdle, "dynamic controls/slide", "y", "null", yposmod);

        GfuiLabelSetText(this->screenHandle, uiId, this->messageLines[i].c_str());
        GfuiVisibilitySet(this->screenHandle, uiId, GFUI_VISIBLE);
        this->notifyUiId.push_back(uiId);
     }

    //reset ypos
    GfParmSetNum(this->menuXMLDescHdle, "dynamic controls/slide", "y", "null", ypos);
}

void NotificationManager::updateStatusUi()
{
    //todo: this was causing some random crash
    //if there is some prev ui around hide it
    if(this->notifyUiIdBusyIcon > 0 && this->screenHandle == this->prevScreenHandle)
    {
        GfuiVisibilitySet(this->prevScreenHandle, this->notifyUiIdBusyIcon, GFUI_INVISIBLE);
        this->notifyUiIdBusyIcon = -1;
    }

    if(this->screenHandle != NULL)
    {
        //if not IDLE display busy icon (sending or receiving)
        if(status != NotificationManager::Idle)
        {
            std::string icon = "busyicon";
            icon.append(std::to_string(status));

            this->notifyUiIdBusyIcon = GfuiMenuCreateStaticImageControl(this->screenHandle, this->menuXMLDescHdle, icon.c_str());
            GfuiVisibilitySet(this->screenHandle, this->notifyUiIdBusyIcon, GFUI_VISIBLE);
        }
    }
}
