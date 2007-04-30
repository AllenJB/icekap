/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef ICECAPNETWORK_H
#define ICECAPNETWORK_H

#include <qptrlist.h>
#include <qstring.h>
#include <qvaluelist.h>

#include "icecapgateway.h"

namespace Icecap
{

    class Network
    {
        public:
            Network (): m_protocol(0), m_name(0) {}
            Network (const QString& newProtocol, const QString& newName);
//            ~Network ();

            QString protocol () const { return m_protocol; }
            QString name () const { return m_name; }
            bool connected () { return m_connected; }

            void setName (const QString& newName);
            void setConnected (bool newStatus);

            Gateway gateway (const QString& hostname, int port);
            void gatewayAdd (const Gateway& gateway);
            void gatewayRemove (const Gateway& gateway);
            void gatewayRemove (const QString& hostname, int port);

            bool operator== (Network compareTo) { return (m_name == compareTo.m_name); }
            bool isNull () { return m_name.isNull(); }

        private:
            QString m_protocol;
            QString m_name;
            bool m_connected;
            QValueList<Gateway> gatewayList;

    };

}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1: