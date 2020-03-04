#include "com/Mustang.h"
#include "ui/tunerthread.h"


namespace plug
{

TunerThread::TunerThread(QObject *parent, bool b) :
    QThread(parent), Stop(b), amp_ops(nullptr)
{
}

// run() will be called when a thread starts
void TunerThread::run()
{
    printf("TunerThread:run()\n");
    std::array<int8_t, 2> data{};
    char const*notes[] = {"A", "A#", "B", "C"
                , "C#" ,"D", "D#", "E"
                , "F", "F#", "G", "G#", "?"};
    uint8_t note;
    int8_t  distance;

    while ( true )
    {
        // prevent other threads from changing the "Stop" value
        if(this->Stop) break;

        data = amp_ops->getTunerData();
       // printf("data.size: %d\n", static_cast<int>(data.size()));
        if (data[0] != -1 ) {
        note=  (data[0] > 12 ? 12 : data[0]);
        distance=static_cast<int8_t>(data[1]);

        // emit the signal for the count label
        emit valueChanged(QString("%1 %2 %3").arg(
            (distance > 3 || note == 12 ? "" : ">>")
            ,notes[note]
            ,(distance < -3  || note == 12 ? "" : "<<"))
            );
        printf(":%d %d:\n",data[0],data[1]);
        }
        // slowdown the count change, msec
        this->msleep(5);
    }
   // amp_ops->flushTunerData();
    printf("TunerThread:run() exit\n");
}

void TunerThread::setStop(bool b)
{

        this->Stop = b;
}

void TunerThread::setAmpOps(std::shared_ptr<com::Mustang>a) 
{
    amp_ops=a;
}
}


#include "ui/moc_tunerthread.moc"
