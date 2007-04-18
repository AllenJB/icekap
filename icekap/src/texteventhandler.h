/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef TEXTEVENTHANDLER_H
#define TEXTEVENTHANDLER_H

#include <qstring.h>
#include <qmap.h>

// #include "icecapserver.h"
class IcecapServer;

class TextEventHandler
{
    public:
        TextEventHandler (IcecapServer* server);
//        ~TextEventHandler ();

        void processEvent (const QString eventType, const QMap<QString, QString> parameter);

    private:
        IcecapServer* m_server;
};

#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
