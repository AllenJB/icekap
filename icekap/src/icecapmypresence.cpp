/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "icecapmypresence.h"
#include "viewcontainer.h"
#include "statuspanel.h"
// #include "icecapoutputfilter.h"
#include "icecapserver.h"

namespace Icecap
{

    MyPresence::MyPresence (ViewContainer* viewContainer, IcecapServer* server, const QString& newName)
    {
//        statusView = 0;
        m_server = server;
        m_name = newName;
        m_connected = false;
        m_autoconnect = false;
        setState (SSDisconnected);
        m_viewContainerPtr = viewContainer;
    }

    MyPresence::MyPresence (ViewContainer* viewContainer, IcecapServer* server, const QString& newName, Network* newNetwork)
    {
//        statusView = 0;
        m_server = server;
        m_name = newName;
        m_network = newNetwork;
        m_connected = false;
        m_autoconnect = false;
        setState (SSDisconnected);
        m_viewContainerPtr = viewContainer;
    }

    MyPresence::MyPresence (ViewContainer* viewContainer, IcecapServer* server, const QString& newName, Network* newNetwork, const QMap<QString,QString>& parameterMap)
    {
//        statusView = 0;
        m_server = server;
        m_name = newName;
        m_network = newNetwork;
        m_connected = parameterMap.contains ("connected");
        m_autoconnect = parameterMap.contains ("autoconnect");
        setState (SSDisconnected);
        if (parameterMap.contains ("presence"))
        {
            m_presence = parameterMap["presence"];
        }
        m_viewContainerPtr = viewContainer;
        if (m_connected) {
            setState (SSConnected);
            init ();
        }
    }

    void MyPresence::init ()
    {
/*
        if (!m_connected) {
            return;
        }
*/
        statusView = getViewContainer()->addStatusView(this);
        statusView->setMyPresence (this);
        statusView->setServer (m_server);
    }

    void MyPresence::setName (const QString& newName)
    {
        m_name = newName;
    }

    void MyPresence::setNetwork (Network* newNetwork)
    {
        m_network = newNetwork;
    }

    void MyPresence::setConnected (bool newStatus)
    {
        m_connected = newStatus;
    }

    void MyPresence::setAutoconnect (bool newStatus)
    {
        m_autoconnect = newStatus;
    }

    void MyPresence::setPresence (QString& presenceName)
    {
        m_presence = presenceName;
    }

    Channel* MyPresence::channel (const QString& channelName)
    {
        QPtrListIterator<Channel> it( channelList );
        Channel* current;
        while ( (current = it.current()) != 0 ) {
            ++it;
            if (current->getName() == channelName) {
                return current;
            }
        }

        return 0;
    }

    void MyPresence::channelAdd (const Channel* channel)
    {
        channelList.append (channel);
    }

    void MyPresence::channelAdd (const QString& channelName)
    {
        channelList.append (new Channel (channelName));
    }

    void MyPresence::channelAdd (const QString& channelName, const QMap<QString, QString>& parameterMap)
    {
        channelList.append (new Channel (channelName, parameterMap));
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
        if ((statusView == 0) && (state == SSConnecting)) {
            init ();
        }
    }

    void MyPresence::appendStatusMessage(const QString& type, const QString& message)
    {
        statusView->appendServerMessage(type,message);
    }

}

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
