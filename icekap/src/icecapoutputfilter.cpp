/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2005 Peter Simonsson <psn@linux.se>
  Copyright (C) 2005 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2005 Eike Hein <sho@eikehein.com>
*/

#include <qstringlist.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>
#include <qmap.h>
#include <qvaluelist.h>
#include <qtextcodec.h>

#include <klocale.h>
#include <kdebug.h>
#include <kio/passdlg.h>
#include <kconfig.h>
#include <kdeversion.h>
#include <kshell.h>

#include <ksocketaddress.h>
#include <kresolver.h>
#include <kreverseresolver.h>

#include "icecapoutputfilter.h"
#include "konversationapplication.h"
#include "konversationmainwindow.h"
#include "ignore.h"
#include "icecapserver.h"
#include "irccharsets.h"
#include "linkaddressbook/addressbook.h"
#include "konviiphelper.h"

#include "query.h"

namespace Konversation
{
    IcecapOutputFilter::IcecapOutputFilter(IcecapServer* server)
        : QObject(server)
    {
        m_server = server;
    }

    IcecapOutputFilter::~IcecapOutputFilter()
    {
    }

    // replace all aliases in the string and return true if anything got replaced at all
    bool IcecapOutputFilter::replaceAliases(QString& line)
    {
        QStringList aliasList=Preferences::aliasList();
        QString cc(Preferences::commandChar());
        // check if the line starts with a defined alias
        for(unsigned int index=0;index<aliasList.count();index++)
        {
            // cut alias pattern from definition
            QString aliasPattern(aliasList[index].section(' ',0,0));
            // cut first word from command line, so we do not wrongly find an alias
            // that starts with the same letters, like /m would override /me
            QString lineStart=line.section(' ',0,0);

            // pattern found?
            // TODO: cc may be a regexp character here ... we should escape it then
            if (lineStart==cc+aliasPattern)
            {
                QString aliasReplace;

                // cut alias replacement from definition
                if ( aliasList[index].contains("%p") )
                    aliasReplace = aliasList[index].section(' ',1);
                else
                    aliasReplace = aliasList[index].section(' ',1 )+' '+line.section(' ',1 );

                // protect "%%"
                aliasReplace.replace("%%","%\x01");
                // replace %p placeholder with rest of line
                aliasReplace.replace("%p",line.section(' ',1));
                // restore "%<1>" as "%%"
                aliasReplace.replace("%\x01","%%");
                // modify line
                line=aliasReplace;
                // return "replaced"
                return true;
            }
        }                                         // for

        return false;
    }

    QStringList IcecapOutputFilter::splitForEncoding(const QString& inputLine, int MAX)
    {
        QString channelCodecName=Preferences::channelEncoding(m_server->getServerGroup(), destination);

        int sublen=0; //The encoded length since the last split
        int charLength=0; //the length of this char

        //MAX= 7; // for testing

        //FIXME should we run this through the encoder first, checking with "canEncode"?
        QString text=inputLine; // the text we'll send, currently in Unicode

        QStringList finals; // The strings we're going to output

        QChar   *c=(QChar*)text.unicode(),  // Pointer to the character we're looking at
                *end=c+text.length();       // If it were a char*, it would be pointing at the \0;

        QChar   *frag=c,                    // The beginning of this fragment
                *lastSBC=c,                 // The last single byte char we saw in case we have to chop
                *lastSpace=c;               // The last space we saw
        bool sbcGood = 0;                   // We can't trust that a single byte char was ever seen

        //Get the codec we're supposed to use. This must not fail. (not verified)
        QTextCodec* codec;

        // I copied this bit straight out of Server::send
        if (channelCodecName.isEmpty())
        {
            codec = m_server->getIdentity()->getCodec();
        }
        else
        {
            // FIXME Doesn't this just return `getIdentity()->getCodec();` if no codec set?
            codec = Konversation::IRCCharsets::self()->codecForName(channelCodecName);
        }

        Q_ASSERT(codec);

        while (c<end)
        {
            // The most important bit - turn the current char into a QCString so we can measure it
            QCString ch=codec->fromUnicode(*c);
            charLength=ch.length();

            // If adding this char puts us over the limit:
            if (charLength+sublen > MAX)
            {
                // If lastSpace isn't pointing to a space, we have to chop
                if ( !lastSpace->isSpace() ) //used in case we end up supporting unicode spaces
                {
                    // FIXME This is only theory, it might not work
                    if (sbcGood) // Copy up to and including the last SBC.
                    {
                        QString curs(frag, (++lastSBC)-frag); // Since there is a continue below, safe to increment here
                        finals+=curs;
                        lastSpace=frag=c=lastSBC;
                    }
                    else //Split right here
                    {
                        QString curs(frag, c-frag); // Don't include c
                        finals+=curs;
                        lastSpace=frag=c; //we need to see this char again, to collect its stats, so no increment
                    }
                }
                else // Most common case, we saw a space we can split at
                {
                    // Copy the current substring, but not the space (unlike the SBC case)
                    QString curs(frag, lastSpace-frag);
                    finals+=curs;

                    // Rewind to the last good splitpoint, dropping the space character as
                    //  it was technically replaced with a \n
                    frag=c=++lastSpace; //pre-increment
                }

                sbcGood=false;
                // Since c always gets reset to the splitpoint, sublen gets recalculated
                sublen=0;
                continue; // SLAM!!!
            }
            else if (*c==' ') // If we want to support typographic spaces, change this to c->isSpace(), and rewrite above
            {
                lastSpace=c;
            }
            // Won't get here if the string was split
            if (charLength==1)
            {
                sbcGood=1;
                lastSBC=c;
            }
            sublen+=charLength;
            ++c;
        }//wend
        //so when we hit this point, frag to end is all thats left
        Q_ASSERT((frag!=c));
        if (frag != c)
        {
            QString curs(frag, c-frag);
            finals+=curs;
        }
        return finals;
    }

