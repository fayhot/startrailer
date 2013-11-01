#include "compositetrailstask.h"
#include "startrailer.h"

#include <QDebug>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMimeType>
#include <QElapsedTimer>

#include "libraw/libraw.h"

CompositeTrailsTask::~CompositeTrailsTask()
{
    delete m_out_image;
    m_out_image=0;
}


void CompositeTrailsTask::run()
{
    qDebug() << "CompositeTrailsTask::run() starts";
    if (*m_stopped)
        return;



    m_out_image->read(m_sourceFiles.first().toStdString());
    Image image;

    if (*m_stopped)
        return;

    QElapsedTimer timer;
    timer.start();


    int counter = 0;
    //foreach (const QString &source, m_sourceFiles) {
//    int send_preview_at = m_preview_each_n_image + m_task_index * m_preview_each_n_image;
    qint64 redraw_each_ms = m_preview_each_n_ms + m_task_index * m_preview_each_n_ms;
    for (int i = 1; i < m_sourceFiles.size(); ++i)
    {
        //        QElapsedTimer timer;
        //        timer.start();
        if (*m_stopped) return;

        image.read(m_sourceFiles[i].toStdString());

        if (*m_stopped) return;

        m_out_image->composite(image, m_compose_op);

        ++counter;

        if (*m_stopped) return;

        //delete image;

        //         qDebug() << "Two images composed in " << timer.elapsed() << "milliseconds";

        // set progress
        QMetaObject::invokeMethod(m_receiver, "announceProgress", Qt::QueuedConnection, Q_ARG(int, counter));
        //if (m_preview_each_n_image>0 && counter==send_preview_at)
        if (m_preview_each_n_ms>0 && timer.elapsed() > redraw_each_ms)
        {
            timer.restart();
            if (m_mutex->tryLock())
            {
                m_preview_image->composite(*m_out_image, m_compose_op);
                QMetaObject::invokeMethod(m_receiver, "redrawPreview", Qt::QueuedConnection);
                m_mutex->unlock();
            }
            //send_preview_at += m_preview_each_n_image * m_tasks_count;
        }

    }

    if (counter > 0)
    {
        m_mutex->lock();
        m_preview_image->composite(*m_out_image, m_compose_op);
        QMetaObject::invokeMethod(m_receiver, "redrawPreview", Qt::QueuedConnection);
        m_mutex->unlock();
    }

    QMetaObject::invokeMethod(m_receiver, "composingFinished", Qt::QueuedConnection);
    qDebug() << "CompositeTrailsTask::run() ended";
}
