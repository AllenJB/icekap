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

#include <qimage.h>
#include <qcstring.h>
#include <qstring.h>
#include <qregexp.h>
#include <qpixmap.h>
#include <qbitmap.h>
#include <qpainter.h>
#include <klocale.h>

#include "common.h"
#include "konversationapplication.h"
#include "config/preferences.h"

namespace Konversation
{

    #include "guess_ja.cpp"
    #include "unicode.cpp"

    static QRegExp colorRegExp("((\003([0-9]|0[0-9]|1[0-5])(,([0-9]|0[0-9]|1[0-5])|)|\017)|\x02|\x09|\x13|\x16|\x1f)");
    static QRegExp urlPattern("((www\\.(?!\\.)|(fish|(f|ht)tp(|s))://)(\\.?[\\d\\w/,\\':~\\?=;#@\\-\\+\\%\\*\\{\\}\\!\\(\\)]|&)+)|"
        "([-.\\d\\w]+@[-.\\d\\w]{2,}\\.[\\w]{2,})");
    static QRegExp tdlPattern("(.*)\\.(\\w+),$");

    QString removeIrcMarkup(const QString& text)
    {
        QString escaped = text;
        // Escape text decoration
        escaped.remove(colorRegExp);

        // Remove Mirc's 0x03 characters too, they show up as rectangles
        escaped.remove(QChar(0x03));

        return escaped;
    }

    QString tagURLs(const QString& text, const QString& fromNick, bool useCustomColor)
    {
        // QTime timer;
        // timer.start();

        QString filteredLine = text;
        QString linkColor = Preferences::color(Preferences::Hyperlink).name();
        QString link;

        if(useCustomColor)
        {
            link = "\\1<font color=\""+linkColor+"\"><a href=\"#\\2\">\\2</a></font>";
        }
        else
        {
            link = "\\1<a href=\"#\\2\">\\2</a>";
        }

        if(filteredLine.contains("#"))
        {
          QRegExp chanExp("(^|\\s|^\"|\\s\"|,|'|\\(|\\:|!|@|%|\\+)(#[^,\\s;\\)\\:\\/\\(\\<\\>]*[^.,\\s;\\)\\:\\/\\(\"\''\\<\\>])");
            filteredLine.replace(chanExp, link);
        }

        int pos = 0;
        int urlLen = 0;
        QString append;
        QString href;
        QString insertText;

        urlPattern.setCaseSensitive(false);
        QString protocol;

        if(useCustomColor)
        {
            link = "<font color=\"" + linkColor + "\"><u><a href=\"%1%2\">%3</a></u></font>";
        }
        else
        {
            link = "<u><a href=\"%1%2\">%3</a></u>";
        }

        while((pos = urlPattern.search(filteredLine, pos)) >= 0)
        {
            // check if the matched text is already replaced as a channel
            if ( filteredLine.findRev( "<a", pos ) > filteredLine.findRev( "</a>", pos ) )
            {
                ++pos;
                continue;
            }

            protocol="";
            urlLen = urlPattern.matchedLength();
            href = filteredLine.mid( pos, urlLen );

            // Don't consider trailing comma part of link.
            if (href.right(1) == ",")
            {
                href.truncate(href.length()-1);
                append = ',';
            }

            // Don't consider trailing closing parenthesis part of link when
            // there's an opening parenthesis preceding the beginning of the
            // URL or there is no opening parenthesis in the URL at all.
            if (href.right(1) == ")" && (filteredLine.mid(pos-1,1) == "(" || !href.contains("(")))
            {
                href.truncate(href.length()-1);
                append.prepend(")");
            }

            // Qt doesn't support (?<=pattern) so we do it here
            if((pos > 0) && filteredLine[pos-1].isLetterOrNumber())
            {
                pos++;
                continue;
            }

            if (urlPattern.cap(1).startsWith("www.", false))
                protocol = "http://";
            else if (urlPattern.cap(1).isEmpty())
                protocol = "mailto:";

            // Use \003 as a placeholder for & so we can readd them after changing all & in the normal text to &amp;
            insertText = link.arg(protocol, QString(href).replace('&', "\x0b"), href) + append;
            filteredLine.replace(pos, urlLen, insertText);
            pos += insertText.length();
            KonversationApplication::instance()->storeUrl(fromNick, href);
        }

        // Change & to &amp; to prevent html entities to do strange things to the text
        filteredLine.replace('&', "&amp;");
        filteredLine.replace("\x0b", "&");

        // kdDebug() << "Took (msecs) : " << timer.elapsed() << " for " << filteredLine << endl;

        return filteredLine;
    }

    //TODO: there's room for optimization as pahlibar said. (strm)

    // the below two functions were taken from kopeteonlinestatus.cpp.
    QBitmap overlayMasks( const QBitmap *under, const QBitmap *over )
    {
        if ( !under && !over ) return QBitmap();
        if ( !under ) return *over;
        if ( !over ) return *under;

        QBitmap result = *under;
        bitBlt( &result, 0, 0, over, 0, 0, over->width(), over->height(), Qt::OrROP );
        return result;
    }

    QPixmap overlayPixmaps( const QPixmap &under, const QPixmap &over )
    {
        if ( over.isNull() ) return under;

        QImage imResult; imResult=under;
        QImage imOver; imOver=over;
        QPixmap result;

        bitBlt(&imResult,0,0,&imOver,0,0,imOver.width(),imOver.height(),0);
        result=imResult;
        return result;
    }

}
