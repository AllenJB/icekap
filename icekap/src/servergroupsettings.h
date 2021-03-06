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
#ifndef KONVERSATIONSERVERGROUPSETTINGS_H
#define KONVERSATIONSERVERGROUPSETTINGS_H

#include <qvaluelist.h>
#include <qstringlist.h>

#include <ksharedptr.h>

#include "serversettings.h"
#include "identity.h"

namespace Konversation
{

    class ChannelSettings
    {
        public:
            ChannelSettings();
            ChannelSettings(const ChannelSettings& settings);
            explicit ChannelSettings(const QString& name);
            ChannelSettings(const QString& name, const QString& password);
            ChannelSettings(const QString& name, const QString& password, bool enableNotifications);
            ~ChannelSettings();

            void setName(const QString& name) { m_name = name; }
            QString name() const { return m_name; }

            void setPassword(const QString& password) { m_password = password; }
            QString password() const { return m_password; }

            void setNotificationsEnabled(bool enable) { m_enableNotifications = enable; }
            bool enableNotifications() const { return m_enableNotifications; }

        private:
            QString m_name;
            QString m_password;

            bool m_enableNotifications;
    };

    class ServerGroupSettings;
    typedef KSharedPtr<ServerGroupSettings> ServerGroupSettingsPtr;
    typedef QValueList<ServerGroupSettingsPtr> ServerGroupList;
    typedef QValueList<ServerSettings> ServerList;
    typedef QValueList<ChannelSettings> ChannelList;

    class ServerGroupSettings : public KShared
    {
        public:
            ServerGroupSettings();
            ServerGroupSettings(int id);
            ServerGroupSettings(const ServerGroupSettings& settings);
            ServerGroupSettings(const QString& name);
            ~ServerGroupSettings();

            void setName(const QString& name) { m_name = name; }
            QString name() const { return m_name; }

            void setServerList(const ServerList& list);
            void addServer(const ServerSettings& settings) { m_serverList.append(settings); }
            void removeServer(const ServerSettings settings);
            ServerList serverList(bool hideQuickServer=false) const;
            ServerSettings serverByIndex(unsigned int index) const;

            ServerList quickServerList() const { return m_quickServerList; }
            void setQuickServerList(const ServerSettings& settings) { m_quickServerList.append(settings); }
            void clearQuickServerList() { m_quickServerList.clear(); }

            void setIdentityId(int identityId) { m_identityId = identityId; }
            int identityId() const { return m_identityId; }
            IdentityPtr identity() const;

            void setChannelList(const ChannelList& list);
            void addChannel(const ChannelSettings& channel) { m_channelList.append(channel); }
            ChannelList channelList() const { return m_channelList; }
            ChannelSettings channelByIndex(unsigned int index) const;

            void setConnectCommands(const QString& commands) { m_connectCommands = commands; }
            QString connectCommands() const { return m_connectCommands; }

            void setAutoConnectEnabled(bool enabled) { m_autoConnect = enabled; }
            bool autoConnectEnabled() const { return m_autoConnect; }

            int id() const { return m_id; }

            void setSortIndex(int sortIndex) { m_sortIndex = sortIndex; }
            int sortIndex() const { return m_sortIndex; }

            void setChannelHistory(const ChannelList& list) { m_channelHistory = list; }
            void appendChannelHistory(const ChannelSettings& channel);
            ChannelList channelHistory() const { return m_channelHistory; }
            ChannelSettings channelByNameFromHistory(const QString& channelName);

            void setNotificationsEnabled(bool enable) { m_enableNotifications = enable; }
            bool enableNotifications() const { return m_enableNotifications; }

            void setExpanded(bool enable) { m_expanded = enable; }
            bool expanded() const { return m_expanded; }

        private:
            static int s_availableId;
            int m_sortIndex;
            QString m_name;
            ServerList m_serverList;
            ServerList m_quickServerList;
            int m_identityId;
            ChannelList m_channelList;
            ChannelList m_channelHistory;
            QString m_connectCommands;
            bool m_autoConnect;
            QString m_group;
            int m_id;
            bool m_enableNotifications;
            bool m_expanded;
    };

}
#endif
