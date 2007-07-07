/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2005 Peter Simonsson <psn@linux.se>
  Copyright (C) 2005 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2005 Eike Hein <sho@eikehein.com>
*/

#ifndef ICECAPOUTPUTFILTER_H
#define ICECAPOUTPUTFILTER_H

#include <qobject.h>
#include <qstring.h>
#include <kurl.h>
#include <kio/global.h>

#include "identity.h"

/*
  @author Dario Abatianni
*/

class IcecapServer;
class ChatWindow;

namespace Icecap
{
    class Channel;

    typedef enum MessageType
    {
        Message,
        Action,
        Command,
        Program,
        PrivateMessage
    };

    struct OutputFilterResult
    {
        /// What will be seen in the IRCView
        QString output;
        /// What will be seen in the IRCView (multiline)
        QStringList outputList;
        /// What will be sent to the server
        // TODO: Is this used by anything now?
        QString toServer;
        /// What will be sent to the server (multiline)
        // TODO: Is this used by anything now?
        QStringList toServerList;
        /// Prefix string. eg. "-->" or "***"
        QString typeString;
        /// Type of output
        MessageType type;
    };


    class OutputFilter : public QObject
    {
        Q_OBJECT

        public:
            explicit OutputFilter(IcecapServer* server);
            ~OutputFilter();

            QStringList splitForEncoding(const QString& inputLine, int MAX);
            OutputFilterResult parse (const QString& myNick, const QString& line, const QString& networkName = "", const QString& mypresenceName = "", const QString& channelName = "");
            bool replaceAliases(QString& line);

        signals:
            void openRawLog(bool show);
            void closeRawLog();
            void openKonsolePanel();
            void openChannelList(const QString& parameter, bool getList);
            void sendToAllChannels(const QString& text);
            void launchScript(const QString& target, const QString& parameter);
            void banUsers(const QStringList& userList,const QString& channel,const QString& option);
            void unbanUsers(const QString& mask,const QString& channel);
            void multiServerCommand(const QString& command, const QString& parameter);
            void reconnectServer();
            void disconnectServer();
            void connectToServerGroup(const QString& serverGroup);
            void connectToServer(const QString& server, const QString& port, const QString& password);

            void showView(ChatWindow* view);


        public slots:
            void setCommandChar();

        protected:
            OutputFilterResult parseNetwork (QStringList& parameter);
            OutputFilterResult parseMyPresence (QStringList& parameter);
            OutputFilterResult parseChannel (QStringList& parameter);
            OutputFilterResult parseGateway (QStringList& parameter);

            void parseServer(const QString& parameter);
            void parseReconnect();
            OutputFilterResult parseConnect(const QString& parameter);
            OutputFilterResult parseExec(const QString& parameter);
            OutputFilterResult parseRaw(const QString& parameter);
            void parseKonsole();
            OutputFilterResult parsePrefs(const QString& parameter);
            void parseCycle();
            OutputFilterResult parseDNS(const QString& parameter);

            OutputFilterResult usage(const QString& check);
            OutputFilterResult info(const QString& check);
            OutputFilterResult error(const QString& check);

            QString addNickToEmptyNickList(const QString& nick, const QString& parameter);

            bool isAChannel (const QString& name);

        private:
            QString destination;
            QString commandChar;

            IcecapServer* m_server;
    };
}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
