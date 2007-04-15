/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/
#ifndef ICECAPSERVERSETTINGS_H
#define ICECAPSERVERSETTINGS_H

#include <qstring.h>

// TODO: IP < - > hostname stuff
namespace Icecap
{

    class IcecapServerSettings
    {
        public:
            IcecapServerSettings();
            IcecapServerSettings(const IcecapServerSettings& settings);
            explicit IcecapServerSettings(const QString& server);
//            ~IcecapServerSettings();

            void setName (const QString& name) { m_name = name; }
            QString name() const { return m_name; }

            void setServer(const QString& server) { m_server = server; }
            QString server() const { return m_server; }

            void setPort(int port) { m_port = port; }
            int port() const { return m_port;}

            void setPassword(const QString& password) { m_password = password; }
            QString password() const { return m_password; }

            void setSSLEnabled(bool enabled) { m_SSLEnabled = enabled; }
            bool SSLEnabled() const { return m_SSLEnabled; }

            bool operator== (const IcecapServerSettings& settings) const;

        private:
            QString m_name;
            QString m_server;
            int m_port;
            QString m_password;
            bool m_SSLEnabled;

    };

}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
