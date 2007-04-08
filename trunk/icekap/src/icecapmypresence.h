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

#include "icecapchannel.h"
#include "icecapnetwork.h"

namespace Icecap
{

    class MyPresence
    {
        public:
            MyPresence (): name(0) {}
            MyPresence (const QString& newName);
            MyPresence (const QString& newName, const Network& newNetwork);
//            ~MyPresence ();

            QString getName () { return name; }
            Network getNetwork () { return network; }
            bool getConnected () { return connected; }
            bool getAutoconnect () { return autoconnect; }

            void setName (const QString& newName);
            void setNetwork (const Network& newNetwork);
            void setConnected (bool newStatus);
            void setAutoconnect (bool newStatus);

            Channel channel (const QString& channelName);
            void channelAdd (const Channel& channel);
            void channelAdd (const QString& channelName);
            void channelRemove (const Channel& channel);
            void channelRemove (const QString& channelName);

            bool operator== (MyPresence compareTo);
            bool isNull ();

        private:
            QString name;
            bool connected;
            bool autoconnect;
            QValueList<Channel> channelList;
            Network network;
    };

}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
