#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QModelIndexList>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMimeType>
#include <QElapsedTimer>
#include "startrailer.h"
#include <QMessageBox>
#include <QListIterator>
#include <QtConcurrent/QtConcurrent>
#include <QThread>
#include <QMutex>
#include <Magick++.h>
#include <QFileDialog>
#include "playbackreader.h"

#include "compositetrailstask.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{    
    preview_image=0;
    compose_op = MagickCore::LightenIntensityCompositeOp;

    ui->setupUi(this);

    //    ui->progressBar->hide();

    model = new QFileSystemModel;

    QString start_path = QDir::currentPath();
    QStringList cmdline_args = QCoreApplication::arguments();

    if (cmdline_args.size()>1)
    {
        cmdline_args.pop_front();
        start_path = cmdline_args.first();
    }

    model->setRootPath(start_path);
    qDebug() << "QDir::currentPath(): " << start_path;
    //model->setFilter( QDir::AllDirs | QDir::NoDotAndDotDot );   


    ui->filesList->setModel(model);
    //ui->filesList->setSortingEnabled(true);
    ui->filesList->setRootIndex(model->index(start_path));    
    model->sort(0);

    item = new QGraphicsPixmapItem();

    scene = new QGraphicsScene();
    scene->addItem(item);

    qDebug() << "QThread::idealThreadCount(): " << QThread::idealThreadCount();

    ui->graphicsView->setBackgroundBrush(QBrush(QColor(32,32,32), Qt::SolidPattern));
    ui->graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    //scene->addItem(item);
    ui->graphicsView->setScene(scene);
    item->setTransformationMode(Qt::SmoothTransformation);

    ui->splitter->setStretchFactor(1, 10);

    QObject::connect(ui->filesList->selectionModel(), &QItemSelectionModel::selectionChanged,
                     this, &MainWindow::selectionChanged);

    progress_bar = new QProgressBar(this);
    progress_bar->hide();
    ui->statusBar->addPermanentWidget(progress_bar);
    started_threads=0;

    preview_each_n_image=1;

    preview_each_n_group = new QActionGroup( this );
    ui->actionPreviewEach_image->setActionGroup(preview_each_n_group);
    ui->actionPreviewEach_5th->setActionGroup(preview_each_n_group);
    ui->actionPreviewEach_10th->setActionGroup(preview_each_n_group);
    ui->actionPreviewEach_25th->setActionGroup(preview_each_n_group);
    ui->actionPreviewEach_50th->setActionGroup(preview_each_n_group);
    ui->actionPreviewEach_100th->setActionGroup(preview_each_n_group);
    ui->actionWithout_preview->setActionGroup(preview_each_n_group);
}

MainWindow::~MainWindow()
{       
    delete progress_bar;
    delete scene;
    //delete item;
    delete ui;
    delete model;
    delete preview_each_n_group;
}

void MainWindow::handleFinished()
{
    static int finished_runners = 0;
    ++finished_runners;
    qDebug() << "handleFinished(): " << finished_runners;

    if (finished_runners >= QThread::idealThreadCount())
    {
        finished_runners=0;


        //drawMagickImage(out);
    }
}

void MainWindow::on_filesList_doubleClicked(const QModelIndex &index)
{
    QString path = model->filePath(index);

    if (model->fileInfo(index).isDir())
    {
        ui->filesList->setRootIndex(index);
        model->setRootPath(model->filePath(index));
        model->sort(0);
    }
    else
    {
        QMimeDatabase mimeDatabase;
        QMimeType mimeType;
        mimeType = mimeDatabase.mimeTypeForFile(path);

        //        QImage image(path);
        //        item->setPixmap(QPixmap::fromImage(image));
        //        item->setTransformationMode(Qt::SmoothTransformation);
        //        ui->graphicsView->fitInView(item, Qt::KeepAspectRatio);
    }

}


void MainWindow::on_actionBack_triggered()
{
    QModelIndex parent_index = model->parent(ui->filesList->rootIndex());
    ui->filesList->setRootIndex(parent_index);
    model->setRootPath(model->filePath(parent_index));
    model->sort(0);
}


void MainWindow::on_actionComposite_triggered()
{
    compositeSelected();
}

void MainWindow::on_filesList_clicked(const QModelIndex &index)
{
//    QString path = model->filePath(index);
//    //        qDebug() << path;

//    QMimeDatabase mimeDatabase;
//    QMimeType mimeType;
//    mimeType = mimeDatabase.mimeTypeForFile(path);
    //        qDebug() << mimeType.name();
    //        qDebug() << "Valid: " << mimeType.isValid();

    //    QImage image(path);
    //    if (item)
    //        delete item;
    //    item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    //    scene->addItem(item);
    //    ui->graphicsView->setScene(scene);
    //    item->setTransformationMode(Qt::SmoothTransformation);
    //    ui->graphicsView->fitInView(item, Qt::KeepAspectRatio);


}

