/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2004 Peter Simonsson <psn@linux.se>
  Copyright (C) 2006 Eike Hein <sho@eikehein.com>
*/

#include <qdatastream.h>
#include <qstringlist.h>
#include <qdatetime.h>
#include <qregexp.h>
#include <qmap.h>

#include <klocale.h>
#include <kdeversion.h>
#include <kstringhandler.h>

#include <config.h>
#include <kdebug.h>

#include "icecapinputfilter.h"
#include "icecapserver.h"
#include "replycodes.h"
#include "konversationapplication.h"
#include "commit.h"
#include "version.h"
#include "query.h"
#include "channel.h"
#include "statuspanel.h"
#include "common.h"
#include "notificationhandler.h"

#include <kresolver.h>

IcecapInputFilter::IcecapInputFilter()
{
    m_connecting = false;
}

IcecapInputFilter::~IcecapInputFilter()
{
}

void IcecapInputFilter::setServer(IcecapServer* newServer)
{
    server=newServer;
}

void IcecapInputFilter::parseLine(const QString& a_newLine)
{
    QString trailing;
    QString newLine(a_newLine);

    // Remove white spaces at the end and beginning
    newLine = newLine.stripWhiteSpace();

    QStringList part;
    int posSep = newLine.find (';');
    if (posSep != -1) {
        part = QStringList::split(";", newLine);
    }

    // Is this an event?
    QString tag = part[0];
    if (tag == "*") {
        // This is an event
        // Format: *;<event-name-underscored>;<parameters>
        // Parameters can be static (cacheable, no prefix) or dynamic (not cacheable, prefix "$")
        // The time parameter is the time that the event was created on the server, NOT the current time

        QString eventName = part[1];
        QStringList param = part;
        // Remove the type and event name from the params list
        param.pop_front();
        param.pop_front();

        parseIcecapEvent (eventName, param);
    } else {
        // This is a command reply
        // Format: <tag>;<status>;<params>
        // tag is the tag sent by the client when the command was issued. Can be just about anything but "*"
        //  tag is basically the only way we have of identifying command replies
        // status:
        //  +  Success; Response possibly in subsequent fields
        //  -  Failue; 3rd field is error identifier
        //  >  Success so far; More to follow

        QString status = part[1];
        QStringList param = part;
        // Remove the type and event name from the params list
        param.pop_front();
        param.pop_front();
        parseIcecapCommand (tag, status, param);
    }
/*
    // Server command, if no "!" was found in prefix
    if((prefix.find('!') == -1) && (prefix != server->getNickname()))
    {
        parseServerCommand(prefix, command, parameterList, trailing);
    }
    else
    {
        parseClientCommand(prefix, command, parameterList, trailing);
    }
*/
}

void IcecapInputFilter::parseIcecapEvent (const QString &eventName, const QStringList &parameterList)
{
    // I assume this ensures there's a valid server to communicate with
    Q_ASSERT(server); if(!server) return;

    if (eventName == "preauth") {
        // preauth - currently this is just a welcome signal
        // eventually icecap will implement authentication so we'll need to add support for that

        // Send the welcome signal, so the server class knows we are connected properly
        emit welcome("");
        m_connecting = true;
        server->appendStatusMessage(i18n("Welcome"), "");
    }
}

void IcecapInputFilter::parseIcecapCommand (const QString &tag, const QString &status, QStringList &parameterList)
{
    // I assume this ensures there's a valid server to communicate with
    Q_ASSERT(server); if(!server) return;

    if (tag == "netlist")
    {
        if(getAutomaticRequest("netlist",QString::null)==0)
        {
            if (status == "+") {
                server->appendMessageToFrontmost (i18n ("End of network list"), "End of network list");
            }
            else if (status == ">")
            {
                // Split into key, value pairs around the first occurence of =
                // TODO: Is there an easier way to do this?
                // TODO: Should this be moved up to the top of this method? Possibly even to parseLine?
                // TODO: Better handling for keys that appear multiple times
                QMap<QString, QString> parameterMap;
                for ( QStringList::Iterator it = parameterList.begin(); it != parameterList.end(); ++it ) {
                    QStringList thisParam = QStringList::split("=", *it);
                    QString key = thisParam.first ();
                    thisParam.pop_front ();
                    QString value = thisParam.join ("=");
                    parameterMap.insert (key, value, TRUE);
                }

                QString message;
                message = i18n ("%1 Network: %2", "%1 Network: %2").arg (parameterMap["protocol"]).arg (parameterMap["network"]);
                server->appendMessageToFrontmost (i18n ("Network List"), message);

                // Put it here for now while testing
                if (parameterMap["protocol"] != "IRC") {
                    return;
                }

                Konversation::ServerGroupSettingsPtr network = new Konversation::ServerGroupSettings (parameterMap["network"]);
                Preferences::addServerGroup (network);
                static_cast<KonversationApplication*>(kapp)->saveOptions(true);
//                emit serverGroupsChanged();

            } else {
                // TODO: Are there any known circumstances that would cause this?
                server->appendMessageToFrontmost (i18n ("Network List Error"), "Network List: An unhandled error occurred.");
            }
        }
        else                              // send them to /LIST window
        {
            QMap<QString, QString> parameterMap;
            for ( QStringList::Iterator it = parameterList.begin(); it != parameterList.end(); ++it ) {
                QStringList thisParam = QStringList::split("=", *it);
                QString key = thisParam.first ();
                thisParam.pop_front ();
                QString value = thisParam.join ("=");
                parameterMap.insert (key, value, TRUE);
            }

            if (parameterMap["protocol"] != "IRC") {
                return;
            }

            Konversation::ServerGroupSettingsPtr network = new Konversation::ServerGroupSettings (parameterMap["network"]);
            Preferences::addServerGroup (network);
//            emit addToNetworkList (parameterMap[protocol], parameterMap[network]);
        }
    }

}

