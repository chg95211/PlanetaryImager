/*
 * Copyright (C) 2016  Marco Gulino <marco@gulinux.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "serwriter.h"
#include <QFile>
#include <QDebug>
#include <QDateTime>
#include "ser_header.h"


using namespace std;
using namespace std::placeholders;


DPTR_IMPL(SERWriter) {
  SERWriter *q;
  QFile file;
  SER_Header *header;
  uint32_t frames = 0;
  vector<QDateTime> frames_datetimes;
  SER_Timestamp timestamp(const QDateTime &datetime) const;
};

SERWriter::SERWriter ( const QString& deviceName, Configuration& configuration ) : dptr(this)
{
  d->file.setFileName(configuration.savefile());
  qDebug() << "Using buffered output: " << configuration.buffered_output();
  if(configuration.buffered_output()) {
    d->file.open(QIODevice::ReadWrite);
  }
  else {
    d->file.open(QIODevice::ReadWrite | QIODevice::Unbuffered);
  }
  SER_Header empty_header;
  empty_header.datetime = d->timestamp(QDateTime::currentDateTime());
  empty_header.datetime_utc = d->timestamp(QDateTime::currentDateTimeUtc());
  ::strcpy(empty_header.camera, deviceName.left(40).toLatin1());
  ::strcpy(empty_header.observer, configuration.observer().left(40).toLatin1());
  ::strcpy(empty_header.telescope, configuration.telescope().left(40).toLatin1());
  d->file.write(reinterpret_cast<char*>(&empty_header), sizeof(empty_header));
  d->file.flush();
  d->header = reinterpret_cast<SER_Header*>(d->file.map(0, sizeof(SER_Header)));
  if(!d->header) {
    qDebug() << d->file.errorString();
  }
}

SER_Timestamp SERWriter::Private::timestamp(const QDateTime& datetime) const
{
  static const QDateTime reference{{1, 1, 1}, {0,0,0}};
  return reference.msecsTo(datetime) * 10000;
}


SERWriter::~SERWriter()
{
  qDebug() << "closing file..";
  d->header->frames = d->frames;
  for(auto datetime: d->frames_datetimes) {
    SER_Timestamp timestamp = d->timestamp(datetime);
    d->file.write(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
  }
  d->file.close();
  qDebug() << "file correctly closed.";
}

QString SERWriter::filename() const
{
  return d->file.fileName();
}

void SERWriter::handle ( const Frame::ptr &frame )
{
  static map<Frame::ColorFormat, SER_Header::ColorId> color_formats{
    {Frame::Mono, SER_Header::MONO},
    {Frame::Bayer_RGGB, SER_Header::BAYER_RGGB},
    {Frame::Bayer_GRBG, SER_Header::BAYER_GRBG},
    {Frame::Bayer_GBRG, SER_Header::BAYER_GBRG},
    {Frame::Bayer_BGGR, SER_Header::BAYER_BGGR},
        // TODO: not yet handled
//        BAYER_CYYM = 16,
//        BAYER_YCMY = 17,
//        BAYER_YMCY = 18,
//        BAYER_MYYC = 19,
    {Frame::RGB, SER_Header::RGB},
    {Frame::BGR, SER_Header::BGR},
  };
  
  if(! d->header->imageWidth) {
    d->header->colorId = color_formats[frame->colorFormat()];
    d->header->pixelDepth = frame->bpp();
    qDebug() << "SER PixelDepth set to " << d->header->pixelDepth << "bpp";
    d->header->imageWidth = frame->resolution().width();
    d->header->imageHeight = frame->resolution().height();
  }
  d->frames_datetimes.push_back(frame->created_utc());
  auto frame_bytes = frame->size();
  size_t wrote_bytes = d->file.write(reinterpret_cast<const char*>(frame->mat().data), frame->size());
  if(wrote_bytes == frame->size() ) {
    ++d->frames;
  } else {
    qWarning() << "Error writing frame: wrote only " << wrote_bytes << " instead of " << frame_bytes;
  }
}