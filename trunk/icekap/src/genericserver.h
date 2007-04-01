#ifndef GENERICSERVER_H
#define GENERICSERVER_H

#include <ksharedptr.h>
#include <kprocess.h>
#include <ksocketbase.h>
#include <kbufferedsocket.h>
#include <kstreamsocket.h>

#include "channelnick.h"
#include "inputfilter.h"
#include "outputfilter.h"
#include "servergroupsettings.h"

class Channel;
class ChannelListPanel;
class Query;
class RawLog;
class ViewContainer;
class StatusPanel;

using namespace KNetwork;

class GenericServer : public QObject
{
	Q_OBJECT

	public:
        virtual ChannelListPanel* addChannelListPanel() = 0;
        virtual ChannelNickPtr addNickToJoinedChannelsList(const QString& channelName, const QString& nickname) = 0;
        virtual QString allowedChannelModes() const = 0;
        virtual void appendMessageToFrontmost(const QString& type,const QString& message, bool parseURL = true) = 0;
        virtual void appendStatusMessage(const QString& type,const QString& message) = 0;

        virtual bool connected() const = 0;

        virtual void dcopSay(const QString& target,const QString& command) = 0;

        virtual void emitChannelNickChanged(const ChannelNickPtr channelNick) = 0;
        virtual void emitNickInfoChanged(const NickInfoPtr nickInfo) = 0;

        virtual QPtrList<Channel> getChannelList() const = 0;
        virtual ChannelListPanel* getChannelListPanel() const = 0;
        virtual ChannelNickPtr getChannelNick(const QString& channelName, const QString& nickname) = 0;
        virtual IdentityPtr getIdentity() const = 0;
//        virtual GenericInputFilter* getInputFilter() = 0;
        virtual const ChannelNickMap *getJoinedChannelMembers(const QString& channelName) const = 0;
        virtual int getLag() const = 0;
        virtual QStringList getNickChannels(const QString& nickname) = 0;
        virtual QString getNickname() const = 0;
//        virtual Konversation::GenericOutputFilter* getOutputFilter() = 0;
        virtual int getPort() const = 0;
        virtual int getPreLength(const QString& command, const QString& dest) = 0;
        virtual Query* getQueryByName(const QString& name) = 0;
        virtual QString getServerGroup() const = 0;
        virtual QString getServerName() const = 0;
        virtual StatusPanel* getStatusView() const = 0;
        virtual ViewContainer* getViewContainer() const = 0;

        virtual bool isAChannel(const QString &channel) const = 0;
        virtual bool isAway() const = 0;
        virtual bool isConnected() const = 0;
        virtual bool isConnecting() const = 0;

        virtual QString loweredNickname() const = 0;

        virtual void mangleNicknameWithModes(QString &nickname,bool& isAdmin,bool& isOwner,bool &isOp, bool& isHalfop,bool &hasVoice) = 0;

        virtual NickInfoPtr obtainNickInfo(const QString& nickname) = 0;

        virtual QString parseWildcards(const QString& toParse, const QString& nickname, const QString& channelName, const QString &channelKey, const QStringList &nickList, const QString& parameter) = 0;
        virtual QString parseWildcards(const QString& toParse, const QString& nickname, const QString& channelName, const QString &channelKey, const QString& nick, const QString& parameter) = 0;

        virtual void removeChannel(Channel* channel) = 0;

        virtual void sendURIs(const QStrList& uris, const QString& nick) = 0;
        virtual Konversation::ServerGroupSettingsPtr serverGroupSettings() const = 0;
        virtual void setAwayReason(const QString& reason) = 0;
        virtual void setKeyForRecipient(const QString& recipient, const QCString& key) = 0;


	public slots:
        virtual void addDccSend(const QString &recipient,KURL fileURL, const QString &altFileName = QString::null, uint fileSize = 0) = 0;
        virtual Query *addQuery(const NickInfoPtr & nickInfo, bool weinitiated) = 0;

        virtual void closeChannel(const QString &name) = 0;
        virtual void closeChannelListPanel() = 0;
        virtual void closeQuery(const QString &name) = 0;
        virtual void closeRawLog() = 0;

        virtual void queue(const QString &buffer) = 0;
        virtual void queueList(const QStringList &buffer) = 0;
        virtual void quitServer() = 0;

        virtual void reconnect() = 0;
        virtual void removeQuery(Query *query) = 0;
        virtual void requestBan(const QStringList& users,const QString& channel,const QString& option) = 0;
        virtual void requestTopic(const QString& channel) = 0;
        virtual void requestUnban(const QString& mask,const QString& channel) = 0;
        virtual void requestUserhost(const QString& nicks) = 0;
        virtual void requestWho(const QString& channel) = 0;
        virtual void resolveUserhost(const QString& nickname) = 0;

        virtual void sendJoinCommand(const QString& channelName, const QString& password = QString::null) = 0;

};

#endif