void IcecapInputFilter::parseClientCommand(const QString &prefix, const QString &command, const QStringList &parameterList, const QString &_trailing)
{
    KonversationApplication* konv_app = static_cast<KonversationApplication *>(KApplication::kApplication());
    Q_ASSERT(konv_app);
    Q_ASSERT(server);
    // Extract nickname from prefix
    int pos = prefix.find("!");
    QString sourceNick = prefix.left(pos);
    QString sourceHostmask = prefix.mid(pos + 1);
    // remember hostmask for this nick, it could have changed
    server->addHostmaskToNick(sourceNick,sourceHostmask);
    QString trailing = _trailing;

    server->appendMessageToFrontmost(command,parameterList.join(" ")+' '+trailing);
}

void IcecapInputFilter::parseServerCommand(const QString &prefix, const QString &command, const QStringList &parameterList, const QString &trailing)
{
    bool isNumeric;
    int numeric = command.toInt(&isNumeric);

    Q_ASSERT(server); if(!server) return;

    if(!isNumeric)
    {
        if(command=="ping")
        {
            QString text;
            text = (!trailing.isEmpty()) ? trailing : parameterList.join(" ");

            if(!trailing.isEmpty())
            {
                text = prefix + " :" + text;
            }

            if(!text.startsWith(" "))
            {
                text.prepend(' ');
            }

            // queue the reply to send it as soon as possible
            server->queueAt(0,"PONG"+text);
        }
        else if(command=="error :closing link:")
        {
            kdDebug() << "link closed" << endl;
        }
        else if(command=="pong")
        {
            // double check if we are in lag measuring mode since some servers fail to send
            // the LAG cookie back in PONG
            if(trailing.startsWith("LAG") || getLagMeasuring())
            {
                server->pongReceived();
            }
        }
        else if(command=="mode")
        {
            parseModes(prefix,parameterList);
        }
        else if(command=="notice")
        {
            server->appendStatusMessage(i18n("Notice"),i18n("-%1- %2").arg(prefix).arg(trailing));
        }
        // All yet unknown messages go into the frontmost window unaltered
        else
        {
            server->appendMessageToFrontmost(command,parameterList.join(" ")+' '+trailing);
        }
    }
    else
    {
        switch (numeric)
        {
            case RPL_WELCOME:
            case RPL_YOURHOST:
            case RPL_CREATED:
            {
                if(numeric==RPL_WELCOME)
                {
                    QString host;

                    if(trailing.contains("@"))
                        host = trailing.section("@",1);

                    // re-set nickname, since the server may have truncated it
                    if(parameterList[0]!=server->getNickname())
                        server->renameNick(server->getNickname(), parameterList[0]);

                    // Remember server's insternal name
                    server->setIrcName(prefix);

                    // Send the welcome signal, so the server class knows we are connected properly
                    emit welcome(host);
                    m_connecting = true;
                }
                server->appendStatusMessage(i18n("Welcome"),trailing);
                break;
            }
            case RPL_MYINFO:
            {
                server->appendStatusMessage(i18n("Welcome"),
                    i18n("Server %1 (Version %2), User modes: %3, Channel modes: %4")
                    .arg(parameterList[1])
                    .arg(parameterList[2])
                    .arg(parameterList[3])
                    .arg(parameterList[4])
                    );
                server->setAllowedChannelModes(parameterList[4]);
                break;
            }
            //case RPL_BOUNCE:   // RFC 1459 name, now seems to be obsoleted by ...
            case RPL_ISUPPORT:                    // ... DALnet RPL_ISUPPORT
            {
                server->appendStatusMessage(i18n("Support"),parameterList.join(" "));

                // The following behavoiur is neither documented in RFC 1459 nor in 2810-2813
                // Nowadays, most ircds send server capabilities out via 005 (BOUNCE).
                // refer to http://www.irc.org/tech_docs/005.html for a kind of documentation.
                // More on http://www.irc.org/tech_docs/draft-brocklesby-irc-isupport-03.txt

                QStringList::const_iterator it = parameterList.begin();
                // don't want the user name
                ++it;
                for (; it != parameterList.end(); ++it )
                {
                    QString property, value;
                    int pos;
                    if ((pos=(*it).find( '=' )) !=-1)
                    {
                        property = (*it).left(pos);
                        value = (*it).mid(pos+1);
                    }
                    else
                    {
                        property = *it;
                    }
                    if (property=="PREFIX")
                    {
                        pos = value.find(')',1);
                        if(pos==-1)
                        {
                            server->setPrefixes (QString::null, value);
                            // XXX if ) isn't in the string, NOTHING should be there. anyone got a server
                            if (value.length() || property.length())
                                server->appendStatusMessage("","XXX Server sent bad PREFIX in RPL_ISUPPORT, please report.");
                        }
                        else
                        {
                            server->setPrefixes (value.mid(1, pos-1), value.mid(pos+1));
                        }
                    }
                    else if (property=="CHANTYPES")
                    {
                        server->setChannelTypes(value);
                    }
                    else if (property == "CAPAB")
                    {
                        // Disable as we don't use this for anything yet
                        //server->queue("CAPAB IDENTIFY-MSG");
                    }
                    else
                    {
                        //kdDebug() << "Ignored server-capability: " << property << " with value '" << value << "'" << endl;
                    }
                }                                 // endfor
                break;
            }
            case RPL_CHANNELMODEIS:
            {
                const QString modeString=parameterList[2];
                // This is the string the user will see
                QString modesAre(QString::null);
                QString message = i18n("Channel modes: ") + modeString;

                for(unsigned int index=0;index<modeString.length();index++)
                {
                    QString parameter(QString::null);
                    int parameterCount=3;
                    char mode=modeString[index];
                    if(mode!='+')
                    {
                        if(!modesAre.isEmpty())
                            modesAre+=", ";
                        if(mode=='t')
                            modesAre+=i18n("topic protection");
                        else if(mode=='n')
                            modesAre+=i18n("no messages from outside");
                        else if(mode=='s')
                            modesAre+=i18n("secret");
                        else if(mode=='i')
                            modesAre+=i18n("invite only");
                        else if(mode=='p')
                            modesAre+=i18n("private");
                        else if(mode=='m')
                            modesAre+=i18n("moderated");
                        else if(mode=='k')
                        {
                            parameter=parameterList[parameterCount++];
                            message += ' ' + parameter;
                            modesAre+=i18n("password protected");
                        }
                        else if(mode=='a')
                            modesAre+=i18n("anonymous");
                        else if(mode=='r')
                            modesAre+=i18n("server reop");
                        else if(mode=='c')
                            modesAre+=i18n("no colors allowed");
                        else if(mode=='l')
                        {
                            parameter=parameterList[parameterCount++];
                            message += ' ' + parameter;
                            modesAre+=i18n("limited to %n user", "limited to %n users", parameter.toInt());
                        }
                        else
                        {
                            modesAre+=mode;
                        }
                        server->updateChannelModeWidgets(parameterList[1],mode,parameter);
                    }
                }                                 // endfor
                if(!modesAre.isEmpty())
                    if (Preferences::useLiteralModes())
                {
                    server->appendCommandMessageToChannel(parameterList[1],i18n("Mode"),message);
                }
                else
                {
                    server->appendCommandMessageToChannel(parameterList[1],i18n("Mode"),
                        i18n("Channel modes: ") + modesAre
                        );
                }
                break;
            }
            case RPL_CHANNELCREATED:
            {
                QDateTime when;
                when.setTime_t(parameterList[2].toUInt());
                server->appendCommandMessageToChannel(parameterList[1],i18n("Created"),
                    i18n("This channel was created on %1.")
                    .arg(when.toString(Qt::LocalDate))
                    );
                break;
            }
            case RPL_WHOISACCOUNT:
            {
                server->appendMessageToFrontmost(i18n("Whois"),i18n("%1 is logged in as %2.").arg(parameterList[1]).arg(parameterList[2]));

                break;
            }
            case RPL_NAMREPLY:
            {
                QStringList nickList;

                if(!trailing.isEmpty())
                {
                    nickList = QStringList::split(" ", trailing);
                }
                else if(parameterList.count() > 3)
                {
                    for(uint i = 3; i < parameterList.count(); i++) {
                        nickList.append(parameterList[i]);
                    }
                }
                else
                {
                    kdDebug() << "Hmm seems something is broken... can't get to the names!" << endl;
                }

                // send list to channel
                server->addPendingNickList(parameterList[2], nickList);

                // Display message only if this was not an automatic request.
                if(!getAutomaticRequest("NAMES",parameterList[2])==1)
                {
                    server->appendMessageToFrontmost(i18n("Names"),trailing);
                }
                break;
            }
            case RPL_ENDOFNAMES:
            {
                if(getAutomaticRequest("NAMES",parameterList[1])==1)
                {
                    // This code path was taken for the automatic NAMES input on JOIN, upcoming
                    // NAMES input for this channel will be manual invocations of /names
                    setAutomaticRequest("NAMES",parameterList[1],false);
                }
                else
                {
                    server->appendMessageToFrontmost(i18n("Names"),i18n("End of NAMES list."));
                }
                break;
            }
            // Topic set messages
            case RPL_NOTOPIC:
            {
                server->appendMessageToFrontmost(i18n("TOPIC"),i18n("The channel %1 has no topic set.").arg(parameterList[1]) /*.arg(parameterList[2])*/); //FIXME ok, whats the second parameter supposed to be?

                break;
            }
            case RPL_TOPIC:
            {
                QString topic = Konversation::removeIrcMarkup(trailing);

                // FIXME: This is an abuse of the automaticRequest system: We're
                // using it in an inverted manner, i.e. the automaticRequest is
                // set to true by a manual invocation of /topic. Bad bad bad -
                // needs rethinking of automaticRequest.
                if(getAutomaticRequest("TOPIC",parameterList[1])==0)
                {
                    // Update channel window
                    server->setChannelTopic(parameterList[1],topic);
                }
                else
                {
                    server->appendMessageToFrontmost(i18n("Topic"),i18n("The channel topic for %1 is: \"%2\"").arg(parameterList[1]).arg(topic));
                }

                break;
            }
            case RPL_TOPICSETBY:
            {
                // Inform user who set the topic and when
                QDateTime when;
                when.setTime_t(parameterList[3].toUInt());

                // See FIXME in RPL_TOPIC
                if(getAutomaticRequest("TOPIC",parameterList[1])==0)
                {
                    server->appendCommandMessageToChannel(parameterList[1],i18n("Topic"),
                        i18n("The topic was set by %1 on %2.")
                        .arg(parameterList[2]).arg(when.toString(Qt::LocalDate))
                        );

                    emit topicAuthor(parameterList[1],parameterList[2]);
                }
                else
                {
                    server->appendMessageToFrontmost(i18n("Topic"),i18n("The topic for %1 was set by %2 on %3.")
                        .arg(parameterList[1])
                        .arg(parameterList[2])
                        .arg(when.toString(Qt::LocalDate))
                        );
                    setAutomaticRequest("TOPIC",parameterList[1],false);
                }

                break;
            }
            case RPL_WHOISACTUALLY:
            {
                server->appendMessageToFrontmost(i18n("Whois"),i18n("%1 is actually using the host %2.").arg(parameterList[1]).arg(parameterList[2]));

                break;
            }
            case ERR_NOSUCHNICK:
            {
                // Display slightly different error message in case we performed a WHOIS for
                // IP resolve purposes, and clear it from the automaticRequest list
                if(getAutomaticRequest("DNS",parameterList[1])==0)
                {
                    server->appendMessageToFrontmost(i18n("Error"),i18n("%1: No such nick/channel.").arg(parameterList[1]));
                }
                else
                {
                    server->appendMessageToFrontmost(i18n("Error"),i18n("No such nick: %1.").arg(parameterList[1]));
                    setAutomaticRequest("DNS", parameterList[1], false);
                }

                break;
            }
            case ERR_NOSUCHCHANNEL:
            {
                server->appendMessageToFrontmost(i18n("Error"),i18n("%1: No such channel.").arg(parameterList[1]));

                break;
            }
            // Nick already on the server, so try another one
            case ERR_NICKNAMEINUSE:
            {
                // if we are already connected, don't try tro find another nick ourselves
                if(server->connected())
                {                                 // Show message
                    server->appendMessageToFrontmost(i18n("Nick"),i18n("Nickname already in use, try a different one."));
                }
                else                              // not connected yet, so try to find a nick that's not in use
                {
                    // Get the next nick from the list or ask for a new one
                    QString newNick = server->getNextNickname();

                    // The user chose to disconnect
                    if (newNick.isNull())
                    {
                        server->disconnect();
                    }
                    else
                    {
                        // Update Server window
                        server->obtainNickInfo(server->getNickname()) ;
                        server->renameNick(server->getNickname(), newNick);
                        // Show message
                        server->appendMessageToFrontmost(i18n("Nick"), i18n("Nickname already in use. Trying %1.").arg(newNick));
                        // Send nickchange request to the server
                        server->queue("NICK "+newNick);
                    }
                }
                break;
            }
            case ERR_ERRONEUSNICKNAME:
            {
                if(server->connected())
                {                                 // We are already connected. Just print the error message
                    server->appendMessageToFrontmost(i18n("Nick"), trailing);
                }
                else                              // Find a new nick as in ERR_NICKNAMEINUSE
                {
                    QString newNick = server->getNextNickname();

                    // The user chose to disconnect
                    if (newNick.isNull())
                    {
                        server->disconnect();
                    }
                    else
                    {
                        server->obtainNickInfo(server->getNickname()) ;
                        server->renameNick(server->getNickname(), newNick);
                        server->appendMessageToFrontmost(i18n("Nick"), i18n("Erroneus nickname. Changing nick to %1." ).arg(newNick)) ;
                        server->queue("NICK "+newNick);
                    }
                }
                break;
            }
            case ERR_NOTONCHANNEL:
            {
                server->appendMessageToFrontmost(i18n("Error"),i18n("You are not on %1.").arg(parameterList[1]));

                break;
            }
            case RPL_MOTDSTART:
            {
                if(!m_connecting || !Preferences::skipMOTD())
                  server->appendStatusMessage(i18n("MOTD"),i18n("Message of the day:"));
                break;
            }
            case RPL_MOTD:
            {
                if(!m_connecting || !Preferences::skipMOTD())
                    server->appendStatusMessage(i18n("MOTD"),trailing);
                break;
            }
            case RPL_ENDOFMOTD:
            {
                if(!m_connecting || !Preferences::skipMOTD())
                    server->appendStatusMessage(i18n("MOTD"),i18n("End of message of the day"));

                if(m_connecting)
                    server->autoCommandsAndChannels();

                m_connecting = false;
                break;
            }
            case ERR_NOMOTD:
            {
                if(m_connecting)
                    server->autoCommandsAndChannels();

                m_connecting = false;
                break;
            }
            case RPL_YOUREOPER:
            {
                server->appendMessageToFrontmost(i18n("Notice"),i18n("You are now an IRC operator on this server."));

                break;
            }
            case RPL_GLOBALUSERS:                 // Current global users: 589 Max: 845
            {
                QString current(trailing.section(' ',3));
                //QString max(trailing.section(' ',5,5));
                server->appendStatusMessage(i18n("Users"),i18n("Current users on the network: %1").arg(current));
                break;
            }
            case RPL_LOCALUSERS:                  // Current local users: 589 Max: 845
            {
                QString current(trailing.section(' ',3));
                //QString max(trailing.section(' ',5,5));
                server->appendStatusMessage(i18n("Users"),i18n("Current users on %1: %2.").arg(prefix).arg(current));
                break;
            }
            case RPL_ISON:
            {
                // Tell server to start the next notify timer round
                emit notifyResponse(trailing);
                break;
            }
            case RPL_AWAY:
            {
                NickInfo* nickInfo = server->getNickInfo(parameterList[1]);
                if(nickInfo)
                {
                    nickInfo->setAway(true);
                    if( nickInfo->getAwayMessage() == trailing )
                        break;
                    nickInfo->setAwayMessage(trailing);
                }

                if(getAutomaticRequest("WHOIS",parameterList[1])==0)
                {
                    server->appendMessageToFrontmost(i18n("Away"),i18n("%1 is away: %2")
                        .arg(parameterList[1]).arg(trailing)
                        );
                }

                break;
            }
            case RPL_INVITING:
            {
                server->appendMessageToFrontmost(i18n("Invite"),
                    i18n("You invited %1 to channel %2.")
                    .arg(parameterList[1]).arg(parameterList[2])
                    );
                break;
            }
            //Sample WHOIS response
            //"/WHOIS psn"
            //[19:11] :zahn.freenode.net 311 PhantomsDad psn ~psn h106n2fls23o1068.bredband.comhem.se * :Peter Simonsson
            //[19:11] :zahn.freenode.net 319 PhantomsDad psn :#kde-devel #koffice
            //[19:11] :zahn.freenode.net 312 PhantomsDad psn irc.freenode.net :http://freenode.net/
            //[19:11] :zahn.freenode.net 301 PhantomsDad psn :away
            //[19:11] :zahn.freenode.net 320 PhantomsDad psn :is an identified user
            //[19:11] :zahn.freenode.net 317 PhantomsDad psn 4921 1074973024 :seconds idle, signon time
            //[19:11] :zahn.freenode.net 318 PhantomsDad psn :End of /WHOIS list.
            case RPL_WHOISUSER:
            {
                NickInfo* nickInfo = server->getNickInfo(parameterList[1]);
                if(nickInfo)
                {
                    nickInfo->setHostmask(i18n("%1@%2").arg(parameterList[2]).arg(parameterList[3]));
                    nickInfo->setRealName(trailing);
                }
                // Display message only if this was not an automatic request.
                if(getAutomaticRequest("WHOIS",parameterList[1])==0)
                {
                    // escape html tags
                    QString escapedRealName(trailing);
                    escapedRealName.replace("<","&lt;").replace(">","&gt;");
                    server->appendMessageToFrontmost(i18n("Whois"),
                        i18n("%1 is %2@%3 (%4)")
                        .arg(parameterList[1])
                        .arg(parameterList[2])
                        .arg(parameterList[3])
                        .arg(escapedRealName), false);   // Don't parse any urls
                }
                else
                {
                    // This WHOIS was requested by Server for DNS resolve purposes; try to resolve the host
                    if(getAutomaticRequest("DNS",parameterList[1])==1)
                    {
                        KNetwork::KResolverResults resolved = KNetwork::KResolver::resolve(parameterList[3],"");
                        if(resolved.error() == KResolver::NoError && resolved.size() > 0)
                        {
                            QString ip = resolved.first().address().nodeName();
                            server->appendMessageToFrontmost(i18n("DNS"),
                                i18n("Resolved %1 (%2) to address: %3")
                                .arg(parameterList[1])
                                .arg(parameterList[3])
                                .arg(ip)
                                );
                        }
                        else
                        {
                            server->appendMessageToFrontmost(i18n("Error"),
                                i18n("Unable to resolve address for %1 (%2)")
                                .arg(parameterList[1])
                                .arg(parameterList[3])
                                );
                        }

                        // Clear this from the automaticRequest list so it works repeatedly
                        setAutomaticRequest("DNS", parameterList[1], false);
                    }
                }
                break;
            }
            // From a WHOIS.
            //[19:11] :zahn.freenode.net 320 PhantomsDad psn :is an identified user
            case RPL_IDENTIFIED:
            {
                NickInfo* nickInfo = server->getNickInfo(parameterList[1]);
                if(nickInfo)
                {
                    nickInfo->setIdentified(true);
                }
                if(getAutomaticRequest("WHOIS",parameterList[1])==0)
                {
                    // Prints "psn is an identified user"
                    //server->appendStatusMessage(i18n("Whois"),parameterList.join(" ").section(' ',1)+' '+trailing);
                    // The above line works fine, but can't be i18n'ised. So use the below instead.. I hope this is okay.
                    server->appendMessageToFrontmost(i18n("Whois"), i18n("%1 is an identified user.").arg(parameterList[1]));
                }
                break;
            }
            // Sample WHO response
            //"/WHO #lounge"
            //[21:39] [352] #lounge jasmine bots.worldforge.org irc.worldforge.org jasmine H 0 jasmine
            //[21:39] [352] #lounge ~Nottingha worldforge.org irc.worldforge.org SherwoodSpirit H 0 Arboreal Entity
            case RPL_WHOREPLY:
            {
                NickInfo* nickInfo = server->getNickInfo(parameterList[5]);
                                                  // G=away G@=away,op G+=away,voice
                bool bAway = parameterList[6].upper().startsWith("G");
                if(nickInfo)
                {
                    nickInfo->setHostmask(i18n("%1@%2").arg(parameterList[2]).arg(parameterList[3]));
                                                  //Strip off the "0 "
                    nickInfo->setRealName(trailing.section(" ", 1));
                    nickInfo->setAway(bAway);
                    if(!bAway)
                    {
                        nickInfo->setAwayMessage(QString::null);
                    }
                }
                // Display message only if this was not an automatic request.
                if(!whoRequestList.isEmpty())     // for safe
                {
                    if(getAutomaticRequest("WHO",whoRequestList.front())==0)
                    {
                        server->appendMessageToFrontmost(i18n("Who"),
                            i18n("%1 is %2@%3 (%4)%5").arg(parameterList[5])
                            .arg(parameterList[2])
                            .arg(parameterList[3])
                            .arg(trailing.section(" ", 1))
                            .arg(bAway?i18n(" (Away)"):QString::null)
                            , false); // Don't parse as url
                    }
                }
                break;
            }
            case RPL_ENDOFWHO:
            {
                if(!whoRequestList.isEmpty())
                {                                 // for safety
                    if(parameterList[1].lower()==whoRequestList.front())
                    {
                        if(getAutomaticRequest("WHO",whoRequestList.front())==0)
                        {
                            server->appendMessageToFrontmost(i18n("Who"),
                                i18n("End of /WHO list for %1")
                                .arg(parameterList[1]));
                        }
                        else
                        {
                            setAutomaticRequest("WHO",whoRequestList.front(),false);
                        }
                        whoRequestList.pop_front();
                    }
                    else
                    {
                        // whoReauestList seems to be broken.
                        kdDebug()   << "IcecapInputFilter::parseServerCommand(): RPL_ENDOFWHO: malformed ENDOFWHO. retrieved: "
                            << parameterList[1] << " expected: " << whoRequestList.front()
                            << endl;
                        whoRequestList.clear();
                    }
                }
                else
                {
                    kdDebug()   << "IcecapInputFilter::parseServerCommand(): RPL_ENDOFWHO: unexpected ENDOFWHO. retrieved: "
                        << parameterList[1]
                        << endl;
                }
                emit endOfWho(parameterList[1]);
                break;
            }
            case RPL_WHOISCHANNELS:
            {
                QStringList userChannels,voiceChannels,opChannels,halfopChannels,ownerChannels,adminChannels;

                // get a list of all channels the user is in
                QStringList channelList=QStringList::split(' ',trailing);
                channelList.sort();

                // split up the list in channels where they are operator / user / voice
                for(unsigned int index=0; index < channelList.count(); index++)
                {
                    QString lookChannel=channelList[index];
                    if(lookChannel.startsWith("*") || lookChannel.startsWith("&"))
                    {
                        adminChannels.append(lookChannel.mid(1));
                        server->setChannelNick(lookChannel.mid(1), parameterList[1], 16);
                    }
                                                  // See bug #97354 part 2
                    else if((lookChannel.startsWith("!") || lookChannel.startsWith("~")) && server->isAChannel(lookChannel.mid(1)))
                    {
                        ownerChannels.append(lookChannel.mid(1));
                        server->setChannelNick(lookChannel.mid(1), parameterList[1], 8);
                    }
                                                  // See bug #97354 part 1
                    else if(lookChannel.startsWith("@+"))
                    {
                        opChannels.append(lookChannel.mid(2));
                        server->setChannelNick(lookChannel.mid(2), parameterList[1], 4);
                    }
                    else if(lookChannel.startsWith("@"))
                    {
                        opChannels.append(lookChannel.mid(1));
                        server->setChannelNick(lookChannel.mid(1), parameterList[1], 4);
                    }
                    else if(lookChannel.startsWith("%"))
                    {
                        halfopChannels.append(lookChannel.mid(1));
                        server->setChannelNick(lookChannel.mid(1), parameterList[1], 2);
                    }
                    else if(lookChannel.startsWith("+"))
                    {
                        voiceChannels.append(lookChannel.mid(1));
                        server->setChannelNick(lookChannel.mid(1), parameterList[1], 1);
                    }
                    else
                    {
                        userChannels.append(lookChannel);
                        server->setChannelNick(lookChannel, parameterList[1], 0);
                    }
                }                                 // endfor
                // Display message only if this was not an automatic request.
                if(getAutomaticRequest("WHOIS",parameterList[1])==0)
                {
                    if(userChannels.count())
                    {
                        server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 is a user on channels: %2")
                            .arg(parameterList[1])
                            .arg(userChannels.join(" "))
                            );
                    }
                    if(voiceChannels.count())
                    {
                        server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 has voice on channels:  %2")
                            .arg(parameterList[1]).arg(voiceChannels.join(" "))
                            );
                    }
                    if(halfopChannels.count())
                    {
                        server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 is a halfop on channels: %2")
                            .arg(parameterList[1]).arg(halfopChannels.join(" "))
                            );
                    }
                    if(opChannels.count())
                    {
                        server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 is an operator on channels: %2")
                            .arg(parameterList[1]).arg(opChannels.join(" "))
                            );
                    }
                    if(ownerChannels.count())
                    {
                        server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 is owner of channels: %2")
                            .arg(parameterList[1]).arg(ownerChannels.join(" "))
                            );
                    }
                    if(adminChannels.count())
                    {
                        server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 is admin on channels: %2")
                            .arg(parameterList[1]).arg(adminChannels.join(" "))
                            );
                    }
                }
                break;
            }
            case RPL_WHOISSERVER:
            {
                NickInfo* nickInfo = server->getNickInfo(parameterList[1]);
                if(nickInfo)
                {
                    nickInfo->setNetServer(parameterList[2]);
                    nickInfo->setNetServerInfo(trailing);
                    // Clear the away state on assumption that if nick is away, this message will be followed
                    // by a 301 RPL_AWAY message.  Not necessary a invalid assumption, but what can we do?
                    nickInfo->setAway(false);
                    nickInfo->setAwayMessage(QString::null);
                }
                // Display message only if this was not an automatic request.
                if(getAutomaticRequest("WHOIS",parameterList[1])==0)
                {
                    server->appendMessageToFrontmost(i18n("Whois"),
                        i18n("%1 is online via %2 (%3).").arg(parameterList[1])
                        .arg(parameterList[2]).arg(trailing)
                        );
                }
                break;
            }
            case RPL_WHOISIDENTIFY:
            {
                // Display message only if this was not an automatic request.
                if(getAutomaticRequest("WHOIS",parameterList[1])==0)
                {
                    server->appendMessageToFrontmost(i18n("Whois"),
                        i18n("%1 has identified for this nick.")
                        .arg(parameterList[1])
                        );
                }
                break;
            }
            case RPL_WHOISHELPER:
            {
                // Display message only if this was not an automatic request.
                if(getAutomaticRequest("WHOIS",parameterList[1])==0)
                {
                    server->appendMessageToFrontmost(i18n("Whois"),
                        i18n("%1 is available for help.")
                        .arg(parameterList[1])
                        );
                }
                break;
            }
            case RPL_WHOISOPERATOR:
            {
                // Display message only if this was not an automatic request.
                if(getAutomaticRequest("WHOIS",parameterList[1])==0)
                {
                    server->appendMessageToFrontmost(i18n("Whois"),
                        i18n("%1 is a network admin.")
                        .arg(parameterList[1])
                        );
                }
                break;
            }
            case RPL_WHOISIDLE:
            {
                // get idle time in seconds
                long seconds=parameterList[2].toLong();
                long minutes=seconds/60;
                long hours  =minutes/60;
                long days   =hours/24;

                // if idle time is longer than a day
                // Display message only if this was not an automatic request.
                if(getAutomaticRequest("WHOIS",parameterList[1])==0)
                {
                    if(days)
                    {
                        const QString daysString = i18n("1 day", "%n days", days);
                        const QString hoursString = i18n("1 hour", "%n hours", (hours % 24));
                        const QString minutesString = i18n("1 minute", "%n minutes", (minutes % 60));
                        const QString secondsString = i18n("1 second", "%n seconds", (seconds % 60));

                        server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 = name of person, %2 = (x days), %3 = (x hours), %4 = (x minutes), %5 = (x seconds)",
                            "%1 has been idle for %2, %3, %4, and %5.")
                            .arg(parameterList[1])
                            .arg(daysString).arg(hoursString).arg(minutesString).arg(secondsString)
                            );
                        // or longer than an hour
                    }
                    else if(hours)
                    {
                        const QString hoursString = i18n("1 hour", "%n hours", hours);
                        const QString minutesString = i18n("1 minute", "%n minutes", (minutes % 60));
                        const QString secondsString = i18n("1 second", "%n seconds", (seconds % 60));
                        server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 = name of person, %2 = (x hours), %3 = (x minutes), %4 = (x seconds)",
                            "%1 has been idle for %2, %3, and %4.")
                            .arg(parameterList[1])
                            .arg(hoursString).arg(minutesString).arg(secondsString)
                            );
                        // or longer than a minute
                    }
                    else if(minutes)
                    {
                        const QString minutesString = i18n("1 minute", "%n minutes", minutes);
                        const QString secondsString = i18n("1 second", "%n seconds", (seconds % 60));
                        server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 = name of person, %2 = (x minutes), %3 = (x seconds)",
                            "%1 has been idle for %2 and %3.")
                            .arg(parameterList[1])
                            .arg(minutesString).arg(secondsString)
                            );
                        // or just some seconds
                    }
                    else
                    {
                        server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 has been idle for 1 second.", "%1 has been idle for %n seconds.", seconds)
                            .arg(parameterList[1])
                            );
                    }
                }

                if(parameterList.count()==4)
                {
                    QDateTime when;
                    when.setTime_t(parameterList[3].toUInt());
                    NickInfo* nickInfo = server->getNickInfo(parameterList[1]);
                    if(nickInfo)
                    {
                        nickInfo->setOnlineSince(when);
                    }
                    // Display message only if this was not an automatic request.
                    if(getAutomaticRequest("WHOIS",parameterList[1])==0)
                    {
                        server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 has been online since %2.")
                            .arg(parameterList[1]).arg(when.toString(Qt::LocalDate))
                            );
                    }
                }
                break;
            }
            case RPL_ENDOFWHOIS:
            {
                //NickInfo* nickInfo = server->getNickInfo(parameterList[1]);
                // Display message only if this was not an automatic request.
                if(getAutomaticRequest("WHOIS",parameterList[1])==0)
                {
                    server->appendMessageToFrontmost(i18n("Whois"),i18n("End of WHOIS list."));
                }
                // was this an automatic request?
                if(getAutomaticRequest("WHOIS",parameterList[1])!=0)
                {
                    setAutomaticRequest("WHOIS",parameterList[1],false);
                }
                break;
            }
            case RPL_USERHOST:
            {
                // iterate over all nick/masks in reply
                QStringList uhosts=QStringList::split(" ",trailing);

                for(unsigned int index=0;index<uhosts.count();index++)
                {
                    // extract nickname and hostmask from reply
                    QString nick(uhosts[index].section('=',0,0));
                    QString mask(uhosts[index].section('=',1));

                    // get away and IRC operator flags
                    bool away=(mask[0]=='-');
                    bool ircOp=(nick[nick.length()-1]=='*');

                    // cut flags from nick/hostmask
                    mask=mask.mid(1);
                    if(ircOp)
                    {
                        nick=nick.left(nick.length()-1);
                    }

                    // inform server of this user's data
                    emit userhost(nick,mask,away,ircOp);

                    // display message only if this was no automatic request
                    if(getAutomaticRequest("USERHOST",nick)==0)
                    {
                        server->appendMessageToFrontmost(i18n("Userhost"),
                            i18n("%1 = nick, %2 = shows if nick is op, %3 = hostmask, %4 = shows away", "%1%2 is %3%4.")
                            .arg(nick)
                            .arg((ircOp) ? i18n(" (IRC Operator)") : QString::null)
                            .arg(mask)
                            .arg((away) ? i18n(" (away)") : QString::null));
                    }

                    // was this an automatic request?
                    if(getAutomaticRequest("USERHOST",nick)!=0)
                    {
                        setAutomaticRequest("USERHOST",nick,false);
                    }
                }                                 // for
                break;
            }
            case RPL_LISTSTART:                   //FIXME This reply is obsolete!!!
            {
                if(getAutomaticRequest("LIST",QString::null)==0)
                {
                    server->appendMessageToFrontmost(i18n("List"),i18n("List of channels:"));
                }
                break;
            }
            case RPL_LIST:
            {
                if(getAutomaticRequest("LIST",QString::null)==0)
                {
                    QString message;
                    message=i18n("%1 (%n user): %2", "%1 (%n users): %2", parameterList[2].toInt());
                    server->appendMessageToFrontmost(i18n("List"),message.arg(parameterList[1]).arg(trailing));
                }
                else                              // send them to /LIST window
                {
                    emit addToChannelList(parameterList[1],parameterList[2].toInt(),trailing);
                }

                break;
            }
            case RPL_LISTEND:
            {
                // was this an automatic request?
                if(getAutomaticRequest("LIST",QString::null)==0)
                {
                    server->appendMessageToFrontmost(i18n("List"),i18n("End of channel list."));
                }
                else
                {
                    setAutomaticRequest("LIST",QString::null,false);
                }
                break;
            }
            case RPL_NOWAWAY:
            {
                NickInfo* nickInfo = server->getNickInfo(parameterList[0]);
                if (nickInfo) nickInfo->setAway(true);
                server->appendMessageToFrontmost(i18n("Away"),i18n("You are now marked as being away."));
                if (!server->isAway()) emit away();
                break;
            }
            case RPL_UNAWAY:
            {
                NickInfo* nickInfo = server->getNickInfo(parameterList[0]);
                if(nickInfo)
                {
                    nickInfo->setAway(false);
                    nickInfo->setAwayMessage(QString::null);
                }

                Identity identity = *(server->getIdentity());

                if(server->isAway())
                {
                    if(identity.getShowAwayMessage())
                    {
                        QString message = identity.getReturnMessage();
                        server->sendToAllChannels(message.replace(QRegExp("%t", false), server->awayTime()));
                    }
                    server->appendMessageToFrontmost(i18n("Away"),i18n("You are no longer marked as being away."));
                    emit unAway();
                }
                else
                {
                    server->appendMessageToFrontmost(i18n("Away"),i18n("You are not marked as being away."));
                }

                break;
            }
            case RPL_BANLIST:
            {
                if (getAutomaticRequest("BANLIST", parameterList[1]))
                {
                    server->addBan(parameterList[1], parameterList.join(" ").section(' ', 2, 4));
                } else {
                    QDateTime when;
                    when.setTime_t(parameterList[4].toUInt());

                    server->appendMessageToFrontmost(i18n("BanList:%1").arg(parameterList[1]), i18n("BanList message: e.g. *!*@aol.com set by MrGrim on <date>", "%1 set by %2 on %3").arg(parameterList[2]).arg(parameterList[3].section('!', 0, 0)).arg(when.toString(Qt::LocalDate)));
                }
                break;
            }
            case RPL_ENDOFBANLIST:
            {
                if (getAutomaticRequest("BANLIST", parameterList[1]))
                {
                    setAutomaticRequest("BANLIST", parameterList[1], false);
                } else {
                    server->appendMessageToFrontmost(i18n("BanList:%1").arg(parameterList[1]), i18n("End of Ban List."));
                }
                break;
            }
            case ERR_NOCHANMODES:
            {
                ChatWindow *chatwindow = server->getChannelByName(parameterList[1]);
                if(chatwindow)
                {
                    chatwindow->appendServerMessage(i18n("Channel"), trailing);
                }
                else                              // We couldn't join the channel , so print the error. with [#channel] : <Error Message>
                {
                    server->appendMessageToFrontmost(i18n("Channel"), trailing);
                }
                break;
            }
            case ERR_NOSUCHSERVER:
            {
                //Some servers don't know their name, so they return an error instead of the PING data
                if (getLagMeasuring() && trailing.startsWith(prefix))
                {
                    server->pongReceived();
                }
                break;
            }
            case ERR_UNAVAILRESOURCE:
            {
                server->appendMessageToFrontmost(i18n("Error"),i18n("%1 is currently unavailable.").arg(parameterList[1]));

                break;
            }
            case RPL_HIGHCONNECTCOUNT:
            case RPL_LUSERCLIENT:
            case RPL_LUSEROP:
            case RPL_LUSERUNKNOWN:
            case RPL_LUSERCHANNELS:
            case RPL_LUSERME:
            {
                server->appendStatusMessage(i18n("Users"), parameterList.join(" ").section(' ',1) + ' '+trailing);
                break;
            }
            case ERR_UNKNOWNCOMMAND:
            {
                server->appendMessageToFrontmost(i18n("Error"),i18n("%1: Unknown command.").arg(parameterList[1]));

                break;
            }
            case ERR_NOTREGISTERED:
            {
                server->appendMessageToFrontmost(i18n("Error"),i18n("Not registered."));

                break;
            }
            case ERR_NEEDMOREPARAMS:
            {
                server->appendMessageToFrontmost(i18n("Error"),i18n("%1: This command requires more parameters.").arg(parameterList[1]));

                break;
            }
            case RPL_CAPAB: // Special freenode reply afaik
            {
                // Disable as we don't use this for anything yet
                if(trailing.contains("IDENTIFY-MSG"))
                {
                    server->enableIdentifyMsg(true);
                    break;
                }

            /* don't break; - this is also used as RPL_DATASTR on ircu and some others */
            }
            // FALLTHROUGH to default to let the error display otherwise
            default:
            {
                // All yet unknown messages go into the frontmost window without the
                // preceding nickname
                server->appendMessageToFrontmost(command, parameterList.join(" ").section(' ',1) + ' '+trailing);
            }
        }                                         // end of numeric switch
    }
}

