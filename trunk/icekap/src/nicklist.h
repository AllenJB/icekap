/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef NICKLIST_H
#define NICKLIST_H

#include <qptrlist.h>
#include <qstring.h>
#include <qstringlist.h>

namespace Icecap {
    class ChannelPresence;
}

class NickList : public QPtrList<Icecap::ChannelPresence>
{
    public:
        NickList();

        typedef enum CompareMethod { AlphaNumeric, TimeStamp };

        QString completeNick (const QString& pattern, bool& complete, QStringList& found, bool skipNonAlfaNum, bool caseSensitive);

        void setCompareMethod (CompareMethod method);

        bool containsNick (const QString& nickname);

    protected:
        virtual int compareItems (QPtrCollection::Item item1, QPtrCollection::Item item2);

    private:
        CompareMethod m_compareMethod;
};

#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
