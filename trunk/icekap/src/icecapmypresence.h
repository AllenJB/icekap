/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef ICECAPMYPRESENCE_H
#define ICECAPMYPRESENCE_H

#include <qobject.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>

#include "icecapchannel.h"
#include "icecapmisc.h"
#include "icecapnetwork.h"

class StatusPanel;
class ViewContainer;
class IcecapServer;

namespace Icecap
{
    typedef enum
    {
        SSDisconnected,
        SSConnecting,
        SSConnected
    } State;

    class MyPresence : public QObject
    {
        Q_OBJECT

        public:
            // TODO: Do we really need all these constructors?
            MyPresence (): m_name(0) {}
            MyPresence (ViewContainer* viewContainer, IcecapServer* server, const QString& newName);
            MyPresence (ViewContainer* viewContainer, IcecapServer* server, const QString& newName, Network* newNetwork);
            MyPresence (ViewContainer* viewContainer, IcecapServer* server, const QString& newName, Network* newNetwork, const QMap<QString, QString>& parameterMap);
//            ~MyPresence ();

            void update (const QMap<QString,QString>& parameterMap);

            QString name () { return m_name; };
            Network* network () { return m_network; };
            bool connected () { return m_connected; };
            bool autoconnect () { return m_autoconnect; };
            QString getServerName() const { return m_network->name(); };
            IcecapServer* server() { return m_server; };
            State state () { return m_state; };

            void setNetwork (Network* newNetwork);
            void setConnected (bool newStatus);
            void setAutoconnect (bool newStatus);
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
            ViewContainer* getViewContainer() const { return m_viewContainerPtr; }

            void appendStatusMessage(const QString& type,const QString& message);

        public slots:
            void eventFilter (Icecap::Cmd result);

        private:
            void init ();

            IcecapServer* m_server;

            QString m_name;
            State m_state;
            bool m_connected;
            bool m_autoconnect;
            QPtrList<Channel> channelList;
            Network* m_network;

            ViewContainer* m_viewContainerPtr;
            StatusPanel* statusView;
            bool statusViewActive;
    };

}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1: