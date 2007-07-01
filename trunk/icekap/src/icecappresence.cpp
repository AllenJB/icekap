/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "icecappresence.h"

namespace Icecap
{

    Presence::Presence (const QString& name)
    {
        m_name = name;
        m_connected = false;
        m_away = false;
        m_realName = "Unimplemented";
    }

    Presence::Presence (const QString& name, const QString& address)
    {
        m_name = name;
        m_address = address;
        m_connected = false;
        m_away = false;
        m_realName = "Unimplemented";
    }

    void Presence::setName (const QString& name)
    {
        m_name = name;
        emit presenceChanged (this, "name");
    }

    void Presence::setAddress (const QString& address)
    {
        m_address = address;
        emit presenceChanged (this, "address");
    }

    void Presence::setConnected (bool status)
    {
        m_connected = status;
        emit presenceChanged (this, "connected");
    }

    bool Presence::operator== (Presence compareTo)
    {
        return (compareTo.name() == m_name);
    }

    bool Presence::isNull ()
    {
        return (m_name.isNull());
    }

    // TODO: What other information can we get here? Modes? Away status?
    // TODO: idle_started
    void Presence::update (QMap<QString, QString> parameterList)
    {
        if (parameterList.contains ("address")) {
            setAddress (parameterList["address"]);
        }
    }

}

#include "icecappresence.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
