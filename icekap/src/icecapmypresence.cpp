/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "icecapmypresence.h"

namespace Icecap
{

    MyPresence::MyPresence (const QString& newName)
    {
        name = newName;
        connected = false;
        autoconnect = false;
    }

    MyPresence::MyPresence (const QString& newName, const Network& newNetwork)
    {
        name = newName;
        network = newNetwork;
        connected = false;
        autoconnect = false;
    }

    void MyPresence::setName (const QString& newName)
    {
        name = newName;
    }

    void MyPresence::setNetwork (const Network& newNetwork)
    {
        network = newNetwork;
    }

    void MyPresence::setConnected (bool newStatus)
    {
        connected = newStatus;
    }

    void MyPresence::setAutoconnect (bool newStatus)
    {
        autoconnect = newStatus;
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

/*
        QPtrListIterator<Channel> it( channelList );
        Channel* current;
        while ( (current = it.current()) != 0 ) {
            ++it;
            if (current.name() == channelName) {
                break;
            }
        }
        return current;
*/
    }

    void MyPresence::channelAdd (const Channel& channel)
    {
        channelList.append (channel);
    }

    void MyPresence::channelAdd (const QString& channelName)
    {
        channelList.append (Channel (channelName));
    }

    void MyPresence::channelRemove (const Channel& channel)
    {
        channelList.remove (channel);
    }

    void MyPresence::channelRemove (const QString& channelName)
    {
        channelList.remove (channel (channelName));
    }

    bool MyPresence::operator== (MyPresence compareTo)
    {
        return ( (name == compareTo.name) && (network == compareTo.network) );
    }

    bool MyPresence::isNull ()
    {
        return name.isNull();
    }

}

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
