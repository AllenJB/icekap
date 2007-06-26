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
namespace Icecap
{

    class Presence : public QObject
    {
        Q_OBJECT

        public:
            Presence (): m_name(0) {}
            Presence (const QString& name);
            Presence (const QString& name, const QString& address);

            QString name () { return m_name; }
            QString address () { return m_address; }
            bool connected () { return m_connected; }

            void setName (const QString& name);
            void setAddress (const QString& address);
            void setConnected (bool status);

            bool operator== (Presence compareTo);
            bool isNull ();

        signals:
            void presenceChanged (const Presence* presence, QString field);

        private:
            QString m_name;
            QString m_address;
            QString m_modes;
            bool m_connected;
    };

}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
