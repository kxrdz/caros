/***************************************************************************
                    webserver.h -- Interface file for The Gaming Framework
                             -------------------
    created              : 04/11/2015
    copyright            : (C) 2015 by MadBad
    email                : madbad82@gmail.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file
        The Gaming Framework API (client part).
    @author     <a href=mailto:madbad82@gmail.com>MadBad</a>
*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <playerpref.h>
#include <tgf.h>
#include "webserver.h"
#include <portability.h>

WebServer* pStatsServer = NULL;

WebServer& webServer()
{
    if(pStatsServer == NULL)
        pStatsServer = new WebServer;
    return *pStatsServer;
}

void gfuiInitWebStats()
{
	if(pStatsServer == NULL)
		pStatsServer = new WebServer;
}

void gfuiShutdownWebStats()
{
	if(pStatsServer != NULL)
		delete pStatsServer;
	pStatsServer = NULL;
}

//string splitting utils
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

static size_t WriteStringCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}
//unique id generator
int uniqueId=1;
int getUniqueId(){
    return uniqueId++;
}

//

WebServer::WebServer()
{
    //set some default
    this->raceId = -1;
    this->userId = -1;
    this->raceEndSent = false;
    this->pendingAsyncRequestId = 0;
    this->isWebServerEnabled = false;

    //initialize some curl var
    this->multi_handle = curl_multi_init();
    this->handle_count = 0;

    //todo:even if this was the appropriate place to do this, it is not working as intended
    //i guess some of the Gf function are not available to be call that early in the game initialization process
    //so we get a segfault.
    // moved this call to the start of the functions 'addAsyncRequest' and 'sendGenericRequest'

    //initialize the configuration
    //this->readConfiguration();
}

WebServer::~WebServer()
{
    //cleanup curl
    curl_multi_cleanup(this->multi_handle);
}

void WebServer::updateStatus()
{
    std::clock_t currentTime =  std::clock();

    //run the webserver update process and ui at 30 FPS
    if( ( currentTime - notifications.animationLastExecTime ) > 0.033333333 ){

        updateAsyncStatus();
        notifications.updateStatus();

    }
}

int WebServer::updateAsyncStatus()
{
    //if we have some ordered async request pending
    if( this->pendingAsyncRequestId == 0 && !this->orderedAsyncRequestQueque.empty() )
    {
        webRequest_t nextRequest;
        nextRequest = this->orderedAsyncRequestQueque.front();

        //replace the {{tags}} with actual data received from previous requests replies
        replaceAll(nextRequest.data, "{{race_id}}", std::to_string(this->raceId));
        replaceAll(nextRequest.data, "{{user_id}}", std::to_string(this->userId));

        GfLogInfo("WebServer: Adding AsyncRequest from orderedAsyncRequestQueque with id: %i\n", nextRequest.id);
        this->pendingAsyncRequestId = nextRequest.id;
        this->addAsyncRequest(nextRequest.data);
    }

    //perform the pending requests
    curl_multi_perform(this->multi_handle, &this->handle_count);

    if( this->handle_count > 0)
    {
        GfLogDebug("WebServer: Number of async request waiting for a reply from the server: %i\n", this->handle_count);
        //display some UI to the user to inform him we are waiting a reply from the server
        notifications.status = NotificationManager::Receiving;
    }
    else
    {
        notifications.status = NotificationManager::Idle;
    }

    CURLMsg *msg;
    CURL *eh=NULL;
    CURLcode return_code;
    int http_status_code;
    const char *szUrl;

    while ((msg = curl_multi_info_read(this->multi_handle, &this->handle_count)))
    {
        if (msg->msg == CURLMSG_DONE)
        {
            eh = msg->easy_handle;

            return_code = msg->data.result;

            if(return_code!=CURLE_OK)
            {
                fprintf(stderr, "CURL error code: %d\n", msg->data.result);

                //something went wrong. anyway we are no more busy
                notifications.status = NotificationManager::Idle;

                continue;
            }

            // Get HTTP status code
            http_status_code=0;
            szUrl = NULL;

            curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_status_code);

            curl_easy_getinfo(eh, CURLINFO_EFFECTIVE_URL, &szUrl);

            //the server replyed
            if(http_status_code==200)
            {
                GfLogInfo("WebServer: successfull reply from the server from url: %s\n", szUrl);

                GfLogInfo("WebServer: The reply from the server is:\n%s\n", this->curlServerReply.c_str());
                //manage server replyes...

                //read the xml reply of the server
                void *xmlReply;
                xmlReply = GfParmReadBuf(const_cast<char*>(this->curlServerReply.c_str()));

                if(GfParmExistsSection(xmlReply, "content"))
                {
                    int requestId = GfParmGetNum(xmlReply, "content", "request_id", "null",0);

                    if( requestId == this->pendingAsyncRequestId )
                    {
                        //reset the pending request id
                        this->pendingAsyncRequestId=0;
                        // remove this request from the queque... so we can continue with the next
                        GfLogInfo("WebServer: Removing successfull AsyncRequest from the orderedAsyncRequestQueque with id: %i\n", this->orderedAsyncRequestQueque.front().id);
                        this->orderedAsyncRequestQueque.erase (this->orderedAsyncRequestQueque.begin());
                    }
                }

                //the server want we display something?
                if(GfParmExistsSection(xmlReply, "content/reply/messages"))
                {
                    int msgsCount = GfParmGetNum(xmlReply, "content/reply/messages", "number", "null",0);

                    if( msgsCount > 0 )
                    {
                        for( int dispatchedMsgs = 0; dispatchedMsgs < msgsCount; dispatchedMsgs = dispatchedMsgs + 1 )
                           {
                            std::string msgTag = "message";
                            msgTag.append(std::to_string(dispatchedMsgs));
                            GfLogInfo("WebServer: Adding messagge to be displayed to the NotificationQueque:\n%s\n", msgTag.c_str());

                            notifications.msglist.push_back(GfParmGetStr(xmlReply, "content/reply/messages", msgTag.c_str(), "null"));
                           }
                    }
                }

                //race reply
                //store the webServer assigned race id
                if(GfParmExistsSection(xmlReply, "content/reply/races"))
                {
                    if(GfParmGetNum(xmlReply, "content/reply/races", "id", "null", 0) != 0)
                    {
                        this->raceId = (int)GfParmGetNum(xmlReply, "content/reply/races", "id", "null", 0);
                        GfLogInfo("WebServer: Assigned raceId by the server is: %i\n", this->raceId);
                    }
                }

                //login reply
                //store the webServer assigned race id
                if(GfParmExistsSection(xmlReply, "content/reply/login"))
                {
                    if(GfParmGetNum(xmlReply, "content/reply/login", "id", "null",0) != 0)
                    {
                        //store the webServer session and id assigned
                        this->sessionId = GfParmGetStr(xmlReply, "content/reply/login", "sessionid", "null");
                        this->userId = GfParmGetNum(xmlReply, "content/reply/login", "id", "null",0);
                        GfLogInfo("WebServer: Successfull Login as userId: %i\n", this->userId);
                    }
                    else
                    {
                        GfLogInfo("WebServer: Login Failed: Wrong username or password. Disabling WebServer features.\n");
                        //notifications.msglist.push_back("WebServer: Login Failed: Wrong username or password.");
                        //since we cant login: disable the webserver
                        this->isWebServerEnabled = false;

                        //since we cant login: remove all the already registered and pending requests
                        this->orderedAsyncRequestQueque.clear();
                        return 1;
                    }

                    GfLogInfo("WebServer: Assigned session id is: %s\n", this->sessionId);
                }

                //empty the string
                this->curlServerReply.clear();
            }
            else
            {
                fprintf(stderr, "GET of %s returned http status code %d\n", szUrl, http_status_code);
            }

            curl_multi_remove_handle(this->multi_handle, eh);
            curl_easy_cleanup(eh);
        }
        else
        {
            fprintf(stderr, "error: after curl_multi_info_read(), CURLMsg=%d\n", msg->msg);
        }
    }

    return 0;
}

int WebServer::addAsyncRequest(const std::string &data)
{
    GfLogInfo("WebServer: Performing ASYNC request:\n%s\n", data.c_str());

    //read the webserver configuration
    this->readConfiguration();

    CURL* curl = NULL;
    curl = curl_easy_init();

    if(curl)
    {
        //set url to be called
        curl_easy_setopt(curl, CURLOPT_URL, this->url);

        // send all data to this function
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);

        //pass our std::string to the WriteStringCallback functions so it can write into it
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &this->curlServerReply);

        // some servers don't like requests that are made without a user-agent
        // field, so we provide one
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        //prepare the form-post to be sent to the server
        curl_mime *mime;
        curl_mimepart *part;

        // Build an HTTP form with a single field named "data"
        mime = curl_mime_init(curl);
        part = curl_mime_addpart(mime);
        curl_mime_data(part, data.c_str(), CURL_ZERO_TERMINATED);
        curl_mime_name(part, "data");

        //inform curl to send the form-post
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

    }

    //add the request to the queque
    curl_multi_add_handle(this->multi_handle, curl);

    //pending request
    notifications.status = NotificationManager::Sending;

    return 0;
}

int WebServer::addOrderedAsyncRequest(const std::string &data)
{
    //prepare the request object
    webRequest_t request;

    request.id = getUniqueId();
    request.data = data;

    replaceAll(request.data, "{{request_id}}", std::to_string(request.id));

    this->orderedAsyncRequestQueque.push_back(request);

    return 0;
}

void WebServer::readConfiguration ()
{
    //read the preferencies file
    void *configHandle = GfParmReadFileLocal("config/webserver.xml", GFPARM_RMODE_REREAD);

    //get webServer url from the config
    this->url = GfParmGetStr(configHandle, "WebServer Settings", "url","val");

    GfLogInfo("WebServer - webserver url is: %s\n", this->url);
}

int WebServer::readUserConfig (int userId)
{
    void *prHandle;
    char xmlPath[1024];

    // find the xmlPath to our specific user in the preferencies xml file
    snprintf(xmlPath, sizeof(xmlPath), "%s/%i", HM_SECT_DRVPREF, userId);

    //read the preferencies file
    prHandle = GfParmReadFileLocal(HM_PREF_FILE, GFPARM_RMODE_REREAD);

    //get webServer user id for current user
    this->username = GfParmGetStr(prHandle, xmlPath, "WebServerUsername","val");

    //get webServer user password for current user
    this->password = GfParmGetStr(prHandle, xmlPath, "WebServerPassword","val");

    //get webServer enabled/disabled status
    this->isWebServerEnabled = (bool)GfParmGetNum(prHandle, xmlPath, "WebServerEnabled", (char*)NULL, (int)0);
    if (!this->isWebServerEnabled){
        GfLogInfo("WebServer - Webserver is disabled as per user setting\n");
    }



    return 0;
}

int WebServer::sendLogin (int userId)
{
    //read username and password and save it in as webserver properties
    this->readUserConfig(userId);

    //if the webserver is disabled exit immediately
    if(!this->isWebServerEnabled){
        return 1;
    }

    std::string serverReply;
    std::string username="username";
    std::string password="password";
    std::string emptyString;

    //if the user has not setup the webserver login info abort the login
    if((username==this->username && password==this->password) || this->username==emptyString || this->password==emptyString){
        GfLogInfo("WebServer: Send of login info aborted (the user is not correctly setup in this client).\n");
        GfLogInfo("WebServer: Disabling the webserver!.\n");
        this->isWebServerEnabled = false;

        return 1;
    }

    this->sendLogin(this->username, this->password);

    return 0;
}

int WebServer::sendLogin (const char* username, const char* password)
{
    //if the webserver is disabled exit immediately
    /*
    if(!this->isWebServerEnabled){
        return 1;
    }
    */
    // Commented out above to allow the 'Test login' button from Player config menu to work
    // for pre-race loginthe above check has already been done in WebServer::sendLogin (int userId)
    std::string serverReply;

    //prepare the string to send
    std::string dataToSend("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                           "<content>"
                           "<request_id>{{request_id}}</request_id>"
                           "<request>"
                           "<login>"
                           "<username>{{username}}</username>"
                           "<password>{{password}}</password>"
                           "</login>"
                           "</request>"
                           "</content>");

    //replace the {{tags}} with the respecting values
    replaceAll(dataToSend, "{{username}}", username);
    replaceAll(dataToSend, "{{password}}", password);

    this->addOrderedAsyncRequest(dataToSend);

    return 0;
}

