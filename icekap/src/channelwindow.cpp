/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
    The class that controls a channel
    begin:     Wed Jan 23 2002
    copyright: (C) 2002 by Dario Abatianni <eisfuchs@tigress.com>
               (C) 2004-2006 by Peter Simonsson <psn@linux.se>
               (C) 2005 by Ian Monroe <ian@monroe.nu>
*/

#include <qlabel.h>
#include <qvbox.h>
#include <qevent.h>
#include <qhbox.h>
#include <qgrid.h>
#include <qdragobject.h>
#include <qsizepolicy.h>
#include <qheader.h>
#include <qregexp.h>
#include <qtooltip.h>
#include <qsplitter.h>
#include <qcheckbox.h>
#include <qtimer.h>
#include <qcombobox.h>
#include <qtextcodec.h>
#include <qwhatsthis.h>
#include <qtoolbutton.h>
#include <qlayout.h>

#include <kprocess.h>

#include <klineedit.h>
#include <kinputdialog.h>
#include <kpassdlg.h>
#include <klocale.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kdeversion.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kwin.h>

#include "konversationapplication.h"
#include "channelwindow.h"
#include "icecapserver.h"
#include "nicklistview.h"
#include "nicklistviewitem.h"
#include "quickbutton.h"
#include "modebutton.h"
#include "ircinput.h"
#include "ircviewbox.h"
#include "ircview.h"
#include "common.h"
#include "topiclabel.h"
#include "channeloptionsdialog.h"
#include "notificationhandler.h"
#include "icecapchannel.h"
#include "icecapchannelpresence.h"
#include "nicklist.h"

