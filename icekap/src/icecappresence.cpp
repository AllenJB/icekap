/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "icecappresence.h"

namespace Icecap
{

    Presence::Presence (const QString& newName)
    {
        name = newName;
        connected = false;
    }

    void Presence::setName (const QString& newName)
    {
        name = newName;
    }

    void Presence::setAddress (const QString& newAddress)
    {
        address = newAddress;
    }

    void Presence::setModes (const QString& newModes)
    {
        modes = newModes;
    }

    void Presence::setConnected (bool newStatus)
    {
        connected = newStatus;
    }

    bool Presence::operator== (Presence compareTo)
    {
        return (compareTo.getName() == name);
    }

    bool Presence::isNull ()
    {
        return (name.isNull());
    }

}

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