void IcecapInputFilter::parseModes(const QString &sourceNick, const QStringList &parameterList)
{
    const QString modestring=parameterList[1];

    bool plus=false;
    int parameterIndex=0;
    // List of modes that need a parameter (note exception with -k and -l)
    // Mode q is quiet on freenode and acts like b... if this is a channel mode on other
    //  networks then more logic is needed here. --MrGrim
    QString parameterModes="aAoOvhkbleIq";
    QString message = sourceNick + i18n(" sets mode: ") + modestring;

    for(unsigned int index=0;index<modestring.length();index++)
    {
        unsigned char mode=modestring[index];
        QString parameter;

        // Check if this is a mode or a +/- qualifier
        if(mode=='+' || mode=='-')
        {
            plus=(mode=='+');
        }
        else
        {
            // Check if this was a parameter mode
            if(parameterModes.find(mode)!=-1)
            {
                // Check if the mode actually wants a parameter. -k and -l do not!
                if(plus || (!plus && (mode!='k') && (mode!='l')))
                {
                    // Remember the mode parameter
                    parameter=parameterList[2+parameterIndex];
                    message += ' ' + parameter;
                    // Switch to next parameter
                    ++parameterIndex;
                }
            }
            // Let the channel update its modes
            if(parameter.isEmpty())               // XXX Check this to ensure the braces are in the correct place
            {
                kdDebug()   << "in updateChannelMode.  sourceNick: '" << sourceNick << "'  parameterlist: '"
                    << parameterList.join(", ") << "'"
                    << endl;
            }
            server->updateChannelMode(sourceNick,parameterList[0],mode,plus,parameter);
        }
    }                                             // endfor

    if (Preferences::useLiteralModes())
    {
        server->appendCommandMessageToChannel(parameterList[0],i18n("Mode"),message);
    }
}

