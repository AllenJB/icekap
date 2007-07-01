/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2005 Peter Simonsson <psn@linux.se>
  Copyright (C) 2005 Eike Hein <sho@eikehein.com>
*/
#ifndef ICECAPMISC_H
#define ICECAPMISC_H

namespace Icecap
{
    typedef struct
    {
        QString tag;
        QString status;  // either +, > or - (success / finished, more-to-come or failure)
        QString command; // Holds event name if an event
        QMap<QString,QString> parameterList;
        // Use of parameters is discourages - use parameterList instead
        QString parameters;

        // The below values are only used for the IcecapServer::event slot
        QString sentCommand;
        QString network;
        QString mypresence;
        QString channel;
        QMap<QString, QString> sentParameterList;
        // Use of sentParameters is discouraged - use sentParameterList instead
        QString sentParameters;
    } Cmd;
}

#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
