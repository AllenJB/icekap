/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2005-2006 Peter Simonsson <psn@linux.se>
  Copyright (C) 2005 Eike Hein <sho@eikehein.com>
*/

#include <qregexp.h>
#include <qhostaddress.h>
#include <qtextcodec.h>
#include <qdatetime.h>

#include <kapplication.h>
#include <klocale.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kresolver.h>
#include <ksocketdevice.h>
#include <kaction.h>
#include <kstringhandler.h>
#include <kdeversion.h>
#include <kwin.h>

#include "icecapserver.h"
#include "icecapserversettings.h"
#include "konversationapplication.h"
#include "irccharsets.h"
#include "viewcontainer.h"
#include "icecapstatuspanel.h"
#include "rawlog.h"
#include "channellistpanel.h"
#include "common.h"
#include "notificationhandler.h"
#include "blowfish.h"

#include <config.h>
// TODO: Remove unnecessary / unsupported crap from constructor
// TODO: Remove anything pertaining to nickname
// TODO: Remove anything pertaining to channels
// TODO: Implement support for icecap authorisation (when it supports authorisation itself)
// TODO: Implement support for SSL connections

// TODO: (Re-)Implement adding and removing networks, gateways, presences and channels using the new event system
// TODO: (Re-)Implement gateway lists using the new event system
// TODO: (Re-)Implement ability to obtain a list of all known networks, gateways, presences and channels

IcecapServer::IcecapServer(ViewContainer* viewContainer, const QString& name, const QString& hostName, const QString& port, const QString& password, const bool& useSSL)
{
    setName (QString ("server"+name).ascii());
    quickConnect = true;

    m_server.setName (name);
    m_server.setServer(hostName);
    m_server.setPort(port.toInt());
    m_server.setPassword(password);
    m_server.setSSLEnabled(useSSL);

    init (viewContainer);
}

IcecapServer::~IcecapServer()
{
    //send queued messages
    kdDebug() << "IcecapServer::~IcecapServer(" << name() << ")" << endl;
    // Send out the last messages (usually the /QUIT)
    send();

    // Make sure no signals get sent to a soon to be dying Server Window
    if (m_socket)
    {
        m_socket->blockSignals(true);
        m_socket->deleteLater();
    }

    closeRawLog();
    closeChannelListPanel();

    // notify KonversationApplication that this server is gone
    emit deleted(this);
}

void IcecapServer::init(ViewContainer* viewContainer)
{
    m_processingIncoming = false;
    m_tryReconnect = true;
    reconnectCounter = 0;
    rawLog = 0;
    channelListPanel = 0;
    alreadyConnected = false;
    connecting = false;
    m_socket = 0;
    keepViewsOpenAfterQuit = true;
    reconnectAfterQuit = false;

    timerInterval = 1;

    setViewContainer(viewContainer);
    statusView = getViewContainer()->addStatusView(this);

//    if(Preferences::rawLog())
//        addRawLog(false);
    addRawLog(true);

    inputFilter.setServer(this);
    outputFilter = new Icecap::OutputFilter(this);

    connectToServer();

    initTimers();

    connectSignals();

    emit serverOnline(false);
    emit connectionChangedState(this, SSDisconnected);
}

void IcecapServer::initTimers()
{
    incomingTimer.setName("incoming_timer");
    outgoingTimer.setName("outgoing_timer");
}

void IcecapServer::connectSignals()
{

    // Timers
    connect(&incomingTimer, SIGNAL(timeout()), this, SLOT(processIncomingData()));
    connect(&outgoingTimer, SIGNAL(timeout()), this, SLOT(send()));
    connect(&unlockTimer, SIGNAL(timeout()), this, SLOT(unlockSending()));

    // OutputFilter
    connect(outputFilter, SIGNAL(multiServerCommand(const QString&, const QString&)),
        this, SLOT(sendMultiServerCommand(const QString&, const QString&)));
    connect(outputFilter, SIGNAL(reconnectServer()), this, SLOT(reconnect()));
    connect(outputFilter, SIGNAL(disconnectServer()), this, SLOT(disconnect()));
    connect(outputFilter, SIGNAL(connectToServer(const QString&, const QString&, const QString&)),
        this, SLOT(connectToNewServer(const QString&, const QString&, const QString&)));

    connect(outputFilter, SIGNAL(openRawLog(bool)), this, SLOT(addRawLog(bool)));
    connect(outputFilter, SIGNAL(closeRawLog()), this, SLOT(closeRawLog()));

   // ViewContainer
    connect(this, SIGNAL(showView(ChatWindow*)), getViewContainer(), SLOT(showView(ChatWindow*)));
    connect(outputFilter, SIGNAL(openKonsolePanel()), getViewContainer(), SLOT(addKonsolePanel()));
    connect(outputFilter, SIGNAL(openChannelList(const QString&, bool)), getViewContainer(), SLOT(openChannelList(const QString&, bool)));

    // Inputfilter
    connect(&inputFilter, SIGNAL(welcome(const QString&)), this, SLOT(connectionEstablished(const QString&)));

    // Status View
    connect(this, SIGNAL(serverOnline(bool)), statusView, SLOT(serverOnline(bool)));

    // Event stream
    connect (this, SIGNAL (event(Icecap::Cmd)), this, SLOT (eventFilter(Icecap::Cmd)));

}

