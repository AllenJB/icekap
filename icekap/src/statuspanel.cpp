/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The panel where the server status messages go
  begin:     Sam Jan 18 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qpushbutton.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qhbox.h>
#include <qtextcodec.h>
#include <qlineedit.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <kspy.h>

// #include "channel.h"
#include "statuspanel.h"
#include "konversationapplication.h"
#include "ircinput.h"
#include "ircview.h"
#include "ircviewbox.h"
#include "icecapserver.h"

StatusPanel::StatusPanel(QWidget* parent) : IcecapStatusPanel (parent, true)
{
    setChannelEncodingSupported(true);
    setType (ChatWindow::Status);

    awayChanged=false;
    awayState=false;

    QHBox* commandLineBox=new QHBox(this);
    commandLineBox->setSpacing(spacing());
    commandLineBox->setMargin(0);

    nicknameCombobox=new QComboBox(commandLineBox);
    nicknameCombobox->setEditable(true);
    nicknameCombobox->insertStringList(Preferences::nicknameList());
    oldNick=nicknameCombobox->currentText();

    awayLabel=new QLabel(i18n("(away)"),commandLineBox);
    awayLabel->hide();

    statusInput=new IRCInput(commandLineBox);

    getTextView()->installEventFilter(statusInput);
    statusInput->installEventFilter(this);

    setLog(Preferences::log());

    connect(getTextView(),SIGNAL (autoText(const QString&)),this,SLOT (sendStatusText(const QString&)) );
    connect(statusInput,SIGNAL (submit()),this,SLOT(statusTextEntered()) );

    connect(statusInput,SIGNAL (textPasted(const QString&)),this,SLOT(textPasted(const QString&)) );
    connect(getTextView(), SIGNAL(textPasted(bool)), statusInput, SLOT(paste(bool)));

    connect(getTextView(),SIGNAL (gotFocus()),statusInput,SLOT (setFocus()) );

    connect(nicknameCombobox,SIGNAL (activated(int)),this,SLOT(nicknameComboboxChanged()));
    Q_ASSERT(nicknameCombobox->lineEdit());       //it should be editedable.  if we design it so it isn't, remove these lines.
    if(nicknameCombobox->lineEdit())
        connect(nicknameCombobox->lineEdit(), SIGNAL (lostFocus()),this,SLOT(nicknameComboboxChanged()));

    updateAppearance();
}

StatusPanel::~StatusPanel()
{
}

Icecap::MyPresence* StatusPanel::getMyPresence()
{
    return m_mypresence;
}

void StatusPanel::setMyPresence (Icecap::MyPresence* mypresence)
{
    IcecapStatusPanel::setMyPresence (mypresence);
//    m_mypresence = mypresence;
//    setServer (mypresence->server());
    nicknameCombobox->clear();
//    nicknameCombobox->insertStringList(mypresence->getNicknameList());
//    nicknameCombobox->setCurrentText (mypresence->getNickname (0));
    nicknameCombobox->setCurrentText (mypresence->name ());
}

void StatusPanel::setNickname(const QString& newNickname)
{
    nicknameCombobox->setCurrentText(newNickname);
}

void StatusPanel::updateAppearance()
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

    statusInput->unsetPalette();
    statusInput->setPaletteForegroundColor(fg);
    statusInput->setPaletteBackgroundColor(bg);

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
        nicknameCombobox->setFont(Preferences::textFont());
        statusInput->setFont(Preferences::textFont());
    }
    else
    {
        getTextView()->setFont(KGlobalSettings::generalFont());
        nicknameCombobox->setFont(KGlobalSettings::generalFont());
        statusInput->setFont(KGlobalSettings::generalFont());
    }

    showNicknameBox(Preferences::showNicknameBox());

    ChatWindow::updateAppearance();
}

void StatusPanel::sendFileMenu()
{
    emit sendFile();
}

void StatusPanel::indicateAway(bool show)
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

// fix Qt's broken behavior on hidden QListView pages
void StatusPanel::showEvent(QShowEvent*)
{
    if(awayChanged)
    {
        awayChanged=false;
        indicateAway(awayState);
    }
}

// TODO: Shift IRC command somewhere sensible
void StatusPanel::nicknameComboboxChanged()
{
    QString newNick=nicknameCombobox->currentText();
//    oldNick=m_server->getNickname();
//    oldNick=m_mypresence->getNickname(0);
    oldNick = m_mypresence->name ();
    if(oldNick!=newNick)
    {
      nicknameCombobox->setCurrentText(oldNick);
      m_server->queue("NICK "+newNick);
    }
    // return focus to input line
    statusInput->setFocus();
}

// TODO: Shift IRC command somewhere sensible
void StatusPanel::changeNickname(const QString& newNickname)
{
    m_server->queue("NICK "+newNickname);
}

                                                  // virtual
void StatusPanel::setChannelEncoding(const QString& encoding)
{
//    Preferences::setChannelEncoding(m_server->getServerGroup(), ":server", encoding);
}

QString StatusPanel::getChannelEncoding()         // virtual
{
//    return Preferences::channelEncoding(m_server->getServerGroup(), ":server");
    return "utf8";
}

                                                  // virtual
QString StatusPanel::getChannelEncodingDefaultDesc()
{
//    return i18n("Identity Default ( %1 )").arg(getServer()->getIdentity()->getCodecName());
    return "Unimplemented (utf8)";
}

//Used to disable functions when not connected
void StatusPanel::serverOnline(bool online)
{
    //statusInput->setEnabled(online);
    getTextView()->setNickAndChannelContextMenusEnabled(online);
    nicknameCombobox->setEnabled(online);
}

void StatusPanel::showNicknameBox(bool show)
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

void StatusPanel::emitUpdateInfo()
{
//    QString info = mypresencename();
//    emit updateInfo(info);
}

void StatusPanel::statusTextEntered()
{
    QString line=statusInput->text();
    statusInput->clear();

    if(line.lower()==Preferences::commandChar()+"clear") textView->clear();
    else if (line.lower()==Preferences::commandChar()+"kspy") KSpy::invoke();
    else
    {
        if(line.length()) sendStatusText(line);
    }
}

void StatusPanel::sendStatusText(const QString& sendLine)
{
    // create a work copy
    QString outputAll(sendLine);
    // replace aliases and wildcards
    if(m_server->getOutputFilter()->replaceAliases(outputAll))
    {
        outputAll = m_server->parseWildcards (outputAll, m_mypresence->name(), QString::null, QString::null, QString::null, QString::null);
    }

    Icecap::OutputFilter* outputFilter = m_server->getOutputFilter ();

    // Send all strings, one after another
    QStringList outList=QStringList::split('\n',outputAll);
    for(unsigned int index=0;index<outList.count();index++)
    {
        QString output(outList[index]);

        // encoding stuff is done in Server()
        Icecap::OutputFilterResult result = outputFilter->parse("", output, m_mypresence->network()->name(), m_mypresence->name());

        if(!result.output.isEmpty())
        {
            appendServerMessage(result.typeString, result.output);
        }
        else if (!result.outputList.isEmpty ())
        {
            QStringList::const_iterator end = result.outputList.end();
            for ( QStringList::ConstIterator it = result.outputList.begin(); it != end; ++it ) {
                appendServerMessage(result.typeString, *it);
            }
        }
        m_server->queue(result.toServer);
    } // for
}

#include "statuspanel.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
