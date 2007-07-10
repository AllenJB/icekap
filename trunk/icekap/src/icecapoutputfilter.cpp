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
#include "konviiphelper.h"
#include "icecapchannel.h"
#include "icecapmypresence.h"

namespace Icecap
{
    OutputFilter::OutputFilter(IcecapServer* server)
        : QObject(server)
    {
        m_server = server;
    }

    /**
     * Replace all aliases in the string
     * @param line Line to parse
     * @return Replacements made?
     */
    bool OutputFilter::replaceAliases(QString& line)
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
            // FIXME: cc may be a regexp character here ... we should escape it then
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
                // BUGFIX: Don't strip the command char
                line=cc + aliasReplace;
                // return "replaced"
                return true;
            }
        }                                         // for

        return false;
    }

    QStringList OutputFilter::splitForEncoding(const QString& inputLine, int MAX)
    {
//        QString channelCodecName=Preferences::channelEncoding(m_server->getServerGroup(), destination);
        QString channelCodecName = "utf8";

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

        // FIXME Doesn't this just return `getIdentity()->getCodec();` if no codec set?
        codec = Konversation::IRCCharsets::self()->codecForName(channelCodecName);

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

    /**
     * Parse a command from the client
     * @param myNick (Unused)
     * @param originalLine Original line entered by user
     * @param networkName Name of network the source window belongs to
     * @param mypresenceName Name of mypresence the source window belongs to
     * @param channelName Name of channel / query the source window belongs to
     * @return Object containing client output (and server commands)
     */
    OutputFilterResult OutputFilter::parse (const QString& myNick, const QString& originalLine, const QString& networkName, const QString& mypresenceName, const QString& channelName)
    {
        setCommandChar();

        OutputFilterResult result;
        destination = channelName;

        QString inputLine(originalLine);

        QStringList parameters = QStringList::split (" ", inputLine);
        parameters.pop_front ();

        if(inputLine.isEmpty() || inputLine == "\n")
            return result;

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

        if (line.startsWith (commandChar +"network"))
        {
            result = parseNetwork (parameters);
        }
        else if (line.startsWith (commandChar +"presence"))
        {
            result = parseMyPresence (parameters);
        }
        else if (line.startsWith (commandChar +"channel"))
        {
            result = parseChannel (parameters);
        }
        else if (line.startsWith (commandChar +"gateway"))
        {
            result = parseGateway (parameters);
        }
        else

        if (line.startsWith (commandChar +"join"))
            // TODO: Error handling
        {
            channelJoin (parameters[0], mypresenceName, networkName);
        }
        else if (line.startsWith (commandChar +"part"))
            // TODO: Error handling
        {
            if (parameters[0].length () < 1) {
                channelPart (channelName, mypresenceName, networkName);
            } else {
                channelPart (parameters[0], mypresenceName, networkName);
            }
        }
        else if (line.startsWith (commandChar +"topic"))
        {
            Icecap::Cmd command;
            command.tag = "topic";
            command.command = "channel change";
            command.parameterList.insert ("network", networkName);
            command.parameterList.insert ("mypresence", mypresenceName);
            command.parameterList.insert ("channel", channelName);
            command.parameterList.insert ("topic", parameters.join (" "));

            m_server->queueCommand (command);
        }
        else if (line.startsWith (commandChar +"nick"))
        {
            changeNickname (parameters[0], mypresenceName, networkName);
        }
        else

        // Convert double command chars at the beginning to single ones
        if(line.startsWith(commandChar+commandChar) && !destination.isEmpty())
        {
            inputLine=inputLine.mid(1);
            goto BYPASS_COMMAND_PARSING;
        }
        // Server command?
        else if(line.startsWith(commandChar))
        {
            QString command = inputLine.section(' ', 0, 0).mid(1).lower();
            QString parameter = inputLine.section(' ', 1);

            if(command == "exec")     result = parseExec(parameter);
            else if(command == "raw")      result = parseRaw(parameter);
            else if(command == "konsole")  parseKonsole();
            else if(command == "server")   parseServer(parameter);
            else if(command == "reconnect")  emit reconnectServer();
            else if(command == "disconnect") emit disconnectServer();
            else if(command == "prefs")    result = parsePrefs(parameter);
            else if(command == "dns")      result = parseDNS(parameter);

            else if ((command == "me") || (command == "action"))
            {
                QString escapedLine = parameter;
                escapedLine.replace (";", "\\.");
                if (isAChannel (channelName)) {
                    result.toServer = "m;msg;network="+ networkName +";mypresence="+ mypresenceName +";channel="+ channelName +";type=action;msg="+ escapedLine;
                } else {
                    result.toServer = "m;msg;network="+ networkName +";mypresence="+ mypresenceName +";presence="+ channelName +";type=action;msg="+ escapedLine;
                }
                result.output = parameter;
                result.type = Action;
            }

            else if (command == "msg")
            {
                QString escapedLine = parameter;
                escapedLine.replace (";", "\\.");
                destination = parameters[0];
                if (isAChannel(destination)) {
                    result.toServer = "m;msg;network="+ networkName +";mypresence="+ mypresenceName +";channel="+ destination +";msg="+ escapedLine;
                } else {
                    result.toServer = "m;msg;network="+ networkName +";mypresence="+ mypresenceName +";presence="+ destination +";msg="+ escapedLine;
                }
                result.output = parameter;
                result.type = Message;
            }

            // Forward unknown commands to server
            else
            {
                Icecap::Cmd command;
                command.tag = "raw";
                command.command = inputLine.mid (1);
                m_server->queueCommand (command);
            }
        }
        // Ordinary message to channel/query?
        else if(!destination.isEmpty())
        {
            BYPASS_COMMAND_PARSING:
            QString escapedLine = inputLine;
            escapedLine.replace (";", "\\.");
            if (isAChannel (destination)) {
                result.toServer = "m;msg;network="+ networkName +";mypresence="+ mypresenceName +";channel="+ channelName +";msg="+ escapedLine;
            } else {
                result.toServer = "m;msg;network="+ networkName +";mypresence="+ mypresenceName +";presence="+ channelName +";msg="+ escapedLine;
            }
            result.output = inputLine;
            result.type = Message;
        }
        // Eveything else goes to the server unchanged
        else
        {
            Icecap::Cmd command;
            command.tag = "raw";
            command.command = inputLine;
            m_server->queueCommand (command);
        }

        return result;
    }

    /**
     * Parse /presence command
     * @param parameter Parameters
     * @return Result object containing client output
     */
    OutputFilterResult OutputFilter::parseMyPresence (QStringList& parameter)
    {
        QString command = "presence";
        QStringList usage;
        usage.append ("Usage: /"+ command +" [command] [parameters]");
        usage.append ("  list                           - List presences");
        usage.append ("  add [name] [network]           - Create a new presence called 'name' on 'network'");
        usage.append ("  remove [name] [network]        - Delete an existing presence");
        usage.append ("  connect [name] [network]       - Connect a presence to a given network");
        usage.append ("  disconnect [name] [network]    - Disconnect a presence");

        OutputFilterResult result;
        result.typeString = "Presence";

        if (parameter[0] == "list")
        {
            result.outputList = m_server->presenceListDisplay ();
            result.type = Program;
        }
        else if (parameter.size() > 2) {
            if ((parameter[0] != "add") && (parameter[0] != "remove")
                && (parameter[0] != "connect") && (parameter[0] != "disconnect"))
            {
                result.outputList = usage;
                result.type = Program;
                return result;
            }

            Icecap::Cmd cmd;
            cmd.tag = command;
            cmd.command = command +" "+ parameter[0];
            cmd.parameterList.insert ("mypresence", parameter[1]);
            parameter.pop_front ();
            parameter.pop_front ();
            cmd.parameterList.insert ("network", parameter.join (" "));

            m_server->queueCommand (cmd);
        }
        else
        {
            result.outputList = usage;
            result.type = Program;
        }
        return result;
    }


    /**
     * Parse /network command
     * @param parameter Parameters
     * @return Object containing client output
     * @todo AllenJB: Make sure the functions recieving an OutputFilterResult can deal with empty ones
     */
    OutputFilterResult OutputFilter::parseNetwork (QStringList& parameter)
    {
        QString command = "network";
        QString usage = "Usage: /"+ command +" (list|add|remove) [protocol] [name]";

        OutputFilterResult result;
        result.typeString = "Network";

        // TODO: Redo this command so it goes to current window
        if (parameter[0] == "list")
        {
            result.outputList = m_server->networkListDisplay ();
            result.type = Program;
        }
        else if ((parameter[0] == "add") || (parameter[0] == "remove"))
        {
            if (parameter.size() < 3) {
                result.output = usage;
                result.type = Program;
                return result;
            }

            Icecap::Cmd cmd;
            cmd.tag = command;
            cmd.command = command +" "+ parameter[0];
            cmd.parameterList.insert ("protocol", parameter[1]);

            // Remove the first 2 parameters - allows network to contain spaces
            parameter.pop_front ();
            parameter.pop_front ();
            cmd.parameterList.insert ("network", parameter.join (" "));
            m_server->queueCommand (cmd);
        }
        else
        {
            result.output = usage;
            result.type = Program;
        }
        return result;
    }

    /**
     * Parse /channel command
     * @param parameter Parameters
     * @return Object containing client output
     */
    OutputFilterResult OutputFilter::parseChannel (QStringList& parameter)
    {
        QString command = "channel";
        QString usage = "Usage: /"+ command +" (list|add|remove) [channel] [mypresence] [network]";

        OutputFilterResult result;
        result.typeString = "Channel";

        if (parameter[0] == "list")
        {
            m_server->networkListDisplay ();
        }
        else if ((parameter[0] == "add") || (parameter[0] == "remove"))
        {
            if (parameter.size() < 4) {
                result.output = usage;
                result.type = Program;
                return result;
            }
            Icecap::Cmd cmd;
            cmd.tag = command;
            cmd.command = command +" "+ parameter[0];
            cmd.parameterList.insert ("channel", parameter[1]);
            cmd.parameterList.insert ("mypresence", parameter[2]);

            parameter.pop_front ();
            parameter.pop_front ();
            parameter.pop_front ();
            cmd.parameterList.insert ("network", parameter.join (" "));
            m_server->queueCommand (cmd);
        }
        else
        {
            result.output = usage;
            result.type = Program;
        }
        return result;
    }


    /**
     * Parse /gateway command
     * @param parameter Parameters
     * @return Object containing client output
     * @todo AllenJB: Add support for host, password and priority parameters
     * @todo AllenJB: Checks / error messages for if gateway is in use
     * @todo AllenJB: Gateway list
     */
    OutputFilterResult OutputFilter::parseGateway (QStringList& parameter)
    {
        QString command = "gateway";
        QString usage = "Usage: /"+ command +" (list|add|remove) [network] [host]";

        OutputFilterResult result;
        result.typeString = "Gateway";

        if (parameter[0] == "list")
        {
        }
        else if ((parameter[0] == "add") || (parameter[0] == "remove"))
        {
            if (parameter.size() != 3) {
                result.output = usage;
                result.type = Program;
                return result;
            }
            Icecap::Cmd cmd;
            cmd.tag = command;
            cmd.command = command +" "+ parameter[0];
            cmd.parameterList.insert ("network", parameter[1]);
            cmd.parameterList.insert ("host", parameter[2]);

            m_server->queueCommand (cmd);
        }
        else
        {
            result.output = usage;
            result.type = Program;
        }
        return result;
    }


    /**
     * Parse /exec command
     * @param parameter Parameters
     * @return Object containing client output and server commands
     */
    OutputFilterResult OutputFilter::parseExec(const QString& parameter)
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

    /**
     * Parse /raw command
     * @param parameter Parameters
     * @return Object containing client messages and server commands
     */
    OutputFilterResult OutputFilter::parseRaw(const QString& parameter)
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

    /**
     * Parse /konsole command
     */
    void OutputFilter::parseKonsole()
    {
        emit openKonsolePanel();
    }

    // Accessors

    /**
     * Set the commandChar property from the Preferences
     */
    void OutputFilter::setCommandChar() { commandChar=Preferences::commandChar(); }

    OutputFilterResult OutputFilter::usage(const QString& string)
    {
        OutputFilterResult result;
        result.typeString = i18n("Usage");
        result.output = string;
        result.type = Program;
        return result;
    }

    /**
     * Set up an information result
     * @param string Client output
     * @return OutputFilterResult object
     */
    OutputFilterResult OutputFilter::info(const QString& string)
    {
        OutputFilterResult result;
        result.typeString = i18n("Info");
        result.output = string;
        result.type = Program;
        return result;
    }

    /**
     * Set up an error result
     * @param string Client message
     * @return OutputFilterResult object
     */
    OutputFilterResult OutputFilter::error(const QString& string)
    {
        OutputFilterResult result;
        result.typeString = i18n("Error");
        result.output = string;
        result.type = Program;
        return result;
    }

    /**
     * Parse the /server command
     * @param parameter Parameters
     * @todo AllenJB: This method needs to be re-implemented or removed (it should be an alias for a mypresence connect event)
     * @todo AllenJB: What is serverGroup used for here? It needs to be re-implemented or removed
     */
    void OutputFilter::parseServer(const QString& parameter)
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
/*
            if (Preferences::isServerGroup(host))
            {
                emit connectToServerGroup(host);
            }
            else
            {
                emit connectToServer(host, port, password);
            }
*/
        }
    }


    /**
     * Parse /prefs command
     * @param parameter Parameters
     * @return Object containing client messages and server commands
     */
    OutputFilterResult OutputFilter::parsePrefs(const QString& parameter)
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

    /**
     * Parse /dns command
     * @param parameter Parameters
     * @return Object containing client messages and server commands
     */
    OutputFilterResult OutputFilter::parseDNS(const QString& parameter)
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
        }

        return result;
    }

    /**
     * Determine if a given string is (likely to be) a channel or a presence name
     * The following note comes from the Konversation code:
     * # & + and ! are *often*, but not necessarily, channel identifiers. + and ! are non-RFC, so if a server doesn't offer 005 and
     * supports + and ! channels, I think thats broken behaviour on their part - not ours.
     * @todo AllenJB: Query list of known channels in relevent mypresence
     * @param name String to check
     * @return Is a channel?
     */
    bool OutputFilter::isAChannel (const QString& name)
    {
        QString channelPrefixes = "#&";
        return (channelPrefixes.contains(name.at(0)) > 0);
    }


    /**
     * Change mypresence nickname
     * @param newNickname New nickname
     */
    void OutputFilter::changeNickname (const QString& newNickname, const QString& mypresenceName, const QString& networkName)
    {
        if (newNickname.isEmpty())
            return;

        Icecap::Cmd command;
        command.tag = "nick";
        command.command = "presence change";
        command.parameterList.insert ("network", networkName);
        command.parameterList.insert ("mypresence", mypresenceName);
        command.parameterList.insert ("name", newNickname);

        m_server->queueCommand (command);
    }

    /**
     * Join a channel
     * @param channelName Name of channel to join
     * @param mypresenceName MyPresence name
     * @param networkName Network name
     */
    void OutputFilter::channelJoin (const QString& channelName, const QString& mypresenceName, const QString& networkName)
    {
        if (channelName.isEmpty())
            return;

        Icecap::Cmd command;
        command.tag = "join";
        command.command = "channel join";
        command.parameterList.insert ("channel", channelName);
        command.parameterList.insert ("network", networkName);
        command.parameterList.insert ("mypresence", mypresenceName);

        m_server->queueCommand (command);
    }

    /**
     * Part a channel
     * @param channelName Name of channel to part
     * @param mypresenceName MyPresence name
     * @param networkName Network name
     */
    void OutputFilter::channelPart (const QString& channelName, const QString& mypresenceName, const QString& networkName)
    {
        if (channelName.isEmpty())
            return;

        Icecap::Cmd command;
        command.tag = "part";
        command.command = "channel part";
        command.parameterList.insert ("channel", channelName);
        command.parameterList.insert ("network", networkName);
        command.parameterList.insert ("mypresence", mypresenceName);

        m_server->queueCommand (command);
    }

}

#include "icecapoutputfilter.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
