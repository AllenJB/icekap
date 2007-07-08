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
#include "statuspanel.h"
#include "common.h"
#include "notificationhandler.h"

#include <kresolver.h>

IcecapInputFilter::IcecapInputFilter()
{
}

void IcecapInputFilter::setServer(IcecapServer* newServer)
{
    server = newServer;
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

/**
 * Parse an Icecap Event. Sends it through the Icecap::Server event signal
 * @param eventName Event name
 * @param parameterList Event parameters
 * @todo AllenJB: Should this be moved up to parseLine?
 * @todo AllenJB: Handling for keys that appear multiple times
 * @todo AllenJB: Get rid of welcome signal (can now be called directly from Server's eventFilter
 */
void IcecapInputFilter::parseIcecapEvent (const QString &eventName, const QStringList &parameterList)
{
    // I assume this ensures there's a valid server to communicate with
    Q_ASSERT(server); if(!server) return;

    // Split into key, value pairs around the first occurence of =
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
        // Don't return or anything - we want this to go through the event system
    }

    // Send it through the generic IcecapServer::event slot (via IcecapServer::emitEvent)
    Icecap::Cmd result;
    result.tag = "*";
    result.command = eventName;
    result.parameterList = parameterMap;
    result.network = parameterMap["network"];
    result.mypresence = parameterMap["mypresence"];
    result.channel = parameterMap["channel"];
    server->emitEvent (result);
}

/**
 * Parse an icecap command response
 * @param tag Command tag
 * @param status Command status
 * @param parameterList Response parameters
 * @todo AllenJB: Should this be moved up to parseLine?
 * @todo AllenJB: Better handling for keys that appear multiple times
 */
void IcecapInputFilter::parseIcecapCommand (const QString &tag, const QString &status, QStringList &parameterList)
{
    // I assume this ensures there's a valid server to communicate with
    Q_ASSERT(server); if(!server) return;

    // Split into key, value pairs around the first occurence of =
    QString error;
    if (status == "-") {
        error = parameterList[0];
        parameterList.pop_front ();
    }

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

    Icecap::Cmd result;
    result.tag = tag;
    result.status = status;
    if (result.status == "-") {
        result.error = error;
    }
    result.parameterList = parameterMap;
    server->emitEvent (result);
}



/**
 * Is this sender on the ignore list?
 * @param sender Sender to check
 * @param type Ignore type to check for
 * @return Match found?
 */
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

/**
 * Reset who request list
 */
void IcecapInputFilter::reset()
{
    whoRequestList.clear();
}

#include "icecapinputfilter.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
