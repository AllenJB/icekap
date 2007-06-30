/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2004 Peter Simonsson <psn@linux.se>
  Copyright (C) 2006 Eike Hein <sho@eikehein.com>
*/

#include <qdatastream.h>
#include <qstringlist.h>
#include <qdatetime.h>
#include <qregexp.h>
#include <qmap.h>

#include <klocale.h>
#include <kdeversion.h>
#include <kstringhandler.h>

#include <config.h>
#include <kdebug.h>

#include "icecapmisc.h"
#include "icecapinputfilter.h"
#include "icecapserver.h"
#include "replycodes.h"
#include "konversationapplication.h"
#include "commit.h"
#include "version.h"
#include "query.h"
#include "statuspanel.h"
#include "common.h"
#include "notificationhandler.h"
#include "texteventhandler.h"

#include <kresolver.h>

IcecapInputFilter::IcecapInputFilter()
{
    m_connecting = false;
    netlistInProgress = false;
    myplistInProgress = false;
}

IcecapInputFilter::~IcecapInputFilter()
{
}

void IcecapInputFilter::setServer(IcecapServer* newServer)
{
    server=newServer;
    textEventHnd = server->getTextEventHandler ();
}

void IcecapInputFilter::parseLine(const QString& a_newLine)
{
    QString trailing;
    QString newLine(a_newLine);

    // Remove white spaces at the end and beginning
    newLine = newLine.stripWhiteSpace();

    QStringList part;
    int posSep = newLine.find (';');
    if (posSep != -1) {
        part = QStringList::split(";", newLine);
    }

    // Is this an event?
    QString tag = part[0];
    if (tag == "*") {
        // This is an event
        // Format: *;<event-name-underscored>;<parameters>
        // Parameters can be static (cacheable, no prefix) or dynamic (not cacheable, prefix "$")
        // The time parameter is the time that the event was created on the server, NOT the current time

        QString eventName = part[1];
        QStringList param = part;
        // Remove the type and event name from the params list
        param.pop_front();
        param.pop_front();

        parseIcecapEvent (eventName, param);
    } else {
        // This is a command reply
        // Format: <tag>;<status>;<params>
        // tag is the tag sent by the client when the command was issued. Can be just about anything but "*"
        //  tag is basically the only way we have of identifying command replies
        // status:
        //  +  Success; Response possibly in subsequent fields
        //  -  Failue; 3rd field is error identifier
        //  >  Success so far; More to follow

        QString status = part[1];
        QStringList param = part;
        // Remove the type and event name from the params list
        param.pop_front();
        param.pop_front();
        parseIcecapCommand (tag, status, param);
    }
}

// TODO: Re-implement all this stuff using the new signal based system
void IcecapInputFilter::parseIcecapEvent (const QString &eventName, const QStringList &parameterList)
{
    // I assume this ensures there's a valid server to communicate with
    Q_ASSERT(server); if(!server) return;

    // Split into key, value pairs around the first occurence of =
    // TODO: Is there an easier way to do this?
    // TODO: Should this be moved up to parseLine?
    // TODO: Better handling for keys that appear multiple times
    QMap<QString, QString> parameterMap;
    QStringList::const_iterator end = parameterList.end();
    for ( QStringList::const_iterator it = parameterList.begin(); it != end; ++it ) {
        QStringList thisParam = QStringList::split("=", *it);
        QString key = thisParam.first ();
        thisParam.pop_front ();
        QString value = thisParam.join ("=");
        parameterMap.insert (key, value, TRUE);
    }

    bool processTextEvent = true;
    if (eventName == "preauth") {
        // preauth - currently this is just a welcome signal
        // eventually icecap will implement authentication so we'll need to add support for that

        // Send the welcome signal, so the server class knows we are connected properly
        emit welcome("");
        m_connecting = true;
    }
    else if (eventName == "network_init")
    {
        server->networkAdd (parameterMap["protocol"], parameterMap["network"]);
    }
    else if (eventName == "network_deinit")
    {
        server->networkRemove (parameterMap["network"]);
    }
    else if (eventName == "local_presence_init")
    {
        server->mypresenceAdd (parameterMap ["mypresence"], parameterMap["network"]);
    }
    else if (eventName == "local_presence_deinit")
    {
        server->mypresenceRemove (parameterMap ["mypresence"], parameterMap["network"]);
    }
    else if (eventName == "channel_init")
    {
        server->mypresence (parameterMap["mypresence"], parameterMap["network"])->channelAdd (parameterMap["channel"]);
    }
    else if (eventName == "channel_deinit")
    {
        server->mypresence (parameterMap["mypresence"], parameterMap["network"])->channelRemove (parameterMap["channel"]);
    }
    else if (eventName == "channel_connection_init")
    {
        server->mypresence(parameterMap["mypresence"], parameterMap["network"])->channel (parameterMap["channel"])->setConnected (true);
    }
    else if (eventName == "channel_connection_deinit")
    {
        server->mypresence(parameterMap["mypresence"], parameterMap["network"])->channel (parameterMap["channel"])->setConnected (false);
    }
    else if (eventName == "gateway_connecting")
    {
        Icecap::MyPresence* myp = server->mypresence(parameterMap["mypresence"], parameterMap["network"]);
        myp->setState (Icecap::SSConnecting);
        QString message = i18n ("Connecting to gateway: %1:%2").arg(parameterMap["ip"]).arg (parameterMap["port"]);
    }
    else if ((eventName == "gateway_disconnected") || (eventName == "gateway_motd") || (eventName == "gateway_motd_end"))
    {
        // Do nothing
    }
    else if ((eventName == "channel_presence_removed") || (eventName == "channel_presence_added"))
    {
        // Do nothing
    }
/*
    else if (eventName != "msg") {
        processTextEvent = false;
    }
*/
    else {
        // If there's no specific handler, we send it through the generic IcecapServer::event slot (via IcecapServer::emitEvent)
        // TODO: Move specific coding for as much as possible directly to the relevent classes and use this.
        Icecap::Cmd result;
        result.tag = "*";
        result.command = eventName;
        result.parameterList = parameterMap;
        result.network = parameterMap["network"];
        result.mypresence = parameterMap["mypresence"];
        result.channel = parameterMap["channel"];
        server->emitEvent (result);
        processTextEvent = false;
    }

    if (processTextEvent) {
        textEventHnd->processEvent(eventName, parameterMap);
    }
}

