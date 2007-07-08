/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef ICECAPQUERY_H
#define ICECAPQUERY_H

#include "qobject.h"

#include "icecapmisc.h"

class QueryWindow;
class ViewContainer;

namespace Icecap
{
    class Presence;
    class MyPresence;

    /**
     * Representation of a query / PM with another presence
     * @todo AllenJB: I think this no longer needs to be a QObject
     */
    class Query : public QObject
    {
        Q_OBJECT

        public:
            Query (MyPresence* mypresence, Presence* presence);

            Presence* presence () { return m_presence; }
            MyPresence* mypresence () { return m_mypresence; }

            void append (const QString& nickname, const QString& message);
            void appendAction (const QString& nickname,const QString& message, bool usenotifications = false);

        private:
            Presence* m_presence;
            MyPresence* m_mypresence;
            ViewContainer* m_viewContainerPtr;
            QueryWindow* m_window;
    };

}

#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
