/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2006 by Eike Hein
  email:     sho@eikehein.com
*/

#ifndef KONVERSATIONSTATUSBAR_H
#define KONVERSATIONSTATUSBAR_H

#include <qobject.h>

class QLabel;

class KonversationMainWindow;
class KSqueezedTextLabel;
class SSLLabel;
class IcecapServer;

class KonversationStatusBar : public QObject
{
    Q_OBJECT

    public:
        explicit KonversationStatusBar(KonversationMainWindow* parent);
        ~KonversationStatusBar();

    public slots:
        void updateAppearance();

        void resetStatusBar();

        void setMainLabelText(const QString& text);

        void setMainLabelTempText(const QString& text);
        void clearMainLabelTempText();

        void setInfoLabelShown(bool shown);
        void updateInfoLabel(const QString& text);
        void clearInfoLabel();

        void setLagLabelShown(bool shown);
        void updateLagLabel(IcecapServer* lagServer, int msec);
        void resetLagLabel();
        void setTooLongLag(IcecapServer* lagServer, int msec);

        void updateSSLLabel(IcecapServer* server);
        void removeSSLLabel();

    private:
        KonversationMainWindow* m_window;

        KSqueezedTextLabel* m_mainLabel;
        QLabel* m_infoLabel;
        QLabel* m_lagLabel;
        SSLLabel* m_sslLabel;

        QString m_oldMainLabelText;
        QString m_tempMainLabelText;
};

#endif
