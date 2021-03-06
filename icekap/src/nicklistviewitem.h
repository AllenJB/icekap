/***************************************************************************
    begin                : Thu Jul 25 2002
    copyright            : (C) 2002 by Matthias Gierlings
    email                : gismore@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef NICKLISTVIEWITEM_H
#define NICKLISTVIEWITEM_H

#include <klistview.h>

namespace Icecap
{
    class ChannelPresence;
}

/*
  @author Matthias Gierlings
  @author Dario Abatianni (sorting code)
*/

class NickListViewItem : public QObject, public KListViewItem
{
    Q_OBJECT
        public:
        NickListViewItem(KListView* parent,
            QListViewItem *after,
            const QString &passed_label,
            const QString &passed_label2,
            Icecap::ChannelPresence *n);
        ~NickListViewItem();

        // returns a number thar represents the sorting order for the nicknames
        int getSortingValue() const;
        // get the Nick object
        Icecap::ChannelPresence *getNick();

        virtual void paintCell(QPainter * p, const QColorGroup & cg, int column, int width, int align);
        virtual int compare(QListViewItem* item,int col,bool ascending) const;

    public slots:
        void refresh();

        signals:
        void refreshed();

    protected:
        Icecap::ChannelPresence *nick;

        QString label;

        QString calculateLabel1();
        QString calculateLabel2();
        int m_height;
        int m_flags;
        bool m_away;
};
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
