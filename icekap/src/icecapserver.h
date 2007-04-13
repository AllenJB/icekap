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
  Copyright (C) 2005 Eike Hein <sho@eikehein.com>
*/

#ifndef ICECAPSERVER_H
#define ICECAPSERVER_H

#include <qptrlist.h>
#include <qvaluelist.h>
#include <qtimer.h>
#include <qdict.h>
#include <qmap.h>

#include <qdeepcopy.h>

#include <ksharedptr.h>
#include <kprocess.h>
#include <ksocketbase.h>
#include <kbufferedsocket.h>
#include <kstreamsocket.h>

#include "icecapinputfilter.h"
#include "icecapoutputfilter.h"
#include "sslsocket.h"
#include "icecapserversettings.h"
#include "icecapnetwork.h"
#include "icecapmypresence.h"

class StatusPanel;
class Identity;
class RawLog;
class ChannelListPanel;
class QStrList;
class ChatWindow;
class ViewContainer;

using namespace KNetwork;

class IcecapServer : public QObject
{
    Q_OBJECT

    public:
        typedef enum
        {
            SSDisconnected,
            SSConnecting,
            SSConnected
        } State;
        /** Constructor used for a 'fast connect' to a server.
         *  The details are passed in.  Used for example when the user does "/server irc.somewhere.net"
         */
        IcecapServer(ViewContainer* viewContainer, const QString& hostName,const QString& port,
            const QString& password, const bool& useSSL=false);
        ~IcecapServer();

        QString getServerName() const { return "unimplemented"; };

        Icecap::IcecapServerSettings serverSettings() const { return m_server; }

        bool getUseSSL() const;
        QString getSSLInfo() const;

        int getPort() const;

        /** This returns true when we have a socket connection.
         *	Not necessarily 'online' and ready for commands.
         *  @see connected()
         */
        bool isConnected() const;
        bool isConnecting() const;

        IcecapInputFilter* getInputFilter();
        Icecap::IcecapOutputFilter* getOutputFilter();

        void appendStatusMessage(const QString& type,const QString& message);
        void appendMessageToFrontmost(const QString& type,const QString& message, bool parseURL = true);

        void dcopRaw(const QString& command);
        void dcopInfo(const QString& string);

        ChannelListPanel* getChannelListPanel() const;

        StatusPanel* getStatusView() const { return statusView; }

        /** This returns true when we are 'online' - ready to take commands, join channels and so on.
         */
        bool connected() const;

        ViewContainer* getViewContainer() const;

        // Blowfish stuff
        QCString getKeyForRecipient(const QString& recipient) const;
        void setKeyForRecipient(const QString& recipient, const QCString& key);

        ChannelListPanel* addChannelListPanel();

        void networkClear ();
        void networkAdd (const Icecap::Network& network);
        void networkAdd (const QString& protocol, const QString& name);
        void networkRemove (const Icecap::Network& network);
        void networkRemove (const QString& name);
        Icecap::Network network (const QString& name);
        QValueList<Icecap::Network> getNetworkList ();
        void networkListDisplay ();

        void mypresenceAdd (const Icecap::MyPresence& mypresence);
        void mypresenceAdd (const QString& name);
        void mypresenceAdd (const QString& name, const Icecap::Network& network);
        void mypresenceAdd (const QString& name, const QString& networkName);
        void mypresenceAdd (const QString& name, const QString& networkName, QMap<QString, QString>& parameterMap);
        void mypresenceRemove (const Icecap::MyPresence& mypresence);
        void mypresenceRemove (const QString& name, const QString& networkName);
        void mypresenceRemove (const QString& name, const Icecap::Network& network);
        // TODO: These may need to return a pointer / reference
        Icecap::MyPresence mypresence (const QString& name, const Icecap::Network& network);
        Icecap::MyPresence mypresence (const QString& name, const QString& networkName);
        void presenceListDisplay ();
        void mypresenceClear ();

    signals:
        /// will be connected to KonversationApplication::removeServer()
        void deleted(IcecapServer* myself);

        /// Emitted when the server gains/loses connection.
        /// will be connected to all server dependant tabs
        void serverOnline(bool state);

        void sslInitFailure();
        void sslConnected(IcecapServer* server);

        void connectionChangedState(IcecapServer* server, IcecapServer::State state);

        void showView(ChatWindow* view);

    public slots:
        void lookupFinished();
        void connectToServer();
        void queue(const QString &buffer);
        void queueList(const QStringList &buffer);
        void queueAt(uint pos,const QString& buffer);

        void quitServer();

        void addRawLog(bool show);
        void closeRawLog();

        void reconnect();
        void disconnect();
        void connectToNewServer(const QString& server, const QString& port, const QString& password);
        void showSSLDialog();

    protected slots:

        void connectionSuccess();
        void lockSending();
        void unlockSending();
        void incoming();
        void processIncomingData();
        void send();
        /**
         *Because KBufferedSocket has no closed(int) signal we use this slot to call broken(0)
         */
        void closed();
        void broken(int state);
        /** This is connected to the SSLSocket failed.
         * @param reason The reason why this failed.  This is already translated, ready to show the user.
         */
        void sslError(const QString& reason);
        void connectionEstablished(const QString& ownHost);

        void closeChannelListPanel();

    protected:
        // constants
        static const int BUFFER_LEN=513;

        /// Initialize the class
        void init(ViewContainer* viewContainer);

        /// Initialize the timers
        void initTimers();

        /// Connect to the signals used in this class.
        void connectSignals();

        void setViewContainer(ViewContainer* newViewContainer);

        unsigned int reconnectCounter;

        bool quickConnect;
        bool deliberateQuit;
        bool keepViewsOpenAfterQuit;
        bool reconnectAfterQuit;

        ViewContainer* m_viewContainerPtr;

        KNetwork::KStreamSocket* m_socket;
        bool         m_tryReconnect;

        QTimer reconnectTimer;
        QTimer incomingTimer;
        QTimer outgoingTimer;
        // timeout waiting for server to send initial messages
        QTimer unlockTimer;

        // flood protection
        int timerInterval;

        QCString inputBufferIncomplete;
        QStringList inputBuffer;
        QStringList outputBuffer;

        IcecapInputFilter inputFilter;
        Icecap::IcecapOutputFilter* outputFilter;

        StatusPanel* statusView;
        RawLog* rawLog;
        ChannelListPanel* channelListPanel;

        bool alreadyConnected;
        bool sendUnlocked;
        bool connecting;

    private:
        Icecap::IcecapServerSettings m_server;

        // Blowfish key map
        QMap<QString,QCString> keyMap;

        /// Used to lock incomingTimer while processing message.
        bool m_processingIncoming;

        QValueList<Icecap::Network> networkList;
        QValueList<Icecap::MyPresence> mypresenceList;

};
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1: