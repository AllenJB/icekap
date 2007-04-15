/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "icecapmypresence.h"
#include "viewcontainer.h"

namespace Icecap
{

    MyPresence::MyPresence (ViewContainer* viewContainer, const QString& newName)
    {
        m_name = newName;
        m_connected = false;
        m_autoconnect = false;
        m_viewContainerPtr = viewContainer;
    }

    MyPresence::MyPresence (ViewContainer* viewContainer, const QString& newName, const Network& newNetwork)
    {
        m_name = newName;
        m_network = newNetwork;
        m_connected = false;
        m_autoconnect = false;
        m_viewContainerPtr = viewContainer;
    }

    MyPresence::MyPresence (ViewContainer* viewContainer, const QString& newName, const Network& newNetwork, const QMap<QString,QString>& parameterMap)
    {
        m_name = newName;
        m_network = newNetwork;
        m_connected = parameterMap.contains ("connected");
        m_autoconnect = parameterMap.contains ("autoconnect");
        if (parameterMap.contains ("presence"))
        {
            m_presence = parameterMap["presence"];
        }
        m_viewContainerPtr = viewContainer;
        if (m_connected)
        {
            init ();
        }
    }

    void MyPresence::init ()
    {
        statusView = m_viewContainerPtr->addStatusView(this);
    }

    void MyPresence::setName (const QString& newName)
    {
        m_name = newName;
    }

    void MyPresence::setNetwork (const Network& newNetwork)
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

    Channel MyPresence::channel (const QString& channelName)
    {
        QValueList<Channel>::const_iterator end = channelList.end();
        for( QValueListConstIterator<Channel> it( channelList.begin() ); it != end; ++it ) {
            Channel current = *it;
            if (current.getName() == channelName) {
                return current;
            }
        }
        // Return a "null" Presence
        // TODO: Is there a better way?
        return Channel();
    }

    void MyPresence::channelAdd (const Channel& channel)
    {
        channelList.append (channel);
    }

    void MyPresence::channelAdd (const QString& channelName)
    {
        channelList.append (Channel (channelName));
    }

    void MyPresence::channelAdd (const QString& channelName, const QMap<QString, QString>& parameterMap)
    {
        channelList.append (Channel (channelName, parameterMap));
    }

    void MyPresence::channelRemove (const Channel& channel)
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

    bool MyPresence::operator== (MyPresence compareTo)
    {
        return ( (m_name == compareTo.m_name) && (m_network == compareTo.m_network) );
    }

    bool MyPresence::isNull ()
    {
        return m_name.isNull();
    }

}

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
