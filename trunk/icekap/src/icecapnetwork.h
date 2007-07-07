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
#include <qobject.h>

#include "icecapgateway.h"
#include "icecapmisc.h"
#include "icecappresence.h"

class IcecapServer;

namespace Icecap
{

    class Network : public QObject
    {
        Q_OBJECT

        public:
            Network (): m_protocol(0), m_name(0), m_server(0) {}
            Network (IcecapServer* server, const QString& newProtocol, const QString& newName);

            QString protocol () const { return m_protocol; }
            QString name () const { return m_name; }
            bool connected () { return m_connected; }

            void setName (const QString& newName);
            void setConnected (bool newStatus);

            Gateway* gateway (const QString& hostname, int port);
            void gatewayAdd (const Gateway* gateway);
            void gatewayRemove (const Gateway* gateway);
            void gatewayRemove (const QString& hostname, int port);

            Presence* presence (const QString& name);
            void presenceAdd (const Presence* newPresence);
            void presenceRemove (const Presence* presence);
            void presenceRemove (const QString& name);

            bool operator== (Network* compareTo) { return (m_name == compareTo->name()); }
            bool isNull () { return m_name.isNull(); }

        public slots:
            // TODO: What's this used for? Don't forget that networks are not mypresence specific
            void eventFilter (Icecap::Cmd result);

        private:
            QString m_protocol;
            QString m_name;
            bool m_connected;
            QPtrList<Gateway> gatewayList;
            QPtrList<Presence> presenceList;
            IcecapServer* m_server;

    };

}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
