/* Copyright 2017 Konstantin Batoev.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file was modified by Konstantin Batoev to make it comply with the requirements of trikRuntime
 * project. See git revision history for detailed changes. */

#include "connectionManager.h"

ConnectionManager::ConnectionManager()
	: socket(new QTcpSocket(this))
	, cameraIp("192.168.77.1")
	, cameraPort("8080")
	, gamepadIp("192.168.77.1")
	, gamepadPort(4444)
{
	/// passing this to QTcpSocket forces automatically socket->moveToThread()
	/// when calling connectionManaget.moveToThread()
	qRegisterMetaType<QAbstractSocket::SocketState>();
	connect(socket.data(), SIGNAL(stateChanged(QAbstractSocket::SocketState)),
			this, SIGNAL(stateChanged(QAbstractSocket::SocketState)));
}

ConnectionManager::~ConnectionManager()
{
	disconnect(socket.data(), SIGNAL(stateChanged(QAbstractSocket::SocketState)),
			   this, SIGNAL(stateChanged(QAbstractSocket::SocketState)));
}

bool ConnectionManager::isConnected() const
{
	return socket->state() == QTcpSocket::ConnectedState;
}

QString ConnectionManager::getCameraIp() const
{
	return cameraIp;
}

void ConnectionManager::write(const QString &data)
{
	qint64 result = socket->write(data.toLatin1().data());
	emit dataWasWritten(static_cast<int>(result));
}

quint16 ConnectionManager::getGamepadPort() const
{
	return gamepadPort;
}

QString ConnectionManager::getGamepadIp() const
{
	return gamepadIp;
}

void ConnectionManager::connectToHost()
{
	const int timeout = 3 * 1000;
	socket->connectToHost(gamepadIp, gamepadPort);
	if (!socket->waitForConnected(timeout))
		emit connectionFailed();
}

void ConnectionManager::disconnectFromHost()
{
	socket->disconnectFromHost();
}

void ConnectionManager::setCameraIp(const QString &value)
{
	cameraIp = value;
}

QString ConnectionManager::getCameraPort() const
{
	return cameraPort;
}

void ConnectionManager::setCameraPort(const QString &value)
{
	cameraPort = value;
}

void ConnectionManager::setGamepadPort(const quint16 &value)
{
	gamepadPort = value;
}

void ConnectionManager::setGamepadIp(const QString &value)
{
	gamepadIp = value;
}
