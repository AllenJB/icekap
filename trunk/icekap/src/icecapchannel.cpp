/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "icecapchannel.h"

#include "channelwindow.h"
#include "viewcontainer.h"
#include "icecapmypresence.h"

namespace Icecap
{

    Channel::Channel (MyPresence* p_mypresence, const QString& newName)
    {
        mypresence = p_mypresence;
        name = newName;
        connected = false;
        windowIsActive = false;
    }

    Channel::Channel (MyPresence* p_mypresence, const QString& newName, const QMap<QString, QString>& parameterMap)
    {
        mypresence = p_mypresence;
        name = newName;
        connected = parameterMap.contains ("joined");
        windowIsActive = false;

        if (connected) init ();
    }

    void Channel::init ()
    {
        if (windowIsActive) return;

        windowIsActive = true;
        window = getViewContainer()->addChannel (mypresence, name);
//        window->setServer (m_server);
    }

    void Channel::setName (const QString& newName)
    {
        name = newName;
    }

    void Channel::setTopic (const QString& newTopic, const QString& setBy, const QDateTime& timestamp)
    {
        topic = newTopic;
        topicSetBy = setBy;
        topicTimestamp = timestamp;
    }

    void Channel::setTopic (const QString& topic, const QString& setBy)
    {
        setTopic (topic, setBy, QDateTime::currentDateTime ());
    }

    void Channel::setTopic (const QString& newTopic)
    {
        topic = newTopic;
        topicSetBy = QString ();
        topicTimestamp = QDateTime ();
    }

    void Channel::setConnected (bool newStatus)
    {
        connected = newStatus;
        if (connected) init ();
    }

    void Channel::presenceAdd (const Presence& user)
    {
        presenceList.append (user);
    }

    Presence Channel::presenceByName (const QString& userName) {
        QValueList<Presence>::const_iterator end = presenceList.end();
        for( QValueListConstIterator<Presence> it( presenceList.begin() ); it != end; ++it ) {
            Presence current = *it;
            if (current.getName() == userName) {
                return current;
            }
        }
        // Return a "null" Presence
        // TODO: Is there a better way?
        return Presence();
    }

    Presence Channel::presenceByAddress (const QString& userAddress) {
        QValueList<Presence>::const_iterator end = presenceList.end();
        for( QValueListConstIterator<Presence> it( presenceList.begin() ); it != end; ++it ) {
            Presence current = *it;
            if (current.getAddress() == userAddress) {
                return current;
            }
        }
        // Return a "null" Presence
        // TODO: Is there a better way?
        return Presence();
    }

    void Channel::presenceRemove (const Presence& user)
    {
        presenceList.remove (user);
    }

    void Channel::presenceRemoveByName (const QString& userName)
    {
        presenceList.remove (presenceByName (userName));
    }

    void Channel::presenceRemoveByAddress (const QString& userAddress)
    {
        Channel::presenceList.remove (presenceByAddress (userAddress));
    }

    bool Channel::operator== (Channel compareTo)
    {
        return (name == compareTo.name);
    }

    bool Channel::isNull ()
    {
        return name.isNull();
    }

}

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
