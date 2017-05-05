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


#pragma once

#include <QTcpSocket>
#include <QIODevice>
#include <QScopedPointer>


class ConnectionManager : public QObject
{
	Q_OBJECT

private:
	ConnectionManager(const ConnectionManager &other);
	ConnectionManager & operator=(const ConnectionManager &other);

public:
	ConnectionManager();
	~ConnectionManager();

	bool isConnected() const;

	void setCameraIp(const QString &value);
	QString getCameraIp() const;

	QString getCameraPort() const;
	void setCameraPort(const QString &value);

	void setGamepadIp(const QString &value);

	void setGamepadPort(const quint16 &value);

	QString getGamepadIp() const;

	quint16 getGamepadPort() const;

public slots:
	void connectToHost();
	void disconnectFromHost();
	void write(const QString &);

signals:
	void stateChanged(QAbstractSocket::SocketState socketState);
	void dataWasWritten(int);
	void connectionFailed();

private:
	QScopedPointer<QTcpSocket> socket;
	QString cameraIp;
	QString cameraPort;

	QString gamepadIp;
	quint16 gamepadPort;
};
