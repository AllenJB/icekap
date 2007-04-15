/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef ICECAPGATEWAY_H
#define ICECAPGATEWAY_H

#include <qstring.h>

namespace Icecap
{

    class Gateway
    {
        public:
            Gateway (): hostname(0) {}
            Gateway (const QString& host);
            Gateway (const QString& host, int portNo);
//            ~Gateway ();

            QString getPassword () { return password; }
            QString getHostname () { return hostname; }
            int getPort () { return port; }
            int getPriority () { return priority; }
            bool getConnected () { return connected; }

            void setHostname (const QString& host);
            void setPassword (const QString& pass);
            void setPort (int portNo);
            void setPriority (int pri);
            void setConnected (bool conn);

            bool operator== (Gateway compareTo);
            bool isNull ();

        private:
            QString hostname;
            int port;
            QString password;
            int priority;
            bool connected;
    };

}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
