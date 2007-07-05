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
        m_nickColor = 0;
    }

    Presence::Presence (const QString& name, const QString& address)
    {
        m_name = name;
        m_address = address;
        m_connected = false;
        m_away = false;
        m_realName = "Unimplemented";
        m_nickColor = 0;
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

    uint Presence::getNickColor()
    {
        // do we already have a color?
        if(!m_nickColor)
        {
            int nickvalue = 0;
            for (uint index = 0; index < m_name.length(); index++)
            {
                nickvalue += m_name[index].unicode();
            }
            m_nickColor = (nickvalue % 8) + 1;
        }
        // return color offset -1 (since we store it +1 for 0 checking)
        return m_nickColor-1;
    }

}

#include "icecappresence.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
