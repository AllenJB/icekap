/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "icecapchannel.h"

// #include <klistview.h>

#include "channelwindow.h"
#include "viewcontainer.h"
#include "nicklistview.h"
#include "ircview.h"

#include "icecapmypresence.h"

// TODO: Handle GUI tab closing
// TODO: Switch topic to a signal based system
namespace Icecap
{

    Channel::Channel (MyPresence* p_mypresence, const QString& name)
    {
        m_mypresence = p_mypresence;
        m_name = name;
        connected = false;
        windowIsActive = false;
        QObject::setName (QString ("channel"+name).ascii());

        // Connect to server event stream (command results)
        connect (m_mypresence->server(), SIGNAL (Icecap::event(Cmd)), this, SLOT (eventFilter(Icecap::Cmd)));
    }

    Channel::Channel (MyPresence* p_mypresence, const QString& name, const QMap<QString, QString>& parameterMap)
    {
        m_mypresence = p_mypresence;
        m_name = name;
        connected = parameterMap.contains ("joined");
        if (parameterMap.contains ("topic")) {
            topic = parameterMap["topic"];
        }
        windowIsActive = false;

        if (connected) init ();

        // Connect to server event stream (command results)
        connect (m_mypresence->server(), SIGNAL (event(Icecap::Cmd)), this, SLOT (eventFilter(Icecap::Cmd)));
    }

    void Channel::init ()
    {
        if (windowIsActive) return;

        windowIsActive = true;
        window = getViewContainer()->addChannel (this);

        // Initialise nick listView items
        NickListView* listView = window->getNickListView ();
        QPtrListIterator<ChannelPresence> it( presenceList );
        ChannelPresence* current;
        while ( (current = it.current()) != 0 ) {
            ++it;
            current->setListView (listView);
        }


        // TODO: This could be done with signals instead
        if (topic.length () > 0) {
            if (topicSetBy.length () > 0) {
                window->setTopic (topic);
            } else {
                window->setTopic (topicSetBy, topic);
            }
        }

        Cmd listNames;
        listNames.tag = "cn";
        listNames.command = "channel names";
        listNames.parameterList.insert ("channel", m_name);
        listNames.parameterList.insert ("mypresence", m_mypresence->name ());
        listNames.parameterList.insert ("network", m_mypresence->network()->name ());
        m_mypresence->server()->queueCommand (listNames);

//        m_mypresence->server()->queue (QString ("chplist;channel names;mypresence=%1;network=%2;channel=%3").arg(m_mypresence->name()).arg(m_mypresence->network()->name()).arg(m_name));

//        m_mypresence->server()->channelNames (<mypresence>, <network>, <channel>);
// OR:    m_mypresence->server()->channelNames (this);
    }
/*
    void Channel::setTopic (const QString& newTopic, const QString& setBy, const QDateTime& timestamp)
    {
        topic = newTopic;
        topicSetBy = setBy;
        topicTimestamp = timestamp;
        if (windowIsActive) {
            window->setTopic (topicSetBy, topic);
        }
    }
*/
    void Channel::setTopic (const QString& newTopic, const QString& setBy, const QString& timestampStr)
    {
        topic = newTopic;
        topicSetBy = setBy;
        topicTimestamp = QDateTime ();
        topicTimestamp.setTime_t (timestampStr.toUInt ());
        // TODO: This could be done with signals instead
        if (windowIsActive) {
            window->setTopic (topicSetBy, topic);
        }
    }

/*
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
*/
    void Channel::setConnected (bool newStatus)
    {
        connected = newStatus;
        if (connected) init ();
    }

    void Channel::presenceAdd (const ChannelPresence* user)
    {
        presenceList.append (user);
        if (windowIsActive) {
            append ("--CHDEBUG--", QString ("Added presence: %1 :: %2").arg (user->getNickname ()).arg (user->getHostmask()));
        }
    }

    ChannelPresence* Channel::presence (const QString& userName) {
        QString lookupName = userName.lower ();
        QPtrListIterator<ChannelPresence> it( presenceList );
        ChannelPresence* current;
        while ( (current = it.current()) != 0 ) {
            ++it;
            if (current->loweredNickname () == lookupName) {
                return current;
            }
        }

        return 0;
    }

