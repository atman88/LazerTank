#ifndef QLTXMLHANDLER_H
#define QLTXMLHANDLER_H

#include <QXmlDeclHandler>

/*
 * @brief Xml handler with suitable logging
 */
class QltXmlHandler : public QXmlDefaultHandler
{
public:
    QltXmlHandler() = default;

    /**
     * @brief Indicates whether this parser is finished. Error reporting is suppressed when this method returns true.
     * This method is used to allow termination without causing nefarious logging to be generated.
     */
    virtual bool isDone() = 0;

    bool fatalError(const QXmlParseException &exception) override;
    bool error(     const QXmlParseException &exception) override;
    bool warning(   const QXmlParseException &exception) override;
    QString errorString() const override;

private:
    void printException( const char* prefix, const QXmlParseException &exception );

    QString mErrorString;
};

#endif // QLTXMLHANDLER_H