void MainWindow::compositeSelected()
{
    static QMutex mutex;
    mutex.lock();
    stopped = true;
    QThreadPool::globalInstance()->waitForDone();
    qDebug() << "Start composing...";

    QElapsedTimer timer;
    timer.start();

    //    StarTrailer st;
    //    const QByteArray *image_bytes = st.q_compose(model->filePath(list[0]).toStdString(), model->filePath(list[1]).toStdString());

    //const QByteArray *image_bytes = st.q_compose_model_list(model, ui->filesList->selectionModel()->selectedIndexes());

    QStringList files;
    QModelIndexList selected_rows = ui->filesList->selectionModel()->selectedRows(0);
    QListIterator<QModelIndex> i(selected_rows);
    if (selected_rows.size()>1 && i.hasNext())
    {
        while (i.hasNext())
        {
            QModelIndex index = i.next();
            if (!model->isDir(index))
                files << model->filePath(index);
        }

        files.sort();

        //qDebug() << "Files: " << files;
        qDebug() << "files.count(): " << files.count();

        QVector<int> sizes = chunkSizes(files.count(), QThread::idealThreadCount());

        qDebug() << "schunk sizes: " << sizes;

        progress_bar->setMaximum(files.size() + sizes.size() - sizes.count(0));
        progress_bar->setValue(0);
        progress_bar->show();
        int offset=0;
        //preview_image.read(files[0].toStdString());
        if (preview_image)
        {
            delete preview_image;
            preview_image=0;
        }
        preview_image = st.read_image(files[0].toStdString());

        qDebug() << "Files list made from selection and first image prepared in " << timer.elapsed() << "milliseconds";

        QThreadPool::globalInstance()->waitForDone();

        started_threads=0;
        for (int n=0; n<sizes.size(); n++)
        {
            if (sizes[n]>0)
            {
                CompositeTrailsTask *task = new CompositeTrailsTask(this, &stopped, files.mid(offset, sizes[n]), preview_each_n_image, compose_op);
                QThreadPool::globalInstance()->start(task);
                ++started_threads;
                offset += sizes[n];
            }
        }
    }
    else
    {
        //QMessageBox::information(this, tr("No files selected"), tr("Please select files to composite."));
        ui->statusBar->showMessage(tr("Please select files to composite."), 3000);
    }
    qDebug() << "---------------------------\n";
    mutex.unlock();
}

void MainWindow::selectEachNRow(int n)
{
    QItemSelection selection;
    int row=0;
    while(ui->filesList->rootIndex().child(row, 0).isValid())
    {
        selection.append(QItemSelectionRange(ui->filesList->rootIndex().child(row, 0)));
        row+=n;
    }
    ui->filesList->selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

void MainWindow::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{

    static QTimer *timer=0;
    if (timer)
    {
        timer->stop();
        delete timer;
        timer=0;
    }

    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(slot_compositeSelected()));
    timer->start(20);
}

void MainWindow::drawMagickImage(Magick::Image image)
{    
    const QByteArray *image_bytes = st.image_to_qbyte_array(image);
    QPixmap qpm;
    qpm.loadFromData(*image_bytes);    
    delete image_bytes;
    item->setPixmap(qpm);
    ui->graphicsView->fitInView(item, Qt::KeepAspectRatio);
}


void MainWindow::on_actionUpdateFileList_triggered()
{
    ui->filesList->update(ui->filesList->rootIndex());
}

void MainWindow::on_actionClearSelection_triggered()
{
    ui->filesList->selectionModel()->clearSelection();
    ui->statusBar->showMessage(tr("Selection cleared"), 5000);
}

void MainWindow::announceProgress(int counter)
{
    progress_bar->setValue( progress_bar->value() + 1 );
}

void MainWindow::receiveMagickImage(Magick::Image *image)
{
    static QMutex mutex;
    //mutex.lock();
    //qDebug() << "receiveMagickImage()";
    //    StarTrailer st;

    st.compose_first_with_second(preview_image, image, compose_op);
    drawMagickImage(*preview_image);
    delete image;
    ui->action_Save_as->setEnabled(true);
    //mutex.unlock();
}

void MainWindow::composingFinished()
{
    //qDebug() << "QThreadPool::globalInstance()->activeThreadCount(): " << QThreadPool::globalInstance()->activeThreadCount();
    --started_threads;
    progress_bar->setValue( progress_bar->value() + 1 );
    if (started_threads<=0)
    {
        progress_bar->hide();
        progress_bar->setValue(0);
    }
}



void MainWindow::on_action_Save_as_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                    model->filePath(ui->filesList->rootIndex()),
                                                    tr("Images (*.png *.jpg *.tif *.bmp)"));
    preview_image->write(fileName.toStdString());
}

void MainWindow::on_actionE_xit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_action_About_triggered()
{
    QMessageBox::about(this, tr("Startrailer"), tr("<h1>Startrailer</h1>"
                                                   "Helps to make star trails from your photos.<br>"
                                                   "<a href=\"https://github.com/zuf/startrailer\">https://github.com/zuf/startrailer</a><br><br>"
                                                   "Git: %1<br>"
                                                   "From: %2").arg(APP_REVISION).arg(QString::fromLocal8Bit(BUILDDATE)));
}