void IcecapInputFilter::parseIcecapCommand (const QString &tag, const QString &status, QStringList &parameterList)
{
    // I assume this ensures there's a valid server to communicate with
    Q_ASSERT(server); if(!server) return;

    // Split into key, value pairs around the first occurence of =
    // TODO: Is there an easier way to do this?
    // TODO: Should this be moved up to parseLine?
    // TODO: Better handling for keys that appear multiple times
    QMap<QString, QString> parameterMap;
    QStringList::const_iterator end = parameterList.end();
    for ( QStringList::const_iterator it = parameterList.begin(); it != end; ++it ) {
        QStringList thisParam = QStringList::split("=", *it);
        QString key = thisParam.first ();
        thisParam.pop_front ();
        QString value = key;
        if (thisParam.size () > 0) {
            value = thisParam.join ("=");
        }
        parameterMap.insert (key, value, TRUE);
    }

    // TODO: Move specific coding for as much as possible directly to the relevent classes and use this.
    Icecap::Cmd result;
    result.tag = tag;
    result.status = status;
    result.parameterList = parameterMap;
    server->emitEvent (result);
}


// # & + and ! are *often*, but not necessarily, Channel identifiers. + and ! are non-RFC,
// so if a server doesn't offer 005 and supports + and ! channels, I think thats broken behaviour
// on their part - not ours. --Argonel
bool IcecapInputFilter::isAChannel(const QString &check)
{
    Q_ASSERT(server);
    // if we ever see the assert, we need the ternary
//    return server? server->isAChannel(check) : QString("#&").contains(check.at(0));
    return true;
}

// TODO: What's this used for?
bool IcecapInputFilter::isIgnore(const QString &sender, Ignore::Type type)
{
    bool doIgnore = false;

    QPtrList<Ignore> list = Preferences::ignoreList();

    for(unsigned int index =0; index<list.count(); index++)
    {
        Ignore* item = list.at(index);
        QRegExp ignoreItem(QRegExp::escape(item->getName()).replace("\\*", "(.*)"),false);
        if (ignoreItem.exactMatch(sender) && (item->getFlags() & type))
            doIgnore = true;
        if (ignoreItem.exactMatch(sender) && (item->getFlags() & Ignore::Exception))
            return false;
    }

    return doIgnore;
}

void IcecapInputFilter::reset()
{
    automaticRequest.clear();
    whoRequestList.clear();
}

void IcecapInputFilter::setAutomaticRequest(const QString& command, bool yes)
{
    automaticRequest[command] += (yes) ? 1 : -1;
    if(automaticRequest[command]<0)
    {
        kdDebug()   << "IcecapInputFilter::automaticRequest( " << command << " ) was negative! Resetting!"
            << endl;
        automaticRequest[command]=0;
    }
}

int IcecapInputFilter::getAutomaticRequest(const QString& command)
{
    return automaticRequest[command];
}

#include "icecapinputfilter.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