bool IcecapServer::isConnected() const
{
    if (!m_socket)
        return false;

    return (m_socket->state() == KNetwork::KClientSocketBase::Connected);
}

void IcecapServer::connectToServer()
{
    deliberateQuit = false;
    keepViewsOpenAfterQuit = false;
    reconnectAfterQuit = false;

    connecting = true;

    outputBuffer.clear();

    if(m_socket)
        m_socket->blockSignals(false);

    // prevent sending queue until server has sent something or the timeout is through
    lockSending();

    if (!isConnected())
    {
        // This is needed to support server groups with mixed SSL and nonSSL servers
        delete m_socket;

        // connect() will do a async lookup too
        if(!m_server.SSLEnabled())
        {
            m_socket = new KNetwork::KBufferedSocket(QString::null, QString::null, 0L, "serverSocket");
            connect(m_socket,SIGNAL (connected(const KResolverEntry&)),this,SLOT (connectionSuccess()));
        }
        else
        {
            m_socket = new SSLSocket(getViewContainer()->getWindow(), 0L, "serverSSLSocket");
            connect(m_socket,SIGNAL (sslInitDone()),this,SLOT (connectionSuccess()));
            connect(m_socket,SIGNAL (sslFailure(QString)),this,SIGNAL(sslInitFailure()));
            connect(m_socket,SIGNAL (sslFailure(QString)),this,SLOT(sslError(QString)));
        }

        connect(m_socket,SIGNAL (hostFound()),this,SLOT(lookupFinished()));
        connect(m_socket,SIGNAL (gotError(int)),this,SLOT (broken(int)) );
        connect(m_socket,SIGNAL (readyRead()),this,SLOT (incoming()) );
        connect(m_socket,SIGNAL (readyWrite()),this,SLOT (send()) );
        connect(m_socket,SIGNAL (closed()),this,SLOT(closed()));

        m_socket->connect(m_server.server(),
            QString::number(m_server.port()));

        // set up the connection details
        statusView->appendServerMessage(i18n("Info"),i18n("Looking for server %1:%2...")
            .arg(m_server.server())
            .arg(m_server.port()));
//        reset InputFilter (auto request info, /WHO request info)
        inputFilter.reset();
        emit connectionChangedState(this, SSConnecting);
    }
}

void IcecapServer::showSSLDialog()
{
    static_cast<SSLSocket*>(m_socket)->showInfoDialog();
}

void IcecapServer::lookupFinished()
{
    // error during lookup
    if(m_server.SSLEnabled() && m_socket->status())
    {
        // inform user about the error
        statusView->appendServerMessage(i18n("Error"),i18n("Server %1 not found.  %2")
            .arg(m_server.server())
            .arg(m_socket->errorString(m_socket->error())));

        m_socket->resetStatus();
        // prevent retrying to connect
        m_tryReconnect = false;
        // broken connection
        broken(m_socket->error());
    }
    else
    {
        statusView->appendServerMessage(i18n("Info"),i18n("Server found, connecting..."));
    }
}

void IcecapServer::connectionSuccess()
{
    reconnectCounter = 0;

    statusView->appendServerMessage(i18n("Info"),i18n("Connected"));

    m_socket->enableRead(true);

    // wait at most 2 seconds for server to send something before sending the queue ourselves
    unlockTimer.start(2000);

    connecting = false;
}

