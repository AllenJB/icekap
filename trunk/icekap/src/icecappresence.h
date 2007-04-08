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

#include <ksharedptr.h>

namespace Icecap
{

    class Presence : public KShared
    {
        public:
            Presence (): name(0) {}
            Presence (const QString& newName);
//            ~Presence ();

            QString getName () { return name; }
            QString getAddress () { return address; }
            QString getModes () { return modes; }
            bool getConnected () { return connected; }

            void setName (const QString& newName);
            void setAddress (const QString& newAddress);
            void setModes (const QString& newModes);
            void setConnected (bool newStatus);

            bool operator== (Presence compareTo);
            bool isNull ();

        private:
            QString name;
            QString address;
            QString modes;
            bool connected;
    };

}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
