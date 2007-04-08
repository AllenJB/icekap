/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "icecapnetwork.h"

namespace Icecap
{

    Network::Network (const QString& newProtocol, const QString& newName)
    {
        protocol = newProtocol;
        name = newName;
        connected = false;
    }

    void Network::setName (const QString& newName)
    {
        name = newName;
    }

    void Network::setConnected (bool newStatus)
    {
        connected = newStatus;
    }

    Gateway Network::gateway (const QString& hostname, int port)
    {
        QValueList<Gateway>::const_iterator end = gatewayList.end();
        for( QValueListConstIterator<Gateway> it( gatewayList.begin() ); it != end; ++it ) {
            Gateway current = *it;
            if ( (current.getHostname() == hostname) && (current.getPort() == port) ) {
                return current;
            }
        }
        // Return a "null" Presence
        // TODO: Is there a better way?
        return Gateway();
    }

    void Network::gatewayAdd (const Gateway& gateway)
    {
        gatewayList.append (gateway);
    }

    void Network::gatewayRemove (const QString& hostname, int port)
    {
        gatewayList.remove (gateway (hostname, port));
    }

    void Network::gatewayRemove (const Gateway& gateway)
    {
        gatewayList.remove (gateway);
    }

    bool Network::operator== (Network compareTo)
    {
        return ( (protocol == compareTo.protocol) && (name == compareTo.name) );
    }

    bool Network::isNull ()
    {
        return name.isNull();
    }

}


// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