// # & + and ! are *often*, but not necessarily, Channel identifiers. + and ! are non-RFC,
// so if a server doesn't offer 005 and supports + and ! channels, I think thats broken behaviour
// on their part - not ours. --Argonel
bool IcecapInputFilter::isAChannel(const QString &check)
{
    Q_ASSERT(server);
    // if we ever see the assert, we need the ternary
    return server? server->isAChannel(check) : QString("#&").contains(check.at(0));
}

bool IcecapInputFilter::isIgnore(const QString &sender, Ignore::Type type)
{
    bool doIgnore = false;

    QPtrList<Ignore> list = Preferences::ignoreList();

    for(unsigned int index =0; index<list.count(); index++)
    {
        Ignore* item = list.at(index);
        QRegExp ignoreItem(QRegExp::escape(item->getName()).replace("\\*", "(.*)"),false);
        if (ignoreItem.exactMatch(sender) && (item->getFlags() & type))
            doIgnore = true;
        if (ignoreItem.exactMatch(sender) && (item->getFlags() & Ignore::Exception))
            return false;
    }

    return doIgnore;
}

void IcecapInputFilter::reset()
{
    automaticRequest.clear();
    whoRequestList.clear();
}

void IcecapInputFilter::setAutomaticRequest(const QString& command, const QString& name, bool yes)
{
    automaticRequest[command][name.lower()] += (yes) ? 1 : -1;
    if(automaticRequest[command][name.lower()]<0)
    {
        kdDebug()   << "IcecapInputFilter::automaticRequest( " << command << ", " << name
            << " ) was negative! Resetting!"
            << endl;
        automaticRequest[command][name.lower()]=0;
    }
}

int IcecapInputFilter::getAutomaticRequest(const QString& command, const QString& name)
{
    return automaticRequest[command][name.lower()];
}

void IcecapInputFilter::addWhoRequest(const QString& name) { whoRequestList << name.lower(); }

bool IcecapInputFilter::isWhoRequestUnderProcess(const QString& name) { return (whoRequestList.contains(name.lower())>0); }

void IcecapInputFilter::setLagMeasuring(bool state) { lagMeasuring=state; }

bool IcecapInputFilter::getLagMeasuring()           { return lagMeasuring; }

#include "icecapinputfilter.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
