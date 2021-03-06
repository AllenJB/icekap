/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "icecapchannel.h"

#include <klocale.h>

#include "channelwindow.h"
#include "viewcontainer.h"
#include "nicklistview.h"
#include "ircview.h"
#include "nicklist.h"

#include "icecapmypresence.h"

namespace Icecap
{

    Channel::Channel (MyPresence* p_mypresence, const QString& name)
    {
        m_mypresence = p_mypresence;
        m_name = name;
        m_connected = false;
        windowIsActive = false;
        QObject::setName (QString ("channel"+name).ascii());
        m_numberOfOps = 0;

        // Delete every contained object when nickList is deleted
        m_nickList.setAutoDelete (true);

        // Connect to server event stream (command results)
        connect (m_mypresence->server(), SIGNAL (event(Icecap::Cmd)), this, SLOT (eventFilter(Icecap::Cmd)));
        // Watch for server / network state changes
        connect (m_mypresence, SIGNAL (serverOnline(bool)), this, SLOT (serverStateChanged(bool)));
    }

    Channel::Channel (MyPresence* p_mypresence, const QString& name, const QMap<QString, QString>& parameterMap)
    {
        m_mypresence = p_mypresence;
        m_name = name;
        m_connected = parameterMap.contains ("joined");
        if (parameterMap.contains ("topic")) {
            topic = parameterMap["topic"];
        }
        windowIsActive = false;
        QObject::setName (QString ("channel"+name).ascii());
        m_numberOfOps = 0;

        // Connect to server event stream (command results)
        connect (m_mypresence->server(), SIGNAL (event(Icecap::Cmd)), this, SLOT (eventFilter(Icecap::Cmd)));

        if (m_connected) init ();
    }

    Channel::~Channel ()
    {
        delete m_window;
    }

    /**
     * Initialise GUI window, populate the NickListView, set the topic and request a list of users from the server.
     */
    void Channel::init ()
    {
        if (windowIsActive) return;

        if (m_mypresence->connected() == false) {
            m_mypresence->setConnected (true);
        }

        windowIsActive = true;
        m_window = getViewContainer()->addChannel (this);

        // Initialise nick listView items
        NickListView* listView = m_window->getNickListView ();
        QPtrListIterator<ChannelPresence> it( m_nickList );
        ChannelPresence* current;
        while ( (current = it.current()) != 0 ) {
            ++it;
            current->setListView (listView);
        }


        // TODO: This could be done with signals instead
        if (topic.length () > 0) {
            if (topicSetBy.length () > 0) {
                m_window->setTopic (topic);
            } else {
                m_window->setTopic (topicSetBy, topic);
            }
        }

        // Channel names
        Cmd listNames;
        listNames.tag = "cn";
        listNames.command = "channel names";
        listNames.parameterList.insert ("channel", m_name);
        listNames.parameterList.insert ("mypresence", m_mypresence->name ());
        listNames.parameterList.insert ("network", m_mypresence->network()->name ());
        m_mypresence->server()->queueCommand (listNames);
    }

    /**
     * Set the topic
     * @param newTopic New topic
     * @param setBy Name of user who set the topic
     * @param timestampStr Time at which topic was set
     */
    void Channel::setTopic (const QString& newTopic, const QString& setBy, const QString& timestampStr)
    {
        topic = newTopic;
        topicSetBy = setBy;
        topicTimestamp = QDateTime ();
        topicTimestamp.setTime_t (timestampStr.toUInt ());
        // TODO: This could be done with signals instead
        if (windowIsActive) {
            m_window->setTopic (topicSetBy, topic);
        }
    }

    /**
     * Set the connected state
     * @param newStatus New state
     */
    void Channel::setConnected (bool newStatus)
    {
        if (newStatus == m_connected) return;
        m_connected = newStatus;
        if (windowIsActive) {
            m_window->serverOnline (m_connected);
        }
        if (m_connected) {
            init ();
        }
    }

