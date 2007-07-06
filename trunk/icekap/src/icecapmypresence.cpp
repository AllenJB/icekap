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
#include "icecappresence.h"

namespace Icecap
{
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

        QString presenceName;
        if (parameterMap.contains ("name")) {
            presenceName = parameterMap["name"];
        } else {
            presenceName = parameterMap["presence"];
        }

        if (m_network->presence (presenceName)) {
            m_presence = m_network->presence (presenceName);
        } else {
            Presence* presence = new Presence (presenceName);
            m_network->presenceAdd (presence);
            m_presence = presence;
        }

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
        if ((parameterMap.contains ("name")) || (parameterMap.contains ("presence")))
        {
            QString presenceName;
            if (parameterMap.contains ("name")) {
                presenceName = parameterMap["name"];
            } else {
                presenceName = parameterMap["presence"];
            }

            if (m_network->presence (presenceName)) {
                m_presence = m_network->presence (presenceName);
            } else {
                Presence* presence = new Presence (presenceName);
                m_network->presenceAdd (presence);
                m_presence = presence;
            }
        }
        emit nameChanged ();
    }

    void MyPresence::init ()
    {
        if (statusViewActive) return;

        statusViewActive = true;
        statusView = getViewContainer()->addStatusView(this);
        statusView->setServer (m_server);
    }

    void MyPresence::setConnected (bool newStatus)
    {
        m_connected = newStatus;
        if (m_connected) {
            setState (SSConnected);
            init ();
        }
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

    bool MyPresence::operator== (MyPresence* compareTo)
    {
        return ( (m_name == compareTo->name()) && (m_network->name() == compareTo->network()->name()) );
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

        if (ev.tag == "*")
        {
            if (ev.command == "msg")
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
            else if ( ev.command == "channel_init" ) {
                channelAdd (ev.channel);
                appendStatusMessage ( i18n("Channel"), i18n ("Channel %1 added.").arg (ev.channel));
            } else if ( ev.command == "channel_deinit" ) {
                channelRemove (ev.channel);
                appendStatusMessage ( i18n("Channel"), i18n ("Channel %1 deleted.").arg (ev.channel));
            } else if (ev.command == "gateway_connecting") {
                setState (Icecap::SSConnecting);
                QString message = i18n ("Connecting to gateway: %1:%2").arg(ev.parameterList["ip"]).arg (ev.parameterList["port"]);
                appendStatusMessage (i18n ("Gateway"), message);
            } else
            if (ev.command == "gateway_connected") {
                QString message = i18n ("Connected to gateway: %1:%2 - in_charsets: %3 - out_charset: %4").arg(ev.parameterList["ip"]).arg (ev.parameterList["port"]).arg (ev.parameterList["in_charsets"]).arg (ev.parameterList["out_charset"]);
                appendStatusMessage (i18n ("Gateway"), message);
            }
            else if (ev.command == "gateway_disconnected") {
                appendStatusMessage (i18n ("Gateway"), i18n ("Disconnected from gateway."));
            }
            else if (ev.command == "gateway_motd") {
                appendStatusMessage (i18n ("MOTD"), ev.parameterList["data"]);
            }
            else if (ev.command == "gateway_motd_end") {
                appendStatusMessage (i18n ("MOTD"), i18n ("End of MOTD."));
            }
            else if (ev.command == "gateway_logged_in") {
                appendStatusMessage (i18n ("Gateway"), i18n ("Logged in to gateway."));
            }
            else if (ev.command == "presence_changed")
            {
                if (ev.parameterList["presence"] == m_name)
                {
                    emit nameChanged ();
                }
            }

        } // end if (ev.tag == "*")
    }

}

#include "icecapmypresence.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
