/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2005 Peter Simonsson <psn@linux.se>
  Copyright (C) 2005 Eike Hein <sho@eikehein.com>
*/
#ifndef ICECAPCHANNELPRESENCE_H
#define ICECAPCHANNELPRESENCE_H

#include <qobject.h>

#include "icecappresence.h"

class KListView;

class NickListViewItem;

namespace Icecap
{

    class Channel;

    /**
     * Represents a given presence on a given channel.
     */
    class ChannelPresence : public QObject
    {
        Q_OBJECT

        public:
            ChannelPresence () : m_presence (0) {};
            ChannelPresence (Channel* channel, Presence* presence);
            ~ChannelPresence ();

            QString modes () { return m_modes; }
            void setMode (const QString& modes);
            void modeChange (const bool add, const QString& mode);

            Presence* presence () { return m_presence; }
            Channel* channel () { return m_channel; }

            // Shortcuts to the Presence
            QString name () { return m_presence->name(); }
            QString address () { return m_presence->address(); }
            QString getRealName () { return m_presence->getRealName (); }
            QString loweredNickname () { return m_presence->loweredNickname (); }

            bool operator== (ChannelPresence compareTo);
            bool isNull () { return (m_presence == 0); }

            bool isOp() const { return m_op; }
            bool isAdmin() const { return m_admin; }
            bool isOwner() const { return m_owner; }
            bool isHalfop() const { return m_halfop; }

            /** Return true if the may have any privillages at all
            * @return true if isOp() || isAdmin() || isOwner() || isHalfOp()
            */
            bool isAnyTypeOfOp() const { return (m_op || m_admin || m_owner || m_halfop); }
            bool hasVoice() const { return m_voice; }

            bool isSelected () const;

            uint timeStamp() const { return m_timestamp; }
            void setTimeStamp (uint stamp) { m_timestamp = stamp; }

            void setListView (KListView *listView);
            KListView* listView () { return m_listView; }

        // "shortcuts" to the Icecap::Presence methods
            QString getNickname () const { return m_presence->name (); }
            QString getHostmask () const { return m_presence->address (); }

            bool isAway () const { return m_presence->isAway (); }

            uint getNickColor () { return m_presence->getNickColor (); }

        signals:
            void channelNickChanged ();
            void nickInfoChanged ();

        public slots:
            void emitNickInfoChanged () { emit nickInfoChanged (); }

        private:
            Presence* m_presence;
            Channel* m_channel;
            QString m_modes;

            bool m_op;
            bool m_admin;
            bool m_owner;
            bool m_halfop;
            bool m_voice;

            // Used for nick completion by last action
            uint m_timestamp;

            KListView* m_listView;
            NickListViewItem* m_listViewItem;
    };

}

#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