    /**
     * Add a channel presence
     * @param user Presence to add
     */
    void Channel::presenceAdd (ChannelPresence* user)
    {
        if (presence (user->name())) {
            return;
        }

        if (windowIsActive) {
            user->setListView (m_window->getNickListView ());
        }

        m_nickList.append (user);
    }

    /**
     * Find a presence by username
     * @param userName Username to search for
     * @return Requested presence; 0 if no presence found
     */
    ChannelPresence* Channel::presence (const QString& userName) {
        QString lookupName = userName.lower ();
        QPtrListIterator<ChannelPresence> it( m_nickList );
        ChannelPresence* current;
        while ( (current = it.current()) != 0 ) {
            ++it;
            if (current->loweredNickname () == lookupName) {
                return current;
            }
        }

        return 0;
    }

    /**
     * Find a presence by address (hostmask)
     * @param userAddress Address to search for
     * @return Requested presence; 0 if no presence found
     */
    ChannelPresence* Channel::presenceByAddress (const QString& userAddress) {
        QPtrListIterator<ChannelPresence> it( m_nickList );
        ChannelPresence* current;
        while ( (current = it.current()) != 0 ) {
            ++it;
            if (current->address () == userAddress) {
                return current;
            }
        }

        return 0;
    }

    /**
     * Remove a channel presence
     * @param user Presence to remove
     */
    void Channel::presenceRemove (const ChannelPresence* user)
    {
        m_nickList.remove (user);
        delete user;
    }

    /**
     * Remove a channel presence by name
     * @param userName Name of presence to remove
     */
    void Channel::presenceRemoveByName (const QString& userName)
    {
        ChannelPresence* user = presence (userName);
        if (0 == user)
            return;
        presenceRemove (user);
    }

    /**
     * Remove presence by address
     * @param userAddress Address of user to remove
     */
    void Channel::presenceRemoveByAddress (const QString& userAddress)
    {
        presenceRemove (presenceByAddress (userAddress));
    }

    /**
     * Return view container. Shortcut to m_mypresence->getViewContainer
     * @return View container
     */
    ViewContainer* Channel::getViewContainer () const
    {
        return m_mypresence->getViewContainer ();
    }

    /**
     * Append message to channel
     * @param nickname User who posted the message
     * @param message Message
     */
    void Channel::append(const QString& nickname,const QString& message)
    {
        if (!windowIsActive) return;
        m_window->append (nickname, message);
    }

    /**
     * Append user action to channel
     * @param nickname User who posted the action
     * @param message Action message
     * @param usenotifications Use notifications?
     */
    void Channel::appendAction(const QString& nickname,const QString& message, bool usenotifications)
    {
        if (!windowIsActive) return;
        m_window->appendAction (nickname, message, usenotifications);
    }

    /**
     * Append command message to channel
     * @param command Source command
     * @param message Message
     * @param important Is this important?
     * @param parseURL Parse URLs?
     * @param self
     */
    void Channel::appendCommandMessage (const QString& command, const QString& message, bool important, bool parseURL, bool self)
    {
        if (!windowIsActive) return;
        m_window->appendCommandMessage (command, message, important, parseURL, self);
    }

