#pragma once
#include "ui/mainwindow.h"
#include <QThread>
#include <QString>

namespace Ui
{
    class TunerThread;
}

namespace plug
{

    class TunerThread : public QThread
    {
        Q_OBJECT
        public:
            explicit TunerThread(QObject *parent = 0, bool b = false);
            void setAmpOps(com::Mustang *amp_ops);
            void run();

            // if Stop = true, the thread will break
            // out of the loop, and will be disposed
            bool Stop;
        private:
            com::Mustang *amp_ops;
        signals:
            // To communicate with Gui Thread
            // we need to emit a signal
            void valueChanged(QString);

        public slots:
            void setStop(bool b);
    };
}