    OutputFilterResult IcecapOutputFilter::parse(const QString& myNick,const QString& originalLine,const QString& name)
    {
        setCommandChar();

        OutputFilterResult result;
        destination=name;

        QString inputLine(originalLine);

        if(inputLine.isEmpty() || inputLine == "\n")
            return result;

        //Protect against nickserv auth being sent as a message on the off chance
        // someone didn't notice leading spaces
        {
            QString testNickServ( inputLine.stripWhiteSpace() );
            if(testNickServ.startsWith(commandChar+"nickserv", false)
              || testNickServ.startsWith(commandChar+"ns", false))
            {
                    inputLine = testNickServ;
            }
        }

        if(!Preferences::disableExpansion())
        {
            // replace placeholders
            inputLine.replace("%%","%\x01");      // make sure to protect double %%
            inputLine.replace("%B","\x02");       // replace %B with bold char
            inputLine.replace("%C","\x03");       // replace %C with color char
            inputLine.replace("%G","\x07");       // replace %G with ASCII BEL 0x07
            inputLine.replace("%I","\x09");       // replace %I with italics char
            inputLine.replace("%O","\x0f");       // replace %O with reset to default char
            inputLine.replace("%S","\x13");       // replace %S with strikethru char
            //  inputLine.replace(QRegExp("%?"),"\x15");
            inputLine.replace("%R","\x16");       // replace %R with reverse char
            inputLine.replace("%U","\x1f");       // replace %U with underline char
            inputLine.replace("%\x01","%");       // restore double %% as single %
        }

        QString line=inputLine.lower();

        // Action?
        if(line.startsWith(commandChar+"me ") && !destination.isEmpty())
        {
            result.toServer = "PRIVMSG " + name + " :" + '\x01' + "ACTION " + inputLine.mid(4) + '\x01';
            result.output = inputLine.mid(4);
            result.type = Action;
        }
        // Convert double command chars at the beginning to single ones
        else if(line.startsWith(commandChar+commandChar) && !destination.isEmpty())
        {
            inputLine=inputLine.mid(1);
            goto BYPASS_COMMAND_PARSING;
        }
        // Server command?
        else if(line.startsWith(commandChar))
        {
            QString command = inputLine.section(' ', 0, 0).mid(1).lower();
            QString parameter = inputLine.section(' ', 1);

            if (command !="topic")
                parameter = parameter.stripWhiteSpace();

            if     (command == "join")     result = parseJoin(parameter);
            else if(command == "part")     result = parsePart(parameter);
            else if(command == "leave")    result = parsePart(parameter);
            else if(command == "quit")     result = parseQuit(parameter);
            else if(command == "notice")   result = parseNotice(parameter);
            else if(command == "j")        result = parseJoin(parameter);
            else if(command == "msg")      result = parseMsg(myNick,parameter, false);
            else if(command == "m")        result = parseMsg(myNick,parameter, false);
            else if(command == "smsg")     result = parseSMsg(parameter);
            else if(command == "query")    result = parseMsg(myNick,parameter, true);
            else if(command == "op")       result = parseOp(parameter);
            else if(command == "deop")     result = parseDeop(myNick,parameter);
            else if(command == "hop")      result = parseHop(parameter);
            else if(command == "dehop")    result = parseDehop(myNick,parameter);
            else if(command == "voice")    result = parseVoice(parameter);
            else if(command == "unvoice")  result = parseUnvoice(myNick,parameter);
            else if(command == "devoice")  result = parseUnvoice(myNick,parameter);
            else if(command == "ctcp")     result = parseCtcp(parameter);
            else if(command == "ping")     result = parseCtcp(parameter.section(' ', 0, 0) + " ping");
            else if(command == "kick")     result = parseKick(parameter);
            else if(command == "topic")    result = parseTopic(parameter);
            else if(command == "away")     result = parseAway(parameter);
            else if(command == "unaway")   result = parseBack();
            else if(command == "back")     result = parseBack();
            else if(command == "invite")   result = parseInvite(parameter);
            else if(command == "exec")     result = parseExec(parameter);
            else if(command == "notify")   result = parseNotify(parameter);
            else if(command == "oper")     result = parseOper(myNick,parameter);
            else if(command == "ban")      result = parseBan(parameter);
            else if(command == "unban")    result = parseUnban(parameter);
            else if(command == "kickban")  result = parseBan(parameter,true);
            else if(command == "ignore")   result = parseIgnore(parameter);
            else if(command == "unignore") result = parseUnignore(parameter);
            else if(command == "quote")    result = parseQuote(parameter);
            else if(command == "say")      result = parseSay(parameter);
            else if(command == "list")     result = parseList(parameter);
            else if(command == "names")    result = parseNames(parameter);
            else if(command == "raw")      result = parseRaw(parameter);
            else if(command == "dcc")      result = parseDcc(parameter);
            else if(command == "konsole")  parseKonsole();
            else if(command == "aaway")    parseAaway(parameter);
            else if(command == "aback")    emit multiServerCommand("back", QString::null);
            else if(command == "ame")      result = parseAme(parameter);
            else if(command == "amsg")     result = parseAmsg(parameter);
            else if(command == "omsg")     result = parseOmsg(parameter);
            else if(command == "onotice")  result = parseOnotice(parameter);
            else if(command == "server")   parseServer(parameter);
            else if(command == "reconnect")  emit reconnectServer();
            else if(command == "disconnect") emit disconnectServer();
            else if(command == "prefs")    result = parsePrefs(parameter);
            else if(command == "charset")  parseCharset(parameter);
            else if(command == "setkey")   result = parseSetKey(parameter);
            else if(command == "delkey")   result = parseDelKey(parameter);
            else if(command == "dns")      result = parseDNS(parameter);

            // Forward unknown commands to server
            else
            {
                result.toServer = inputLine.mid(1);
                result.type = Message;
            }
        }
        // Ordinary message to channel/query?
        else if(!destination.isEmpty())
        {
            BYPASS_COMMAND_PARSING:

            QStringList outputList=splitForEncoding(inputLine, m_server->getPreLength("PRIVMSG", destination));
            if (outputList.count() > 1)
            {
                result.output=QString();
                result.outputList=outputList;
                for ( QStringList::Iterator it = outputList.begin(); it != outputList.end(); ++it )
                {
                    result.toServerList += "PRIVMSG " + destination + " :" + *it;
                }
            }
            else
            {
                result.output = inputLine;
                result.toServer = "PRIVMSG " + destination + " :" + inputLine;
            }

            result.type = Message;
        }
        // Eveything else goes to the server unchanged
        else
        {
            result.toServer = inputLine;
            result.output = inputLine;
            result.typeString = i18n("Raw");
            result.type = Program;
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseOp(const QString &parameter)
    {
        return changeMode(parameter,'o','+');
    }

    OutputFilterResult IcecapOutputFilter::parseDeop(const QString &ownNick, const QString &parameter)
    {
        return changeMode(addNickToEmptyNickList(ownNick,parameter),'o','-');
    }

    OutputFilterResult IcecapOutputFilter::parseHop(const QString &parameter)
    {
        return changeMode(parameter, 'h', '+');
    }

    OutputFilterResult IcecapOutputFilter::parseDehop(const QString &ownNick, const QString &parameter)
    {
        return changeMode(addNickToEmptyNickList(ownNick,parameter), 'h', '-');
    }

    OutputFilterResult IcecapOutputFilter::parseVoice(const QString &parameter)
    {
        return changeMode(parameter,'v','+');
    }

    OutputFilterResult IcecapOutputFilter::parseUnvoice(const QString &ownNick, const QString &parameter)
    {
        return changeMode(addNickToEmptyNickList(ownNick,parameter),'v','-');
    }

    OutputFilterResult IcecapOutputFilter::parseJoin(QString& channelName)
    {
        OutputFilterResult result;

        if(channelName.contains(",")) // Protect against #foo,0 tricks
            channelName = channelName.remove(",0");
        //else if(channelName == "0") // FIXME IRC RFC 2812 section 3.2.1

        if (channelName.isEmpty())
        {
            result = usage(i18n("Usage: %1JOIN <channel> [password]").arg(commandChar));
        }
        else
        {
            if(!isAChannel(channelName))
                result.toServer = "JOIN #" + channelName.stripWhiteSpace();
            else
                result.toServer = "JOIN " + channelName;
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseKick(const QString &parameter)
    {
        OutputFilterResult result;

        if(isAChannel(destination))
        {
            // get nick to kick
            QString victim = parameter.left(parameter.find(" "));

            if(victim.isEmpty())
            {
                result = usage(i18n("Usage: %1KICK <nick> [reason]").arg(commandChar));
            }
            else
            {
                // get kick reason (if any)
                QString reason = parameter.mid(victim.length() + 1);

                // if no reason given, take default reason
                if(reason.isEmpty())
                {
                    reason = m_server->getIdentity()->getKickReason();
                }

                result.toServer = "KICK " + destination + ' ' + victim + " :" + reason;
            }
        }
        else
        {
            result = error(i18n("%1KICK only works from within channels.").arg(commandChar));
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parsePart(const QString &parameter)
    {
        OutputFilterResult result;

        // No parameter, try default part message
        if(parameter.isEmpty())
        {
            // But only if we actually are in a channel
            if(isAChannel(destination))
            {
                result.toServer = "PART " + destination + " :" + m_server->getIdentity()->getPartReason();
            }
            else
            {
                result = error(i18n("%1PART without parameters only works from within a channel or a query.").arg(commandChar));
            }
        }
        else
        {
            // part a given channel
            if(isAChannel(parameter))
            {
                // get channel name
                QString channel = parameter.left(parameter.find(" "));
                // get part reason (if any)
                QString reason = parameter.mid(channel.length() + 1);

                // if no reason given, take default reason
                if(reason.isEmpty())
                {
                    reason = m_server->getIdentity()->getPartReason();
                }

                result.toServer = "PART " + channel + " :" + reason;
            }
            // part this channel with a given reason
            else
            {
                if(isAChannel(destination))
                {
                    result.toServer = "PART " + destination + " :" + parameter;
                }
                else
                {
                    result = error(i18n("%1PART without channel name only works from within a channel.").arg(commandChar));
                }
            }
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseTopic(const QString &parameter)
    {
        OutputFilterResult result;

        // No parameter, try to get current topic
        if(parameter.isEmpty())
        {
            // But only if we actually are in a channel
            if(isAChannel(destination))
            {
                result.toServer = "TOPIC " + destination;
            }
            else
            {
                result = error(i18n("%1TOPIC without parameters only works from within a channel.").arg(commandChar));
            }
        }
        else
        {
            // retrieve or set topic of a given channel
            if(isAChannel(parameter))
            {
                // get channel name
                QString channel=parameter.left(parameter.find(" "));
                // get topic (if any)
                QString topic=parameter.mid(channel.length()+1);
                // if no topic given, retrieve topic
                if(topic.isEmpty())
                {
                    m_server->requestTopic(channel);
                }
                // otherwise set topic there
                else
                {
                    result.toServer = "TOPIC " + channel + " :";
                    //If we get passed a ^A as a topic its a sign we should clear the topic.
                    //Used to be a \n, but those get smashed by QStringList::split and readded later
                    //Now is not the time to fight with that. FIXME
                    //If anyone out there *can* actually set the topic to a single ^A, now they have to
                    //specify it twice to get one.
                    if (topic =="\x01\x01")
                        result.toServer += '\x01';
                    else if (topic!="\x01")
                        result.toServer += topic;

                }
            }
            // set this channel's topic
            else
            {
                if(isAChannel(destination))
                {
                    result.toServer = "TOPIC " + destination + " :" + parameter;
                }
                else
                {
                    result = error(i18n("%1TOPIC without channel name only works from within a channel.").arg(commandChar));
                }
            }
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseAway(QString &reason)
    {
        OutputFilterResult result;

        if (reason.isEmpty())
            reason = i18n("Gone away for now.");

        if (m_server->getIdentity()->getShowAwayMessage())
        {
            QString message = m_server->getIdentity()->getAwayMessage();
            emit sendToAllChannels(message.replace(QRegExp("%s",false),reason));
        }

        m_server->setAwayReason(reason);
        result.toServer = "AWAY :" + reason;

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseBack()
    {
        OutputFilterResult result;
        result.toServer = "AWAY";
        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseNames(const QString &parameter)
    {
        OutputFilterResult result;
        result.toServer = "NAMES ";
        if (parameter.isNull())
        {
            return error(i18n("%1NAMES with no target may disconnect you from the server. Specify '*' if you really want this.").arg(commandChar));
        }
        else if (parameter != QChar('*'))
        {
            result.toServer.append(parameter);
        }
        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseQuit(const QString &reason)
    {
        OutputFilterResult result;

        result.toServer = "QUIT :";
        // if no reason given, take default reason
        if(reason.isEmpty())
            result.toServer += m_server->getIdentity()->getPartReason();
        else
            result.toServer += reason;

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseNotice(const QString &parameter)
    {
        OutputFilterResult result;
        QString recipient = parameter.left(parameter.find(" "));
        QString message = parameter.mid(recipient.length()+1);

        if(parameter.isEmpty() || message.isEmpty())
        {
            result = usage(i18n("Usage: %1NOTICE <recipient> <message>").arg(commandChar));
        }
        else
        {
            result.typeString = i18n("Notice");
            result.toServer = "NOTICE " + recipient + " :" + message;
            result.output=i18n("%1 is the message, %2 the recipient nickname","Sending notice \"%2\" to %1.").arg(recipient).arg(message);
            result.type = Program;
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseMsg(const QString &myNick, const QString &parameter, bool isQuery)
    {
        OutputFilterResult result;
        QString recipient = parameter.section(" ", 0, 0, QString::SectionSkipEmpty);
        QString message = parameter.section(" ", 1);
        QString output;

        if(recipient.isEmpty())
        {
            result = error("Error: You need to specify a recipient.");
            return result;
        }

        if(message.stripWhiteSpace().isEmpty())
        {
            //empty result - we don't want to send any message to the server
        }
        else if(message.startsWith(commandChar+"me"))
        {
            result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "ACTION " + message.mid(4) + '\x01';
            output = QString("* %1 %2").arg(myNick).arg(message.mid(4));
        }
        else
        {
            result.toServer = "PRIVMSG " + recipient + " :" + message;
            output = message;
        }

        ::Query *query;

        if(isQuery || output.isEmpty())
        {
            //if this is a /query, always open a query window.
            //treat "/msg nick" as "/query nick"

            //Note we have to be a bit careful here.
            //We only want to create ('obtain') a new nickinfo if we have done /query
            //or "/msg nick".  Not "/msg nick message".
            NickInfoPtr nickInfo = m_server->obtainNickInfo(recipient);
            query = m_server->addQuery(nickInfo, true /*we initiated*/);
            //force focus if the user did not specify any message
            if (output.isEmpty()) emit showView(query);
        }
        else
        {
            //We have  "/msg nick message"
            query = m_server->getQueryByName(recipient);
        }

        if(query && !output.isEmpty())
        {
            if(message.startsWith(commandChar+"me"))
                                                  //log if and only if the query open
                query->appendAction(m_server->getNickname(), message.mid(4));
            else
                                                  //log if and only if the query open
                query->appendQuery(m_server->getNickname(), output);
        }

        if(output.isEmpty()) return result;       //result should be completely empty;
        //FIXME - don't do below line if query is focused
        result.output = output;
        result.typeString= "-> " + recipient;
        result.type = PrivateMessage;
        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseSMsg(const QString &parameter)
    {
        OutputFilterResult result;
        QString recipient = parameter.left(parameter.find(" "));
        QString message = parameter.mid(recipient.length() + 1);

        if(message.startsWith(commandChar + "me"))
        {
            result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "ACTION " + message.mid(4) + '\x01';
        }
        else
        {
            result.toServer = "PRIVMSG " + recipient + " :" + message;
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseCtcp(const QString &parameter)
    {
        OutputFilterResult result;
                                                  // who is the recipient?
        QString recipient = parameter.section(' ', 0, 0);
                                                  // what is the first word of the ctcp?
        QString request = parameter.section(' ', 1, 1, QString::SectionSkipEmpty);
                                                  // what is the complete ctcp command?
        QString message = parameter.section(' ', 1, 0xffffff, QString::SectionSkipEmpty);

        if(request.lower() == "ping")
        {
            unsigned int time_t = QDateTime::currentDateTime().toTime_t();
            result.toServer = QString("PRIVMSG %1 :\x01PING %2\x01").arg(recipient).arg(time_t);
            result.output = i18n("Sending CTCP-%1 request to %2.").arg("PING").arg(recipient);
        }
        else
        {
            result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + message + '\x01';
            result.output = i18n("Sending CTCP-%1 request to %2.").arg(message).arg(recipient);
        }

        result.typeString = i18n("CTCP");
        result.type = Program;
        return result;
    }

    OutputFilterResult IcecapOutputFilter::changeMode(const QString &parameter,char mode,char giveTake)
    {
        OutputFilterResult result;
        // TODO: Make sure this works with +l <limit> and +k <password> also!
        QString token;
        QString tmpToken;
        QStringList nickList = QStringList::split(' ', parameter);

        if(nickList.count())
        {
            // Check if the user specified a channel
            if(isAChannel(nickList[0]))
            {
                token = "MODE " + nickList[0];
                // remove the first element
                nickList.remove(nickList.begin());
            }
            // Add default destination if it is a channel
            else if(isAChannel(destination))
            {
                token = "MODE " + destination;
            }

            // Only continue if there was no error
            if(token.length())
            {
                unsigned int modeCount = nickList.count();
                QString modes;
                modes.fill(mode, modeCount);

                token += QString(" ") + QChar(giveTake) + modes;
                tmpToken = token;

                for(unsigned int index = 0; index < modeCount; index++)
                {
                    if((index % 3) == 0)
                    {
                        result.toServerList.append(token);
                        token = tmpToken;
                    }
                    token += ' ' + nickList[index];
                }

                if(token != tmpToken)
                {
                    result.toServerList.append(token);
                }
            }
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseDcc(const QString &parameter)
    {
        OutputFilterResult result;

        // No parameter, just open DCC panel
        if(parameter.isEmpty())
        {
            emit addDccPanel();
        }
        else
        {
            QString tmpParameter = parameter;
            QStringList parameterList = QStringList::split(' ', tmpParameter.replace("\\ ", "%20"));

            QString dccType = parameterList[0].lower();

            if(dccType=="close")
            {
                emit closeDccPanel();
            }
            else if(dccType=="send")
            {
                if(parameterList.count()==1)      // DCC SEND
                {
                    emit requestDccSend();
                }                                 // DCC SEND <nickname>
                else if(parameterList.count()==2)
                {
                    emit requestDccSend(parameterList[1]);
                }                                 // DCC SEND <nickname> <file> [file] ...
                else if(parameterList.count()>2)
                {
                    // TODO: make sure this will work:
                    //output=i18n("Usage: %1DCC SEND nickname [fi6lename] [filename] ...").arg(commandChar);
                    KURL fileURL(parameterList[2]);

                    //We could easily check if the remote file exists, but then we might
                    //end up asking for creditionals twice, so settle for only checking locally
                    if(!fileURL.isLocalFile() || QFile::exists( fileURL.path() ))
                    {
                        emit openDccSend(parameterList[1],fileURL);
                    }
                    else
                    {
                        result = error(i18n("File \"%1\" does not exist.").arg(parameterList[2]));
                    }
                }
                else                              // Don't know how this should happen, but ...
                {
                    result = usage(i18n("Usage: %1DCC [SEND nickname filename]").arg(commandChar));
                }
            }
            // TODO: DCC Chat etc. comes here
            else if(dccType=="chat")
            {
                if(parameterList.count()==2)
                {
                    emit requestDccChat(parameterList[1]);
                }
                else
                {
                    result = usage(i18n("Usage: %1DCC [CHAT nickname]").arg(commandChar));
                }
            }
            else
            {
                result = error(i18n("Unrecognized command %1DCC %2. Possible commands are SEND, CHAT, CLOSE.").arg(commandChar).arg(parameterList[0]));
            }
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::sendRequest(const QString &recipient,const QString &fileName,const QString &address,const QString &port,unsigned long size)
    {
        OutputFilterResult result;
        QString niftyFileName(fileName);
        /*QFile file(fileName);
        QFileInfo info(file);*/

        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC SEND "
            + fileName
            + ' ' + address + ' ' + port + ' ' + QString::number(size) + '\x01';

        // Dirty hack to avoid printing ""name with spaces.ext"" instead of "name with spaces.ext"
        if ((fileName.startsWith("\"")) && (fileName.endsWith("\"")))
            niftyFileName = fileName.mid(1, fileName.length()-2);

        return result;
    }

    // Accepting Resume Request
    OutputFilterResult IcecapOutputFilter::acceptRequest(const QString &recipient,const QString &fileName,const QString &port,int startAt)
    {
        QString niftyFileName(fileName);

        OutputFilterResult result;
        result.toServer = "PRIVMSG " + recipient + " :" + '\x01' + "DCC ACCEPT " + fileName + ' ' + port
            + ' ' + QString::number(startAt) + '\x01';

        // Dirty hack to avoid printing ""name with spaces.ext"" instead of "name with spaces.ext"
        if ((fileName.startsWith("\"")) && (fileName.endsWith("\"")))
            niftyFileName = fileName.mid(1, fileName.length()-2);

        return result;
    }

    OutputFilterResult IcecapOutputFilter::resumeRequest(const QString &sender,const QString &fileName,const QString &port,KIO::filesize_t startAt)
    {
        QString niftyFileName(fileName);

        OutputFilterResult result;
        /*QString newFileName(fileName);
        newFileName.replace(" ", "_");*/
        result.toServer = "PRIVMSG " + sender + " :" + '\x01' + "DCC RESUME " + fileName + ' ' + port + ' '
            + QString::number(startAt) + '\x01';

        // Dirty hack to avoid printing ""name with spaces.ext"" instead of "name with spaces.ext"
        if ((fileName.startsWith("\"")) && (fileName.endsWith("\"")))
            niftyFileName = fileName.mid(1, fileName.length()-2);

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseInvite(const QString &parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1INVITE <nick> [channel]").arg(commandChar));
        }
        else
        {
            QString nick = parameter.section(' ', 0, 0, QString::SectionSkipEmpty);
            QString channel = parameter.section(' ', 1, 1, QString::SectionSkipEmpty);

            if(channel.isEmpty())
            {
                if(isAChannel(destination))
                {
                    channel = destination;
                }
                else
                {
                    result = error(i18n("%1INVITE without channel name works only from within channels.").arg(commandChar));
                }
            }

            if(!channel.isEmpty())
            {
                if(isAChannel(channel))
                {
                    result.toServer = "INVITE " + nick + ' ' + channel;
                }
                else
                {
                    result = error(i18n("%1 is not a channel.").arg(channel));
                }
            }
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseExec(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1EXEC <script> [parameter list]").arg(commandChar));
        }
        else
        {
            QStringList parameterList = QStringList::split(' ', parameter);

            if(parameterList[0].find("../") == -1)
            {
                emit launchScript(destination, parameter);
            }
            else
            {
                result = error(i18n("Script name may not contain \"../\"!"));
            }
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseRaw(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty() || parameter == "open")
        {
            emit openRawLog(true);
        }
        else if(parameter == "close")
        {
            emit closeRawLog();
        }
        else
        {
            result = usage(i18n("Usage: %1RAW [OPEN | CLOSE]").arg(commandChar));
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseNotify(const QString& parameter)
    {
        OutputFilterResult result;

        QString groupName = m_server->getServerGroup();
        int serverGroupId = m_server->serverGroupSettings()->id();

        if (!parameter.isEmpty())
        {
            QStringList list = QStringList::split(' ', parameter);

            for(unsigned int index = 0; index < list.count(); index++)
            {
                // Try to remove current pattern
                if(!Preferences::removeNotify(groupName, list[index]))
                {
                    // If remove failed, try to add it instead
                    if(!Preferences::addNotify(serverGroupId, list[index]))
                    {
                        kdDebug() << "IcecapOutputFilter::parseNotify(): Adding failed!" << endl;
                    }
                }
            }                                     // endfor
        }

        // show (new) notify list to user
        QString list = Preferences::notifyStringByGroupName(groupName) + ' ' + Konversation::Addressbook::self()->allContactsNicksForServer(m_server->getServerName(), m_server->getServerGroup()).join(" ");

        result.typeString = i18n("Notify");

        if(list.isEmpty())
            result.output = i18n("Current notify list is empty.");
        else
            result.output = i18n("Current notify list: %1").arg(list);

        result.type = Program;
        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseOper(const QString& myNick,const QString& parameter)
    {
        OutputFilterResult result;
        QStringList parameterList = QStringList::split(' ', parameter);

        if(parameter.isEmpty() || parameterList.count() == 1)
        {
            QString nick((parameterList.count() == 1) ? parameterList[0] : myNick);
            QString password;
            bool keep = false;

            int ret = KIO::PasswordDialog::getNameAndPassword
                (
                nick,
                password,
                &keep,
                i18n("Enter user name and password for IRC operator privileges:"),
                false,
                i18n("IRC Operator Password")
                );

            if(ret == KIO::PasswordDialog::Accepted)
            {
                result.toServer = "OPER " + nick + ' ' + password;
            }
        }
        else
        {
            result.toServer = "OPER " + parameter;
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseBan(const QString& parameter, bool kick)
    {
        OutputFilterResult result;
        // assume incorrect syntax first
        bool showUsage = true;

        if(!parameter.isEmpty())
        {
            QStringList parameterList=QStringList::split(' ',parameter);
            QString channel;
            QString option;
            // check for option
            bool host = (parameterList[0].lower() == "-host");
            bool domain = (parameterList[0].lower() == "-domain");
            bool uhost = (parameterList[0].lower() == "-userhost");
            bool udomain = (parameterList[0].lower() == "-userdomain");

            // remove possible option
            if (host || domain || uhost || udomain)
            {
                option = parameterList[0].mid(1);
                parameterList.pop_front();
            }

            // look for channel / ban mask
            if (parameterList.count())
            {
                // user specified channel
                if (isAChannel(parameterList[0]))
                {
                    channel = parameterList[0];
                    parameterList.pop_front();
                }
                // no channel, so assume current destination as channel
                else if (isAChannel(destination))
                    channel = destination;
                else
                {
                    // destination is no channel => error
                    if (!kick)
                        result = error(i18n("%1BAN without channel name works only from inside a channel.").arg(commandChar));
                    else
                        result = error(i18n("%1KICKBAN without channel name works only from inside a channel.").arg(commandChar));

                    // no usage information after error
                    showUsage = false;
                }
                // signal server to ban this user if all went fine
                if (!channel.isEmpty())
                {
                    if (kick)
                    {
                        QString victim = parameterList[0];
                        parameterList.pop_front();

                        QString reason = parameterList.join(" ");

                        result.toServer = "KICK " + channel + ' ' + victim + " :" + reason;

                        emit banUsers(QStringList(victim),channel,option);
                    }
                    else
                    {
                        emit banUsers(parameterList,channel,option);
                    }

                    // syntax was correct, so reset flag
                    showUsage = false;
                }
            }
        }

        if (showUsage)
        {
            if (!kick)
                result = usage(i18n("Usage: %1BAN [-HOST | -DOMAIN] [channel] <user|mask>").arg(commandChar));
            else
                result = usage(i18n("Usage: %1KICKBAN [-HOST | -DOMAIN] [channel] <user|mask> [reason]").arg(commandChar));
        }

        return result;
    }

    // finally set the ban
    OutputFilterResult IcecapOutputFilter::execBan(const QString& mask,const QString& channel)
    {
        OutputFilterResult result;
        result.toServer = "MODE " + channel + " +b " + mask;
        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseUnban(const QString& parameter)
    {
        OutputFilterResult result;
        // assume incorrect syntax first
        bool showUsage=true;

        if(!parameter.isEmpty())
        {
            QStringList parameterList = QStringList::split(' ', parameter);
            QString channel;
            QString mask;

            // if the user specified a channel
            if(isAChannel(parameterList[0]))
            {
                // get channel
                channel = parameterList[0];
                // remove channel from parameter list
                parameterList.pop_front();
            }
            // otherwise the current destination must be a channel
            else if(isAChannel(destination))
                channel = destination;
            else
            {
                // destination is no channel => error
                result = error(i18n("%1UNBAN without channel name works only from inside a channel.").arg(commandChar));
                // no usage information after error
                showUsage = false;
            }
            // if all went good, signal server to unban this mask
            if(!channel.isEmpty())
            {
                emit unbanUsers(parameterList[0], channel);
                // syntax was correct, so reset flag
                showUsage = false;
            }
        }

        if(showUsage)
        {
            result = usage(i18n("Usage: %1UNBAN [channel] pattern").arg(commandChar));
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::execUnban(const QString& mask,const QString& channel)
    {
        OutputFilterResult result;
        result.toServer = "MODE " + channel + " -b " + mask;
        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseIgnore(const QString& parameter)
    {
        OutputFilterResult result;
        // assume incorrect syntax first
        bool showUsage = true;

        // did the user give parameters at all?
        if(!parameter.isEmpty())
        {
            QStringList parameterList = QStringList::split(' ', parameter);

            // if nothing else said, only ignore channels and queries
            int value = Ignore::Channel | Ignore::Query;

            // user specified -all option
            if(parameterList[0].lower() == "-all")
            {
                // ignore everything
                value = Ignore::All;
                parameterList.pop_front();
            }

            // were there enough parameters?
            if(parameterList.count() >= 1)
            {
                for(unsigned int index=0;index<parameterList.count();index++)
                {
                    if(!parameterList[index].contains('!'))
                    {
                        parameterList[index] += "!*";
                    }

                    Preferences::addIgnore(parameterList[index] + ',' + QString::number(value));
                }

                result.output = i18n("Added %1 to your ignore list.").arg(parameterList.join(", "));
                result.typeString = i18n("Ignore");
                result.type = Program;

                // all went fine, so show no error message
                showUsage = false;
            }
        }

        if(showUsage)
        {
            result = usage(i18n("Usage: %1IGNORE [ -ALL ] <user 1> <user 2> ... <user n>").arg(commandChar));
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseUnignore(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1UNIGNORE <user 1> <user 2> ... <user n>").arg(commandChar));
        }
        else
        {
            QString unignore = parameter.simplifyWhiteSpace();
            QStringList unignoreList = QStringList::split(' ',unignore);

            QStringList succeeded;
            QStringList failed;

            // Iterate over potential unignores
            for (QStringList::Iterator it = unignoreList.begin(); it != unignoreList.end(); ++it)
            {
                // If pattern looks incomplete, try to complete it
                if (!(*it).contains('!'))
                {
                    QString fixedPattern = (*it);
                    fixedPattern += "!*";

                    bool success = false;

                    // Try to remove completed pattern
                    if (Preferences::removeIgnore(fixedPattern))
                    {
                        succeeded.append(fixedPattern);
                        success = true;
                    }

                    // Try to remove the incomplete version too, in case it was added via the GUI ...
                    // FIXME: Validate patterns in GUI?
                    if (Preferences::removeIgnore((*it)))
                    {
                        succeeded.append((*it));
                        success = true;
                    }

                    if (!success)
                        failed.append((*it) + "[!*]");
                }
                // Try to remove seemingly complete pattern
                else if (Preferences::removeIgnore((*it)))
                    succeeded.append((*it));
                // Failed to remove given complete pattern
                else
                    failed.append((*it));
            }

            // Print all successful unignores, in case there were any
            if (succeeded.count()>=1)
            {
                m_server->appendMessageToFrontmost(i18n("Ignore"),i18n("Removed %1 from your ignore list.").arg(succeeded.join(", ")));
            }

            // One failed unignore
            if (failed.count()==1)
            {
                m_server->appendMessageToFrontmost(i18n("Error"),i18n("No such ignore: %1").arg(failed.join(", ")));
            }

            // Multiple failed unignores
            if (failed.count()>1)
            {
                m_server->appendMessageToFrontmost(i18n("Error"),i18n("No such ignores: %1").arg(failed.join(", ")));
            }
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseQuote(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1QUOTE command list").arg(commandChar));
        }
        else
        {
            result.toServer = parameter;
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseSay(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1SAY text").arg(commandChar));
        }
        else
        {
            result.toServer = "PRIVMSG " + destination + " :" + parameter;
            result.output = parameter;
        }

        return result;
    }

    void IcecapOutputFilter::parseKonsole()
    {
        emit openKonsolePanel();
    }

    // Accessors

    void IcecapOutputFilter::setCommandChar() { commandChar=Preferences::commandChar(); }

    // # & + and ! are *often*, but not necessarily, channel identifiers. + and ! are non-RFC, so if a server doesn't offer 005 and
    // supports + and ! channels, I think thats broken behaviour on their part - not ours.
    bool IcecapOutputFilter::isAChannel(const QString &check)
    {
        Q_ASSERT(m_server);
                                                  // XXX if we ever see the assert, we need the ternary
        return m_server? m_server->isAChannel(check) : QString("#&").contains(check.at(0));
    }

    OutputFilterResult IcecapOutputFilter::usage(const QString& string)
    {
        OutputFilterResult result;
        result.typeString = i18n("Usage");
        result.output = string;
        result.type = Program;
        return result;
    }

    OutputFilterResult IcecapOutputFilter::info(const QString& string)
    {
        OutputFilterResult result;
        result.typeString = i18n("Info");
        result.output = string;
        result.type = Program;
        return result;
    }

    OutputFilterResult IcecapOutputFilter::error(const QString& string)
    {
        OutputFilterResult result;
        result.typeString = i18n("Error");
        result.output = string;
        result.type = Program;
        return result;
    }

    void IcecapOutputFilter::parseAaway(const QString& parameter)
    {
        emit multiServerCommand("away", parameter);
    }

    OutputFilterResult IcecapOutputFilter::parseAme(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1AME text").arg(commandChar));
        }

        emit multiServerCommand("me", parameter);
        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseAmsg(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1AMSG text").arg(commandChar));
        }

        emit multiServerCommand("msg", parameter);
        return result;
    }

    void IcecapOutputFilter::parseServer(const QString& parameter)
    {
        if(parameter.isEmpty())
        {
            emit reconnectServer();
        }
        else
        {
            QStringList splitted = QStringList::split(" ", parameter);
            QString host = splitted[0];
            QString port = "6667";
            QString password;

            // 'hostname port password'
            if (splitted.count()==3)
            {
                KonviIpHelper hostParser(host);
                host = hostParser.host();

                port = splitted[1];
                password = splitted[2];
            }
            // 'hostname:port password' or 'hostname port'
            else if (splitted.count()==2)
            {
                KonviIpHelper hostParser(host);
                host = hostParser.host();

                if (!hostParser.port().isEmpty())
                {
                    port = hostParser.port();
                    password = splitted[1];
                }
                else
                {
                    port = splitted[1];
                }
            }
            // 'hostname:port' or 'hostname'
            else
            {
                KonviIpHelper hostParser(host);
                host = hostParser.host();

                if (!hostParser.port().isEmpty())
                {
                    port = hostParser.port();
                }
            }

            if (Preferences::isServerGroup(host))
            {
                emit connectToServerGroup(host);
            }
            else
            {
                emit connectToServer(host, port, password);
            }
        }
    }

    OutputFilterResult IcecapOutputFilter::parsePrefs(const QString& parameter)
    {
        OutputFilterResult result;
        bool showUsage = false;

        if (parameter.isEmpty())
            showUsage = true;
        else
        {
            KConfig* config=KApplication::kApplication()->config();

            QStringList splitted = KShell::splitArgs(parameter);

            if (splitted.count() > 0)
            {
                QString group = splitted[0];
                QStringList groupList(config->groupList());
                uint i;
                if (group.lower() == "list")
                {
                    // List available groups.
                    result = usage(i18n("Available preferences groups: ") + groupList.join("|"));
                }
                else
                {
                    // Validate group.
                    bool validGroup = false;
                    for (i = 0; i < groupList.count(); ++i)
                    {
                        if (group.lower() == groupList[i].lower())
                        {
                            validGroup = true;
                            group = groupList[i];
                            break;
                        }
                    }
                    if (validGroup && splitted.count() > 1)
                    {
                        QString option = splitted[1];
                        QMap<QString,QString> options = config->entryMap(group);
                        QValueList<QString> optionList = options.keys();
                        QValueList<QString> optionValueList = options.values();

                        if (option.lower() == "list")
                        {
                            // List available options in group.
                            QString output = i18n("Available options in group %1:").arg( group );

                            for (i = 0; i < optionList.count(); ++i)
                            {
                                output += optionList[i] + '(' + optionValueList[i] + ")|";
                            }

                            result = usage(output);
                        }
                        else
                        {
                            // Validate option.
                            bool validOption = false;
                            for (i = 0; i < optionList.count(); ++i)
                            {
                                if (option.lower() == optionList[i].lower())
                                {
                                    validOption = true;
                                    option = optionList[i];
                                    break;
                                }
                            }
                            if (validOption)
                            {
                                if (splitted.count() > 2)
                                {
                                    // Set the desired option.
                                    config->setGroup(group);
                                    config->writeEntry(option, splitted[2]);
                                    config->sync();
                                    // Reload preferences object.
                                    dynamic_cast<KonversationApplication*>(kapp)->readOptions();
                                }
                                // If no value given, just display current value.
                                else
                                {
                                    result = usage(group + '/' + option + " = " + options[option]);
                                }
                            }
                            else
                            {
                                showUsage = true;
                            }
                        }
                    }
                    else
                    {
                        showUsage = true;
                    }
                }
            }
            else
            {
                showUsage = true;
            }
        }

        if (showUsage)
        {
            result = usage(i18n("Usage: %1PREFS <group> <option> <value> or %2PREFS LIST to list groups or %3PREFS group LIST to list options in group.  Quote parameters if they contain spaces.").arg(commandChar, commandChar, commandChar));
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseOmsg(const QString& parameter)
    {
        OutputFilterResult result;

        if(!parameter.isEmpty())
        {
            result.toServer = "PRIVMSG @"+destination+" :"+parameter;
        }
        else
        {
            result = usage(i18n("Usage: %1OMSG text").arg(commandChar));
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseOnotice(const QString& parameter)
    {
        OutputFilterResult result;

        if(!parameter.isEmpty())
        {
            result.toServer = "NOTICE @"+destination+' '+parameter;
        }
        else
        {
            result = usage(i18n("Usage: %1ONOTICE text").arg(commandChar));
        }

        return result;
    }

    void IcecapOutputFilter::parseCharset(const QString& charset)
    {
        QString shortName = Konversation::IRCCharsets::self()->ambiguousNameToShortName(charset);
        if(!shortName.isEmpty())
            m_server->getIdentity()->setCodecName(shortName);
    }

    OutputFilterResult IcecapOutputFilter::parseSetKey(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1setkey <nick> or <channel> <key> Sets the encryption key for nick or channel").arg(commandChar) );
        }
        else
        {
            QStringList tmp = QStringList::split(" ",parameter);
            m_server->setKeyForRecipient(tmp[0], tmp[1].local8Bit());
            result = info(i18n("The key for %1 is successfully set.").arg(tmp[0]));
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseDelKey(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1delkey <nick> or <channel> Deletes the encryption key for nick or channel").arg(commandChar));
        }
        else
        {
            m_server->setKeyForRecipient(parameter, "");
            result = info(i18n("The key for %1 is now deleted.").arg(parameter));
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseList(const QString& parameter)
    {
        OutputFilterResult result;

        emit openChannelList(parameter, true);

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseDNS(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1DNS <nick>").arg(commandChar));
        }
        else
        {
            QStringList splitted = QStringList::split(" ", parameter);
            QString target = splitted[0];

            KIpAddress address(target);

            // Parameter is an IP address
            if (address.isIPv4Addr() || address.isIPv6Addr())
            {
            // Disable the reverse resolve codepath on older KDE versions due to many
            // distributions shipping visibility-enabled KDE 3.4 and KNetwork not
            // coping with it.
#if KDE_IS_VERSION(3,5,1)
                KNetwork:: KInetSocketAddress socketAddress(address,0);
                QString resolvedTarget;
                QString serv; // We don't need this, but KReverseResolver::resolve does.

                if (KNetwork::KReverseResolver::resolve(socketAddress,resolvedTarget,serv))
                {
                    result.typeString = i18n("DNS");
                    result.output = i18n("Resolved %1 to: %2").arg(target).arg(resolvedTarget);
                    result.type = Program;
                }
                else
                {
                    result = error(i18n("Unable to resolve %1").arg(target));
                }
#else
                result = error(i18n("Reverse-resolving requires KDE version 3.5.1 or higher."));
#endif
            }
            // Parameter is presumed to be a host due to containing a dot. Yeah, it's dumb.
            // FIXME: The reason we detect the host by occurrence of a dot is the large penalty
            // we would incur by using inputfilter to find out if there's a user==target on the
            // server - once we have a better API for this, switch to it.
            else if (target.contains('.'))
            {
                KNetwork::KResolverResults resolved = KNetwork::KResolver::resolve(target,"");
                if(resolved.error() == KResolver::NoError && resolved.size() > 0)
                {
                    QString resolvedTarget = resolved.first().address().nodeName();
                    result.typeString = i18n("DNS");
                    result.output = i18n("Resolved %1 to: %2").arg(target).arg(resolvedTarget);
                    result.type = Program;
                }
                else
                {
                    result = error(i18n("Unable to resolve %1").arg(target));
                }
            }
            // Parameter is either host nor IP, so request a lookup from server, which in
            // turn lets inputfilter do the job.
            else
            {
                m_server->resolveUserhost(target);
            }
        }

        return result;
    }


    QString IcecapOutputFilter::addNickToEmptyNickList(const QString& nick, const QString& parameter)
    {
        QStringList nickList = QStringList::split(' ', parameter);
        QString newNickList;

        if (nickList.count() == 0)
        {
            newNickList = nick;
        }
        // check if list contains only target channel
        else if (nickList.count() == 1 && isAChannel(nickList[0]))
        {
            newNickList = nickList[0] + ' ' + nick;
        }
        // list contains at least one nick
        else
        {
            newNickList = parameter;
        }

        return newNickList;
    }

}
#include "icecapoutputfilter.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1: