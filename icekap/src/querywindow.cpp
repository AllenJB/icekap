/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005 Eike Hein <sho@eikehein.com>
*/

#include <qhbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qtextcodec.h>
#include <qtooltip.h>
#include <qtextstream.h>
#include <qwhatsthis.h>
#include <qsplitter.h>

#include <klocale.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kstringhandler.h>
#include <kpopupmenu.h>

#include "querywindow.h"

#include "icecapserver.h"
#include "konversationapplication.h"
#include "ircinput.h"
#include "ircview.h"
#include "ircviewbox.h"
#include "common.h"
#include "topiclabel.h"
#include "icecappresence.h"
#include "icecapquery.h"

QueryWindow::QueryWindow (QWidget* parent) : ChatWindow(parent)
{
    m_nickInfo = 0;
    // don't setName here! It will break logfiles!
    //   setName("QueryWidget");
    setType(ChatWindow::Query);

    setChannelEncodingSupported(true);

    m_headerSplitter = new QSplitter(Qt::Vertical, this);

    m_initialShow = true;
    awayChanged=false;
    awayState=false;
    QHBox *box = new QHBox(m_headerSplitter);
    addresseeimage = new QLabel(box, "query_image");
    addresseeimage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    addresseeimage->hide();
    addresseelogoimage = new QLabel(box, "query_logo_image");
    addresseelogoimage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    addresseelogoimage->hide();

    queryHostmask=new Konversation::TopicLabel(box, "query_hostmask");

    QString whatsthis = i18n("<qt>Some details of the person you are talking to in this query is shown in this bar.  The full name and hostmask is shown, along with any image or logo this person has associated with them in the KDE Addressbook.<p>See the <i>Konversation Handbook</i> for information on associating a nick with a contact in the Addressbook, and for an explanation of what the hostmask is.</qt>");
    QWhatsThis::add(addresseeimage, whatsthis);
    QWhatsThis::add(addresseelogoimage, whatsthis);
    QWhatsThis::add(queryHostmask, whatsthis);

    IRCViewBox* ircBox = new IRCViewBox(m_headerSplitter,0);
    setTextView(ircBox->ircView());               // Server will be set later in setServer();
    textView->setAcceptDrops(true);
    connect(textView,SIGNAL(popupCommand(int)),this,SLOT(popup(int)));

    // link "Whois", "Ignore" ... menu items into ircview popup
    QPopupMenu* popup=textView->getPopup();
    popup->insertItem(i18n("&Whois"),Konversation::Whois);
    popup->insertItem(i18n("&Version"),Konversation::Version);
    popup->insertItem(i18n("&Ping"),Konversation::Ping);
    popup->insertSeparator();

    popup->insertItem(i18n("Ignore"), Konversation::IgnoreNick);
    popup->insertItem(i18n("Unignore"), Konversation::UnignoreNick);
    popup->setItemVisible(Konversation::IgnoreNick, false);
    popup->setItemVisible(Konversation::UnignoreNick, false);
    popup->insertSeparator();

    if (kapp->authorize("allow_downloading"))
    {
        popup->insertItem(SmallIcon("2rightarrow"),i18n("Send &File..."),Konversation::DccSend);
        popup->insertSeparator();
    }

    popup->insertItem(i18n("Add to Watched Nicks"), Konversation::AddNotify);

    // This box holds the input line
    QHBox* inputBox=new QHBox(this, "input_log_box");
    inputBox->setSpacing(spacing());

    awayLabel=new QLabel(i18n("(away)"),inputBox);
    awayLabel->hide();
    queryInput=new IRCInput(inputBox);

    getTextView()->installEventFilter(queryInput);
    queryInput->installEventFilter(this);

    // connect the signals and slots
    connect(queryInput,SIGNAL (submit()),this,SLOT (queryTextEntered()) );
    connect(queryInput,SIGNAL (envelopeCommand()),this,SLOT (queryPassthroughCommand()) );
    connect(queryInput,SIGNAL (textPasted(const QString&)),this,SLOT (textPasted(const QString&)) );
    connect(getTextView(), SIGNAL(textPasted(bool)), queryInput, SLOT(paste(bool)));
    connect(getTextView(),SIGNAL (gotFocus()),queryInput,SLOT (setFocus()) );

    connect(textView, SIGNAL(updateTabNotification(Konversation::TabNotifyType)),
        this, SLOT(activateTabNotification(Konversation::TabNotifyType)));
    connect(textView,SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
    connect(textView,SIGNAL (extendedPopup(int)),this,SLOT (popup(int)) );
    connect(textView,SIGNAL (autoText(const QString&)),this,SLOT (sendQueryText(const QString&)) );

    updateAppearance();

    setLog(Preferences::log());
}

QueryWindow::~QueryWindow ()
{
}

void QueryWindow::setName(const QString& newName)
{
    if(ChatWindow::getName() == newName) return;  // no change, so return

    ChatWindow::setName(newName);

    // don't change logfile name if query name changes
    // This will prevent Nick-Changers to create more than one log file,
    if (logName.isEmpty())
    {
        QString logName =  (Preferences::lowerLog()) ? getName().lower() : getName() ;

        if(Preferences::addHostnameToLog())
        {
            if(m_nickInfo)
                logName += m_nickInfo->getHostmask();
        }

        setLogfileName(logName);
    }
}

void QueryWindow::queryTextEntered()
{
    QString line=queryInput->text();
    queryInput->clear();
    if(line.lower()==Preferences::commandChar()+"clear")
    {
        textView->clear();
    }
    else if(line.lower()==Preferences::commandChar()+"part")
    {
        m_mypresence->queryRemove (m_query);
    }
    else if(line.length())
    {
         sendQueryText(line);
    }
}

void QueryWindow::queryPassthroughCommand()
{
    QString commandChar = Preferences::commandChar();
    QString line = queryInput->text();

    queryInput->clear();

    if(!line.isEmpty())
    {
        // Prepend commandChar on Ctrl+Enter to bypass outputfilter command recognition
        if (line.startsWith(commandChar))
        {
            line = commandChar + line;
        }
        sendQueryText(line);
    }
}

void QueryWindow::sendQueryText(const QString& sendLine)
{
    // create a work copy
    QString outputAll(sendLine);
    // replace aliases and wildcards
    if(m_server->getOutputFilter()->replaceAliases(outputAll))
    {
        outputAll = m_server->parseWildcards(outputAll, m_mypresence->presence()->getNickname(), getName(), QString::null, QString::null, QString::null);
    }

    // Send all strings, one after another
    QStringList outList=QStringList::split('\n',outputAll);
    for(unsigned int index=0;index<outList.count();index++)
    {
        QString output(outList[index]);

        // encoding stuff is done in Server()
        Icecap::OutputFilterResult result = m_server->getOutputFilter()->parse (m_mypresence->presence()->getNickname(), output, m_mypresence->network()->name(), m_mypresence->name(), m_query->presence()->name());

        if(!result.output.isEmpty())
        {
            if(result.type == Icecap::Action) appendAction(m_mypresence->presence()->getNickname(), result.output);
            else if(result.type == Icecap::Command) appendCommandMessage(result.typeString, result.output);
            else if(result.type == Icecap::Program) appendServerMessage(result.typeString, result.output);
            else if(!result.typeString.isEmpty()) appendQuery(result.typeString, result.output);
            else appendQuery(m_mypresence->presence()->getNickname(), result.output);
        }
        else if (result.outputList.count())
        {
            Q_ASSERT(result.type==Icecap::Message);
            for ( QStringList::Iterator it = result.outputList.begin(); it != result.outputList.end(); ++it )
            {
                append(m_mypresence->presence()->getNickname(), *it);
            }
        }

        // TODO: Ensure this isn't used anywhere, then remove it
        if (result.toServerList.count())
        {
            m_server->queueList(result.toServerList);
        }
        else //per the original code, an empty string could be queued
        {
            m_server->queue(result.toServer);
        }
    } // for
}

void QueryWindow::updateAppearance()
{
    QColor fg;
    QColor bg;

    if(Preferences::inputFieldsBackgroundColor())
    {
        fg=Preferences::color(Preferences::ChannelMessage);
        bg=Preferences::color(Preferences::TextViewBackground);
    }
    else
    {
        fg=colorGroup().foreground();
        bg=colorGroup().base();
    }

    queryInput->unsetPalette();
    queryInput->setPaletteForegroundColor(fg);
    queryInput->setPaletteBackgroundColor(bg);

    getTextView()->unsetPalette();

    if (Preferences::showBackgroundImage())
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
        queryInput->setFont(Preferences::textFont());
    }
    else
    {
        getTextView()->setFont(KGlobalSettings::generalFont());
        queryInput->setFont(KGlobalSettings::generalFont());
    }

    ChatWindow::updateAppearance();
}