int WebServer::sendLap (int race_id, bool valid, double laptime, double fuel,
    int position, int wettness)
{
    //if the webserver is disabled exit immediately
    if(!this->isWebServerEnabled){
        return 1;
    }
/*
    //Do some sanity-checks befor proceding... If something is wrong do nothing
    //are we logged in?
    if(this->sessionId=='\0'){
        GfLogInfo("Comunication of lap info aborted. No session ID assigned (we are not logged in)");
        return 1;
    }

    //Was the raceStart comunication successfull?
    if(this->raceId > -1){
        GfLogInfo("Comunication of lap info aborted. This race was not being tracked.");
        return 1;
    }
*/
    //prepare the string to send
    std::string dataToSend("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                           "<content>"
                           "<request_id>{{request_id}}</request_id>"
                           "<request>"
                           "<laps>"
                           "<race_id>{{race_id}}</race_id>"
                           "<laptime>{{laptime}}</laptime>"
                           "<fuel>{{fuel}}</fuel>"
                           "<position>{{position}}</position>"
                           "<wettness>{{wettness}}</wettness>"
                           "<valid>{{valid}}</valid>"
                           "</laps>"
                           "</request>"
                           "</content>");

    //replace the {{tags}} with the respecting values
    //The following tags will be replaced later because some other request must be done before: {{race_id}}
    replaceAll(dataToSend, "{{laptime}}", std::to_string(laptime));
    replaceAll(dataToSend, "{{fuel}}", std::to_string(fuel));
    replaceAll(dataToSend, "{{position}}", std::to_string(position));
    replaceAll(dataToSend, "{{wettness}}", std::to_string(wettness));
    replaceAll(dataToSend, "{{valid}}", std::to_string(valid));

    this->addOrderedAsyncRequest(dataToSend);

    return 0;
}

int WebServer::sendRaceStart (tSkillLevel user_skill, const char *track_id, char *car_id, int type, void *setup, int startposition, const char *sdversion)
{
    //if the webserver is disabled exit immediately
    if(!this->isWebServerEnabled){
        return 1;
    }

    std::string serverReply;
    std::string mysetup;
    std::string dataToSend;
    this->raceEndSent = false;

    //Do some sanity-checks befor proceding... If something is wrong do nothing
    //are we logged in?
/*
    if(this->sessionId=='\0'){
        GfLogInfo("Comunication of race start to the webserver aborted. No session ID assigned (we are not logged in)");
        return 1;
    }
*/

    //Sanity-checks passed we continue
    //read the setup
    GfParmWriteString(setup, mysetup);

    //prepare the string to send
    dataToSend.append(  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                        "<content>"
                        "<request_id>{{request_id}}</request_id>"
                        "<request>"
                        "<races>"
                        "<user_id>{{user_id}}</user_id>"
                        "<user_skill>{{user_skill}}</user_skill>"
                        "<track_id>{{track_id}}</track_id>"
                        "<car_id>{{car_id}}</car_id>"
                        "<type>{{type}}</type>"
                        "<setup><![CDATA[{{setup}}]]></setup>"
                        "<startposition>{{startposition}}</startposition>"
                        "<sdversion>{{sdversion}}</sdversion>"
                        "</races>"
                        "</request>"
                        "</content>");

    //replace the {{tags}} with the respecting values
    //The following tags will be replaced later because some other request must be done before: {{user_id}}
    replaceAll(dataToSend, "{{user_skill}}", std::to_string(user_skill));
    replaceAll(dataToSend, "{{track_id}}", std::string(track_id));
    replaceAll(dataToSend, "{{car_id}}", std::string(car_id));
    replaceAll(dataToSend, "{{type}}", std::to_string(type));
    replaceAll(dataToSend, "{{setup}}", mysetup);
    replaceAll(dataToSend, "{{startposition}}", std::to_string(startposition));
    replaceAll(dataToSend, "{{sdversion}}", std::string(sdversion));

    this->addOrderedAsyncRequest(dataToSend);

    return 0;
}

