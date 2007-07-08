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
#include "icecapchannelpresence.h"

#include <klistview.h>

#include "nicklistviewitem.h"

#include "icecapchannel.h"

namespace Icecap
{

    ChannelPresence::ChannelPresence (Channel* channel, Presence* presence)
    {
        QObject::setName (QString ("chap_"+ presence->name()).ascii());
        m_channel = channel;
        m_presence = presence;
        m_modes = "";
        m_listView = 0;
        m_owner = false;
        m_admin = false;
        m_op = false;
        m_halfop = false;
        m_voice = false;

        connect (m_presence, SIGNAL (nickInfoChanged()), this, SLOT (emitNickInfoChanged()));
    }

    ChannelPresence::~ChannelPresence ()
    {
        delete m_listViewItem;
    }

    /**
     * Set a given mode for this user
     * @param modes Mode to set
     * @todo AllenJB: Warning for unrecognised modes
     * @todo AllenJB: Get rid of m_modes (still used for operator== )
     * @todo AllenJB: Convert remaining modes to icecap equivalents
     */
    void ChannelPresence::setMode (const QString& modes)
    {
        m_modes = modes;

//        m_owner  = modes.contains ("q");
//        m_admin  = modes.contains ("a");
        m_op     = (modes == "op");
//        m_halfop = modes.contains ("h");
        m_voice  = (modes == "voice");

        emit channelNickChanged ();
    }

    /**
     * Change a given mode
     * @param add New mode state
     * @param mode Mode to change
     * @todo AllenJB: Add remaining icecap mode names
     */
    void ChannelPresence::modeChange (const bool add, const QString& mode)
    {
        if (mode == "voice") {
            m_voice = add;
        }
        else if (mode == "op") {
            m_op = add;
        }
        emit channelNickChanged();
    }

    /**
     * Is the NickListView entry for this user currently selected?
     * @return Selected?
     */
    bool ChannelPresence::isSelected () const
    {
        return m_listViewItem->isSelected ();
    }

    /**
     * Comparison operator
     * @param compareTo object to compare to
     * @return Same object?
     */
    bool ChannelPresence::operator== (ChannelPresence compareTo)
    {
        return ((m_channel == compareTo.m_channel) &&
            (m_presence == compareTo.m_presence));
    }

    /**
     * Set the ListView for this user and create a ListViewItem
     * @param listView List view
     */
    void ChannelPresence::setListView (KListView* listView) {
        if (m_listView != 0) {
            return;
        }

        m_listView = listView;
        m_listViewItem = new NickListViewItem (listView, listView->lastItem(), getNickname(), getHostmask(), this);
        connect (m_listViewItem, SIGNAL (refreshed()), listView, SLOT (startResortTimer()));
    }
}

#include "icecapchannelpresence.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