    /**
     * Parse events from the server
     * @param ev Event
     * @todo AllenJB: Get rid of irc modes in favour of pure icecap modes?
     */
    void Channel::eventFilter (Cmd ev)
    {
        Network* network = m_mypresence->network();
        // Is the event relevent to this channel?
        if ((ev.channel != m_name) || (ev.mypresence != m_mypresence->name()) || (ev.network != network->name())) {
            return;
        }

        // Response to request for channel list
        if (ev.sentCommand == "channel names") {
            if (ev.status == "-") {
                // TODO: Error handling
                if (ev.error == "notfound") {
                    appendCommandMessage("Join", "Unable to retrieve channel names list: Channel not found.");
                }
                return;
            } else if (ev.status == "+") {
                // Ignore - successful completion
                return;
            }

            QString presenceName = ev.parameterList["presence"];
            Presence *user = network->presence (presenceName);
            // If the user doesn't exist, create it
            if (user == 0) {
                user = new Presence (presenceName, ev.parameterList["address"]);
                network->presenceAdd (user);
            }

            ChannelPresence* channelUser = new ChannelPresence (this, user);
            if (ev.parameterList.contains ("mode")) {
                channelUser->setMode (ev.parameterList["mode"]);
            }
            if (channelUser->isAnyTypeOfOp ()) {
                m_numberOfOps++;
            }

            presenceAdd (channelUser);
            emit userListUpdated ();
            m_window->getNickListView ()->startResortTimer ();
        }
        else if (ev.sentCommand == "channel change")
        {
            if (ev.error == "noperm") {
                appendCommandMessage (i18n ("Modes"), i18n ("Permission Denied"));
            }
        }
        else if (ev.tag == "*")
        {
            if (ev.command == "channel_changed")
            {
                if (ev.parameterList.contains ("topic")) {
                    setTopic (ev.parameterList["topic"], ev.parameterList["topic_set_by"], ev.parameterList["timestamp"]);
                    if (! ev.parameterList.contains ("init")) {
                        appendCommandMessage(i18n("Topic"), i18n("%1 changed the topic to: %2").arg (ev.parameterList["topic_set_by"]).arg(ev.parameterList["topic"]));
                    }
                }
                else if (ev.parameterList.contains ("initial_presences_added"))
                {
                    m_window->getNickListView ()->startResortTimer ();
                }
            }
            else if (ev.command == "msg")
            {
                /**
                 * @todo AllenJB: Do we need to escape the presence name too?
                 */
                QString escapedMsg = ev.parameterList["msg"];
                escapedMsg.replace ("\\.", ";");
                if ((ev.parameterList.contains ("type")) && (ev.parameterList["type"] == "action")) {
                    appendAction (ev.parameterList["presence"], escapedMsg);
                } else {
                    append (ev.parameterList["presence"], escapedMsg);
                }
                presence (ev.parameterList["presence"])->setTimeStamp (ev.parameterList["timestamp"].toUInt());
            }
            else if (ev.command == "channel_presence_added")
            {
                QString presenceName = ev.parameterList["presence"];
                Presence *user = network->presence (presenceName);
                // If the user doesn't exist, create it
                if (user == 0) {
                    user = new Presence (presenceName, ev.parameterList["address"]);
                    network->presenceAdd (user);
                }

                ChannelPresence* channelUser = new ChannelPresence (this, user);
                if (ev.parameterList.contains ("mode")) {
                    channelUser->setMode (ev.parameterList["mode"]);
                }

                presenceAdd (channelUser);
                if (! ev.parameterList.contains ("init")) {
                    appendCommandMessage ("-->", i18n ("Join: %1 (%2)").arg (presenceName).arg (channelUser->address()));
                }
                if (channelUser->isAnyTypeOfOp ()) {
                    m_numberOfOps++;
                }
                emit userListUpdated ();
            }
            else if (ev.command == "channel_presence_removed")
            {
                ChannelPresence* user = presence (ev.parameterList["presence"]);
                QString userAddress = user->address ();
                if (user->isAnyTypeOfOp ()) {
                    m_numberOfOps--;
                }
                // TODO: Source presence for kicks
                presenceRemoveByName (ev.parameterList["presence"]);
                if (! ev.parameterList.contains ("deinit")) {
                    if (ev.parameterList["type"] == "quit") {
                        appendCommandMessage ("<--", i18n ("Quit: %1 (%2) :: %3").arg (ev.parameterList["presence"]).arg (userAddress).arg (ev.parameterList["reason"]));
                    } else if (ev.parameterList["type"] == "kick") {
                        appendCommandMessage ("<--", i18n ("Kick: %1 (%2) :: %3").arg (ev.parameterList["presence"]).arg (userAddress).arg (ev.parameterList["reason"]));
                    } else {
                        appendCommandMessage ("<--", i18n ("Part: %1 (%2) :: %3").arg (ev.parameterList["presence"]).arg (userAddress).arg (ev.parameterList["reason"]));
                    }
                }
                emit userListUpdated ();
            }
            else if (ev.command == "channel_presence_mode_changed")
            {
                ChannelPresence* user = presence (ev.parameterList["presence"]);
                bool beforeVal = user->isAnyTypeOfOp ();
                QString sourceNick;
                if (ev.parameterList.contains ("source_presence")) {
                    sourceNick = ev.parameterList["source_presence"];
                } else {
                    sourceNick = ev.parameterList["irc_source_nick"];
                }
                if (ev.parameterList.contains ("add")) {
                    user->modeChange (true, ev.parameterList["add"]);
                    appendCommandMessage (i18n ("Modes"), i18n ("Mode change: +%1 %2 by %3").arg (ev.parameterList["add"]).arg (ev.parameterList["presence"]).arg (sourceNick));
                } else if (ev.parameterList.contains ("remove")) {
                    user->modeChange (false, ev.parameterList["remove"]);
                    appendCommandMessage (i18n ("Modes"), i18n ("Mode change: -%1 %2 by %3").arg (ev.parameterList["remove"]).arg (ev.parameterList["presence"]).arg (sourceNick));
                }
                if (user->isAnyTypeOfOp () != beforeVal) {
                    if (beforeVal) {
                        m_numberOfOps--;
                    } else {
                        m_numberOfOps++;
                    }
                }
                emit userListUpdated ();
            }
            else if (ev.command == "channel_connection_init")
            {
                setConnected (true);
                appendCommandMessage (i18n ("Channel"), "Connected to channel: "+ ev.channel);
            }
            else if (ev.command == "channel_connection_deinit")
            {
                setConnected (false);
                if (ev.parameterList["reason"].length () > 0) {
                    appendCommandMessage (i18n ("Channel"), "Disconnected from channel: "+ ev.channel +": "+ ev.parameterList["reason"]);
                } else {
                    appendCommandMessage (i18n ("Channel"), "Disconnected from channel: "+ ev.channel);
                }
            }
        } // end if (ev.tag == "*")
    }