void IcecapServer::broken(int state)
{
    m_socket->enableRead(false);
    m_socket->enableWrite(false);
    m_socket->blockSignals(true);

    alreadyConnected = false;
    connecting = false;
    outputBuffer.clear();

    emit connectionChangedState(this, SSDisconnected);

    emit serverOnline(false);

    kdDebug() << "Connection broken (Socket fd " << m_socket->socketDevice()->socket() << ") " << state << "!" << endl;


    if (!deliberateQuit)
    {
        static_cast<KonversationApplication*>(kapp)->notificationHandler()->connectionFailure(statusView, m_server.server());

        ++reconnectCounter;

        if (Preferences::autoReconnect() && reconnectCounter <= Preferences::reconnectCount())
        {
            QString error = i18n("Connection to Server %1 lost: %2. Trying to reconnect.")
                .arg(m_server.server())
                .arg(KNetwork::KSocketBase::errorString((KNetwork::KSocketBase::SocketError)state));

            statusView->appendServerMessage(i18n("Error"), error);

            QTimer::singleShot(5000, this, SLOT(connectToServer()));
        }
        else if (!Preferences::autoReconnect() || reconnectCounter > Preferences::reconnectCount())
        {
            QString error = i18n("Connection to Server %1 failed: %2.")
                .arg(m_server.server())
                .arg(KNetwork::KSocketBase::errorString((KNetwork::KSocketBase::SocketError)state));

            statusView->appendServerMessage(i18n("Error"),error);
            reconnectCounter = 0;

            if (Preferences::autoReconnect())
            {
                error = i18n("Waiting for 2 minutes before another reconnection attempt...");
                statusView->appendServerMessage(i18n("Info"),error);
                QTimer::singleShot(2*60*1000, this, SLOT(connectToServer()));
            }
        }
        else
        {
            QString error = i18n("Connection to Server %1 failed: %2.")
                .arg(m_server.server());
            statusView->appendServerMessage(i18n("Error"),error);
        }
    }   // If we quit the connection with the server
    else
    {
        if (keepViewsOpenAfterQuit)
        {
            keepViewsOpenAfterQuit = false;
            statusView->appendServerMessage(i18n("Info"),i18n("Disconnected from server."));

            if (reconnectAfterQuit)
            {
                reconnectAfterQuit = false;
                reconnectCounter = 0;
                QTimer::singleShot(3000, this, SLOT(connectToServer()));
            }
        }
        else
        {
            getViewContainer()->serverQuit(this);
        }
    }
}

void IcecapServer::sslError(const QString& reason)
{
    QString error = i18n("Could not connect to %1:%2 using SSL encryption.Maybe the server does not support SSL, or perhaps you have the wrong port? %3")
        .arg(m_server.server())
        .arg(m_server.port())
        .arg(reason);
    statusView->appendServerMessage(i18n("SSL Connection Error"),error);
    m_tryReconnect = false;

}

// Will be called from InputFilter as soon as the Welcome message was received
void IcecapServer::connectionEstablished(const QString& ownHost)
{
    // Some servers don't include the userhost in RPL_WELCOME, so we
    // need to use RPL_USERHOST to get ahold of our IP later on
    if (!ownHost.isEmpty())
        KNetwork::KResolver::resolveAsync(this,SLOT(gotOwnResolvedHostByWelcome(KResolverResults)),ownHost,"0");

    emit serverOnline(true);
    emit connectionChangedState(this, SSConnected);

    if(!alreadyConnected)
    {
        alreadyConnected=true;
    }
    else
    {
        kdDebug() << "alreadyConnected == true! How did that happen?" << endl;
    }

    queueCommand ("netlist", "network list");
    queueCommand ("myplist", "presence list");
    queueCommand ("chlist",  "channel list");
}

QCString IcecapServer::getKeyForRecipient(const QString& recipient) const
{
    return keyMap[recipient];
}

void IcecapServer::setKeyForRecipient(const QString& recipient, const QCString& key)
{
    keyMap[recipient] = key;
}

// TODO: This should be handled by OutputFilter
void IcecapServer::quitServer()
{
    QString command(Preferences::commandChar()+"QUIT");
//    Icecap::OutputFilterResult result = outputFilter->parse(getNickname(),command, QString::null);
//    queue(result.toServer);
    if (m_socket) m_socket->enableRead(false);
}

