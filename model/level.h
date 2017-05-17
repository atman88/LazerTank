#ifndef LEVEL_H
#define LEVEL_H

#include <QObject>
#include <QWidget>
#include <QWidgetAction>
#include <QList>

#include "modelpoint.h"
#include "util/workerthread.h"

class GameRegistry;
class Board;

/**
 * @brief Representation of a level and its attributes
 */
class Level : public QWidgetAction
{
    Q_OBJECT

public:
    explicit Level( int number, int width, int height, QObject* parent = 0 );

    bool operator==( const Level& other ) const;
    bool operator<( const Level& other ) const;

    /**
     * @brief Get the number for this level. The level number is both the displayed number and the key value.
     */
    int getNumber() const;

    /**
     * @brief Get the size for this level in model space (i.e. number of columns and rows)
     */
    const QSize& getSize() const;

    void onBoardLoaded();

private:
    int mNumber;
    QSize mSize;
};

class LevelWidget : public QWidget
{
    Q_OBJECT

public:
    LevelWidget( Level& source, QWidget* parent = 0 );

    QSize sizeHint() const override;

protected:
    void paintEvent( QPaintEvent* ) override;

private:
    int mNumber;
    QSize mBoardPixelSize;
};

#endif // LEVEL_H