int WebServer::sendRaceEnd (int race_id, int endposition)
{
    //if the webserver is disabled exit immediately
    if(!this->isWebServerEnabled){
        return 1;
    } else if (this->raceEndSent) {
        return 0;
    }

    std::string serverReply;

    //Do some sanity-checks befor proceding... If something is wrong do nothing
    //are we logged in?
/*
    if(this->sessionId=='\0'){
        GfLogInfo("Comunication of race end to the webserver aborted. No session ID assigned (we are not logged in)");
        return 1;
    }

    //Was the raceStart comunication successfull?
    if(this->raceId > -1){
        GfLogInfo("Comunication of race end to the webserver aborted. This race was not being tracked.");
        return 1;
    }
*/
    //Sanity-checks passed we continue
    //prepare the string to send
    std::string dataToSend("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                           "<content>"
                           "<request_id>{{request_id}}</request_id>"
                           "<request>"
                           "<races>"
                           "<id>{{race_id}}</id>"
                           "<endposition>{{endposition}}</endposition>"
                           "</races>"
                           "</request>"
                           "</content>");

    //replace the {{tags}} with the respecting values
    //The following tags will be replaced later because some other request must be done before: {{race_id}}
    replaceAll(dataToSend, "{{endposition}}", std::to_string(endposition));

    this->addOrderedAsyncRequest(dataToSend);

    this->raceEndSent = true;
    //restore default race id
    //this->raceId = -1;

    return 0;
}
