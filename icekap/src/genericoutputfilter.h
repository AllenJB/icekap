#ifndef GENERICOUTPUTFILTER_H
#define GENERICOUTPUTFILTER_H

#include <qobject.h>
#include <qstring.h>

namespace Konversation {

    typedef enum MessageType
    {
        Message,
        Action,
        Command,
        Program,
        PrivateMessage
    };

    struct OutputFilterResult
    {
        QString output;
        QStringList outputList;
        QString toServer;
        QStringList toServerList;
        QString typeString;
        MessageType type;
    };
/*
	class GenericOutputFilter : public QObject
//	class GenericOutputFilter
	{
		Q_OBJECT

		public:
            virtual OutputFilterResult parse(const QString& myNick,const QString& line,const QString& name) = 0;
            virtual bool replaceAliases(QString& line) = 0;

	};
*/
}

#endif

