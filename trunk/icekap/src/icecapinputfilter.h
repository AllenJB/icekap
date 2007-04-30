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

#ifndef ICECAPINPUTFILTER_H
#define ICECAPINPUTFILTER_H

#include <qobject.h>
#include <qstringlist.h>

#include "ignore.h"
#include "servergroupsettings.h"
// #include "texteventhandler.h"

/*
  @author Dario Abatianni
*/

class TextEventHandler;
class IcecapServer;
class QWidget;
class Query;

class IcecapInputFilter : public QObject
{
    Q_OBJECT

    public:
        IcecapInputFilter();
        ~IcecapInputFilter();

        void setServer(IcecapServer* newServer);
        void parseLine(const QString &line);

        // reset AutomaticRequest, WhoRequestList
        void reset();

        // use this when the client does automatics, like userhost for finding hostmasks
        void setAutomaticRequest(const QString& command, bool yes);
        int getAutomaticRequest(const QString& command);

        void setLagMeasuring(bool yes);
        bool getLagMeasuring();

    signals:
        void welcome(const QString& ownHost);

    protected:
        void parseModes(const QString &sourceNick, const QStringList &parameterList);
        void parseIcecapEvent (const QString &eventName, const QStringList &parameterList);
        void parseIcecapCommand (const QString &tag, const QString &status, QStringList &parameterList);

        void parseNetworkAdd  (const QString& status, QMap<QString, QString>& parameterMap);
        void parseNetworkList (const QString& status, QMap<QString, QString>& parameterMap);
        void parseNetworkDel  (const QString& status, QMap<QString, QString>& parameterMap);

        void parsePresenceAdd  (const QString& status, QMap<QString, QString>& parameterMap);
        void parsePresenceList (const QString& status, QMap<QString, QString>& parameterMap);
        void parsePresenceDel  (const QString& status, QMap<QString, QString>& parameterMap);

        void parseChannelAdd   (const QString& status, QMap<QString, QString>& parameterMap);
        void parseChannelList  (const QString& status, QMap<QString, QString>& parameterMap);
        void parseChannelDel   (const QString& status, QMap<QString, QString>& parameterMap);

        bool isAChannel(const QString &check);
        bool isIgnore(const QString &pattern, Ignore::Type type);

        IcecapServer* server;
            // automaticRequest[command]=count
        QMap< QString, int > automaticRequest;
        QStringList whoRequestList;
        int lagMeasuring;

        Query* query;

        QStringList newNickList;
        int m_debugCount;

        /// Used when handling MOTD
        bool m_connecting;

        bool netlistInProgress;
        bool prslistInProgress;
        bool chlistInProgress;

        TextEventHandler* textEventHnd;
};
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1: