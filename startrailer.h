#ifndef STARTRAILER_H
#define STARTRAILER_H
#include <Magick++.h>
#include <QByteArray>
#include <QList>
#include <QStringList>
#include <QModelIndexList>
#include <QFileSystemModel>

//typedef QList<QModelIndex> QModelIndexList;

class StarTrailer
{
public:
    StarTrailer();
//    void compose(const Image::Image &one, const Image::Image &another, CompositeOperator method);
    Magick::Image *compose_first_with_second(Magick::Image *first, Magick::Image *second);

    const Magick::Image *compose(const Magick::Image &image_one_path, const Magick::Image &image_another_path);
    const Magick::Image *compose(const Magick::Image &one, const std::string &another_image_path);
    const Magick::Image *compose(const std::string &image_one_path, const std::string &another_image_path);

    const QByteArray *image_to_qbyte_array(Magick::Image *image);
    const QByteArray *image_to_qbyte_array(Magick::Image &image);

    const QByteArray *q_compose(const std::string &image_one_path, const std::string &image_another_path);
    const QByteArray *q_compose_list(QStringList files);
    const QByteArray *q_compose_model_list(const QFileSystemModel *model, QModelIndexList list);
};

#endif // STARTRAILER_H
