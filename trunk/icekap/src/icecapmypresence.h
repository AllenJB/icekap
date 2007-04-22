/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef ICECAPMYPRESENCE_H
#define ICECAPMYPRESENCE_H

#include <qptrlist.h>
#include <qstring.h>
#include <qmap.h>

#include "icecapchannel.h"
#include "icecapnetwork.h"
// #include "statuspanel.h"
// #include "icecapoutputfilter.h"
// #include "icecapserver.h"

class StatusPanel;
class ViewContainer;
class IcecapServer;

namespace Icecap
{
//    class OutputFilter;
    typedef enum
    {
        SSDisconnected,
        SSConnecting,
        SSConnected
    } State;

    class MyPresence
    {
        public:
            MyPresence (): m_name(0) {}
            MyPresence (ViewContainer* viewContainer, IcecapServer* server, const QString& newName);
            MyPresence (ViewContainer* viewContainer, IcecapServer* server, const QString& newName, Network* newNetwork);
            MyPresence (ViewContainer* viewContainer, IcecapServer* server, const QString& newName, Network* newNetwork, const QMap<QString, QString>& parameterMap);
//            ~MyPresence ();

            QString name () { return m_name; };
            Network* network () { return m_network; };
            bool connected () { return m_connected; };
            bool autoconnect () { return m_autoconnect; };
            QString presence () { return m_presence; };
            QString getServerName() const { return m_network->name(); };
            IcecapServer* server() { return m_server; };
//            QString icecapServerName () { return m_serverName; }
            State state () { return m_state; };

            void setName (const QString& newName);
            void setNetwork (Network* newNetwork);
            void setConnected (bool newStatus);
            void setAutoconnect (bool newStatus);
            void setPresence (QString& presenceName);
//            void setServer (IcecapServer* server) { m_server = server; }
//            void setIcecapServerName (const QString serverName) { m_serverName = serverName; }
            void setState (State state);

            Channel* channel (const QString& channelName);
            void channelAdd (const Channel* channel);
            void channelAdd (const QString& channelName);
            void channelAdd (const QString& channelName, const QMap<QString, QString>& parameterMap);
            void channelRemove (const Channel* channel);
            void channelRemove (const QString& channelName);
            void channelClear ();

            bool operator== (MyPresence* compareTo);
            bool isNull ();

            void setViewContainer(ViewContainer* newViewContainer) { m_viewContainerPtr = newViewContainer; }
            ViewContainer* viewContainer () { return m_viewContainerPtr; }

            void appendStatusMessage(const QString& type,const QString& message);

            ViewContainer* getViewContainer() const { return m_viewContainerPtr; }

////            OutputFilter* outputFilter () { return m_outputFilter; }
////            void setOutputFilter (const OutputFilter& outputFilter) { m_outputFilter = outputFilter; }
//            IcecapOutputFilter* getOutputFilter();

        private:
            void init ();

            IcecapServer* m_server;

            QString m_name;
            QString m_presence;
            State m_state;
            bool m_connected;
            bool m_autoconnect;
            QPtrList<Channel> channelList;
            Network* m_network;
//            QString m_serverName;

            ViewContainer* m_viewContainerPtr;
            StatusPanel* statusView;
//            OutputFilter m_outputFilter;
    };

}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
