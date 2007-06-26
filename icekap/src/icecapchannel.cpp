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

// TODO: Handle GUI tab closing
namespace Icecap
{

    Channel::Channel (MyPresence* p_mypresence, const QString& name)
    {
        m_mypresence = p_mypresence;
        m_name = name;
        connected = false;
        windowIsActive = false;

        // Connect to server event stream (command results)
        connect (m_mypresence->server(), SIGNAL (event(Cmd)), this, SLOT (eventFilter(Cmd)));
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
    }

    void Channel::init ()
    {
        if (windowIsActive) return;

        windowIsActive = true;
        window = getViewContainer()->addChannel (this);

        // TODO: This could be done with signals instead
        if (topic.length () > 0) {
            if (topicSetBy.length () > 0) {
                window->setTopic (topic);
            } else {
                window->setTopic (topicSetBy, topic);
            }
        }

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
    }

    ChannelPresence* Channel::presence (const QString& userName) {
        QPtrListIterator<ChannelPresence> it( presenceList );
        ChannelPresence* current;
        while ( (current = it.current()) != 0 ) {
            ++it;
            if (current->name () == userName) {
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
        window->append (nickname, message);
    }

    void Channel::appendAction(const QString& nickname,const QString& message, bool usenotifications)
    {
        window->appendAction (nickname, message, usenotifications);
    }

    void Channel::appendCommandMessage (const QString& command, const QString& message, bool important,
        bool parseURL, bool self)
    {
        window->appendCommandMessage(command, message, important, parseURL, self);
    }

    void Channel::eventFilter (Cmd result)
    {
        Network* network = m_mypresence->network();
        // Is the event relevent to this channel?
        if ((result.channel == m_name) && (result.mypresence == m_mypresence->name()) && (result.network == network->name())) {
            // Response to request for channel list
            // TODO: Do we need to deal with user initiated channel lists in a different way?
            if (result.sentCommand == "channel names") {
                QString presenceName = result.parameterList.find ("presence").data ();
                Presence *user = network->presence (presenceName);
                // If the user doesn't exist, create it
                if (user == 0) {
                    user = new Presence (presenceName, result.parameterList.find ("address").data ());
                    network->presenceAdd (user);
                }

                ChannelPresence* channelUser = new ChannelPresence (this, user);
                if (result.parameterList.contains ("irc_mode")) {
                    channelUser->setModes (result.parameterList.find ("irc_mode").data ());
                }
                presenceAdd (channelUser);
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

}

#include "icecapchannel.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
