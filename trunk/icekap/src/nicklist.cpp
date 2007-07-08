/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "nicklist.h"

#include <qregexp.h>

#include "icecapchannelpresence.h"

#include "preferences.h"

NickList::NickList() : QPtrList<Icecap::ChannelPresence>()
{
    m_compareMethod = NickList::AlphaNumeric;
}

void NickList::setCompareMethod(CompareMethod method)
{
    m_compareMethod = method;
}

int NickList::compareItems(QPtrCollection::Item item1, QPtrCollection::Item item2)
{
    if(m_compareMethod == NickList::TimeStamp) {
        int returnValue = static_cast<Icecap::ChannelPresence*>(item2)->timeStamp() - static_cast<Icecap::ChannelPresence*>(item1)->timeStamp();

        if(returnValue == 0) {
            returnValue = QString::compare(static_cast<Icecap::ChannelPresence*>(item1)->loweredNickname(),
                                           static_cast<Icecap::ChannelPresence*>(item2)->loweredNickname());
        }
        return returnValue;
    }

    return QString::compare(static_cast<Icecap::ChannelPresence*>(item1)->loweredNickname(),
                            static_cast<Icecap::ChannelPresence*>(item2)->loweredNickname());
}

QString NickList::completeNick(const QString& pattern, bool& complete, QStringList& found, bool skipNonAlfaNum, bool caseSensitive)
{
    found.clear();
    QString prefix = "^";
    QString newNick;
    QString prefixCharacter = Preferences::prefixCharacter();
    NickList foundNicks;
    foundNicks.setCompareMethod(NickList::TimeStamp);

    if((pattern.find(QRegExp("^(\\d|\\w)")) != -1) && skipNonAlfaNum)
    {
        prefix = "^([^\\d\\w]|[\\_]){0,}";
    }

    QRegExp regexp(prefix + QRegExp::escape(pattern));
    regexp.setCaseSensitive(caseSensitive);
    QPtrListIterator<Icecap::ChannelPresence> it(*this);

    while(it.current() != 0)
    {
        newNick = it.current()->getNickname();

        if(!prefix.isEmpty() && newNick.contains(prefixCharacter))
        {
            newNick = newNick.section( prefixCharacter,1 );
        }

        if(newNick.find(regexp) != -1)
        {
            foundNicks.append(it.current());
        }

        ++it;
    }

    foundNicks.sort();

    QPtrListIterator<Icecap::ChannelPresence> it2(foundNicks);
    while(it2.current() != 0)
    {
        found.append(it2.current()->getNickname());
        ++it2;
    }

    if(found.count() > 1)
    {
        bool ok = true;
        unsigned int patternLength = pattern.length();
        QString firstNick = found[0];
        unsigned int firstNickLength = firstNick.length();
        unsigned int foundCount = found.count();

        while(ok && ((patternLength) < firstNickLength))
        {
            ++patternLength;
            QStringList tmp = found.grep(firstNick.left(patternLength), caseSensitive);

            if(tmp.count() != foundCount)
            {
                ok = false;
                --patternLength;
            }
        }

        complete = false;
        return firstNick.left(patternLength);
    }
    else if(found.count() == 1)
    {
        complete = true;
        return found[0];
    }

    return QString();
}

bool NickList::containsNick(const QString& nickname)
{
    QPtrListIterator<Icecap::ChannelPresence> it(*this);
    while (it.current() != 0)
    {
        if (it.current()->getNickname()==nickname)
            return true;

        ++it;
    }
    return false;
}

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
