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
        Found
    } State;

public:
    HelpXmlHandler( QString key ) : mKey(key), mState(Idle)
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
            if ( localName == "img" ) {
                if ( attributes.value( "src" ) == mKey ) {
                    mState = ReadingRow;
                }
            }
            break;

        case ReadingRow:
            if ( localName == "td" ) {
                mState = ReadingColumn;
            }

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
        if ( mState == ReadingColumn ) {
            mResult = ch;
            std::cout << "found: " << qPrintable(mResult) << std::endl;
            mState = Found;
        }
        return true;
    }

    bool found()
    {
        return mState == Found;
    }

    QString& getResult()
    {
        return mResult;
    }

private:
    QString mKey;
    State mState;
    QString mResult;
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
            HelpXmlHandler handler( QString(":/images/%1.png").arg(mName) );
            xml.setContentHandler( &handler );
            QXmlInputSource xmlInputSource( &source );
            MyErrorHandler errorHandler;
            xml.setErrorHandler( &errorHandler );

            xml.parse( xmlInputSource );
            if ( handler.found() ) {
                QCoreApplication::postEvent( mReceiver, new QltWhatsThisEvent( mPos, handler.getResult() ) );
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
