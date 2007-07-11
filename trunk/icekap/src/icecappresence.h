/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef ICECAPPRESENCE_H
#define ICECAPPRESENCE_H

#include <qstring.h>
#include <qvaluelist.h>
#include <qobject.h>

// #include <ksharedptr.h>

namespace Icecap
{
    class Query;

    /**
     * Representation of a user on a network.
     * @todo AllenJB: Away status (message?)
     * @todo AllenJB: idle time
     * @todo AllenJB: How do we know when it's safe to delere a Presence?
     * @todo AllenJB: If we don't have information like the address for a user, request it
     */
    class Presence : public QObject
    {
        Q_OBJECT

        public:
            Presence (const QString& name);
            Presence (const QString& name, const QString& address);

            QString name () const { return m_name; }
            QString getNickname () const { return m_name; }
            QString loweredNickname () const { return m_name.lower (); }

            QString getRealName () const { return m_real_name; }

            QString address () const { return m_address; }
            QString getHostmask () const { return m_address; }

            bool connected () const { return m_connected; }

            uint getNickColor ();

            bool isAway () const { return m_away; }

            /**
             * @todo AllenJB: Is this ever used directly?
             */
            void setName (const QString& name);

            void setConnected (bool status);

            bool operator== (Presence compareTo);
//            bool isNull ();

            void update (QMap<QString, QString> parameterList);

            QString getBestAddresseeName();
            void tooltipTableData(QTextStream &tooltip) const;

        signals:
            void nickInfoChanged ();

        private:
            QString m_name;
            QString m_address;
            QString m_real_name;
            QString m_away_reason;
            uint m_irc_signon_time;
            uint m_login_time;
            uint m_idle_started;

            QString m_modes;

            bool m_connected;
            bool m_away;
            bool m_identified;
            uint m_nickColor;
    };

}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
