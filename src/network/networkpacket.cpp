/*
 * GuLinux Planetary Imager - https://github.com/GuLinux/PlanetaryImager
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

#include "networkpacket.h"
#include <QIODevice>
#include <QDataStream>
#include <QJsonDocument>
using namespace std;

DPTR_IMPL(NetworkPacket) {
  QVariantMap properties;
};

NetworkPacket::NetworkPacket() : dptr()
{
}

NetworkPacket::NetworkPacket(const NameType& name) : NetworkPacket()
{
  setName(name);
}


NetworkPacket::~NetworkPacket()
{
}

void NetworkPacket::sendTo(QIODevice *device) const
{
  QByteArray data = QJsonDocument::fromVariant(d->properties).toBinaryData();
  QDataStream s{device};
  s << data;
  qDebug() << "Sent data: " << data;
}

void NetworkPacket::receiveFrom(QIODevice *device)
{
  QByteArray data;
  QDataStream s{device};
  s >> data;
  qDebug() << "Got data: " << data;
  d->properties = QJsonDocument::fromBinaryData(data).toVariant().toMap();
}

NetworkPacket * NetworkPacket::setProperty(const KeyType& property, const QVariant& value)
{
  d->properties[property] = value;
  return this;
}

QVariant NetworkPacket::property(const KeyType& name) const
{
  return d->properties[name];
}

NetworkPacket::NameType NetworkPacket::name() const
{
  return property("name").value<NameType>();
}

NetworkPacket *NetworkPacket::setName(const NameType& name)
{
  return setProperty("name", name);
}

NetworkPacket::ptr operator<<(NetworkPacket::ptr packet, const NetworkPacket::Property& property)
{
  packet->setProperty(property.key, property.value);
  return packet;
}



QDebug operator<<(QDebug dbg, const NetworkPacket& packet)
{
  dbg << packet.d->properties;
  return dbg;
}