    /**
     * Comparison operator
     * @param compareTo Object to compare to
     * @return Same object?
     */
    bool Channel::operator== (Channel compareTo)
    {
        return (m_name == compareTo.m_name);
    }

    /**
     * Is this a null object?
     * @return Null object?
     */
    bool Channel::isNull ()
    {
        return m_name.isNull();
    }

    /**
     * Return list of nicknames that are currently selected in the NickListView
     * @return Selected nicknames
     */
    QStringList Channel::getSelectedNickList()
    {
        QStringList result;

        if (m_window->getChannelCommand ())
            result.append (m_window->getTextView()->getContextNick());
        else
        {
            QPtrListIterator<ChannelPresence> it( m_nickList );
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

    /**
     * Check if this channel contains a given nickname
     * @param nickname Nick to check for
     * @return Specified nick is in this channel?
     */
    bool Channel::containsNick (QString nickname) {
        QPtrListIterator<ChannelPresence> it( m_nickList );
        ChannelPresence* current;
        while ( (current = it.current()) != 0 ) {
            ++it;
            if (current->name () == nickname) {
                return true;
            }
        }
        return false;
    }

    // TODO AllenJB: Disable window if left open
    // TODO AllenJB: Output channel closed message if left open
    void Channel::serverStateChanged (bool state)
    {
        setConnected (state);
        if (Preferences::closeInactiveTabs()) {
            if (state) {
                init();
            } else {
                windowIsActive = false;
                getViewContainer()->closeView (m_window);
                delete m_window;
            }
        }
    }

}

#include "icecapchannel.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
