#include <iostream>

#include "qltxmlhandler.h"

QltXmlHandler::QltXmlHandler()
{
}

bool QltXmlHandler::fatalError( const QXmlParseException &exception ) { if ( !isDone() ) printException("*** ", exception); return false; }
bool QltXmlHandler::error(      const QXmlParseException &exception ) { if ( !isDone() ) printException("** ",  exception); return false; }
bool QltXmlHandler::warning(    const QXmlParseException &exception ) { if ( !isDone() ) printException("* ",   exception); return false; }

QString QltXmlHandler::errorString() const
{
    return mErrorString;
}

void QltXmlHandler::printException(const char *prefix, const QXmlParseException &exception)
{
    mErrorString = QString( "line %1 column %2: %3" ).arg( exception.lineNumber() ).arg( exception.columnNumber() ).arg( exception.message() );
    std::cout << prefix << qPrintable(mErrorString) << std::endl;
}
