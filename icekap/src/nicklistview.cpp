/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  This is the class that shows the channel nick list
  begin:     Fre Jun 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <kpopupmenu.h>
#include <klocale.h>
#include <kdebug.h>
#include <qtooltip.h>
#include <qtimer.h>
#include <kiconloader.h>
#include <qwhatsthis.h>
#include <qdragobject.h>

#include "images.h"

#include "konversationapplication.h"
#include "nicklistview.h"
#include "nicklistviewitem.h"

NickListView::NickListView(QWidget* parent, ChannelWindow *chan) :
KListView(parent)
{
    KListView::setSorting(-1);
    setWhatsThis();
    channel=chan;
    popup=new KPopupMenu(this,"nicklist_context_menu");
    modes=new KPopupMenu(this,"nicklist_modes_context_submenu");
    kickban=new KPopupMenu(this,"nicklist_kick_ban_context_submenu");
    setAcceptDrops(true);
    setDropHighlighter(true);
    setDropVisualizer(false);

    if (popup)
    {
        popup->insertItem(i18n("&Whois"),Konversation::Whois);
        popup->insertItem(i18n("&Version"),Konversation::Version);
        popup->insertItem(i18n("&Ping"),Konversation::Ping);

        popup->insertSeparator();

        if (modes)
        {
            modes->insertItem(i18n("Give Op"),Konversation::GiveOp);
            modes->insertItem(i18n("Take Op"),Konversation::TakeOp);
            modes->insertItem(i18n("Give HalfOp"),Konversation::GiveHalfOp);
            modes->insertItem(i18n("Take HalfOp"),Konversation::TakeHalfOp);
            modes->insertItem(i18n("Give Voice"),Konversation::GiveVoice);
            modes->insertItem(i18n("Take Voice"),Konversation::TakeVoice);
            popup->insertItem(i18n("Modes"),modes,Konversation::ModesSub);
        }

        if (kickban)
        {

            kickban->insertItem(i18n("Kick"),Konversation::Kick);
            kickban->insertItem(i18n("Kickban"),Konversation::KickBan);
            kickban->insertItem(i18n("Ban Nickname"),Konversation::BanNick);
            kickban->insertSeparator();
            kickban->insertItem(i18n("Ban *!*@*.host"),Konversation::BanHost);
            kickban->insertItem(i18n("Ban *!*@domain"),Konversation::BanDomain);
            kickban->insertItem(i18n("Ban *!user@*.host"),Konversation::BanUserHost);
            kickban->insertItem(i18n("Ban *!user@domain"),Konversation::BanUserDomain);
            kickban->insertSeparator();
            kickban->insertItem(i18n("Kickban *!*@*.host"),Konversation::KickBanHost);
            kickban->insertItem(i18n("Kickban *!*@domain"),Konversation::KickBanDomain);
            kickban->insertItem(i18n("Kickban *!user@*.host"),Konversation::KickBanUserHost);
            kickban->insertItem(i18n("Kickban *!user@domain"),Konversation::KickBanUserDomain);
            popup->insertItem(i18n("Kick / Ban"),kickban,Konversation::KickBanSub);
        }

        popup->insertItem(i18n("Ignore"), Konversation::IgnoreNick);
        popup->insertItem(i18n("Unignore"), Konversation::UnignoreNick);

        popup->insertSeparator();

        int newitem;
        newitem = popup->insertItem(i18n("Open &Query"),Konversation::OpenQuery);
        popup->setWhatsThis(newitem, "<qt>Start a private chat between you and this person.<p/><em>Technical note:</em><br>The conversation between you and this person will be sent via the server.  This means that the conversation will be affected by server lag, server stability, and will be terminated when you disconnect from the server.</qt>");
        newitem = popup->insertItem(i18n("Open DCC &Chat"),Konversation::StartDccChat);
        popup->setWhatsThis(newitem, "<qt>Start a private <em>D</em>irect <em>C</em>lient <em>C</em>onnection chat between you and this person.<p/><em>Technical note:</em><br />The conversation between you and this person will be sent directly.  This means it is independent from the server - so if the server connection fails, or use disconnect, your DCC Chat will be unaffected.  It also means that no irc server admin can view or spy on this chat.</qt>");

        if (kapp->authorize("allow_downloading"))
        {
            newitem = popup->insertItem(SmallIcon("2rightarrow"),i18n("Send &File..."),Konversation::DccSend);
            popup->setWhatsThis(newitem, "<qt>Send a file to this person.  If you are having problem sending files, or they are sending slowly, see the Konversation Handbook and DCC preferences page.</qt>");
        }
        popup->insertItem(SmallIconSet("mail_generic"),i18n("&Send Email..."), Konversation::SendEmail);

        popup->insertSeparator();

        popup->insertItem(i18n("Add to Watched Nicks"), Konversation::AddNotify);

        connect (popup, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
        connect (modes, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
        connect (kickban, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));

    }
    else
    {
        kdWarning() << "NickListView::NickListView(): Could not create popup!" << endl;
    }

    #if KDE_IS_VERSION(3,3,90)
    setShadeSortColumn(false);
    #endif

    // We have our own tooltips, don't use the default QListView ones
    setShowToolTips(false);
//    m_tooltip = new Konversation::KonversationNickListViewToolTip(viewport(), this);

    m_resortTimer = new QTimer(this);
    connect(m_resortTimer, SIGNAL(timeout()), SLOT(resort()));
}

NickListView::~NickListView()
{
}

void NickListView::setWhatsThis()
{
    Images* images = KonversationApplication::instance()->images();

    QMimeSourceFactory::defaultFactory()->setImage( "admin", images->getNickIcon( Images::Admin, false ).convertToImage() );
    QMimeSourceFactory::defaultFactory()->setImage( "owner", images->getNickIcon( Images::Owner, false ).convertToImage());
    QMimeSourceFactory::defaultFactory()->setImage( "op", images->getNickIcon( Images::Op, false ).convertToImage() );
    QMimeSourceFactory::defaultFactory()->setImage( "halfop", images->getNickIcon( Images::HalfOp, false ).convertToImage() );
    QMimeSourceFactory::defaultFactory()->setImage( "voice", images->getNickIcon( Images::Voice, false ).convertToImage() );
    QMimeSourceFactory::defaultFactory()->setImage( "normal", images->getNickIcon( Images::Normal, false ).convertToImage() );
    QMimeSourceFactory::defaultFactory()->setImage( "normalaway", images->getNickIcon( Images::Normal, true).convertToImage() );

    if(images->getNickIcon( Images::Normal, false).isNull())
    {
        QWhatsThis::add(this, i18n("<qt>This shows all the people in the channel.  The nick for each person is shown.<br>Usually an icon is shown showing the status of each person, but you do not seem to have any icon theme installed.  See the Konversation settings - <i>Configure Konversation</i> under the <i>Settings</i> menu.  Then view the page for <i>Themes</i> under <i>Appearence</i>.</qt>"));
    }
    else
    {

        QWhatsThis::add(this, i18n("<qt>This shows all the people in the channel.  The nick for each person is shown, with a picture showing their status.<p>"
            "<table>"

            "<tr><th><img src=\"admin\"></th><td>This person has administrator privileges.</td></tr>"
            "<tr><th><img src=\"owner\"></th><td>This person is a channel owner.</td></tr>"
            "<tr><th><img src=\"op\"></th><td>This person is a channel operator.</td></tr>"
            "<tr><th><img src=\"halfop\"></th><td>This person is a channel half-operator.</td></tr>"
            "<tr><th><img src=\"voice\"></th><td>This person has voice, and can therefore talk in a moderated channel.</td></tr>"
            "<tr><th><img src=\"normal\"></th><td>This person does not have any special privileges.</td></tr>"
            "<tr><th><img src=\"normalaway\"></th><td>This indicates that this person is currently away.</td></tr>"
            "</table><p>"
            "The meaning of admin, owner and halfop varies between different IRC servers.<p>"
            "Hovering over any nick shows their current status.  See the Icekap Handbook for more information."
            "</qt>"
            ));
    }

}

void NickListView::refresh()
{
    QPtrList<QListViewItem> nicklist;
    QListViewItemIterator it(this);

    while (it.current())
    {
        static_cast<NickListViewItem*>(it.current())->refresh();
        ++it;
    }
    setWhatsThis();
}

void NickListView::startResortTimer()
{
    if(!m_resortTimer->isActive())
        m_resortTimer->start(3000, true /*single shot*/);
}

void NickListView::resort()
{
    KListView::setSorting(m_column, m_ascending);
    sort();
    KListView::setSorting(-1);
}

void NickListView::contextMenuEvent(QContextMenuEvent* ce)
{
    ce->accept();

    if (selectedItems().count())
    {
        updateActions();
        popup->popup(ce->globalPos());
    }
}

void NickListView::updateActions()
{
    int ignoreCounter = 0;
    int unignoreCounter = 0;
    int notifyCounter = 0;

    int serverGroupId = 0;
/*
    // TODO: What does this code actually do?
    // TODO: Need to remove serverGroupId
    int serverGroupId = channel->getServer()->serverGroupSettings()->id();

    ChannelNickList nickList=channel->channel()->getSelectedChannelNicks();
    ChannelNickList::ConstIterator it;

    for (it = nickList.begin(); it != nickList.end(); ++it)
    {
        if (Preferences::isIgnored((*it)->getNickname()))
            ++unignoreCounter;
        else
            ++ignoreCounter;

        if (Preferences::isNotify(serverGroupId,(*it)->getNickname()))
            ++notifyCounter;
    }
*/

    if (ignoreCounter)
        popup->setItemVisible(Konversation::IgnoreNick, true);
    else
        popup->setItemVisible(Konversation::IgnoreNick, false);

    if (unignoreCounter)
        popup->setItemVisible(Konversation::UnignoreNick, true);
    else
        popup->setItemVisible(Konversation::UnignoreNick, false);

    if (notifyCounter || !Preferences::hasNotifyList(serverGroupId))
        popup->setItemEnabled(Konversation::AddNotify, false);
    else
        popup->setItemEnabled(Konversation::AddNotify, true);
}

void NickListView::setSorting(int column, bool ascending)
{
    m_column = column;
    m_ascending = ascending;
}

bool NickListView::acceptDrag (QDropEvent* event) const
{
    if (event->provides("text/uri-list"))
    {
        if (event->source())
        {
            QStrList uris;

            if (QUriDrag::decode(event,uris))
            {
                QString first = uris.first();

                if (first.startsWith("irc://") || channel->getChannel()->containsNick(first))
                    return false;
            }
            else
                return false;
        }

        return true;
    }
    else
        return false;
}

#include "nicklistview.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
