/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "icecapquery.h"

#include "icecapmypresence.h"
#include "icecappresence.h"

#include "querywindow.h"
#include "viewcontainer.h"

namespace Icecap
{

    Query::Query (MyPresence* mypresence, Presence* presence)
    {
        m_mypresence = mypresence;
        m_presence = presence;
        m_connected = m_presence->connected ();

        m_window = m_mypresence->getViewContainer()->addQuery (this);

        connect (m_presence, SIGNAL(nickInfoChanged()), this, SLOT(presenceChanged()));
    }

    /**
     * Append a presence message
     * @param nickname Source presence
     * @param message Message
     */
    void Query::append (const QString& nickname, const QString& message)
    {
        m_window->append (nickname, message);
    }

    /**
     * Append a presence action
     * @param nickname Source presence
     * @param message Message
     * @param usenotifications Use notifications?
     */
    void Query::appendAction (const QString& nickname,const QString& message, bool usenotifications)
    {
        m_window->appendAction (nickname, message, usenotifications);
    }

    void Query::presenceChanged ()
    {
        bool newState = m_presence->connected();
        if (newState != m_connected) {
            emit online (newState);
            m_connected = newState;
        }
    }

    // TODO AllenJB: Disable window if left open
    // TODO AllenJB: Output channel closed message if left open
    void Query::serverStateChanged (bool state)
    {
        m_connected = state;
        if (Preferences::closeInactiveTabs()) {
            if (state) {
                m_window = m_mypresence->getViewContainer()->addQuery (this);
            } else {
                m_mypresence->getViewContainer()->closeView (m_window);
                delete m_window;
            }
        }
    }

}

#include "icecapquery.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
