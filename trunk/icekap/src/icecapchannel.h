/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef ICECAPCHANNEL_H
#define ICECAPCHANNEL_H

// #include <qptrlist.h>
#include <qstring.h>
#include <qdatetime.h>
#include <qvaluelist.h>
#include <qmap.h>

#include "icecappresence.h"

class ChannelWindow;
class ViewContainer;

namespace Icecap
{
    class MyPresence;

    class Channel
    {
        public:
            Channel (): name(0) {}
            Channel (MyPresence* p_mypresence, const QString& newName);
            Channel (MyPresence* p_mypresence, const QString& newName, const QMap<QString, QString>& parameterMap);
//            ~Channel ();

            QString getName () { return name; }
            QString getTopic () { return topic; }
            QString getTopicSetBy () { return topicSetBy; }
            QDateTime getTopicTimestamp () { return topicTimestamp; }
            QString getModes () { return modes; }
            bool getConnected () { return connected; }
            MyPresence* getMyPresence () { return mypresence; }

            void setName (const QString& newName);
            // In case we don't know who set the topic.
            // Suspect this may not ever be used, atleast for IRC.
            // TODO: A version that will take a standard formatted string and convert it to QDateTime
            void setTopic (const QString& newTopic, const QString& setBy, const QDateTime& timestamp);
            // Timestamp defaults to now
            void setTopic (const QString& newTopic, const QString& setBy);
            void setTopic (const QString& newTopic);
            void setConnected (bool newStatus);

            void presenceAdd (const Presence& user);
            void presenceRemove (const Presence& user);
            void presenceRemoveByName (const QString& userName);
            void presenceRemoveByAddress (const QString& userAddress);
            Presence presenceByName (const QString& userName);
            Presence presenceByAddress (const QString& userAddress);

            void setViewContainer(ViewContainer* newViewContainer) { m_viewContainerPtr = newViewContainer; }
//            ViewContainer* viewContainer () { return m_viewContainerPtr; }
            ViewContainer* getViewContainer() const;

            bool operator== (Channel compareTo);
            bool isNull ();

        private:
            void init ();

            QString name;
            QString topic;
            QString topicSetBy;
            QDateTime topicTimestamp;
            QString modes;
            bool connected;
            // TODO: This probably needs to be changed to a QPtrList
            QValueList<Presence> presenceList;

            MyPresence* mypresence;
            ViewContainer* m_viewContainerPtr;
            ChannelWindow* window;
            bool windowIsActive;
    };

}
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
