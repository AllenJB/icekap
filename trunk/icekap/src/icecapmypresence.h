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
#include "icecapquery.h"

class StatusPanel;
class ViewContainer;
class IcecapServer;

// Small note on this class and how things work in Icekap:
// This class has nothing to do with the users own presence as shown in nick lists on channels
// For that we sort of cheat and just treat it the same as any other network / channel presence
// Unless someone can come up with a good reason to change this, this is the way it will stay.

namespace Icecap
{
    class Presence;
    class Query;

    typedef enum
    {
        SSDisconnected,
        SSConnecting,
        SSConnected
    } State;

    /**
     * Representation of a connection to a specific network
     * @todo Are all these constructors used?
     */
    class MyPresence : public QObject
    {
        Q_OBJECT

        public:
//            MyPresence (): m_name(0) {}
            MyPresence (ViewContainer* viewContainer, IcecapServer* server, const QString& newName, Network* newNetwork);
            MyPresence (ViewContainer* viewContainer, IcecapServer* server, const QString& newName, Network* newNetwork, const QMap<QString, QString>& parameterMap);

            void update (const QMap<QString,QString>& parameterMap);

            QString name () { return m_name; };
            Network* network () { return m_network; };
            bool connected () { return m_connected; };
            bool autoconnect () { return m_autoconnect; };
            QString getServerName() const { return m_network->name(); };
            IcecapServer* server() { return m_server; };
            State state () { return m_state; };
            Presence* presence () { return m_presence; }

            void setConnected (bool newStatus);
            void setAutoconnect (bool newStatus) { m_autoconnect = newStatus; }
            void setState (State state);

            Channel* channel (const QString& channelName);
            void channelAdd (const Channel* channel);
            void channelAdd (const QString& channelName);
            void channelAdd (const QString& channelName, const QMap<QString, QString>& parameterMap);
            void channelRemove (const Channel* channel);
            void channelRemove (const QString& channelName);
            /**
             * @todo AllenJB: Is this used?
             */
            void channelClear () { channelList.clear (); }

            Query* query (const QString& presenceName);
            // Returns the new query object
            Query* queryAdd (const QString& presenceName);
            void queryRemove (const QString& presenceName);
            void queryRemove (Query* query);

            bool operator== (MyPresence* compareTo);
            bool isNull () { return m_name.isNull (); }

            void setViewContainer(ViewContainer* newViewContainer) { m_viewContainerPtr = newViewContainer; }
            ViewContainer* viewContainer () { return m_viewContainerPtr; }
            ViewContainer* getViewContainer() const { return m_viewContainerPtr; }

            void appendStatusMessage (const QString& type,const QString& message);
            void appendCommandMessage (const QString& command, const QString& message, bool important = true,
                bool parseURL = true, bool self = false);

            uint getNickColor ();

        public slots:
            void eventFilter (Icecap::Cmd result);

        signals:
            void nameChanged ();

            /// Emitted whenever the MyPresence goes online or offline
            void serverOnline (bool state);

        private:
            void init ();
            void serverStateChanged (bool state);

            IcecapServer* m_server;

            QString m_name;
            State m_state;
            bool m_connected;
            bool m_autoconnect;
            QPtrList<Channel> channelList;
            QPtrList<Query> queryList;
            Network* m_network;
            Presence* m_presence;

            ViewContainer* m_viewContainerPtr;
            StatusPanel* statusView;
            bool statusViewActive;
    };

}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
