/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef ICECAPCHANNEL_H
#define ICECAPCHANNEL_H

#include <qptrlist.h>
#include <qstring.h>
#include <qdatetime.h>
#include <qvaluelist.h>
#include <qmap.h>
#include <qobject.h>

#include "icecapchannelpresence.h"
#include "icecapmisc.h"

class ChannelWindow;
class ViewContainer;

namespace Icecap
{
    class MyPresence;

    class Channel : public QObject
    {
        Q_OBJECT

        public:
            Channel (): m_name(0) {}
            Channel (MyPresence* p_mypresence, const QString& name);
            Channel (MyPresence* p_mypresence, const QString& name, const QMap<QString, QString>& parameterMap);
//            ~Channel ();

            QString name () { return m_name; }
            QString getTopic () { return topic; }
            QString getTopicSetBy () { return topicSetBy; }
            QDateTime getTopicTimestamp () { return topicTimestamp; }
            QString getModes () { return modes; }
            bool getConnected () { return connected; }
            MyPresence* mypresence () { return m_mypresence; }

            void setName (const QString& name) { m_name = name; }
            // In case we don't know who set the topic.
            // Suspect this may not ever be used, atleast for IRC.
            // TODO: A version that will take a standard formatted string and convert it to QDateTime
//            void setTopic (const QString& newTopic, const QString& setBy, const QDateTime& timestamp);
            void setTopic (const QString& newTopic, const QString& setBy, const QString& timestampStr);
            // Timestamp defaults to now
//            void setTopic (const QString& newTopic, const QString& setBy);
//            void setTopic (const QString& newTopic);
            void setConnected (bool newStatus);

            void presenceAdd (const ChannelPresence* user);
            void presenceRemove (const ChannelPresence* user);
            void presenceRemoveByName (const QString& userName);
            void presenceRemoveByAddress (const QString& userAddress);
            ChannelPresence* presence (const QString& userName);
            ChannelPresence* presenceByAddress (const QString& userAddress);

            bool containsNick (const QString nickname);
            ChannelPresence* getNickByName (const QString& name) { return presence (name); }

            QStringList getSelectedNickList();


            void setViewContainer(ViewContainer* newViewContainer) { m_viewContainerPtr = newViewContainer; }
            ViewContainer* getViewContainer() const;

            void append(const QString& nickname,const QString& message);
            void appendAction(const QString& nickname,const QString& message, bool usenotifications = false);
            void appendCommandMessage (const QString& command, const QString& message, bool important = true,
                bool parseURL = true, bool self = false);

            bool operator== (Channel compareTo);
            bool isNull ();

        signals:
            void presenceJoined (ChannelPresence* presence);

        public slots:
            void eventFilter (Icecap::Cmd result);

        private:
            void init ();

            QString m_name;
            QString topic;
            QString topicSetBy;
            QDateTime topicTimestamp;
            QString modes;
            bool connected;
            QPtrList<ChannelPresence> presenceList;

            MyPresence* m_mypresence;
            ViewContainer* m_viewContainerPtr;
            ChannelWindow* window;
            bool windowIsActive;
    };

}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
