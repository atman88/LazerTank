#ifndef LEVELCOMPLETEDDIALOG_H
#define LEVELCOMPLETEDDIALOG_H

#include <QMessageBox>
#include <QModelIndex>

class GameRegistry;

class LevelCompletedDialog : public QMessageBox
{
public:
    typedef enum {
        None,
        Replay,
        Next,
        Exit
    } ButtonCode;

    LevelCompletedDialog( GameRegistry* registry );

    /**
     * @brief Query which button has been clicked
     */
    ButtonCode getClickedCode() const;

private slots:
    void onLevelUpdated( const QModelIndex& );

private:
    void setLevelCountInternal();

    GameRegistry* mRegistry;
};

#endif // LEVELCOMPLETEDDIALOG_H
