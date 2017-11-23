#include <iostream>
#include <QXmlSimpleReader>
#include <QXmlDefaultHandler>
#include <QXmlParseException>
#include <QCoreApplication>

#include "helputils.h"
#include "imageutils.h"
#include "workerthread.h"
#include "controller/gameregistry.h"

class HelpXmlHandler : public QXmlDefaultHandler
{
    typedef enum {
        Idle,
        ReadingTable,
        ReadingRow,
        ReadingColumn,
        Done
    } State;

public:
    HelpXmlHandler( QString key ) : mKey(key), mCurrentColumn(0), mState(Idle)
    {
    }

    bool startElement(const QString& /*namespaceURI*/, const QString& localName, const QString& /*qName*/,
      const QXmlAttributes& attributes ) override
    {
        switch( mState ) {
        case Idle:
            if ( localName == "table" ) {
                if ( attributes.value( "id" ) == "pieces" ) {
                    mState = ReadingTable;
                }
            }
            break;
        case ReadingTable:
            if ( localName == "tr" && attributes.value( "id" ) == mKey ) {
                mState = ReadingRow;
                mCurrentColumn = 0;
            }
            break;

        case ReadingRow:
            if ( localName == "td" ) {
                ++mCurrentColumn;
            } else {
                QString* p = 0;
                switch( mCurrentColumn ) {
                case 1: p = &mDisplayName; break;
                case 3: p = &mText; break;
                default:
                    ;
                }

                if ( p && localName == "br" ) {
                    *p += "<br/>";
                }
            }
            break;

        default:
            ;
        }

        return true;
    }

    bool endElement( const QString /*&namespaceURI*/, const QString &localName, const QString /*&qName*/ )
    {
        if ( localName == "table" ) {
            mState = Idle;
        }
        return true;
    }

    bool characters( const QString &ch )
    {
        if ( mState == ReadingRow ) {
            switch( mCurrentColumn ) {
            case 1: mDisplayName += ch; break;
            case 3: mText += ch;
                mState = Done;
                break;
            default:
                ;
            }
        }
        return true;
    }

    bool found()
    {
        return mState == Done;
    }

    QString& getKey()
    {
        return mKey;
    }

    QString& getDisplayName()
    {
        return mDisplayName;
    }

    QString& getText()
    {
        return mText;
    }

private:
    QString mKey;
    State mState;
    int mCurrentColumn;
    QString mDisplayName;
    QString mText;
};

class MyErrorHandler : public QXmlErrorHandler
{
public:
    bool fatalError(const QXmlParseException &exception) { std::cout << "*** "; printException(exception); return false; }
    bool error(     const QXmlParseException &exception) { std::cout << "** ";  printException(exception); return false; }
    bool warning(   const QXmlParseException &exception) { std::cout << "** ";  printException(exception); return false; }
    QString errorString() const { return mErrorString; }

private:
    void printException(const QXmlParseException &exception)
    {
        mErrorString = QString( "line %1 column %2: %3" ).arg( exception.lineNumber() ).arg( exception.columnNumber() ).arg( exception.message() );
        std::cout << qPrintable(mErrorString) << std::endl;
    }

    QString mErrorString;
};

class HelpLoadRunnable : public BasicRunnable
{
public:
    HelpLoadRunnable( QString name, QPoint* forPos, QObject* receiver ) : mName(name), mPos(*forPos), mReceiver(receiver)
    {
    }

    void run() override
    {
        QFile source( ":/help/qlthelp.html" );
        if ( source.open( QIODevice::ReadOnly ) ) {
            QXmlSimpleReader xml;
            HelpXmlHandler handler( mName );
            xml.setContentHandler( &handler );
            QXmlInputSource xmlInputSource( &source );
            MyErrorHandler errorHandler;
            xml.setErrorHandler( &errorHandler );

            xml.parse( xmlInputSource );
            if ( handler.found() ) {
                QString text = QString(
                  "<html>"
                   "<table>"
                     "<tr valign=middle>"
                      "<td>%2</td>"
                      "<td><img src=\":/images/%1.png\"/></td>"
                      "<td>%3</td>"
                     "</tr>"
                    "</table>"
                   "<html>" ).arg( mName ).arg( handler.getDisplayName() ).arg( handler.getText() );

                QCoreApplication::postEvent( mReceiver, new QltWhatsThisEvent( mPos, text ) );
            }
        } else {
            std::cout << "** couldn't' read " << qPrintable(source.fileName()) << std::endl;
        }
    }

    bool deleteWhenDone()
    {
        return true;
    }

private:
    QString mName;
    QPoint mPos;
    QObject* mReceiver;
};

void whatsthis( QPoint* pos, unsigned what, GameRegistry* registry, QObject* receiver )
{
    if ( pos ) {
        const ResourcePixmap* pixmap = ResourcePixmap::getPixmap( what );
        const char* name = pixmap->getName();
        if ( const char* multiName = strchr( name, '+' ) ) {
            name = &multiName[1];
        }
        registry->getWorker().doWork( new HelpLoadRunnable( name, pos, receiver ) );
    }
}

QEvent::Type QltWhatsThisEvent::getEventType()
{
    static const QEvent::Type eventType = (QEvent::Type) QEvent::registerEventType();
    return eventType;
}

QPoint& QltWhatsThisEvent::getPos()
{
    return mPos;
}

QString& QltWhatsThisEvent::getHelpText()
{
    return mText;
}
