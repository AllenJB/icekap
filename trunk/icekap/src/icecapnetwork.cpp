/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "icecapnetwork.h"

#include "icecapserver.h"

namespace Icecap
{

    Network::Network (IcecapServer* server, const QString& newProtocol, const QString& newName)
    {
        m_server = server;
        m_protocol = newProtocol;
        m_name = newName;
        m_connected = false;

        // Connect to server event stream (command results)
        connect (m_server, SIGNAL (event(Icecap::Cmd)), this, SLOT (eventFilter(Icecap::Cmd)));
    }

    void Network::setName (const QString& newName)
    {
        m_name = newName;
    }

    void Network::setConnected (bool newStatus)
    {
        m_connected = newStatus;
    }

    Gateway* Network::gateway (const QString& hostname, int port)
    {
        QPtrListIterator<Gateway> it( gatewayList );
        Gateway* current;
        while ( (current = it.current()) != 0 ) {
            ++it;
            if ( (current->getHostname() == hostname) && (current->getPort() == port) ) {
                return current;
            }
        }
        return 0;
    }

    void Network::gatewayAdd (const Gateway* gateway)
    {
        gatewayList.append (gateway);
    }

    void Network::gatewayRemove (const QString& hostname, int port)
    {
        gatewayList.remove (gateway (hostname, port));
    }

    void Network::gatewayRemove (const Gateway* gateway)
    {
        gatewayList.remove (gateway);
    }

    Presence* Network::presence (const QString& name)
    {
        QPtrListIterator<Presence> it( presenceList );
        Presence* current;
        while ( (current = it.current()) != 0 ) {
            ++it;
            if ( current->name() == name ) {
                return current;
            }
        }
        return 0;
    }

    void Network::presenceAdd (const Presence* newPresence)
    {
        if (presence (newPresence->name()) != 0) {
            return;
        }
        presenceList.append (newPresence);
    }

    void Network::presenceRemove (const Presence* presence)
    {
        presenceList.remove (presence);
    }

    void Network::presenceRemove (const QString& name)
    {
        presenceList.remove (presence (name));
    }

    // slot
    void Network::eventFilter (Icecap::Cmd ev)
    {
        if (ev.network != m_name) {
            return;
        }

        if ((ev.tag == "*") && (ev.command == "presence_init"))
        {
            Presence* newPresence;
            if (ev.parameterList.contains ("address")) {
                newPresence = new Presence (ev.parameterList["presence"], ev.parameterList["address"]);
            } else {
                newPresence = new Presence (ev.parameterList["presence"]);
            }
            presenceAdd (newPresence);
            return;
        }
        else if ((ev.tag == "*") && (ev.command == "presence_changed"))
        {
            presence (ev.parameterList["presence"])->update (ev.parameterList);
        }
    }

}

#include "icecapnetwork.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
