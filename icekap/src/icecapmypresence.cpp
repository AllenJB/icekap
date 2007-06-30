/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "icecapmypresence.h"

#include <klocale.h>

#include "viewcontainer.h"
#include "statuspanel.h"

#include "icecapmisc.h"
#include "icecapserver.h"

namespace Icecap
{

    MyPresence::MyPresence (ViewContainer* viewContainer, IcecapServer* server, const QString& newName)
    {
        m_server = server;
        m_name = newName;
        m_connected = false;
        m_autoconnect = false;
        statusViewActive = false;
        setState (SSDisconnected);
        m_viewContainerPtr = viewContainer;

        // Connect to server event stream (command results)
        connect (m_server, SIGNAL (event(Icecap::Cmd)), this, SLOT (eventFilter(Icecap::Cmd)));

    }

    MyPresence::MyPresence (ViewContainer* viewContainer, IcecapServer* server, const QString& newName, Network* newNetwork)
    {
        m_server = server;
        m_name = newName;
        m_network = newNetwork;
        m_connected = false;
        m_autoconnect = false;
        statusViewActive = false;
        setState (SSDisconnected);
        m_viewContainerPtr = viewContainer;

        // Connect to server event stream (command results)
        connect (m_server, SIGNAL (event(Icecap::Cmd)), this, SLOT (eventFilter(Icecap::Cmd)));

    }

    MyPresence::MyPresence (ViewContainer* viewContainer, IcecapServer* server, const QString& newName, Network* newNetwork, const QMap<QString,QString>& parameterMap)
    {
        m_server = server;
        m_name = newName;
        m_network = newNetwork;
        m_connected = parameterMap.contains ("connected");
        m_autoconnect = parameterMap.contains ("autoconnect");
        statusViewActive = false;
        setState (SSDisconnected);

        // Connect to server event stream (command results)
        connect (m_server, SIGNAL (event(Icecap::Cmd)), this, SLOT (eventFilter(Icecap::Cmd)));


        m_viewContainerPtr = viewContainer;
        if (m_connected) {
            setState (SSConnected);
            init ();
        }
    }

    void MyPresence::update (const QMap<QString,QString>& parameterMap)
    {
        setConnected (parameterMap.contains ("connected"));
        setAutoconnect (parameterMap.contains ("autoconnect"));
    }

    void MyPresence::init ()
    {
        if (statusViewActive) return;

        statusViewActive = true;
        statusView = getViewContainer()->addStatusView(this);
        statusView->setServer (m_server);
    }


    void MyPresence::setNetwork (Network* newNetwork)
    {
        m_network = newNetwork;
    }

    void MyPresence::setConnected (bool newStatus)
    {
        m_connected = newStatus;
        if (m_connected) {
            setState (SSConnected);
            init ();
        }
    }

    void MyPresence::setAutoconnect (bool newStatus)
    {
        m_autoconnect = newStatus;
    }

    Channel* MyPresence::channel (const QString& channelName)
    {
        QPtrListIterator<Channel> it( channelList );
        Channel* current;
        while ( (current = it.current()) != 0 ) {
            ++it;
            if (current->name() == channelName) {
                return current;
            }
        }

        return 0;
    }

    void MyPresence::channelAdd (const Channel* ch)
    {
        if (channel (ch->name()) != 0) {
            return;
        }
        channelList.append (ch);
    }

    void MyPresence::channelAdd (const QString& channelName)
    {
        if (channel (channelName) != 0) {
            return;
        }
        channelAdd (new Channel (this, channelName));
    }

    void MyPresence::channelAdd (const QString& channelName, const QMap<QString, QString>& parameterMap)
    {
        if (channel (channelName) != 0) {
            return;
        }
        channelAdd (new Channel (this, channelName, parameterMap));
    }

    void MyPresence::channelRemove (const Channel* channel)
    {
        channelList.remove (channel);
    }

    void MyPresence::channelRemove (const QString& channelName)
    {
        channelList.remove (channel (channelName));
    }

    void MyPresence::channelClear ()
    {
        channelList.clear ();
    }

    bool MyPresence::operator== (MyPresence* compareTo)
    {
        return ( (m_name == compareTo->name()) && (m_network->name() == compareTo->network()->name()) );
    }

    bool MyPresence::isNull ()
    {
        return m_name.isNull();
    }

    void MyPresence::setState (State state)
    {
        m_state = state;
        if (state == SSConnecting) {
            init ();
        }
    }

    void MyPresence::appendStatusMessage(const QString& type, const QString& message)
    {
        if ( statusViewActive ) {
            statusView->appendServerMessage(type,message);
        } else {
            m_server->appendStatusMessage (type, message);
        }
    }


    void MyPresence::eventFilter (Icecap::Cmd ev)
    {
        if ((ev.mypresence != m_name) || (ev.network != m_network->name())) {
            return;
        }

        if ((ev.tag == "*") && (ev.command == "msg"))
        {
            QString escapedMsg = ev.parameterList["msg"];
            escapedMsg.replace ("\\.", ";");
            if (ev.parameterList["presence"].length () < 1) {
                if (ev.parameterList["irc_target"] == "AUTH") {
                    appendStatusMessage (ev.parameterList["irc_target"], escapedMsg);
                } else {
                    appendStatusMessage (i18n("Message"), escapedMsg);
                }
            } else if (ev.parameterList["irc_target"] == "$*") {
                appendStatusMessage (ev.parameterList["presence"], escapedMsg);
            }
        }
    }

}

#include "icecapmypresence.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
