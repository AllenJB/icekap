/*
  This program is free software; you can redistribute it and/or modifydvancedModes
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/
#ifndef KONVERSATIONCHANNELOPTIONSDIALOG_H
#define KONVERSATIONCHANNELOPTIONSDIALOG_H

#include <kdialogbase.h>

#include <qstringlist.h>
#include "channelwindow.h"

namespace Konversation
{

    class ChannelOptionsUI;

    class ChannelOptionsDialog : public KDialogBase
    {
        Q_OBJECT
            public:
            explicit ChannelOptionsDialog(ChannelWindow *channel);
            ~ChannelOptionsDialog();

            QString topic();
            QStringList modes();

        public slots:
            void refreshTopicHistory();
            void refreshAllowedChannelModes();
            void refreshModes();
            void refreshEnableModes();
            void toggleAdvancedModes();

/*
            void refreshBanList();
            void addBan(const QString& newban);
            void addBanClicked();
            void removeBan(const QString& ban);
            void removeBanClicked();
            void banEdited(QListViewItem *edited);
*/

            void changeOptions();

        protected slots:
            void topicHistoryItemClicked(QListViewItem* item);
            void topicBeingEdited(bool state);

        protected:
            bool m_editingTopic;
            QListViewItem *m_NewBan;

        private:
            ChannelOptionsUI* m_widget;
            ChannelWindow *m_channel;
    };

    // This is needed to overcome two deficiencies in KListViewItem
    // First there is no signal emited when a rename is canceled
    // Second there is no way to get the old value of an item after a rename
    class BanListViewItem : public KListViewItem
    {
        public:
            BanListViewItem( QListView *parent );
            BanListViewItem( QListView *parent, bool isNew );
            BanListViewItem ( QListView *parent, const QString& label1, const QString& label2 = QString::null,
                const QString& label3 = QString::null);
            BanListViewItem ( QListView *parent, bool isNew, const QString& label1,
                const QString& label2 = QString::null, const QString& label3 = QString::null);
            virtual void startRename( int col );
            virtual QString getOldValue() { return m_oldValue; }

        protected:
            virtual void cancelRename ( int col );

            QString m_oldValue;
            bool m_isNewBan;
    };

}
#endif