    ChannelPresence* Channel::presenceByAddress (const QString& userAddress) {
        QPtrListIterator<ChannelPresence> it( presenceList );
        ChannelPresence* current;
        while ( (current = it.current()) != 0 ) {
            ++it;
            if (current->address () == userAddress) {
                return current;
            }
        }

        return 0;
    }

    void Channel::presenceRemove (const ChannelPresence* user)
    {
        presenceList.remove (user);
    }

    void Channel::presenceRemoveByName (const QString& userName)
    {
        presenceList.remove (presence (userName));
    }

    void Channel::presenceRemoveByAddress (const QString& userAddress)
    {
        Channel::presenceList.remove (presenceByAddress (userAddress));
    }

    ViewContainer* Channel::getViewContainer () const
    {
        return m_mypresence->getViewContainer ();
    }

    void Channel::append(const QString& nickname,const QString& message)
    {
        if (!windowIsActive) return;
        window->append (nickname, message);
    }

    void Channel::appendAction(const QString& nickname,const QString& message, bool usenotifications)
    {
        if (!windowIsActive) return;
        window->appendAction (nickname, message, usenotifications);
    }

    void Channel::appendCommandMessage (const QString& command, const QString& message, bool important,
        bool parseURL, bool self)
    {
        if (!windowIsActive) return;
        window->appendCommandMessage(command, message, important, parseURL, self);
    }

    void Channel::eventFilter (Cmd result)
    {
        append ("--CHDEBUG--", QString ("Received event: %1 -> %2 -- %3 :: %4 :: network: %5 :: channel: %6").arg (m_mypresence->name()).arg (name()).arg (result.tag).arg (result.sentCommand).arg (result.network).arg (result.channel));

        Network* network = m_mypresence->network();
        // Is the event relevent to this channel?
        if ((result.channel == m_name) && (result.mypresence == m_mypresence->name()) && (result.network == network->name())) {
            // Response to request for channel list
            // TODO: Do we need to deal with user initiated channel lists in a different way?
            if (result.sentCommand == "channel names") {
                append ("--CHDEBUG--", QString ("Received event for channel names: %1 -> %2 -- %3").arg (m_mypresence->name()).arg (name()).arg (result.tag));

                QString presenceName = result.parameterList.find ("presence").data ();
                Presence *user = network->presence (presenceName);
                // If the user doesn't exist, create it
                if (user == 0) {
                    append ("--CHDEBUG--", QString ("User %1 does not exist on this network yet. Creating with address %2").arg (presenceName).arg (result.parameterList.find ("address").data ()));
                    user = new Presence (presenceName, result.parameterList.find ("address").data ());
                    network->presenceAdd (user);
                }

                ChannelPresence* channelUser = new ChannelPresence (this, user);
                if (result.parameterList.contains ("irc_mode")) {
                    channelUser->setModes (result.parameterList.find ("irc_mode").data ());
                }

                if (windowIsActive) {
                    channelUser->setListView (window->getNickListView ());
                }

                presenceAdd (channelUser);
                emit presenceJoined (channelUser);
            }
        }
    }


    bool Channel::operator== (Channel compareTo)
    {
        return (m_name == compareTo.m_name);
    }

    bool Channel::isNull ()
    {
        return m_name.isNull();
    }

    QStringList Channel::getSelectedNickList()
    {
        QStringList result;

        if (window->getChannelCommand ())
            result.append (window->getTextView()->getContextNick());
        else
        {
            QPtrListIterator<ChannelPresence> it( presenceList );
            ChannelPresence* current;
            while ( (current = it.current()) != 0 ) {
                ++it;
                if (current->isSelected ()) {
                    result.append (current->getNickname ());
                }
            }
        }

        return result;
    }

    bool Channel::containsNick (QString nickname) {
        QPtrListIterator<ChannelPresence> it( presenceList );
        ChannelPresence* current;
        while ( (current = it.current()) != 0 ) {
            ++it;
            if (current->name () == nickname) {
                return true;
            }
        }
        return false;
    }
}

#include "icecapchannel.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
