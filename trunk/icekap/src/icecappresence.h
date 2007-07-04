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

// TODO: Away status (message?)
// TODO: idle time
// TODO: How do we know when it's safe to delere a Presence?
// TODO: If we don't have information like the address for a user, request it
namespace Icecap
{

    class Presence : public QObject
    {
        Q_OBJECT

        public:
            Presence (): m_name(0) {}
            Presence (const QString& name);
            Presence (const QString& name, const QString& address);

            QString name () const { return m_name; }
            QString address () const { return m_address; }
            bool connected () const { return m_connected; }

            QString getRealName () const { return m_realName; }

            QString loweredNickname () const { return m_name.lower (); }

            bool isAway () const { return m_away; }

            void setName (const QString& name);
            void setAddress (const QString& address);
            void setConnected (bool status);

            bool operator== (Presence compareTo);
            bool isNull ();

            void update (QMap<QString, QString> parameterList);

        signals:
            void presenceChanged (const Presence* presence, QString field);

        private:
            QString m_name;
            QString m_address;
            QString m_modes;
            QString m_realName;
            bool m_connected;
            bool m_away;
    };

}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1: