/*
  receive a file on DCC protocol
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/
// Copyright (C) 2004,2005 Shintaro Matsuoka <shin@shoegazed.org>
// Copyright (C) 2004,2005 John Tapsell <john@geola.co.uk>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef DCCTRANSFERRECV_H
#define DCCTRANSFERRECV_H

#include <qptrlist.h>

#include "dccresumedialog.h"
#include "dcctransfer.h"

class QFile;
class QTimer;

namespace KIO
{
    class Job;
    class TransferJob;
}

namespace KNetwork
{
    class KStreamSocket;
}

class DccPanel;

class DccTransferRecvWriteCacheHandler;

class DccTransferRecv : public DccTransfer
{
    Q_OBJECT
        friend class DccDetailDialog;
    friend class DccResumeDialog;

    public:
        /** Constructor.  This sets up the variables and updates the view, so the
         * user can see the filename, filesize etc, and can accept it. */
        DccTransferRecv( DccPanel* panel, const QString& partnerNick, const KURL& defaultFolderURL, const QString& fileName, unsigned long fileSize, const QString& partnerIp, const QString& partnerPort );
        virtual ~DccTransferRecv();

        signals:
                                                  // emitted by requestResume()
        void resumeRequest( const QString& partnerNick, const QString& fileName, const QString& partnerPort, KIO::filesize_t filePosition);

    public slots:
        /** The user has accepted the download.
         *  Check we are saving it somewhere valid, create any directories needed, and
         *  connect to remote host.
         */
        virtual void start();
        /** The user has chosen to abort.
         *  Either by chosen to abort directly, or by choosing cancel when
         *  prompted for information on where to save etc.
         *  Not called when it fails due to another problem.
         */
        virtual void abort();
        void startResume( unsigned long position );

    protected slots:
        // Local KIO
        void slotLocalCanResume( KIO::Job* job, KIO::filesize_t size );
        void slotLocalGotResult( KIO::Job* job );
        void slotLocalReady( KIO::Job* job );
        void slotLocalWriteDone();
        void slotLocalGotWriteError( const QString& errorString );

        // Remote DCC
        void connectionSuccess();
        void connectionFailed( int errorCode );
        void readData();
        void sendAck();
        void connectionTimeout();
        void slotSocketClosed();

    protected:
        void cleanUp();
        void failed(const QString& errorMessage = QString::null );

                                                  // (startPosition == 0) means "don't resume"
        void prepareLocalKio( bool overwrite, bool resume, KIO::fileoffset_t startPosition = 0 );
        void askAndPrepareLocalKio( const QString& message, int enabledActions, DccResumeDialog::ReceiveAction defaultAction, KIO::fileoffset_t startPosition = 0 );

        /**
         * This calls KIO::NetAccess::mkdir on all the subdirectories of dirURL, to
         * create the given directory.  Note that a url like  file:/foo/bar  will
         * make sure both foo and bar are created.  It assumes everything in the path is
         * a directory.
         * Note: If the directory already exists, returns true.
         *
         * @param dirURL A url for the directory to create.
         * @return True if the directory now exists.  False if there was a problem and the directory doesn't exist.
         */
        bool createDirs(const KURL &dirURL) const;

        void requestResume();
        void connectToSender();

        void startConnectionTimer( int sec );
        void stopConnectionTimer();

    protected:
        KURL m_saveToTmpFileURL;
        ///Current filesize of the file saved on the disk.
        KIO::filesize_t m_saveToFileSize;
        ///Current filesize of the file+".part" saved on the disk.
        KIO::filesize_t m_partialFileSize;
        DccTransferRecvWriteCacheHandler* m_writeCacheHandler;
        bool m_saveToFileExists;
        bool m_partialFileExists;
        QTimer* m_connectionTimer;
        KNetwork::KStreamSocket* m_recvSocket;

    private:
        virtual QString getTypeText() const;
        virtual QPixmap getTypeIcon() const;
};

class DccTransferRecvWriteCacheHandler : public QObject
{
    Q_OBJECT

        public:
        DccTransferRecvWriteCacheHandler( KIO::TransferJob* transferJob );
        virtual ~DccTransferRecvWriteCacheHandler();

        void append( char* data, int size );
        bool write( bool force );
        void close();
        void closeNow();

        signals:
        void dataFinished();                      // ->  m_transferJob->slotFinished()
        void done();                              // ->  DccTransferRecv::writeDone()
                                                  // ->  DccTransferRecv::slotWriteError()
        void gotError( const QString& errorString );

    protected slots:
                                                  // <-  m_transferJob->dataReq()
        void slotKIODataReq( KIO::Job* job, QByteArray& data );
        void slotKIOResult( KIO::Job* job );      // <-  m_transferJob->result()

    protected:
        KIO::TransferJob* m_transferJob;
        bool m_writeAsyncMode;
        bool m_writeReady;

        QValueList<QByteArray> m_cacheList;
        QDataStream* m_cacheStream;
};
#endif                                            // DCCTRANSFERRECV_H