void MainWindow::slot_compositeSelected()
{
    progress_bar->hide();
    progress_bar->setValue(0);

    ui->statusBar->showMessage(tr("Selected: %1").arg(ui->filesList->selectionModel()->selectedRows().count()));

    if (ui->filesList->selectionModel()->selectedRows().size()==1)
    {
        QModelIndex index = ui->filesList->selectionModel()->selectedIndexes().first();
        if (!model->isDir(index))
        {
            preview_image = st.read_image(model->filePath(index).toStdString());
            drawMagickImage(*preview_image);
        }
    }
    else
    {
        if (ui->actionAuto_trail_on_select->isChecked())
            compositeSelected(); // TODO: Doesn't redraw all. If selection only added, then add them to current preview.
        // If deselected.size()>0 then redraw all
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    //    if (maybeSave()) {
    //        writeSettings();
    //        event->accept();
    //    } else {
    //        event->ignore();
    //    }
    stopped = true;
}

void MainWindow::on_actionClear_2_triggered()
{
    // TODO: Dry with toolbar button
    ui->filesList->selectionModel()->clearSelection();
    ui->statusBar->showMessage(tr("Selection cleared"), 5000);
}

void MainWindow::on_actionAll_triggered()
{
    ui->filesList->selectAll();
}


void MainWindow::on_actionSelectEach_2_triggered()
{
    selectEachNRow(2);
}

void MainWindow::on_actionSelectEach_5th_triggered()
{
    selectEachNRow(5);
}

void MainWindow::on_actionSelectEach_10th_triggered()
{
    selectEachNRow(10);
}

void MainWindow::on_actionSelectEach_25th_triggered()
{
    selectEachNRow(25);
}

void MainWindow::on_actionSelectEach_50th_triggered()
{
    selectEachNRow(50);
}

void MainWindow::on_actionSelectEach_100_triggered()
{
    selectEachNRow(100);
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::on_actionPreviewEach_image_triggered()
{
    preview_each_n_image = 1;
}

void MainWindow::on_actionPreviewEach_5th_triggered()
{
    preview_each_n_image = 5;
}

void MainWindow::on_actionPreviewEach_10th_triggered()
{
    preview_each_n_image = 10;
}

void MainWindow::on_actionPreviewEach_25th_triggered()
{
    preview_each_n_image = 25;
}

void MainWindow::on_actionPreviewEach_50th_triggered()
{
    preview_each_n_image = 50;
}

void MainWindow::on_actionPreviewEach_100th_triggered()
{
    preview_each_n_image = 100;
}

void MainWindow::on_actionWithout_preview_triggered()
{
    preview_each_n_image = 0;
}

void MainWindow::on_actionDifference_triggered()
{
    compose_op = Magick::DifferenceCompositeOp;
}



void MainWindow::on_actionPlay_triggered()
{
    stopped = true;
    QThreadPool::globalInstance()->waitForDone();

    QStringList files;
    QModelIndexList selected_rows = ui->filesList->selectionModel()->selectedRows(0);
    QListIterator<QModelIndex> i(selected_rows);

    static QQueue<const QByteArray*> images_bytes_queue;
    static QMutex images_bytes_queue_mutex;

    if (selected_rows.size()>1 && i.hasNext())
    {
        stopped = false;
        while (i.hasNext())
        {
            QModelIndex index = i.next();
            if (!model->isDir(index))
                files << model->filePath(index);
        }

        files.sort();
        QVector<int> sizes = chunkSizes(files.count(), QThread::idealThreadCount());


        int offset=0;
        if (preview_image)
        {
            delete preview_image;
            preview_image=0;
        }
        preview_image = st.read_image(files[0].toStdString());

        QThreadPool::globalInstance()->waitForDone();

        started_threads=0;
        for (int n=0; n<sizes.size(); n++)
        {
            if (sizes[n]>0)
            {
                //PlaybackReader *task = new PlaybackReader(&images_bytes_queue, &images_bytes_queue_mutex, files);
                PlaybackReader *task = new PlaybackReader(&images_bytes_queue, &images_bytes_queue_mutex, files.mid(offset, sizes[n]));

                QThreadPool::globalInstance()->start(task);
                ++started_threads;
                offset += sizes[n];
            }
        }

        static int processed_files=0;
        processed_files=0;
        while(processed_files<files.size())
        {
            images_bytes_queue_mutex.lock();
            if (images_bytes_queue.size()>1)
            {
                const QByteArray *image_bytes = images_bytes_queue.dequeue();
                QPixmap qpm;
                qpm.loadFromData(*image_bytes);
                delete image_bytes;
                item->setPixmap(qpm);
                ui->graphicsView->fitInView(item, Qt::KeepAspectRatio);
                delete image_bytes;
                ++processed_files;
            }
            images_bytes_queue_mutex.unlock();
            QApplication::processEvents();
        }

    }
    else
    {
        ui->statusBar->showMessage(tr("Please select files to play."), 3000);
    }
}