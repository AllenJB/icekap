/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef ICECAPMYPRESENCE_H
#define ICECAPMYPRESENCE_H

#include <qvaluelist.h>
#include <qstring.h>
#include <qmap.h>

#include "icecapchannel.h"
#include "icecapnetwork.h"
#include "statuspanel.h"

namespace Icecap
{

    class MyPresence
    {
        public:
            MyPresence (): m_name(0) {}
            MyPresence (ViewContainer* viewContainer, const QString& newName);
            MyPresence (ViewContainer* viewContainer, const QString& newName, const Network& newNetwork);
            MyPresence (ViewContainer* viewContainer, const QString& newName, const Network& newNetwork, const QMap<QString, QString>& parameterMap);
//            ~MyPresence ();

            QString name () { return m_name; }
            Network network () { return m_network; }
            bool connected () { return m_connected; }
            bool autoconnect () { return m_autoconnect; }
            QString presence () { return m_presence; }

            void setName (const QString& newName);
            void setNetwork (const Network& newNetwork);
            void setConnected (bool newStatus);
            void setAutoconnect (bool newStatus);
            void setPresence (QString& presenceName);

            Channel channel (const QString& channelName);
            void channelAdd (const Channel& channel);
            void channelAdd (const QString& channelName);
            void channelAdd (const QString& channelName, const QMap<QString, QString>& parameterMap);
            void channelRemove (const Channel& channel);
            void channelRemove (const QString& channelName);
            void channelClear ();

            bool operator== (MyPresence compareTo);
            bool isNull ();

            void setViewContainer(ViewContainer* newViewContainer) { m_viewContainerPtr = newViewContainer; }
            ViewContainer* viewContainer () { return m_viewContainerPtr; }

        private:
            void init ();

            QString m_name;
            QString m_presence;
            bool m_connected;
            bool m_autoconnect;
            QValueList<Channel> channelList;
            Network m_network;

            ViewContainer* m_viewContainerPtr;
            StatusPanel* statusView;
    };

}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