ChannelWindow::ChannelWindow(QWidget* parent, Icecap::Channel* channel)
  : ChatWindow(parent), key(" ")
{
    // init variables
    m_channel = channel;
    m_processingTimer = 0;
    m_delayedSortTimer = 0;
    m_optionsDialog = NULL;
    m_currentIndex = 0;
    completionPosition = 0;
    nickChangeDialog = 0;
    channelCommand = false;

    quickButtonsChanged = false;
    quickButtonsState = false;

    modeButtonsChanged = false;
    modeButtonsState = false;

    awayChanged = false;
    awayState = false;

    splittersInitialized = false;
    topicSplitterHidden = false;
    channelSplitterHidden = false;

    // flag for first seen topic
    topicAuthorUnknown = true;

    setType(ChatWindow::Channel);

    setChannelEncodingSupported(true);

    // Build some size policies for the widgets
    QSizePolicy hfixed = QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QSizePolicy hmodest = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    QSizePolicy vmodest = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QSizePolicy vfixed = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QSizePolicy modest = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QSizePolicy greedy = QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    m_vertSplitter = new QSplitter(Qt::Vertical, this);
    m_vertSplitter->setOpaqueResize(KGlobalSettings::opaqueResize());


    QWidget* topicWidget = new QWidget(m_vertSplitter);
    m_vertSplitter->setResizeMode(topicWidget,QSplitter::KeepSize);

    QGridLayout* topicLayout = new QGridLayout(topicWidget, 2, 3, 0, 0);

    m_topicButton = new QToolButton(topicWidget);
    m_topicButton->setIconSet(SmallIconSet("edit", 16));
    QToolTip::add(m_topicButton, i18n("Edit Channel Settings"));
    connect(m_topicButton, SIGNAL(clicked()), this, SLOT(showOptionsDialog()));

    topicLine = new Konversation::TopicLabel(topicWidget);
    QWhatsThis::add(topicLine, i18n("<qt>Every channel on IRC has a topic associated with it.  This is simply a message that everybody can see.<p>If you are an operator, or the channel mode <em>'T'</em> has not been set, then you can change the topic by clicking the Edit Channel Properties button to the left of the topic.  You can also view the history of topics there.</qt>"));
    connect(topicLine, SIGNAL(setStatusBarTempText(const QString&)), this, SIGNAL(setStatusBarTempText(const QString&)));
    connect(topicLine, SIGNAL(clearStatusBarTempText()), this, SIGNAL(clearStatusBarTempText()));
    connect(topicLine,SIGNAL(popupCommand(int)),this,SLOT(popupChannelCommand(int)));

    topicLayout->addWidget(m_topicButton, 0, 0);
    topicLayout->addMultiCellWidget(topicLine, 0, 1, 1, 1);

    // The box holding the channel modes
    modeBox = new QHBox(topicWidget);
    modeBox->setSizePolicy(hfixed);
    modeT = new ModeButton("T",modeBox,0);
    modeN = new ModeButton("N",modeBox,1);
    modeS = new ModeButton("S",modeBox,2);
    modeI = new ModeButton("I",modeBox,3);
    modeP = new ModeButton("P",modeBox,4);
    modeM = new ModeButton("M",modeBox,5);
    modeK = new ModeButton("K",modeBox,6);
    modeL = new ModeButton("L",modeBox,7);

    QWhatsThis::add(modeT, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p>The <b>T</b>opic mode means that only the channel operator can change the topic for the channel.</qt>"));
    QWhatsThis::add(modeN, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p><b>N</b>o messages from outside means that users that are not in the channel cannot send messages that everybody in the channel can see.  Almost all channels have this set to prevent nuisance messages.</qt>"));
    QWhatsThis::add(modeS, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p>A <b>S</b>ecret channel will not show up in the channel list, nor will any user be able to see that you are in the channel with the <em>WHOIS</em> command or anything similar.  Only the people that are in the same channel will know that you are in this channel, if this mode is set.</qt>"));
    QWhatsThis::add(modeI, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p>An <b>I</b>nvite only channel means that people can only join the channel if they are invited.  To invite someone, a channel operator needs to issue the command <em>/invite nick</em> from within the channel.</qt>"));
    QWhatsThis::add(modeP, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p>A <b>P</b>rivate channel is shown in a listing of all channels, but the topic is not shown.  A user's <em>WHOIS</e> may or may not show them as being in a private channel depending on the IRC server.</qt>"));
    QWhatsThis::add(modeM, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p>A <b>M</b>oderated channel is one where only operators, half-operators and those with voice can talk.</qt>"));
    QWhatsThis::add(modeK, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p>A <b>P</b>rotected channel requires users to enter a password in order to join.</qt>"));
    QWhatsThis::add(modeL, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p>A channel that has a user <b>L</b>imit means that only that many users can be in the channel at any one time.  Some channels have a bot that sits in the channel and changes this automatically depending on how busy the channel is.</qt>"));

    connect(modeT,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
    connect(modeN,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
    connect(modeS,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
    connect(modeI,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
    connect(modeP,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
    connect(modeM,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
    connect(modeK,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
    connect(modeL,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));

    limit=new KLineEdit(modeBox);
    QToolTip::add(limit, i18n("Maximum users allowed in channel"));
    QWhatsThis::add(limit, i18n("<qt>This is the channel user limit - the maximum number of users that can be in the channel at a time.  If you are an operator, you can set this.  The channel mode <b>T</b>opic (button to left) will automatically be set if set this.</qt>"));
    connect(limit,SIGNAL (returnPressed()),this,SLOT (channelLimitChanged()) );
    connect(limit,SIGNAL (lostFocus()), this, SLOT(channelLimitChanged()) );

    topicLayout->addWidget(modeBox, 0, 2);
    topicLayout->setRowStretch(1, 10);
    topicLayout->setColStretch(1, 10);

    showTopic(Preferences::showTopic());
    showModeButtons(Preferences::showModeButtons());

    // (this) The main Box, holding the channel view/topic and the input line
    m_horizSplitter = new QSplitter(m_vertSplitter);
    m_horizSplitter->setOpaqueResize( KGlobalSettings::opaqueResize() );

    // Server will be set later in setServer()
    IRCViewBox* ircViewBox = new IRCViewBox(m_horizSplitter, NULL);
    setTextView(ircViewBox->ircView());
    connect(textView,SIGNAL(popupCommand(int)),this,SLOT(popupChannelCommand(int)));
    connect(topicLine, SIGNAL(currentChannelChanged(const QString&)),textView,SLOT(setCurrentChannel(const QString&)));

    // The box that holds the Nick List and the quick action buttons
    nickListButtons = new QVBox(m_horizSplitter);
    m_horizSplitter->setResizeMode(nickListButtons,QSplitter::KeepSize);
    nickListButtons->setSpacing(spacing());

    nicknameListView=new NickListView(nickListButtons, this);
    nicknameListView->setHScrollBarMode(QScrollView::AlwaysOff);
    nicknameListView->setSelectionModeExt(KListView::Extended);
    nicknameListView->setAllColumnsShowFocus(true);
    nicknameListView->setSorting(1,true);
    nicknameListView->addColumn(QString::null);
    nicknameListView->addColumn(QString::null);
    nicknameListView->setColumnWidthMode(1,KListView::Maximum);

    nicknameListView->header()->hide();

    // setResizeMode must be called after all the columns are added
    nicknameListView->setResizeMode(KListView::LastColumn);

    // separate LED from Text a little more
    nicknameListView->setColumnWidth(0, 10);
    nicknameListView->setColumnAlignment(0, Qt::AlignHCenter);

    nicknameListView->installEventFilter(this);

    // initialize buttons grid, will be set up in updateQuickButtons
    buttonsGrid=0;

    // The box holding the Nickname button and Channel input
    commandLineBox = new QHBox(this);
    commandLineBox->setSpacing(spacing());

    nicknameCombobox = new QComboBox(commandLineBox);
    nicknameCombobox->setEditable(true);
//    nicknameCombobox->insertStringList(Preferences::nicknameList());
    QWhatsThis::add(nicknameCombobox, i18n("<qt>This shows your current nick, and any alternatives you have set up.  If you select or type in a different nickname, then a request will be sent to the IRC server to change your nick.  If the server allows it, the new nickname will be selected.  If you type in a new nickname, you need to press 'Enter' at the end.<p>You can add change the alternative nicknames from the <em>Identities</em> option in the <em>File</em> menu.</qt>"));
    oldNick = nicknameCombobox->currentText();

    awayLabel = new QLabel(i18n("(away)"), commandLineBox);
    awayLabel->hide();
    channelInput = new IRCInput(commandLineBox);

    getTextView()->installEventFilter(channelInput);
    topicLine->installEventFilter(channelInput);
    channelInput->installEventFilter(this);

    // Set the widgets size policies
    m_topicButton->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    topicLine->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum));

    commandLineBox->setSizePolicy(vfixed);

    limit->setMaximumSize(40,100);
    limit->setSizePolicy(hfixed);

    modeT->setMaximumSize(20,100);
    modeT->setSizePolicy(hfixed);
    modeN->setMaximumSize(20,100);
    modeN->setSizePolicy(hfixed);
    modeS->setMaximumSize(20,100);
    modeS->setSizePolicy(hfixed);
    modeI->setMaximumSize(20,100);
    modeI->setSizePolicy(hfixed);
    modeP->setMaximumSize(20,100);
    modeP->setSizePolicy(hfixed);
    modeM->setMaximumSize(20,100);
    modeM->setSizePolicy(hfixed);
    modeK->setMaximumSize(20,100);
    modeK->setSizePolicy(hfixed);
    modeL->setMaximumSize(20,100);
    modeL->setSizePolicy(hfixed);

    getTextView()->setSizePolicy(greedy);
    nicknameListView->setSizePolicy(hmodest);
    // remember alternate background color
    abgCache=nicknameListView->alternateBackground();

    connect(channelInput,SIGNAL (submit()),this,SLOT (channelTextEntered()) );
    connect(channelInput,SIGNAL (envelopeCommand()),this,SLOT (channelPassthroughCommand()) );
    connect(channelInput,SIGNAL (nickCompletion()),this,SLOT (completeNick()) );
    connect(channelInput,SIGNAL (endCompletion()),this,SLOT (endCompleteNick()) );
    connect(channelInput,SIGNAL (textPasted(const QString&)),this,SLOT (textPasted(const QString&)) );

    connect(getTextView(), SIGNAL(textPasted(bool)), channelInput, SLOT(paste(bool)));
    connect(getTextView(),SIGNAL (gotFocus()),channelInput,SLOT (setFocus()) );
    connect(getTextView(), SIGNAL(updateTabNotification(Konversation::TabNotifyType)),
        this, SLOT(activateTabNotification(Konversation::TabNotifyType)));
    connect(getTextView(),SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
    connect(getTextView(),SIGNAL (autoText(const QString&)),this,SLOT (sendChannelText(const QString&)) );

    connect(nicknameListView,SIGNAL (popupCommand(int)),this,SLOT (popupCommand(int)) );
    connect(nicknameListView,SIGNAL (doubleClicked(QListViewItem*)),this,SLOT (doubleClickCommand(QListViewItem*)) );
    connect(nicknameCombobox,SIGNAL (activated(int)),this,SLOT(nicknameComboboxChanged()));

    if(nicknameCombobox->lineEdit())
        connect(nicknameCombobox->lineEdit(), SIGNAL (lostFocus()),this,SLOT(nicknameComboboxChanged()));

    setLog(Preferences::log());

    m_allowNotifications = true;
    m_channel = channel;
    setName (channel->name());
    setMyPresence (channel->mypresence());

    // Emitted whenever the channel user list changes (ie. users are added or removed)
    // Specificically used for updating the "n nicks, n ops" label
    connect (m_channel, SIGNAL (userListUpdated ()), this, SLOT (userListUpdated ()));
    updateAppearance();
}

void ChannelWindow::setMyPresence (Icecap::MyPresence* p_mypresence)
{
    ChatWindow::setMyPresence (p_mypresence);
    connect (p_mypresence, SIGNAL (nameChanged()), this, SLOT (nicknameChanged ()));
    setServer (m_mypresence->server());
    setNickname (m_mypresence->presence()->name());
}

void ChannelWindow::setServer(IcecapServer *server)
{
    ChatWindow::setServer(server);
    topicLine->setServer(server);
    refreshModeButtons();
}

ChannelWindow::~ChannelWindow()
{
    kdDebug() << "ChannelWindow::~ChannelWindow(" << getName() << ")" << endl;

    // Unlink this channel from channel list
    delete m_channel;
}

void ChannelWindow::nicknameChanged ()
{
    setNickname (m_mypresence->presence()->name());
}

void ChannelWindow::showOptionsDialog()
{
    if(!m_optionsDialog)
        m_optionsDialog = new Konversation::ChannelOptionsDialog(this);

    m_optionsDialog->refreshModes();
    m_optionsDialog->show();
}

void ChannelWindow::textPasted(const QString& text)
{
    if(m_server)
    {
        QStringList multiline=QStringList::split('\n',text);
        for(unsigned int index=0;index<multiline.count();index++)
        {
            QString line=multiline[index];
            QString cChar(Preferences::commandChar());
            // make sure that lines starting with command char get escaped
            if(line.startsWith(cChar)) line=cChar+line;
            sendChannelText(line);
        }
    }
}

// Will be connected to IRCView::popupCommand(int)
void ChannelWindow::popupChannelCommand(int id)
{
    channelCommand = true; // Context menu executed from ircview
    popupCommand(id);
    textView->clearContextNick();
    channelCommand = false;
}

// Will be connected to NickListView::popupCommand(int)
// TODO AllenJB: popupCommand: Move all this somewhere sensible (*Filter) or make command calls more generic
void ChannelWindow::popupCommand(int id)
{
    QString pattern;
    QString cc = Preferences::commandChar();
    QString args;
    QString question;
    bool raw=false;
    QStringList nickList = m_channel->getSelectedNickList();

    switch(id)
    {
        case Konversation::GiveOp:
            pattern="MODE %c +o %u";
            raw=true;
            break;
        case Konversation::TakeOp:
            pattern="MODE %c -o %u";
            raw=true;
            break;
        case Konversation::GiveHalfOp:
            pattern="MODE %c +h %u";
            raw=true;
            break;
        case Konversation::TakeHalfOp:
            pattern="MODE %c -h %u";
            raw=true;
            break;
        case Konversation::GiveVoice:
            pattern="MODE %c +v %u";
            raw=true;
            break;
        case Konversation::TakeVoice:
            pattern="MODE %c -v %u";
            raw=true;
            break;
        case Konversation::Version:
            pattern=cc+"CTCP %u VERSION";
            break;
        case Konversation::Whois:
            pattern="WHOIS %u %u";
            raw=true;
            break;
        case Konversation::Topic:
//            m_server->requestTopic(getTextView()->currentChannel());
            break;
        case Konversation::Names:
            m_server->queue("NAMES " + getTextView()->currentChannel());
            break;
        case Konversation::Join:
            m_server->queue("JOIN " + getTextView()->currentChannel());
            break;
        case Konversation::Ping:
        {
            unsigned int time_t = QDateTime::currentDateTime().toTime_t();
            pattern=QString(cc+"CTCP %u PING %1").arg(time_t);
        }
        break;
        case Konversation::Kick:
            pattern=cc+"KICK %u";
            break;
        case Konversation::KickBan:
            pattern=cc+"BAN %u\n"+
                cc+"KICK %u";
            break;
        case Konversation::BanNick:
            pattern=cc+"BAN %u";
            break;
        case Konversation::BanHost:
            pattern=cc+"BAN -HOST %u";
            break;
        case Konversation::BanDomain:
            pattern=cc+"BAN -DOMAIN %u";
            break;
        case Konversation::BanUserHost:
            pattern=cc+"BAN -USERHOST %u";
            break;
        case Konversation::BanUserDomain:
            pattern=cc+"BAN -USERDOMAIN %u";
            break;
        case Konversation::KickBanHost:
            pattern=cc+"KICKBAN -HOST %u";
            break;
        case Konversation::KickBanDomain:
            pattern=cc+"KICKBAN -DOMAIN %u";
            break;
        case Konversation::KickBanUserHost:
            pattern=cc+"KICKBAN -USERHOST %u";
            break;
        case Konversation::KickBanUserDomain:
            pattern=cc+"KICKBAN -USERDOMAIN %u";
            break;
        case Konversation::OpenQuery:
            pattern=cc+"QUERY %u";
            break;
        case Konversation::StartDccChat:
            pattern=cc+"DCC CHAT %u";
            break;
        case Konversation::DccSend:
            pattern=cc+"DCC SEND %u";
            break;
        case Konversation::IgnoreNick:
            if (nickList.size() == 1)
                question=i18n("Do you want to ignore %1?").arg(nickList.first());
            else
                question = i18n("Do you want to ignore the selected users?");
            if (KMessageBox::warningContinueCancel(this, question, i18n("Ignore"), i18n("Ignore"), "IgnoreNick") ==
                KMessageBox::Continue)
                pattern = cc+"IGNORE -ALL %l";
            break;
        case Konversation::UnignoreNick:
        {
            QStringList selectedIgnoredNicks;

            for (QStringList::Iterator it=nickList.begin(); it!=nickList.end(); ++it)
            {
                if (Preferences::isIgnored((*it)))
                    selectedIgnoredNicks.append((*it));
            }

            if (selectedIgnoredNicks.count() == 1)
                question=i18n("Do you want to stop ignoring %1?").arg(selectedIgnoredNicks.first());
            else
                question = i18n("Do you want to stop ignoring the selected users?");
            if (KMessageBox::warningContinueCancel(this, question, i18n("Unignore"), i18n("Unignore"), "UnignoreNick") ==
                KMessageBox::Continue)
            {
                sendChannelText(cc+"UNIGNORE "+selectedIgnoredNicks.join(" "));
            }
            break;
        }
        case Konversation::AddNotify:
            for (QStringList::Iterator it=nickList.begin(); it!=nickList.end(); ++it)
            {
/*
                if (!Preferences::isNotify(m_server->serverGroupSettings()->id(), (*it)))
                    Preferences::addNotify(m_server->serverGroupSettings()->id(), (*it));
*/
            }
            break;
    } // switch

    if (!pattern.isEmpty())
    {
        pattern.replace("%c",getName());

        QString command;

        if (pattern.contains("%l"))
        {
            QStringList list;

            for (QStringList::Iterator it=nickList.begin(); it!=nickList.end(); ++it)
                list.append((*it));

            command = pattern.replace("%l", list.join(" "));

            if (raw)
                m_server->queue(command);
            else
                sendChannelText(command);
        }
        else
        {
            QStringList patternList = QStringList::split('\n',pattern);

            for (QStringList::Iterator it=nickList.begin(); it!=nickList.end(); ++it)
            {
                for (unsigned int index = 0; index<patternList.count(); index++)
                {
                    command = patternList[index];
                    command.replace("%u", (*it));

                    if (raw)
                        m_server->queue(command);
                    else
                        sendChannelText(command);
                }
            }
        }
    }
}

// Will be connected to NickListView::doubleClicked()
void ChannelWindow::doubleClickCommand(QListViewItem* item)
{
    if(item)
    {
        nicknameListView->clearSelection();
        nicknameListView->setSelected(item, true);
        // TODO AllenJB: doubleClickCommand: put the quick button code in another function to make reusal more legitimate
        quickButtonClicked(Preferences::channelDoubleClickAction());
    }
}

void ChannelWindow::completeNick()
{
    int pos, oldPos;
    NickList nicknameList = m_channel->nickList ();

    channelInput->getCursorPosition(&oldPos,&pos);// oldPos is a dummy here, taking the paragraph parameter
    oldPos = channelInput->getOldCursorPosition();

    QString line=channelInput->text();
    QString newLine;
    // Check if completion position is out of range
    // completionPosition is the current position in the list
    if (completionPosition >= nicknameList.count ())
        completionPosition = 0;

    // Check, which completion mode is active
    char mode = channelInput->getCompletionMode();

    if(mode == 'c')
    {
        line.remove(oldPos, pos - oldPos);
        pos = oldPos;
    }

    // If the cursor is at beginning of line, insert last completion
    if(pos == 0 && !channelInput->lastCompletion().isEmpty())
    {
        QString addStart(Preferences::nickCompleteSuffixStart());
        newLine = channelInput->lastCompletion() + addStart;
        // New cursor position is behind nickname
        pos = newLine.length();
        // Add rest of the line
        newLine += line;
    }
    else
    {
        // remember old cursor position in input field
        channelInput->setOldCursorPosition(pos);
        // remember old cursor position locally
        oldPos = pos;
        // step back to last space or start of line
        while(pos && line[pos-1] != ' ') pos--;
        // copy search pattern (lowercase)
        QString pattern = line.mid(pos, oldPos - pos);
        // copy line to newLine-buffer
        newLine = line;

        // did we find any pattern?
        if(!pattern.isEmpty())
        {
            bool complete = false;
            QString foundNick;

            // try to find matching nickname in list of names
            if(Preferences::nickCompletionMode() == 1 ||
                Preferences::nickCompletionMode() == 2)
            { // Shell like completion
                QStringList found;
                foundNick = nicknameList.completeNick(pattern, complete, found,
                    (Preferences::nickCompletionMode() == 2), Preferences::nickCompletionCaseSensitive());

                if(!complete && !found.isEmpty())
                {
                    if(Preferences::nickCompletionMode() == 1)
                    {
                        QString nicksFound = found.join(" ");
                        appendServerMessage(i18n("Completion"), i18n("Possible completions: %1.").arg(nicksFound));
                    }
                    else
                    {
                        channelInput->showCompletionList(found);
                    }
                }
            } // Cycle completion
            else if(Preferences::nickCompletionMode() == 0)
            {
                if(mode == '\0') {
                    QPtrListIterator<Icecap::ChannelPresence> it(nicknameList);
                    uint timeStamp = 0;
                    int listPosition = 0;
                    Icecap::ChannelPresence* nick = 0;

                    while(it.current() != 0)
                    {
                        nick = it.current();

                        if(nick->getNickname().startsWith(pattern, Preferences::nickCompletionCaseSensitive()) &&
                          (nick->timeStamp() > timeStamp))
                        {
                            timeStamp = nick->timeStamp();
                            completionPosition = listPosition;
                        }

                        ++listPosition;
                        ++it;
                    }
                }

                // remember old nick completion position
                unsigned int oldCompletionPosition = completionPosition;
                complete = true;
                QString prefixCharacter = Preferences::prefixCharacter();

                do
                {
                    QString lookNick = nicknameList.at(completionPosition)->getNickname();

                    if(!prefixCharacter.isEmpty() && lookNick.contains(prefixCharacter))
                    {
                        lookNick = lookNick.section( prefixCharacter,1 );
                    }

                    if(lookNick.startsWith(pattern, Preferences::nickCompletionCaseSensitive()))
                    {
                        foundNick = lookNick;
                    }

                    // increment search position
                    completionPosition++;

                    // wrap around
                    if(completionPosition == nicknameList.count())
                    {
                        completionPosition = 0;
                    }

                    // the search ends when we either find a suitable nick or we end up at the
                    // first search position
                } while((completionPosition != oldCompletionPosition) && foundNick.isEmpty());
            }

            // did we find a suitable nick?
            if(!foundNick.isEmpty())
            {
                // set channel nicks completion mode
                channelInput->setCompletionMode('c');

                // remove pattern from line
                newLine.remove(pos, pattern.length());

                // did we find the nick in the middle of the line?
                if(pos && complete)
                {
                    channelInput->setLastCompletion(foundNick);
                    QString addMiddle = Preferences::nickCompleteSuffixMiddle();
                    newLine.insert(pos, foundNick + addMiddle);
                    pos = pos + foundNick.length() + addMiddle.length();
                }
                // no, it was at the beginning
                else if(complete)
                {
                    channelInput->setLastCompletion(foundNick);
                    QString addStart = Preferences::nickCompleteSuffixStart();
                    newLine.insert(pos, foundNick + addStart);
                    pos = pos + foundNick.length() + addStart.length();
                }
                // the nick wasn't complete
                else
                {
                    newLine.insert(pos, foundNick);
                    pos = pos + foundNick.length();
                }
            }
            // no pattern found, so restore old cursor position
            else pos = oldPos;
        }
    }

    // Set new text and cursor position
    channelInput->setText(newLine);
    channelInput->setCursorPosition(0, pos);
}

// make sure to step back one position when completion ends so the user starts
// with the last complete they made
void ChannelWindow::endCompleteNick()
{
    NickList nicknameList = m_channel->nickList ();
    if(completionPosition) completionPosition--;
    else completionPosition=nicknameList.count()-1;
}

void ChannelWindow::setName(const QString& newName)
{
    ChatWindow::setName(newName);
    setLogfileName(newName.lower());
}

void ChannelWindow::setKey(const QString& newKey)
{
    key=newKey;
}

QString ChannelWindow::getKey()
{
    return key;
}

void ChannelWindow::sendFileMenu()
{
    emit sendFile();
}

void ChannelWindow::channelTextEntered()
{
    QString line = channelInput->text();
    channelInput->clear();

    if(line.lower().stripWhiteSpace() == Preferences::commandChar()+"clear")
    {
        textView->clear();
    }
    else if(line.lower().stripWhiteSpace() == Preferences::commandChar()+"cycle")
    {
        cycleChannel();
    }
    else
    {
        if(!line.isEmpty())
            sendChannelText(line);
    }
}

void ChannelWindow::channelPassthroughCommand()
{
    QString commandChar = Preferences::commandChar();
    QString line = channelInput->text();

    channelInput->clear();

    if(!line.isEmpty())
    {
        // Prepend commandChar on Ctrl+Enter to bypass outputfilter command recognition
        if (line.startsWith(commandChar))
        {
            line = commandChar + line;
        }
        sendChannelText(line);
    }
}

void ChannelWindow::sendChannelText(const QString& sendLine)
{
    // create a work copy
    QString outputAll(sendLine);
    // replace aliases and wildcards
    if(m_server->getOutputFilter()->replaceAliases(outputAll))
    {
        outputAll = m_server->parseWildcards(outputAll,m_mypresence->name(), getName(), getKey(),
            m_channel->getSelectedNickList(), QString::null);
    }

    // Send all strings, one after another
    QStringList outList=QStringList::split('\n',outputAll);
    for(unsigned int index=0;index<outList.count();index++)
    {
        QString output(outList[index]);

        Icecap::OutputFilterResult result = m_server->getOutputFilter()->parse (m_mypresence->name(), output, m_mypresence->network()->name(), m_mypresence->name(), m_channel->name());

        // Is there something we need to display for ourselves?
        if(!result.output.isEmpty())
        {
            // TODO AllenJB: sendChannelText: Are all of these output types used?
            if(result.type == Icecap::Action) appendAction(m_mypresence->name(), result.output);
            else if(result.type == Icecap::Command) appendCommandMessage(result.typeString, result.output);
            else if(result.type == Icecap::Program) appendServerMessage(result.typeString, result.output);
            else if(result.type == Icecap::PrivateMessage) appendQuery(result.typeString, result.output);
            else append(m_mypresence->name(), result.output);
        }
        else if (result.outputList.count())
        {
            Q_ASSERT(result.type==Icecap::Message);
            for ( QStringList::Iterator it = result.outputList.begin(); it != result.outputList.end(); ++it )
            {
                append(m_mypresence->name(), *it);
            }
        }

        // TODO AllenJB: sendChannelText: Can we get rid of this result.toServer processing now we have IcecapServer->queueCommand()?
        // Send anything else to the server
        if (!result.toServer.isEmpty()) {
            m_server->queue(result.toServer);
        } else if (!result.toServerList.count()) {
            m_server->queueList(result.toServerList);
        }
    } // for
}

void ChannelWindow::setNickname(const QString& newNickname)
{
    nicknameCombobox->setCurrentText(newNickname);
}

/*
// TODO: Migrate to using Icecap::Channel
// Used by NickListView::updateActions
ChannelNickList ChannelWindow::getSelectedChannelNicks()
{
    ChannelNickList result;
    Icecap::ChannelPresence* nick=nicknameList.first();

    while(nick)
    {
        if(channelCommand)
        {
            if(nick->getNickname() == textView->getContextNick())
            {
                result.append(nick->getChannelNick());
                return result;
            }
        }
        else if(nick->isSelected())
            result.append(nick->getChannelNick());

        nick=nicknameList.next();
    }

    return result;

}
*/

void ChannelWindow::channelLimitChanged()
{
    unsigned int lim=limit->text().toUInt();

    modeButtonClicked(7,lim>0);
}

// TODO: Shift this somewhere sensible (*Filter) or provide a less protocol specific way of doing this
void ChannelWindow::modeButtonClicked(int id,bool on)
{
    char mode[]={'t','n','s','i','p','m','k','l'};
    QString command("MODE %1 %2%3 %4");
    QString args;

    if(mode[id]=='k')
    {
        //FIXME: Apparently we initialize the key as a space so so the
        // channel join code has something to work with when producing
        // things like "JOIN #chan1,#chan2,#chan§ key1, ,key3".
        if(getKey().isEmpty() || getKey() == " ")
        {
            QCString key;

            int result=KPasswordDialog::getPassword(key,i18n("Channel Password"));

            if(result==KPasswordDialog::Accepted && !key.isEmpty()) setKey(key);
        }
        args=getKey();
        if(!on) setKey(QString::null);
    }
    else if(mode[id]=='l')
    {
        if(limit->text().isEmpty() && on)
        {
            bool ok=false;
            // ask user how many nicks should be the limit
            args=KInputDialog::getText(i18n("Nick Limit"),
                i18n("Enter the new nick limit:"),
                limit->text(),                    // will be always "" but what the hell ;)
                &ok,
                this);
            // leave this function if user cancels
            if(!ok) return;
        }
        else if(on)
            args=limit->text();
    }
    // put together the mode command and send it to the server queue
    m_server->queue(command.arg(getName()).arg((on) ? "+" : "-").arg(mode[id]).arg(args));
}

void ChannelWindow::quickButtonClicked(const QString &buttonText)
{
    QString out = m_server->parseWildcards (buttonText, m_mypresence->presence()->getNickname(), m_channel->name(), getKey(), m_channel->getSelectedNickList(), QString::null);
//    QString out = "";

    // are there any newlines in the definition?
    if(out.find('\n')!=-1)
        sendChannelText(out);
    // single line without newline needs to be copied into input line
    else
        channelInput->setText(out);
}

void ChannelWindow::emitUpdateInfo()
{
    QString info = getName() + " - ";
    info += i18n("%n nick", "%n nicks", m_channel->numberOfNicks());
    info += i18n(" (%n op)", " (%n ops)", m_channel->numberOfOps());

    emit updateInfo(info);
}

void ChannelWindow::userListUpdated ()
{
    emitUpdateInfo ();
}

void ChannelWindow::setTopic(const QString &newTopic)
{
    appendCommandMessage(i18n("Topic"), i18n("The channel topic is \"%1\".").arg(newTopic));

    // cut off "nickname" and "time_t" portion of the topic before comparing, otherwise the history
    // list will fill up with the same entries while the user only requests the topic to be seen.
    if(m_topicHistory.first().section(' ', 2) != newTopic)
    {
        m_topicHistory.prepend(QString("%1 "+i18n("unknown")+" %2").arg(QDateTime::currentDateTime().toTime_t()).arg(newTopic));
        QString topic = Konversation::removeIrcMarkup(newTopic);
        topicLine->setText(topic);

        emit topicHistoryChanged();
    }
}

void ChannelWindow::setTopic(const QString &nickname, const QString &newTopic) // Overloaded
{
    m_topicHistory.prepend(QString("%1 %2 %3").arg(QDateTime::currentDateTime().toTime_t()).arg(nickname).arg(newTopic));
    QString topic = Konversation::removeIrcMarkup(newTopic);
    topicLine->setText(topic);

    emit topicHistoryChanged();
}

QStringList ChannelWindow::getTopicHistory()
{
    return m_topicHistory;
}

QString ChannelWindow::getTopic()
{
    return m_topicHistory[0];
}

void ChannelWindow::setTopicAuthor(const QString& newAuthor)
{
    if(topicAuthorUnknown)
    {
        m_topicHistory[0] = m_topicHistory[0].section(' ', 0, 0) + ' ' + newAuthor + ' ' + m_topicHistory[0].section(' ', 2);
        topicAuthorUnknown = false;
    }
}

// TODO: Re-implement commented code
void ChannelWindow::updateMode(const QString& sourceNick, char mode, bool plus, const QString &parameter)
{
    //Note for future expansion: doing m_server->getChannelNick(getName(), sourceNick);  may not return a valid channelNickPtr if the
    //mode is updated by the server.
/*
    QString message(QString::null);
    Icecap::ChannelPresence parameterChannelNick=m_server->getChannelNick(getName(), parameter);

    bool fromMe=false;
    bool toMe=false;

    // remember if this nick had any type of op.
    bool wasAnyOp=false;
    if(parameterChannelNick)
        wasAnyOp=parameterChannelNick->isAnyTypeOfOp();

    if(sourceNick.lower()==m_server->loweredNickname())
        fromMe=true;
    if(parameter.lower()==m_server->loweredNickname())
        toMe=true;

    switch(mode)
    {
        case 'q':
            if(plus)
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You give channel owner privileges to yourself.");
                    else
                        message=i18n("You give channel owner privileges to %1.").arg(parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 gives channel owner privileges to you.").arg(sourceNick);
                    else
                        message=i18n("%1 gives channel owner privileges to %2.").arg(sourceNick).arg(parameter);
                }
            }
            else
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You take channel owner privileges from yourself.");
                    else
                        message=i18n("You take channel owner privileges from %1.").arg(parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 takes channel owner privileges from you.").arg(sourceNick);
                    else
                        message=i18n("%1 takes channel owner privileges from %2.").arg(sourceNick).arg(parameter);
                }
            }
            if(parameterChannelNick)
            {
                parameterChannelNick->setOwner(plus);
                emitUpdateInfo();
                nicknameListView->sort();
            }
            break;

        case 'a':
            if(plus)
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You give channel admin privileges to yourself.");
                    else
                        message=i18n("You give channel admin privileges to %1.").arg(parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 gives channel admin privileges to you.").arg(sourceNick);
                    else
                        message=i18n("%1 gives channel admin privileges to %2.").arg(sourceNick).arg(parameter);
                }
            }
            else
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You take channel admin privileges from yourself.");
                    else
                        message=i18n("You take channel admin privileges from %1.").arg(parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 takes channel admin privileges from you.").arg(sourceNick);
                    else
                        message=i18n("%1 takes channel admin privileges from %2.").arg(sourceNick).arg(parameter);
                }
            }
            if(parameterChannelNick)
            {
                parameterChannelNick->setOwner(plus);
                emitUpdateInfo();
                nicknameListView->sort();
            }
            break;

        case 'o':
            if(plus)
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You give channel operator privileges to yourself.");
                    else
                        message=i18n("You give channel operator privileges to %1.").arg(parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 gives channel operator privileges to you.").arg(sourceNick);
                    else
                        message=i18n("%1 gives channel operator privileges to %2.").arg(sourceNick).arg(parameter);
                }
            }
            else
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You take channel operator privileges from yourself.");
                    else
                        message=i18n("You take channel operator privileges from %1.").arg(parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 takes channel operator privileges from you.").arg(sourceNick);
                    else
                        message=i18n("%1 takes channel operator privileges from %2.").arg(sourceNick).arg(parameter);
                }
            }
            if(parameterChannelNick)
            {
                parameterChannelNick->setOp(plus);
                emitUpdateInfo();
                nicknameListView->sort();
            }
            break;

        case 'h':
            if(plus)
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You give channel halfop privileges to yourself.");
                    else
                        message=i18n("You give channel halfop privileges to %1.").arg(parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 gives channel halfop privileges to you.").arg(sourceNick);
                    else
                        message=i18n("%1 gives channel halfop privileges to %2.").arg(sourceNick).arg(parameter);
                }
            }
            else
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You take channel halfop privileges from yourself.");
                    else
                        message=i18n("You take channel halfop privileges from %1.").arg(parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 takes channel halfop privileges from you.").arg(sourceNick);
                    else
                        message=i18n("%1 takes channel halfop privileges from %2.").arg(sourceNick).arg(parameter);
                }
            }
            if(parameterChannelNick)
            {
                parameterChannelNick->setHalfOp(plus);
                emitUpdateInfo();
                nicknameListView->sort();
            }
            break;

	//case 'O': break;

        case 'v':
            if(plus)
            {
                if(fromMe)
                {
                    if(toMe) message=i18n("You give yourself the permission to talk.");
                    else     message=i18n("You give %1 the permission to talk.").arg(parameter);
                }
                else
                {
                    if(toMe) message=i18n("%1 gives you the permission to talk.").arg(sourceNick);
                    else     message=i18n("%1 gives %2 the permission to talk.").arg(sourceNick).arg(parameter);
                }
            }
            else
            {
                if(fromMe)
                {
                    if(toMe) message=i18n("You take the permission to talk from yourself.");
                    else     message=i18n("You take the permission to talk from %1.").arg(parameter);
                }
                else
                {
                    if(toMe) message=i18n("%1 takes the permission to talk from you.").arg(sourceNick);
                    else     message=i18n("%1 takes the permission to talk from %2.").arg(sourceNick).arg(parameter);
                }
            }
            if(parameterChannelNick)
            {
                parameterChannelNick->setVoice(plus);
                nicknameListView->sort();
            }
            break;

        case 'c':
            if(plus)
            {
                if(fromMe) message=i18n("You set the channel mode to 'no colors allowed'.");
                else message=i18n("%1 sets the channel mode to 'no colors allowed'.").arg(sourceNick);
            }
            else
            {
                if(fromMe) message=i18n("You set the channel mode to 'allow color codes'.");
                else message=i18n("%1 sets the channel mode to 'allow color codes'.").arg(sourceNick);
            }
            modeM->setDown(plus);
            break;

        case 'i':
            if(plus)
            {
                if(fromMe) message=i18n("You set the channel mode to 'invite only'.");
                else message=i18n("%1 sets the channel mode to 'invite only'.").arg(sourceNick);
            }
            else
            {
                if(fromMe) message=i18n("You remove the 'invite only' mode from the channel.");
                else message=i18n("%1 removes the 'invite only' mode from the channel.").arg(sourceNick);
            }
            modeI->setDown(plus);
            break;

        case 'm':
            if(plus)
            {
                if(fromMe) message=i18n("You set the channel mode to 'moderated'.");
                else message=i18n("%1 sets the channel mode to 'moderated'.").arg(sourceNick);
            }
            else
            {
                if(fromMe) message=i18n("You set the channel mode to 'unmoderated'.");
                else message=i18n("%1 sets the channel mode to 'unmoderated'.").arg(sourceNick);
            }
            modeM->setDown(plus);
            break;

        case 'n':
            if(plus)
            {
                if(fromMe) message=i18n("You set the channel mode to 'no messages from outside'.");
                else message=i18n("%1 sets the channel mode to 'no messages from outside'.").arg(sourceNick);
            }
            else
            {
                if(fromMe) message=i18n("You set the channel mode to 'allow messages from outside'.");
                else message=i18n("%1 sets the channel mode to 'allow messages from outside'.").arg(sourceNick);
            }
            modeN->setDown(plus);
            break;

        case 'p':
            if(plus)
            {
                if(fromMe) message=i18n("You set the channel mode to 'private'.");
                else message=i18n("%1 sets the channel mode to 'private'.").arg(sourceNick);
            }
            else
            {
                if(fromMe) message=i18n("You set the channel mode to 'public'.");
                else message=i18n("%1 sets the channel mode to 'public'.").arg(sourceNick);
            }
            modeP->setDown(plus);
            if(plus) modeS->setDown(false);
            break;

        case 's':
            if(plus)
            {
                if(fromMe) message=i18n("You set the channel mode to 'secret'.");
                else message=i18n("%1 sets the channel mode to 'secret'.").arg(sourceNick);
            }
            else
            {
                if(fromMe) message=i18n("You set the channel mode to 'visible'.");
                else message=i18n("%1 sets the channel mode to 'visible'.").arg(sourceNick);
            }
            modeS->setDown(plus);
            if(plus) modeP->setDown(false);
            break;

	//case 'r': break;

        case 't':
            if(plus)
            {
                if(fromMe) message=i18n("You switch on 'topic protection'.");
                else message=i18n("%1 switches on 'topic protection'.").arg(sourceNick);
            }
            else
            {
                if(fromMe) message=i18n("You switch off 'topic protection'.");
                else message=i18n("%1 switches off 'topic protection'.").arg(sourceNick);
            }
            modeT->setDown(plus);
            break;

        //case 'k': break;

        case 'l':
            if(plus)
            {
                if(fromMe) message=i18n("You set the channel limit to %1 nicks.").arg(parameter);
                else message=i18n("%1 sets the channel limit to %2 nicks.").arg(sourceNick).arg(parameter);
            }
            else
            {
                if(fromMe) message=i18n("You remove the channel limit.");
                else message=i18n("%1 removes the channel limit.").arg(sourceNick);
            }
            modeL->setDown(plus);
            if(plus) limit->setText(parameter);
            else limit->clear();
            break;

        case 'b':
            if(plus)
            {
                if(fromMe) message=i18n("You set a ban on %1.").arg(parameter);
                else message=i18n("%1 sets a ban on %2.").arg(sourceNick).arg(parameter);
            }
            else
            {
                if(fromMe) message=i18n("You remove the ban on %1.").arg(parameter);
                else message=i18n("%1 removes the ban on %2.").arg(sourceNick).arg(parameter);
            }
            break;

        case 'e':
            if(plus)
            {
                if(fromMe) message=i18n("You set a ban exception on %1.").arg(parameter);
                else message=i18n("%1 sets a ban exception on %2.").arg(sourceNick).arg(parameter);
            }
            else
            {
                if(fromMe) message=i18n("You remove the ban exception on %1.").arg(parameter);
                else message=i18n("%1 removes the ban exception on %2.").arg(sourceNick).arg(parameter);
            }
            break;

        case 'I':
            if(plus)
            {
                if(fromMe) message=i18n("You set invitation mask %1.").arg(parameter);
                else message=i18n("%1 sets invitation mask %2.").arg(sourceNick).arg(parameter);
            }
            else
            {
                if(fromMe) message=i18n("You remove the invitation mask %1.").arg(parameter);
                else message=i18n("%1 removes the invitation mask %2.").arg(sourceNick).arg(parameter);
            }
            break;
        default:
	    if(plus)
	    {
	        if(fromMe) message=i18n("You set channel mode +%1").arg(mode);
		else message=i18n("%1 sets channel mode +%2").arg(sourceNick).arg(mode);
	    }
	    else
	    {
	        if (fromMe) message=i18n("You set channel mode -%1").arg(mode);
		else message= i18n("%1 sets channel mode -%2").arg(sourceNick).arg(mode);
	    }
    }

    // check if this nick's anyOp-status has changed and adjust ops accordingly
    if(parameterChannelNick)
    {
        if(wasAnyOp && (!parameterChannelNick->isAnyTypeOfOp()))
            adjustOps(-1);
        else if((!wasAnyOp) && parameterChannelNick->isAnyTypeOfOp())
            adjustOps(1);
    }

    if(!message.isEmpty() && !Preferences::useLiteralModes())
    {
        appendCommandMessage(i18n("Mode"),message);
    }

    updateModeWidgets(mode,plus,parameter);
*/
}

void ChannelWindow::clearModeList()
{
    m_modeList.clear();
    modeT->setOn(0);
    modeT->setDown(0);

    modeN->setOn(0);
    modeN->setDown(0);

    modeS->setOn(0);
    modeS->setDown(0);

    modeI->setOn(0);
    modeI->setDown(0);

    modeP->setOn(0);
    modeP->setDown(0);

    modeM->setOn(0);
    modeM->setDown(0);

    modeK->setOn(0);
    modeK->setDown(0);

    modeL->setOn(0);
    modeL->setDown(0);
    limit->clear();
    emit modesChanged();
}

void ChannelWindow::updateModeWidgets(char mode, bool plus, const QString &parameter)
{
    ModeButton* widget=0;

    if(mode=='t') widget=modeT;
    else if(mode=='n') widget=modeN;
    else if(mode=='s') widget=modeS;
    else if(mode=='i') widget=modeI;
    else if(mode=='p') widget=modeP;
    else if(mode=='m') widget=modeM;
    else if(mode=='k') widget=modeK;
    else if(mode=='l')
    {
        widget=modeL;
        if(plus) limit->setText(parameter);
        else limit->clear();
    }

    if(widget) widget->setOn(plus);

    if(plus)
    {
        m_modeList.append(QString(mode + parameter));
    }
    else
    {
        QStringList removable = m_modeList.grep(QRegExp(QString("^%1.*").arg(mode)));

        for(QStringList::iterator it = removable.begin(); it != removable.end(); ++it)
        {
            m_modeList.remove(m_modeList.find((*it)));
        }
    }
    emit modesChanged();
}

void ChannelWindow::updateQuickButtons(const QStringList &newButtonList)
{
    // remove quick buttons from memory and GUI
    while(buttonList.count())
    {
      QuickButton* button=buttonList.at(0);
      // remove tool tips as well
      QToolTip::remove(button);
      buttonList.remove(button);
      delete button;
    }

    if(buttonsGrid)delete buttonsGrid;

    // the grid that holds the quick action buttons
    buttonsGrid = new QGrid(2, nickListButtons);

    // add new quick buttons
    for(unsigned int index=0;index<newButtonList.count();index++)
    {
        // generate empty buttons first, text will be added later
        QuickButton* quickButton = new QuickButton(QString::null, QString::null, buttonsGrid);
        buttonList.append(quickButton);

        connect(quickButton, SIGNAL(clicked(const QString &)), this, SLOT(quickButtonClicked(const QString &)));

        // Get the button definition
        QString buttonText=newButtonList[index];
        // Extract button label
        QString buttonLabel=buttonText.section(',',0,0);
        // Extract button definition
        buttonText=buttonText.section(',',1);

        quickButton->setText(buttonLabel);
        quickButton->setDefinition(buttonText);

        // Add tool tips
        QString toolTip=buttonText.replace("&","&amp;").
            replace("<","&lt;").
            replace(">","&gt;");

        QToolTip::add(quickButton,toolTip);

        quickButton->show();
    } // for

    // set hide() or show() on grid
    showQuickButtons(Preferences::showQuickButtons());
}

void ChannelWindow::showQuickButtons(bool show)
{
    // Qt does not redraw the buttons properly when they are not on screen
    // while getting hidden, so we remember the "soon to be" state here.
    if(isHidden() || !buttonsGrid)
    {
        quickButtonsChanged=true;
        quickButtonsState=show;
    }
    else
    {
        if(show)
            buttonsGrid->show();
        else
            buttonsGrid->hide();
    }
}

void ChannelWindow::showModeButtons(bool show)
{
    // Qt does not redraw the buttons properly when they are not on screen
    // while getting hidden, so we remember the "soon to be" state here.
    if(isHidden())
    {
        modeButtonsChanged=true;
        modeButtonsState=show;
    }
    else
    {
        if(show)
        {
            topicSplitterHidden = false;
            modeBox->show();
            modeBox->parentWidget()->show();
        }
        else
        {
            modeBox->hide();

            if(topicLine->isHidden())
            {
                topicSplitterHidden = true;
                modeBox->parentWidget()->hide();
            }
        }
    }
}

void ChannelWindow::indicateAway(bool show)
{
    // Qt does not redraw the label properly when they are not on screen
    // while getting hidden, so we remember the "soon to be" state here.
    if(isHidden())
    {
        awayChanged=true;
        awayState=show;
    }
    else
    {
        if(show)
            awayLabel->show();
        else
            awayLabel->hide();
    }
}

void ChannelWindow::showEvent(QShowEvent*)
{
    // If the show quick/mode button settings have changed, apply the changes now
    if(quickButtonsChanged)
    {
        quickButtonsChanged=false;
        showQuickButtons(quickButtonsState);
    }

    if(modeButtonsChanged)
    {
        modeButtonsChanged=false;
        showModeButtons(modeButtonsState);
    }

    if(awayChanged)
    {
        awayChanged=false;
        indicateAway(awayState);
    }

    syncSplitters();
}

void ChannelWindow::syncSplitters()
{
    QValueList<int> vertSizes = Preferences::topicSplitterSizes();
    QValueList<int> horizSizes = Preferences::channelSplitterSizes();

    if (vertSizes.isEmpty())
    {
        vertSizes << m_topicButton->height() << (height() - m_topicButton->height());
        Preferences::setTopicSplitterSizes(vertSizes);
    }

    if (horizSizes.isEmpty())
    {
        int listWidth = nicknameListView->columnWidth(0) + nicknameListView->columnWidth(1);
        horizSizes << (width() - listWidth) << listWidth;
        Preferences::setChannelSplitterSizes(horizSizes);
    }

    m_vertSplitter->setSizes(vertSizes);
    m_horizSplitter->setSizes(horizSizes);

    splittersInitialized = true;
}

void ChannelWindow::updateAppearance()
{
    QColor fg,bg,abg;

    if(Preferences::inputFieldsBackgroundColor())
    {
        fg=Preferences::color(Preferences::ChannelMessage);
        bg=Preferences::color(Preferences::TextViewBackground);
        abg=Preferences::color(Preferences::AlternateBackground);
    }
    else
    {
        fg=colorGroup().foreground();
        bg=colorGroup().base();
        // get alternate background color from cache
        abg=abgCache;
    }

    channelInput->unsetPalette();
    channelInput->setPaletteForegroundColor(fg);
    channelInput->setPaletteBackgroundColor(bg);

    limit->unsetPalette();
    limit->setPaletteForegroundColor(fg);
    limit->setPaletteBackgroundColor(bg);

    getTextView()->unsetPalette();

    if(Preferences::showBackgroundImage())
    {
        getTextView()->setViewBackground(Preferences::color(Preferences::TextViewBackground),
            Preferences::backgroundImage());
    }
    else
    {
        getTextView()->setViewBackground(Preferences::color(Preferences::TextViewBackground),
            QString::null);
    }

    if (Preferences::customTextFont())
    {
        getTextView()->setFont(Preferences::textFont());
        topicLine->setFont(Preferences::textFont());
        channelInput->setFont(Preferences::textFont());
        nicknameCombobox->setFont(Preferences::textFont());
        limit->setFont(Preferences::textFont());
    }
    else
    {
        getTextView()->setFont(KGlobalSettings::generalFont());
        topicLine->setFont(KGlobalSettings::generalFont());
        channelInput->setFont(KGlobalSettings::generalFont());
        nicknameCombobox->setFont(KGlobalSettings::generalFont());
        limit->setFont(KGlobalSettings::generalFont());
    }

    nicknameListView->resort();
    nicknameListView->unsetPalette();
    nicknameListView->setPaletteForegroundColor(fg);
    nicknameListView->setPaletteBackgroundColor(bg);
    nicknameListView->setAlternateBackground(abg);

    if (Preferences::customListFont())
        nicknameListView->setFont(Preferences::listFont());
    else
        nicknameListView->setFont(KGlobalSettings::generalFont());

    nicknameListView->refresh();

    showModeButtons(Preferences::showModeButtons());
    showNicknameList(Preferences::showNickList());
    showNicknameBox(Preferences::showNicknameBox());
    showTopic(Preferences::showTopic());

    updateQuickButtons(Preferences::quickButtonList());

    ChatWindow::updateAppearance();
}

void ChannelWindow::updateStyleSheet()
{
    getTextView()->updateStyleSheet();
}

/**
 * Called when the nickname combo box is changed
 * @todo AllenJB: Move nick combo up to ChatWindow and also use for MyPresence status windows?
 */
void ChannelWindow::nicknameComboboxChanged()
{
    QString newNick=nicknameCombobox->currentText();
    oldNick = m_mypresence->presence()->getNickname();
    if(oldNick!=newNick)
    {
      nicknameCombobox->setCurrentText(oldNick);
      m_server->getOutputFilter()->changeNickname (newNick, m_mypresence->name(), m_mypresence->network()->name());
      // return focus to input line
      channelInput->setFocus();
    }
}

void ChannelWindow::childAdjustFocus()
{
    channelInput->setFocus();
    refreshModeButtons();
}

void ChannelWindow::refreshModeButtons()
{
    bool enable = true;
/*
    if(getOwnChannelNick())
    {
        enable=getOwnChannelNick()->isAnyTypeOfOp();
    } // if not channel nick, then enable is true - fall back to assuming they are op
*/

    //don't disable the mode buttons since you can't then tell if they are enabled or not.
    //needs to be fixed somehow

    /*  modeT->setEnabled(enable);
      modeN->setEnabled(enable);
      modeS->setEnabled(enable);
      modeI->setEnabled(enable);
      modeP->setEnabled(enable);
      modeM->setEnabled(enable);
      modeK->setEnabled(enable);
      modeL->setEnabled(enable);*/
    limit->setEnabled(enable);

    // Tooltips for the ModeButtons
    QString opOnly;
    if(!enable) opOnly = i18n("You have to be an operator to change this.");

    QToolTip::add(modeT, i18n("Topic can be changed by channel operator only.  %1").arg(opOnly));
    QToolTip::add(modeN, i18n("No messages to channel from clients on the outside.  %1").arg(opOnly));
    QToolTip::add(modeS, i18n("Secret channel.  %1").arg(opOnly));
    QToolTip::add(modeI, i18n("Invite only channel.  %1").arg(opOnly));
    QToolTip::add(modeP, i18n("Private channel.  %1").arg(opOnly));
    QToolTip::add(modeM, i18n("Moderated channel.  %1").arg(opOnly));
    QToolTip::add(modeK, i18n("Protect channel with a password."));
    QToolTip::add(modeL, i18n("Set user limit to channel."));

}

// TODO: AllenJB: Reimplement this
void ChannelWindow::cycleChannel()
{
//    closeYourself();
    // Tell channel we're going to cycle - this will help avoid unneccessary window closing, etc.
    // m_channel->setCycle (true);
    m_server->getOutputFilter()->channelPart (m_channel->name(), m_mypresence->name(), m_mypresence->network()->name());
    m_server->getOutputFilter()->channelJoin (m_channel->name(), m_mypresence->name(), m_mypresence->network()->name());
//    m_server->sendJoinCommand(getName());
}

QString ChannelWindow::getTextInLine()
{
  return channelInput->text();
}

bool ChannelWindow::canBeFrontView()
{
  return true;
}

bool ChannelWindow::searchView()
{
  return true;
}

void ChannelWindow::appendInputText(const QString& s)
{
    channelInput->setText(channelInput->text() + s);
}

bool ChannelWindow::closeYourself()
{
    int result=KMessageBox::warningContinueCancel(
        this,
        i18n("Do you want to leave %1?").arg(getName()),
        i18n("Leave Channel"),
        i18n("Leave"),
        "QuitChannelTab");

    if(result==KMessageBox::Continue)
    {
        m_server->getOutputFilter()->channelPart (m_channel->name(), m_mypresence->name(), m_mypresence->network()->name());
        Preferences::setSpellChecking(channelInput->checkSpellingEnabled());
        m_mypresence->channelRemove (m_channel);
        return true;
    }
    return false;
}

/**
 * Set online status. Used to disable functions when not connected
 * @param online New state
 */
void ChannelWindow::serverOnline(bool online)
{
    if (online)
    {
        channelInput->setEnabled(true);
        getTextView()->setNickAndChannelContextMenusEnabled(true);
        nicknameCombobox->setEnabled(true);
    }
    else
    {
        channelInput->setEnabled(false);
        getTextView()->setNickAndChannelContextMenusEnabled(false);
        nicknameCombobox->setEnabled(false);
    }
}


void ChannelWindow::showTopic(bool show)
{
    if(show)
    {
        topicSplitterHidden = false;
        topicLine->show();
        m_topicButton->show();
        topicLine->parentWidget()->show();
    }
    else
    {
        topicLine->hide();
        m_topicButton->hide();

        if(modeBox->isHidden())
        {
            topicSplitterHidden = true;
            topicLine->parentWidget()->hide();
        }
    }
}

void ChannelWindow::showNicknameBox(bool show)
{
    if(show)
    {
        nicknameCombobox->show();
    }
    else
    {
        nicknameCombobox->hide();
    }
}

void ChannelWindow::showNicknameList(bool show)
{
    if (show)
    {
        channelSplitterHidden = false;
        nickListButtons->show();
    }
    else
    {
        channelSplitterHidden = true;
        nickListButtons->hide();
    }
}

void ChannelWindow::requestNickListSort()
{
    if(!m_delayedSortTimer)
    {
        m_delayedSortTimer = new QTimer(this);
        connect(m_delayedSortTimer, SIGNAL(timeout()), this, SLOT(sortNickList()));
    }

    if(!m_delayedSortTimer->isActive())
    {
        m_delayedSortTimer->start(1000, true);
    }
}

void ChannelWindow::sortNickList()
{
    nicknameListView->resort();

    if(m_delayedSortTimer)
    {
        m_delayedSortTimer->stop();
    }
}

bool ChannelWindow::eventFilter(QObject* watched, QEvent* e)
{
    if((watched == nicknameListView) && (e->type() == QEvent::Resize) && splittersInitialized && isShown())
    {
        if (!topicSplitterHidden && !channelSplitterHidden)
        {
            Preferences::setChannelSplitterSizes(m_horizSplitter->sizes());
            Preferences::setTopicSplitterSizes(m_vertSplitter->sizes());
        }
        if (!topicSplitterHidden && channelSplitterHidden)
        {
            Preferences::setTopicSplitterSizes(m_vertSplitter->sizes());
        }
        if (!channelSplitterHidden && topicSplitterHidden)
        {
            Preferences::setChannelSplitterSizes(m_horizSplitter->sizes());
        }
    }

    return ChatWindow::eventFilter(watched, e);
}

#include "channelwindow.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