void QueryWindow::textPasted(const QString& text)
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
            sendQueryText(line);
        }
    }
}

void QueryWindow::indicateAway(bool show)
{
    // QT does not redraw the label properly when they are not on screen
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

// fix QTs broken behavior on hidden QListView pages
void QueryWindow::showEvent(QShowEvent*)
{
    if(awayChanged)
    {
        awayChanged=false;
        indicateAway(awayState);
    }

    if(m_initialShow) {
        m_initialShow = false;
        QValueList<int> sizes;
        sizes << queryHostmask->sizeHint().height() << (height() - queryHostmask->sizeHint().height());
        m_headerSplitter->setSizes(sizes);
    }
}

void QueryWindow::popup(int id)
{
    // get the nickname to the context menu popup
    QString name=textView->getContextNick();
    // if there was none (right click into the text view) assume query partner
    if(name.isEmpty()) name=getName();

    switch(id)
    {
        case Konversation::Whois:
            sendQueryText(Preferences::commandChar()+"WHOIS "+name+' '+name);
            break;

        case Konversation::IgnoreNick:
	    {
              sendQueryText(Preferences::commandChar()+"IGNORE -ALL "+name+"!*");
              int rc=KMessageBox::questionYesNo(this,
                i18n("Do you want to close this query after ignoring this nickname?"),
                i18n("Close This Query"),
                i18n("Close"),
                i18n("Keep Open"),
                "CloseQueryAfterIgnore");
              if(rc==KMessageBox::Yes) closeYourself();
              break;
        }
        case Konversation::UnignoreNick:
        {
            QString question = i18n("Do you want to stop ignoring %1?").arg(name);

            if (KMessageBox::warningContinueCancel(this, question, i18n("Unignore"), i18n("Unignore"), "UnignoreNick") ==
                KMessageBox::Continue)
            {
                sendQueryText(Preferences::commandChar()+"UNIGNORE "+name);
            }
            break;
        }
        case Konversation::DccSend:
            sendQueryText(Preferences::commandChar()+"DCC SEND "+name);
            break;

        case Konversation::Version:
            sendQueryText(Preferences::commandChar()+"CTCP "+name+" VERSION");
            break;

        case Konversation::Ping:
            sendQueryText(Preferences::commandChar()+"CTCP "+name+" PING");
            break;

        case Konversation::Names:
            m_server->queue("NAMES " + getTextView()->currentChannel());
            break;
        case Konversation::Join:
            m_server->queue("JOIN " + getTextView()->currentChannel());
            break;

        default:
            kdDebug() << "QueryWindow::popup(): Popup id " << id << " does not belong to me!" << endl;
            break;
    }

    // delete context menu nickname
    textView->clearContextNick();
}

void QueryWindow::sendFileMenu()
{
    emit sendFile(getName());
}

void QueryWindow::childAdjustFocus()
{
    queryInput->setFocus();
}

void QueryWindow::setNickInfo(Icecap::Presence* nickInfo)
{
    if(m_nickInfo != 0)
        disconnect(m_nickInfo, SIGNAL(nickInfoChanged()), this, SLOT(nickInfoChanged()));

    m_nickInfo = nickInfo;
    Q_ASSERT(m_nickInfo); if(!m_nickInfo) return;
    setName(m_nickInfo->getNickname());
    connect(m_nickInfo, SIGNAL(nickInfoChanged()), this, SLOT(nickInfoChanged()));
    nickInfoChanged();
}

void QueryWindow::nickInfoChanged()
{
    if(m_nickInfo)
    {

        setName(m_nickInfo->getNickname());
        QString text = m_nickInfo->getBestAddresseeName();
        if(!m_nickInfo->getHostmask().isEmpty() && !text.isEmpty())
            text += " - ";
        text += m_nickInfo->getHostmask();
// TODO: AllenJB: Away mode
//        if(m_nickInfo->isAway() )
//            text += " (" + KStringHandler::rsqueeze(m_nickInfo->getAwayMessage(),100) + ") ";
        queryHostmask->setText(Konversation::removeIrcMarkup(text));

        addresseeimage->hide();
        addresseelogoimage->hide();

        QString strTooltip;
        QTextStream tooltip( &strTooltip, IO_WriteOnly );

        tooltip << "<qt>";

        tooltip << "<table cellspacing=\"0\" cellpadding=\"0\">";

        m_nickInfo->tooltipTableData(tooltip);

        tooltip << "</table></qt>";
        QToolTip::add(queryHostmask, strTooltip);
        QToolTip::add(addresseeimage, strTooltip);
        QToolTip::add(addresseelogoimage, strTooltip);

    }
    else
    {
        addresseeimage->hide();
        addresseelogoimage->hide();
    }

    emit updateQueryChrome(this,getName());
    emitUpdateInfo();
}

Icecap::Presence* QueryWindow::getNickInfo()
{
    return m_nickInfo;
}

QString QueryWindow::getTextInLine() { return queryInput->text(); }

bool QueryWindow::canBeFrontView()        { return true; }
bool QueryWindow::searchView()       { return true; }

void QueryWindow::appendInputText(const QString& s)
{
    queryInput->setText(queryInput->text() + s);
}

                                                  // virtual
void QueryWindow::setChannelEncoding(const QString& encoding)
{
//    Preferences::setChannelEncoding(m_server->getServerGroup(), getName(), encoding);
}

QString QueryWindow::getChannelEncoding()               // virtual
{
//    return Preferences::channelEncoding(m_server->getServerGroup(), getName());
	return "utf8";
}

QString QueryWindow::getChannelEncodingDefaultDesc()    // virtual
{
//    return i18n("Identity Default ( %1 )").arg(getServer()->getIdentity()->getCodecName());
	return "Unimplemented (utf8)";
}

bool QueryWindow::closeYourself()
{
    int result=KMessageBox::warningContinueCancel(
        this,
        i18n("Do you want to close your query with %1?").arg(getName()),
        i18n("Close Query"),
        i18n("Close"),
        "QuitQueryTab");

    if(result==KMessageBox::Continue)
    {
        m_mypresence->queryRemove (m_query);
        return true;
    }

    return false;
}

void QueryWindow::serverOnline(bool online)
{
    queryInput->setEnabled (online);
    getTextView()->setNickAndChannelContextMenusEnabled(online);

    QPopupMenu* popup = getTextView()->getPopup();

    if (popup)
    {
        popup->setItemEnabled(Konversation::Whois, online);
        popup->setItemEnabled(Konversation::Version, online);
        popup->setItemEnabled(Konversation::Ping, online);
        popup->setItemEnabled(Konversation::IgnoreNick, online);
        popup->setItemEnabled(Konversation::UnignoreNick, online);
        popup->setItemEnabled(Konversation::AddNotify, online);

        if (kapp->authorize("allow_downloading"))
            popup->setItemEnabled(Konversation::DccSend, online);
    }
}

void QueryWindow::emitUpdateInfo()
{
    QString info;
    if(m_nickInfo->loweredNickname() == m_mypresence->presence()->loweredNickname())
        info = i18n("Talking to yourself");
    else if(m_nickInfo)
        info = m_nickInfo->getBestAddresseeName();
    else
        info = getName();

    emit updateInfo(info);
}

// show quit message of nick if we see it
void QueryWindow::quitNick(const QString& reason)
{
    QString displayReason = reason;

    if (displayReason.isEmpty())
    {
        appendCommandMessage(i18n("Quit"),i18n("%1 has left this server.").arg(getName()),false);
    }
    else
    {
        if (displayReason.find(QRegExp("[\\0000-\\0037]"))!=-1)
            displayReason+="\017";

        appendCommandMessage(i18n("Quit"),i18n("%1 has left this server (%2).").arg(getName()).arg(displayReason),false);
    }
}

void QueryWindow::setQuery (Icecap::Query* query)
{
    m_query = query;
    connect (m_query, SIGNAL(online(bool)), this, SLOT(serverOnline(bool)));
    setMyPresence (query->mypresence());
    setNickInfo (query->presence());
}

#include "querywindow.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
