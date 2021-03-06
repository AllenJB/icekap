/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/

#ifndef COMMON_H
#define COMMON_H

class QCString;
class QString;
class QBitmap;
class QPixmap;

namespace Konversation
{
    QString removeIrcMarkup(const QString& text);
    QString tagURLs(const QString& text, const QString& fromNick, bool useCustomColor = true);
    QBitmap overlayMasks( const QBitmap *under, const QBitmap *over );
    QPixmap overlayPixmaps(const QPixmap &under, const QPixmap &over);
    bool isUtf8(const QCString& text);

    enum TabNotifyType
    {
        tnfNick,
        tnfHighlight,
        tnfNormal,
        tnfSystem,
        tnfControl,
        tnfNone
    };
}
#endif