void IcecapServer::processIncomingData()
{
    incomingTimer.stop();

    if(!inputBuffer.isEmpty() && !m_processingIncoming)
    {
        m_processingIncoming = true;
        QString front(inputBuffer.front());
        inputBuffer.pop_front();
        if(rawLog)
        {
            QString toRaw = front;
            rawLog->appendRaw("&gt;&gt; " + toRaw.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;"));
        }
        inputFilter.parseLine(front);
        m_processingIncoming = false;

        if(!inputBuffer.isEmpty())
        {
            incomingTimer.start(0);
        }
    }
}

void IcecapServer::incoming()
{
    if(m_server.SSLEnabled())
        emit sslConnected(this);

    // We read all available bytes here because readyRead() signal will be emitted when there is new data
    // else we will stall when displaying MOTD etc.
    int max_bytes = m_socket->bytesAvailable();

    QByteArray buffer(max_bytes+1);
    int len = 0;

    // Read at max "max_bytes" bytes into "buffer"
    len = m_socket->readBlock(buffer.data(),max_bytes);

    if( len <=0 && m_server.SSLEnabled() )
        return;

    if( len <= 0 ) // Zero means buffer is empty which shouldn't happen because readyRead signal is emitted
    {
        statusView->appendServerMessage(i18n("Error"),
            i18n("There was an error reading the data from the server: %1").
            arg(m_socket->errorString()));

        broken(m_socket->error());
        return;
    }

    buffer[len] = 0;

    QCString qcsBuffer = inputBufferIncomplete + QCString(buffer);

    // split buffer to lines
    QValueList<QCString> qcsBufferLines;
    int lastLFposition = -1;
    for( int nextLFposition ; ( nextLFposition = qcsBuffer.find('\n', lastLFposition+1) ) != -1 ; lastLFposition = nextLFposition )
        qcsBufferLines << qcsBuffer.mid(lastLFposition+1, nextLFposition-lastLFposition-1);

    // remember the incomplete line (split by packets)
    inputBufferIncomplete = qcsBuffer.right(qcsBuffer.length()-lastLFposition-1);

    while(!qcsBufferLines.isEmpty())
    {
        // Pre parsing is needed in case encryption/decryption is needed
        // BEGIN set channel encoding if specified
        QString senderNick;
        bool isServerMessage = false;
        QString channelKey;
//        QTextCodec* codec = getIdentity()->getCodec();
        QTextCodec* codec = QTextCodec::codecForName("utf8");
        QCString front = qcsBufferLines.front();

        QStringList lineSplit = QStringList::split(" ",codec->toUnicode(front));

        if( lineSplit.count() >= 1 )
        {
            if( lineSplit[0][0] == ':' )          // does this message have a prefix?
            {
                if( !lineSplit[0].contains('!') ) // is this a server(global) message?
                    isServerMessage = true;
                else
                    senderNick = lineSplit[0].mid(1, lineSplit[0].find('!')-1);

                lineSplit.pop_front();            // remove prefix
            }
        }

        // BEGIN pre-parse to know where the message belongs to
        QString command = lineSplit[0].lower();
        if( isServerMessage )
        {
            if( lineSplit.count() >= 3 )
            {
                if( command == "332" )            // RPL_TOPIC
                    channelKey = lineSplit[2];
                if( command == "372" )            // RPL_MOTD
                    channelKey = ":server";
            }
        }
        else                                      // NOT a global message
        {
            if( lineSplit.count() >= 2 )
            {
                // query
/*
                if( ( command == "privmsg" ||
                    command == "notice"  ) &&
                    lineSplit[1] == getNickname() )
                {
                    channelKey = senderNick;
                }
                // channel message
                else if( command == "privmsg" ||
                    command == "notice"  ||
                    command == "join"    ||
                    command == "kick"    ||
                    command == "part"    ||
                    command == "topic"   )
                {
                    channelKey = lineSplit[1];
                }
*/
            }
        }
        // END pre-parse to know where the message belongs to

        // Decrypt if necessary
        if(command == "privmsg")
            Konversation::decrypt(channelKey,front,this);
        else if(command == "332")
        {
            Konversation::decryptTopic(channelKey,front,this);
        }

        bool isUtf8 = Konversation::isUtf8(front);

        if( isUtf8 )
            inputBuffer << QString::fromUtf8(front);
        else
        {
            // check setting
            QString channelEncoding;
            channelEncoding = "utf8";
/*
            if( !channelKey.isEmpty() )
            {
                channelEncoding = Preferences::channelEncoding(getServerGroup(), channelKey);
            }
*/
            // END set channel encoding if specified

            if( !channelEncoding.isEmpty() )
                codec = Konversation::IRCCharsets::self()->codecForName(channelEncoding);

            // if channel encoding is utf-8 and the string is definitely not utf-8
            // then try latin-1
            if ( !isUtf8 && codec->mibEnum() == 106 )
                codec = QTextCodec::codecForMib( 4 /* iso-8859-1 */ );

            inputBuffer << codec->toUnicode(front);
        }
        qcsBufferLines.pop_front();
    }

    // refresh lock timer if it was still locked
    if( !sendUnlocked )
        unlockSending();

    if( !incomingTimer.isActive() && !m_processingIncoming )
        incomingTimer.start(0);
}

void IcecapServer::queue(const QString& buffer)
{
    // Only queue lines if we are connected
    if(!buffer.isEmpty())
    {
        outputBuffer.append(buffer);

        timerInterval*=2;

        if(!outgoingTimer.isActive())
        {
            outgoingTimer.start(1);
        }
    }
}

void IcecapServer::queueAt(uint pos,const QString& buffer)
{
    if(buffer.isEmpty())
        return;

    if(pos < outputBuffer.count())
    {
        outputBuffer.insert(outputBuffer.at(pos),buffer);

        timerInterval*=2;
    }
    else
    {
        queue(buffer);
    }

    if(!outgoingTimer.isActive())
    {
        outgoingTimer.start(1);
    }
}

void IcecapServer::queueList(const QStringList& buffer)
{
    // Only queue lines if we are connected
    if(!buffer.isEmpty())
    {
        for(unsigned int i=0;i<buffer.count();i++)
        {
            outputBuffer.append(*buffer.at(i));
            timerInterval*=2;
        }                                         // for

        if(!outgoingTimer.isActive())
        {
            outgoingTimer.start(1);
        }
    }
}

void IcecapServer::send()
{
    // Check if we are still online
    if(!isConnected() || outputBuffer.isEmpty())
    {
        return;
    }

    if(!outputBuffer.isEmpty() && sendUnlocked)
    {
        // NOTE: It's important to add the linefeed here, so the encoding process does not trash it
        //       for some servers.
        QString outputLine=outputBuffer[0]+'\n';
        QStringList outputLineSplit=QStringList::split(" ",outputBuffer[0]);
        outputBuffer.pop_front();

        //Lets cache the uppercase command so we don't miss or reiterate too much
        QString outboundCommand(outputLineSplit[0].upper());

        // wrap server socket into a stream
        QTextStream serverStream;

        serverStream.setDevice(m_socket);

        // set channel encoding if specified
        QString channelCodecName;

        // init stream props
        serverStream.setEncoding(QTextStream::Locale);
//        QTextCodec* codec = getIdentity()->getCodec();
        QTextCodec* codec = QTextCodec::codecForName("utf8");

        if(!channelCodecName.isEmpty())
        {
            codec = Konversation::IRCCharsets::self()->codecForName(channelCodecName);
        }

        // convert encoded data to IRC ascii only when we don't have the same codec locally
        if(QString(QTextCodec::codecForLocale()->name()).lower() != QString(codec->name()).lower())
        {
            serverStream.setCodec(codec);
        }

        serverStream << outputLine;
        if(rawLog) rawLog->appendRaw("&lt;&lt; " + outputLine.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;"));

        // detach server stream
        serverStream.unsetDevice();
    }

    if(outputBuffer.isEmpty()) {
        outgoingTimer.stop();
        timerInterval = 1;
    }

    // Flood-Protection
    if(timerInterval > 1)
    {
        int time;
        timerInterval /= 2;

        if(timerInterval > 40)
        {
            time = 4000;
        }
        else
        {
            time = 1;
        }

        outgoingTimer.changeInterval(time);
    }
}

void IcecapServer::closed()
{
    broken(m_socket->error());
}

void IcecapServer::dcopRaw(const QString& command)
{
    if(command.startsWith(Preferences::commandChar()))
    {
        queue(command.section(Preferences::commandChar(), 1));
    }
    else
        queue(command);
}

void IcecapServer::dcopInfo(const QString& string)
{
    appendMessageToFrontmost(i18n("DCOP"),string);
}

void IcecapServer::appendStatusMessage(const QString& type,const QString& message)
{
    statusView->appendServerMessage(type,message);
}

void IcecapServer::appendMessageToFrontmost(const QString& type,const QString& message, bool parseURL)
{
    getViewContainer()->appendToFrontmost(type,message, statusView, parseURL);
}

void IcecapServer::addRawLog(bool show)
{
    if (!rawLog)
        rawLog=getViewContainer()->addRawLog(this);

    connect(this,SIGNAL (serverOnline(bool)),rawLog,SLOT (serverOnline(bool)) );

    // bring raw log to front since the main window does not do this for us
    if (show) emit showView(rawLog);
}

void IcecapServer::closeRawLog()
{
    if(rawLog)
    {
        delete rawLog;
        rawLog = 0;
    }
}

ChannelListPanel* IcecapServer::addChannelListPanel()
{
    if(!channelListPanel)
    {
        channelListPanel = getViewContainer()->addChannelListPanel(this);
        connect(this, SIGNAL(serverOnline(bool)), channelListPanel, SLOT(serverOnline(bool)));
    }

    return channelListPanel;
}

void IcecapServer::closeChannelListPanel()
{
    if(channelListPanel)
    {
        delete channelListPanel;
        channelListPanel = 0;
    }
}

QString IcecapServer::getSSLInfo() const
{
    return static_cast<SSLSocket*>(m_socket)->details();
}

void IcecapServer::reconnect()
{
    if (isConnected() && !connecting)
    {
        keepViewsOpenAfterQuit = true;
        reconnectAfterQuit = true;
        quitServer();
        send();
    }
    else if (!isConnected())
    {
        reconnectCounter = 0;
        connectToServer();
    }
}

void IcecapServer::disconnect()
{
    if (isConnected())
    {
        keepViewsOpenAfterQuit = true;
        quitServer();
        send();
    }
}

QStringList IcecapServer::networkListDisplay ()
{
    QStringList result;
    result.append ("Network List:");
    QPtrListIterator<Icecap::Network> it( networkList );
    Icecap::Network* current;
    while ( (current = it.current()) != 0 ) {
        ++it;
        result.append (QString ("Network: %1 - %2").arg (current->protocol()).arg (current->name()));
    }
    result.append ("End of Network List");
    return result;
}

Icecap::Network* IcecapServer::network (const QString& name)
{
    QPtrListIterator<Icecap::Network> it( networkList );
    Icecap::Network* current = 0;
    while ( (current = it.current()) != 0 ) {
        ++it;
        if (current->name() == name) {
            return current;
        }
    }

    return current;
}

void IcecapServer::networkAdd (Icecap::Network* network)
{
    networkList.append (network);
}

void IcecapServer::networkAdd (const QString& protocol, const QString& name)
{
    if (network (name) != 0) {
        return;
    }
    networkList.append (new Icecap::Network (this, protocol, name));
}

void IcecapServer::networkRemove (Icecap::Network* network)
{
    networkList.remove (network);
}

void IcecapServer::networkRemove (const QString& name)
{
    networkList.remove (network (name));
}

Icecap::MyPresence* IcecapServer::mypresence (const QString& name, Icecap::Network* network)
{
    QPtrListIterator<Icecap::MyPresence> it( mypresenceList );
    Icecap::MyPresence* current;
    while ( (current = it.current()) != 0 ) {
        ++it;
        if ((current->name() == name) && (current->network() == network)) {
            return current;
        }
    }

    return 0;
}

Icecap::MyPresence* IcecapServer::mypresence (const QString& name, const QString& networkName)
{
    return mypresence (name, network (networkName));
}

void IcecapServer::mypresenceAdd (Icecap::MyPresence* myp)
{
    if (mypresence (myp->name(), myp->network()) != 0) {
        return;
    }
    mypresenceList.append (myp);
}

void IcecapServer::mypresenceAdd (const QString& name, const QString& networkName)
{
    if (mypresence (name, network(networkName)) != 0) {
        return;
    }
    Icecap::MyPresence* myp = new Icecap::MyPresence (m_viewContainerPtr, this, name, network (networkName));
    mypresenceAdd (myp);
}


void IcecapServer::mypresenceAdd (const QString& name, const QString& networkName, QMap<QString, QString>& parameterMap)
{
    if (mypresence (name, network(networkName)) != 0) {
        return;
    }
    Icecap::MyPresence* myp = new Icecap::MyPresence (m_viewContainerPtr, this, name, network (networkName), parameterMap);
    mypresenceAdd (myp);
}


void IcecapServer::mypresenceRemove (Icecap::MyPresence* mypresence)
{
    mypresenceList.remove (mypresence);
}

void IcecapServer::mypresenceRemove (const QString& name, Icecap::Network* network)
{
    mypresenceList.remove (mypresence (name, network));
}

void IcecapServer::mypresenceRemove (const QString& name, const QString& networkName)
{
    mypresenceRemove (name, network (networkName));
}

QStringList IcecapServer::presenceListDisplay ()
{
    QStringList result;

    result.append ("Presence List:");
    QPtrListIterator<Icecap::MyPresence> it( mypresenceList );
    Icecap::MyPresence* current;
    while ( (current = it.current()) != 0 ) {
        ++it;
        result.append (QString ("Presence: %1 - %2").arg (current->name()).arg (current->network()->name()));
    }
    result.append ("End of Presence List");
    return result;
}

// TODO: Replace parameters with parameterList
void IcecapServer::queueCommand (Icecap::Cmd command)
{
    QString parameters;
    if (command.parameters.length() > 0) {
        parameters = command.parameters;
    } else {
        QMap<QString,QString>::const_iterator end = command.parameterList.end();
        for ( QMap<QString,QString>::const_iterator it = command.parameterList.begin(); it != end; ++it ) {
            parameters += it.key() +"="+ it.data() +";";
        }
    }

    uint next = nextCommandId;
    nextCommandId++;
    commandsPending.insert (next, command);
    queue (QString ("%1."+command.tag +";"+ command.command +";"+ parameters).arg (next));
}

// TODO: Replace parameters with parameterList
void IcecapServer::queueCommand (QString command)
{
    Icecap::Cmd cmd;
    QStringList cmdPart = QStringList::split(";", command);
    cmd.tag = cmdPart[0];
    cmd.command = cmdPart[1];
    cmdPart.pop_front ();
    cmdPart.pop_front ();
    cmd.parameters = cmdPart.join (";");

    queueCommand (cmd);
}

void IcecapServer::queueCommand (QString tag, QString command) {
    Icecap::Cmd cmd;
    cmd.tag = tag;
    cmd.command = command;
    queueCommand (cmd);
}

void IcecapServer::queueCommand (QString tag, QString command, QMap<QString, QString> parameterMap) {
    Icecap::Cmd cmd;
    cmd.tag = tag;
    cmd.command = command;
    cmd.parameterList = parameterMap;
    queueCommand (cmd);
}

// TODO: Handling for error events
void IcecapServer::emitEvent (Icecap::Cmd result)
{
    // Events don't have a send command, so skip all the processing
    if (result.tag == "*") {
//        appendStatusMessage ("Debug", QString ("event emitted :: %1 :: %2 :: m: %3 :: n: %4 :: c: %5 :: p: %6").arg (result.tag).arg (result.command).arg (result.mypresence).arg (result.network).arg (result.channel).arg (paramsToText (result.parameterList)));
        emit event (result);
        return;
    }

    // If the command wasn't on the event system, exit now
    if (! result.tag.contains (".")) return;

    // Grab the original command and attach any parameters
    QStringList tagPart = QStringList::split (".", result.tag);
    uint id = tagPart[0].toUInt();
    if (! commandsPending.contains (id)) {
        return;
    }

    // Skip the last+1 response for lists
    // TODO: Improve this, since there may be some commands which are expected to return no parameters
    // We always get id and time parameters, so discount 2 on parameterList (parameters should never be used anyway)
/*
    if ((result.parameters.length() < 1) && (result.parameterList.size() < 3)) {
        if (result.status == "+") {
            commandsPending.remove (id);
        }
        return;
    }
*/

    Icecap::Cmd sendCmd = commandsPending.find (id).data();
    result.sentCommand = sendCmd.command;
    // If the returned message has not command, set it to be the sent command
    if (result.command.length () < 1) {
        result.command = sendCmd.command;
    }
    result.sentParameterList = sendCmd.parameterList;
    // TODO: Get rid of sentParameters
    result.sentParameters = sendCmd.parameters;

    // Set the network, mypresence and channel parameters, if set in the sent command
    if (result.sentParameterList.contains ("network")) {
        result.network = result.sentParameterList.find ("network").data();
    }

    if (result.sentParameterList.contains ("mypresence")) {
        result.mypresence = result.sentParameterList.find ("mypresence").data();
    }

    if (result.sentParameterList.contains ("channel")) {
        result.channel = result.sentParameterList.find ("channel").data();
    }

    // Remove commands once a terminating response has been received
    if ((result.status == "+") || (result.status == "-")) {
        commandsPending.remove (id);
    }

    result.tag = tagPart[1];
    emit event (result);
//    appendStatusMessage ("Debug", QString ("event emitted :: tag: %1 :: cmd: %2 :: m: %3 :: n: %4 :: c: %5 :: p: %6").arg (result.tag).arg (result.command).arg (result.mypresence).arg (result.network).arg (result.channel).arg (paramsToText (result.parameterList)));
//    appendStatusMessage ("Debug", QString ("event(2) :: tag: %1 :: sC: %2 :: sP: %3 :: sPL: %4 :: er: %5").arg (result.tag).arg (result.sentCommand).arg (result.sentParameters).arg (paramsToText (result.sentParameterList)).arg (result.error));

}

// TODO: Implement duplicate checking
void IcecapServer::eventFilter (Icecap::Cmd event) {
    if (event.sentCommand == "network list") {
        if (event.status == ">") {
            networkAdd (event.parameterList["protocol"], event.parameterList["network"]);
        }
    }

    // MyPresence lists are handled here instead of Network because you can't list only presences on a given Network
    else if (event.sentCommand == "presence list") {
        if (event.status == ">") {
            if (mypresence (event.parameterList["mypresence"], event.parameterList["network"]) != 0) {
                mypresence (event.parameterList["mypresence"], event.parameterList["network"])->update (event.parameterList);
            } else {
                mypresenceAdd (event.parameterList["mypresence"], event.parameterList["network"], event.parameterList);
            }
        }
    }

    // Channel lists are handled here instead of MyPresence because you can't list only channels on a given MyPresence
    else if (event.sentCommand == "channel list") {
        if (event.status == ">") {
            if (mypresence (event.parameterList["mypresence"], event.parameterList["network"]) == 0) {
                mypresenceAdd (event.parameterList["mypresence"], event.parameterList["network"]);
                if (event.parameterList.contains ("joined")) {
                    mypresence (event.parameterList["mypresence"], event.parameterList["network"])->setConnected (true);
                }
            }

            mypresence (event.parameterList["mypresence"], event.parameterList["network"])->channelAdd (event.parameterList["channel"], event.parameterList);
        }
    }

    else if ((event.tag == "*") && (event.command == "preauth")) {
        appendStatusMessage (i18n("Welcome"), "Successfully connected to Icecap server.");
    }
    else if (event.command == "network remove") {
        if (event.status == "-") {
            if (event.error == "notfound") {
                appendStatusMessage (i18n ("Network"), i18n ("Remove Network: %1: Specified network not found.").arg (event.network));
            } else {
                appendStatusMessage (i18n ("Network"), i18n ("Remove Network: %1: An unknown error occurred.").arg (event.network));
            }
        } else if (event.status == "+") {
            appendStatusMessage (i18n ("Network"), i18n ("Network %1 removed").arg (event.network));
        }
    }
    else if (event.tag == "*")
    {
        if (event.command == "network_init") {
            networkAdd (event.parameterList["protocol"], event.network);
            appendStatusMessage (i18n("Network"), i18n ("Network added: [%1] %2").arg (event.parameterList["protocol"]).arg (event.network));
        } else if (event.command == "network_deinit") {
            networkRemove (event.network);
            appendStatusMessage (i18n("Network"), i18n ("Network deleted: %2").arg (event.network));
        } else if (event.command == "local_presence_init") {
            mypresenceAdd (event.mypresence, event.network);
            appendStatusMessage (i18n("Presence"), i18n ("Presence added: [%1] %2").arg (event.mypresence).arg (event.network));
        } else if (event.command == "local_presence_deinit") {
            mypresenceRemove (event.mypresence, event.network);
            appendStatusMessage (i18n("Presence"), i18n ("Presence deleted: [%1] %2").arg (event.mypresence).arg (event.network));

        }

    }

}

QString IcecapServer::paramsToText (QMap<QString, QString> parameterList)
{
    QString parameters;
    QMap<QString,QString>::const_iterator end = parameterList.end();
    for ( QMap<QString,QString>::const_iterator it = parameterList.begin(); it != end; ++it ) {
        parameters += ";"+ it.key() +"="+ it.data();
    }
    return parameters;
}

void IcecapServer::emitMessage (Icecap::ClientMsg msg)
{
    emit message (msg);
}

#include "icecapserver.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
