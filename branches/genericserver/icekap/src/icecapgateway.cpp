/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "icecapgateway.h"

namespace Icecap {

    Gateway::Gateway (const QString& host)
    {
        port = 1027;
        hostname = host;
        connected = false;
        priority = 0;
    }

    Gateway::Gateway (const QString& host, int portNo)
    {
        port = portNo;
        hostname = host;
        connected = false;
        priority = 0;
    }

    void Gateway::setHostname (const QString& host)
    {
        hostname = host;
    }

    void Gateway::setPassword (const QString& pass)
    {
        password = pass;
    }

    void Gateway::setPort (int portNo)
    {
        port = portNo;
    }

    void Gateway::setPriority (int pri)
    {
        priority = pri;
    }

    void Gateway::setConnected (bool conn)
    {
        connected = conn;
    }

    bool Gateway::operator== (Gateway compareTo)
    {
        return ( (hostname == compareTo.hostname) && (port == compareTo.port) );
    }

    bool Gateway::isNull ()
    {
        return (hostname.isNull());
    }

}

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
