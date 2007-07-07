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

        m_window = m_mypresence->getViewContainer()->addQuery (this);
    }

    void Query::append (const QString& nickname, const QString& message)
    {
        m_window->append (nickname, message);
    }

    void Query::appendAction (const QString& nickname,const QString& message, bool usenotifications)
    {
        m_window->appendAction (nickname, message, usenotifications);
    }
}

#include "icecapquery.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
