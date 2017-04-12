/* Copyright 2015-2016 CyberTech Labs Ltd.
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
 * This file was modified by Yurii Litvinov, Aleksei Alekseev, Mikhail Wall and Konstantin Batoev to make it comply with the requirements of
 * trikRuntime project. See git revision history for detailed changes. */

#include "gamepadForm.h"
#include "ui_gamepadForm.h"

#include <QtWidgets/QMessageBox>
#include <QtGui/QKeyEvent>

#include <QNetworkRequest>
#include <QMediaContent>
#include <QFontDatabase>

GamepadForm::GamepadForm()
	: QWidget()
	, mUi(new Ui::GamepadForm())
{
	// Here all GUI widgets are created and initialized.
	mUi->setupUi(this);
	this->installEventFilter(this);
	setUpGamepadForm();
	startThread();

	QVideoProbe *probe = new QVideoProbe;
	connect(probe, SIGNAL(videoFrameProbed(QVideoFrame)), this, SLOT(mySlot(QVideoFrame)));
	bool result = probe->setSource(player);
	isFrameNecessary = 0;
	qDebug() << result;
	clipboard = QApplication::clipboard();
}

GamepadForm::~GamepadForm()
{
	// disabling socket from thread where it was enabled
	emit programFinished();
	// stopping thread
	thread.quit();
	// waiting thread to quit
	thread.wait();
}

void GamepadForm::setUpGamepadForm()
{
	createMenu();
	setFontToPadButtons();
	createConnection();
	setVideoController();
	setLabels();
	setUpControlButtonsHash();
	retranslate();
}

void GamepadForm::setVideoController()
{
	player = new QMediaPlayer(this, QMediaPlayer::StreamPlayback);
	connect(player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(handleMediaStatusChanged(QMediaPlayer::MediaStatus)));

	videoWidget = new QVideoWidget(this);
	player->setVideoOutput(videoWidget);
	videoWidget->setMinimumSize(320, 240);
	videoWidget->setVisible(false);
	mUi->verticalLayout->addWidget(videoWidget);
	mUi->verticalLayout->setAlignment(videoWidget, Qt::AlignCenter);

	movie.setFileName(":/images/loading.gif");
	mUi->loadingMediaLabel->setVisible(false);
	mUi->loadingMediaLabel->setMovie(&movie);

	QPixmap pixmap(":/images/noVideoSign.png");
	mUi->invalidMediaLabel->setPixmap(pixmap);

	mUi->loadingMediaLabel->setVisible(false);
	mUi->invalidMediaLabel->setVisible(false);

}

void GamepadForm::handleMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
	movie.setPaused(true);
	switch (status) {
	case QMediaPlayer::StalledMedia:
	case QMediaPlayer::LoadedMedia:
	case QMediaPlayer::BufferingMedia:
		player->play();
		mUi->loadingMediaLabel->setVisible(false);
		mUi->invalidMediaLabel->setVisible(false);
		mUi->label->setVisible(false);
		videoWidget->setVisible(true);
		break;

	case QMediaPlayer::LoadingMedia:
		mUi->invalidMediaLabel->setVisible(false);
		mUi->label->setVisible(false);
		videoWidget->setVisible(false);
		mUi->loadingMediaLabel->setVisible(true);
		movie.setPaused(false);
		break;

	case QMediaPlayer::InvalidMedia:
		mUi->loadingMediaLabel->setVisible(false);
		mUi->label->setVisible(false);
		videoWidget->setVisible(false);
		mUi->invalidMediaLabel->setVisible(true);
		break;

	case QMediaPlayer::NoMedia:
	case QMediaPlayer::EndOfMedia:
		mUi->loadingMediaLabel->setVisible(false);
		mUi->invalidMediaLabel->setVisible(false);
		videoWidget->setVisible(false);
		mUi->label->setVisible(true);
	default:
		break;
	}
}

void GamepadForm::startVideoStream()
{
	const QString ip = connectionManager.getCameraIp();
	const QString port = connectionManager.getCameraPort();
	const auto status = player->mediaStatus();

	if (status == QMediaPlayer::NoMedia || status == QMediaPlayer::EndOfMedia || status == QMediaPlayer::InvalidMedia) {
		const QString url = "http://" + ip + ":" + port + "/?action=stream";
		//QNetworkRequest nr = QNetworkRequest(url);
		//nr.setPriority(QNetworkRequest::LowPriority);
		//nr.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);
		player->setMedia(QUrl(url));
	}
}

void GamepadForm::checkSocket(QAbstractSocket::SocketState state)
{
	switch (state) {
	case QAbstractSocket::ConnectedState:
		mUi->disconnectedLabel->setVisible(false);
		mUi->connectedLabel->setVisible(true);
		mUi->connectingLabel->setVisible(false);
		setButtonsCheckable(true);
		setButtonsEnabled(true);
		break;

	case QAbstractSocket::ConnectingState:
		mUi->disconnectedLabel->setVisible(false);
		mUi->connectedLabel->setVisible(false);
		mUi->connectingLabel->setVisible(true);
		setButtonsCheckable(false);
		setButtonsEnabled(false);
		break;

	case QAbstractSocket::UnconnectedState:
	default:
		mUi->disconnectedLabel->setVisible(true);
		mUi->connectedLabel->setVisible(false);
		mUi->connectingLabel->setVisible(false);
		setButtonsCheckable(false);
		setButtonsEnabled(false);
		break;
	}
}

void GamepadForm::startThread()
{
	connectionManager.moveToThread(&thread);
	thread.start();
}

void GamepadForm::checkBytesWritten(int result)
{
	if (result == -1) {
		setButtonsEnabled(false);
		setButtonsCheckable(false);
	}
}

void GamepadForm::showConnectionFailedMessage()
{
	QMessageBox failedConnectionMessage(this);
	failedConnectionMessage.setText(tr("Couldn't connect to robot"));
	failedConnectionMessage.exec();
}

void GamepadForm::setFontToPadButtons()
{
	const int id = QFontDatabase::addApplicationFont(":/fonts/freemono.ttf");
	const QString family = QFontDatabase::applicationFontFamilies(id).at(0);
	const int pointSize = 50;
	QFont font(family, pointSize);

	mUi->buttonPad1Down->setFont(font);
	mUi->buttonPad1Up->setFont(font);
	mUi->buttonPad1Left->setFont(font);
	mUi->buttonPad1Right->setFont(font);

	mUi->buttonPad2Down->setFont(font);
	mUi->buttonPad2Up->setFont(font);
	mUi->buttonPad2Left->setFont(font);
	mUi->buttonPad2Right->setFont(font);
}
QImage GamepadForm::convertBits(const QVideoFrame &frame) {

	//int resultBits[size];
	qDebug() << frame.mappedBytes();
	qDebug() << frame.size();
	qDebug() << "planes count: " << frame.planeCount();
	qDebug() << "bytes of 0 plane: " << frame.bytesPerLine(0);
	qDebug() << "bytes of 1 plane: " << frame.bytesPerLine(1);
	qDebug() << "bytes of 2 plane: " << frame.bytesPerLine(2);
	qDebug() << "bytes of 3 plane: " << frame.bytesPerLine(3);

	/*
	size.total = size.width * size.height;
	y = yuv[position.y * size.width + position.x];
	u = yuv[(position.y / 2) * (size.width / 2) + (position.x / 2) + size.total];
	v = yuv[(position.y / 2) * (size.width / 2) + (position.x / 2) + size.total + (size.total / 4)];
	*/
	int width = frame.width();
	int height = frame.height();
	int size = height * width;
	const uchar *data = frame.bits();
	QImage img(width, height, QImage::Format_RGB32);
	//img.fill(QColor(Qt::white).rgb());
	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++) {
			int y = static_cast<int> (data[i * width + j]);
			int u = static_cast<int> (data[(i / 2) * (width / 2) + (j / 2) + size]);
			int v = static_cast<int> (data[(i / 2) * (width / 2) + (j / 2) + size + (size / 4)]);

			//float R = Y + 1.402 * (V - 128);
			//float G = Y - 0.344 * (U - 128) - 0.714 * (V - 128);
			//float B = Y + 1.772 * (U - 128);

			int r = y + int(1.13983 * (v - 128));
			int g = y - int(0.39465 * (u - 128)) - int(0.58060 * (v - 128));
			int b = y + int(2.03211 * (u - 128));
			//int r = y + int(1.402 * (v - 128));
			//int g = y - int(0.714 * (v - 128)) - int(0.344 * (u - 128));
			//int b = y + int(1.772 * (u - 128));
			if (r < 0)
				r = 0;
			if (g < 0)
				g = 0;
			if (b < 0)
				b = 0;
			if (r > 255)
				r = 255;
			if (g > 255)
				g = 255;
			if (b > 255)
				b = 255;
			//r = r < 0 ? 0 : r > 255 ? 255 : r;
			//g = g < 0 ? 0 : g > 255 ? 255 : g;
			//r = b < 0 ? 0 : b > 255 ? 255 : b;

			img.setPixel(j, i, qRgb(r, g, b));
		}

	clipboard->setImage(img);
	if (img.save("img.png")) {
		int f = 1;
	}


	/*
	for (int i = 2; i < size; i += 2) {
		int y = static_cast<int> (bits[i - 2]);
		int u = static_cast<int> (bits[i - 1]);
		int v = static_cast<int> (bits[i]);

		int r = y + int(1.13983 * (v - 128));
		int g = y - int(0.39465 * (u - 128)) - int(0.58060 * (v - 128));
		int b = y + int(2.03211 * (u - 128));
		resultBits[i - 2] = r;
		resultBits[i - 1] = g;
		resultBits[i] = b;
	}
	*/
	return img;
}


void GamepadForm::mySlot(QVideoFrame buffer)
{
	if (isFrameNecessary < 5) {

		QImage img;
		QVideoFrame frame(buffer);  // make a copy we can call map (non-const) on
		frame.map(QAbstractVideoBuffer::ReadOnly);
		qDebug() << "pixel format: " << frame.pixelFormat();
		qDebug() << "size: " << frame.size();
		qDebug() << "readable " << frame.isReadable();
		qDebug() << "writeable " << frame.isWritable();
		QImage img1 = convertBits(frame);
		clipboard->setImage(img1);
		//auto bits = frame.bits();
		//QImage img1(bitsForRgb, frame.width(), frame.height(), frame.bytesPerLine(), QImage::Format_RGB32);
		//clipboard->setImage(img1);
		QString res = "";
		//for (int i = 0; i < frame.width() * frame.height() + 10; i++)
		//	res += bits[i];
		qDebug() << res;
		QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(
					frame.pixelFormat());
		qDebug() << frame.mappedBytes();
		qDebug() << frame.bytesPerLine();
		// BUT the frame.pixelFormat() is QVideoFrame::Format_Jpeg, and this is
		// mapped to QImage::Format_Invalid by
		// QVideoFrame::imageFormatFromPixelFormat
		if (imageFormat != QImage::Format_Invalid) {
			img = QImage(frame.bits(),
						 frame.width(),
						 frame.height(),
						 // frame.bytesPerLine(),
						 imageFormat);
		} else {
			// e.g. JPEG
			int nbytes = frame.mappedBytes();
			img = QImage::fromData(frame.bits(), nbytes);
		}
		frame.unmap();

		isFrameNecessary++;
		/*
		qDebug() << "validness: " << frame.isValid();
		qDebug() << "frame geometry: " << frame.size();
		qDebug() << "readable: " << frame.isReadable();
		qDebug() << "writeable: " << frame.isWritable();
		QImage::Format imageFormat =
				QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());

		qDebug() << imageFormat;
		QImage img(frame.bits(),
				   frame.width(),
				   frame.height(),
				   frame.bytesPerLine(),
				   imageFormat);
		//mUi->connectedLabel->setPixmap(QPixmap::fromImage(img));
		QLabel *label = new QLabel;
		label->setMinimumWidth(480);
		label->setMinimumHeight(640);
		qDebug() << img.save("myImg.png");
		label->setPixmap(QPixmap::fromImage(img));
		label->show();
		*/

	}
}

void GamepadForm::setButtonChecked(const int &key, bool checkStatus)
{
	controlButtonsHash[key]->setChecked(checkStatus);
}

void GamepadForm::createConnection()
{
	mMapperPadPressed = new QSignalMapper(this);
	mMapperPadReleased = new QSignalMapper(this);
	mMapperDigitPressed = new QSignalMapper(this);
	mMapperDigitReleased = new QSignalMapper(this);

	mPadHashtable = {
		{mUi->buttonPad1Up, {1, 0, 100}}
		, {mUi->buttonPad1Down, {1, 0, -100}}
		, {mUi->buttonPad1Right, {1, 100, 0}}
		, {mUi->buttonPad1Left, {1, -100, 0}}
		, {mUi->buttonPad2Up, {2, 0, 100}}
		, {mUi->buttonPad2Down, {2, 0, -100}}
		, {mUi->buttonPad2Right, {2, 100 ,0}}
		, {mUi->buttonPad2Left, {2, -100, 0}}
	};

	for (auto button : mPadHashtable.keys()) {
		connect(button, SIGNAL(pressed()), mMapperPadPressed, SLOT(map()));
		mMapperPadPressed->setMapping(button, button);
		connect(button, SIGNAL(released()), mMapperPadReleased, SLOT(map()));
		mMapperPadReleased->setMapping(button, button);
	}

	connect(mMapperPadPressed, SIGNAL(mapped(QWidget *)), this, SLOT(handlePadPress(QWidget*)));
	connect(mMapperPadReleased, SIGNAL(mapped(QWidget *)), this, SLOT(handlePadRelease(QWidget*)));

	mDigitHashtable = {
		{mUi->button1, 1}
		, {mUi->button2, 2}
		, {mUi->button3, 3}
		, {mUi->button4, 4}
		, {mUi->button5, 5}
	};

	for (auto button : mDigitHashtable.keys()) {
		connect(button, SIGNAL(pressed()), mMapperDigitPressed, SLOT(map()));
		mMapperDigitPressed->setMapping(button, button);
		connect(button, SIGNAL(released()), mMapperDigitReleased, SLOT(map()));
		mMapperDigitReleased->setMapping(button, button);
	}

	connect(mMapperDigitPressed, SIGNAL(mapped(QWidget *)), this, SLOT(handleDigitPress(QWidget*)));
	connect(mMapperDigitReleased, SIGNAL(mapped(QWidget *)), this, SLOT(handleDigitRelease(QWidget*)));

	connect(&connectionManager, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(checkSocket(QAbstractSocket::SocketState)));
	connect(&connectionManager, SIGNAL(dataWasWritten(int)), this, SLOT(checkBytesWritten(int)));
	connect(&connectionManager, SIGNAL(connectionFailed()), this, SLOT(showConnectionFailedMessage()));
	connect(this, SIGNAL(commandReceived(QString)), &connectionManager, SLOT(write(QString)));
	connect(this, SIGNAL(programFinished()), &connectionManager, SLOT(disconnectFromHost()));
}

void GamepadForm::createMenu()
{
	// Path to your local directory. Set up if you don't put .qm files into your debug folder.
	const QString pathToDir = "languages/trikDesktopGamepad";

	const QString russian = pathToDir + "_ru";
	const QString english = pathToDir + "_en";
	const QString french = pathToDir + "_fr";
	const QString german = pathToDir + "_de";

	mMenuBar = new QMenuBar(this);

	mConnectionMenu = new QMenu(this);
	mMenuBar->addMenu(mConnectionMenu);

	mLanguageMenu=  new QMenu(this);
	mMenuBar->addMenu(mLanguageMenu);

	mConnectAction = new QAction(this);
	// Set to QKeySequence for Ctrl+N shortcut
	mConnectAction->setShortcuts(QKeySequence::New);
	connect(mConnectAction, &QAction::triggered, this, &GamepadForm::openConnectDialog);

	mExitAction = new QAction(this);
	mExitAction->setShortcuts(QKeySequence::Quit);
	connect(mExitAction, &QAction::triggered, this, &GamepadForm::exit);

	mRussianLanguageAction = new QAction(this);
	mEnglishLanguageAction = new QAction(this);
	mFrenchLanguageAction = new QAction(this);
	mGermanLanguageAction = new QAction(this);

	mLanguages = new QActionGroup(this);
	mLanguages->addAction(mRussianLanguageAction);
	mLanguages->addAction(mEnglishLanguageAction);
	mLanguages->addAction(mFrenchLanguageAction);
	mLanguages->addAction(mGermanLanguageAction);
	mLanguages->setExclusive(true);

	// Set up language actions checkable
	mEnglishLanguageAction->setCheckable(true);
	mRussianLanguageAction->setCheckable(true);
	mFrenchLanguageAction->setCheckable(true);
	mGermanLanguageAction->setCheckable(true);
	mEnglishLanguageAction->setChecked(true);

	// Connecting languages to menu items
	connect(mRussianLanguageAction, &QAction::triggered, this, [this, russian](){changeLanguage(russian);});
	connect(mEnglishLanguageAction, &QAction::triggered, this, [this, english](){changeLanguage(english);});
	connect(mFrenchLanguageAction, &QAction::triggered, this, [this, french](){changeLanguage(french);});
	connect(mGermanLanguageAction, &QAction::triggered, this, [this, german](){changeLanguage(german);});

	mAboutAction = new QAction(this);
	connect(mAboutAction, &QAction::triggered, this, &GamepadForm::about);

	mConnectionMenu->addAction(mConnectAction);
	mConnectionMenu->addAction(mExitAction);

	mLanguageMenu->addAction(mRussianLanguageAction);
	mLanguageMenu->addAction(mEnglishLanguageAction);
	mLanguageMenu->addAction(mFrenchLanguageAction);
	mLanguageMenu->addAction(mGermanLanguageAction);

	mMenuBar->addAction(mAboutAction);

	this->layout()->setMenuBar(mMenuBar);
}

void GamepadForm::setButtonsEnabled(bool enabled)
{
	// Here we enable or disable pads and "magic buttons" depending on given parameter.
	for (auto button : controlButtonsHash.values())
		button->setEnabled(enabled);
}

void GamepadForm::setButtonsCheckable(bool checkableStatus)
{
	for (auto button : controlButtonsHash.values())
		button->setCheckable(checkableStatus);
}

void GamepadForm::setUpControlButtonsHash()
{
	controlButtonsHash.insert(Qt::Key_1, mUi->button1);
	controlButtonsHash.insert(Qt::Key_2, mUi->button2);
	controlButtonsHash.insert(Qt::Key_3, mUi->button3);
	controlButtonsHash.insert(Qt::Key_4, mUi->button4);
	controlButtonsHash.insert(Qt::Key_5, mUi->button5);

	controlButtonsHash.insert(Qt::Key_A, mUi->buttonPad1Left);
	controlButtonsHash.insert(Qt::Key_D, mUi->buttonPad1Right);
	controlButtonsHash.insert(Qt::Key_W, mUi->buttonPad1Up);
	controlButtonsHash.insert(Qt::Key_S, mUi->buttonPad1Down);

	controlButtonsHash.insert(Qt::Key_Left, mUi->buttonPad2Left);
	controlButtonsHash.insert(Qt::Key_Right, mUi->buttonPad2Right);
	controlButtonsHash.insert(Qt::Key_Up, mUi->buttonPad2Up);
	controlButtonsHash.insert(Qt::Key_Down, mUi->buttonPad2Down);
}

void GamepadForm::setLabels()
{
	QPixmap redBall(":/images/redBall.png");
	mUi->disconnectedLabel->setPixmap(redBall);
	mUi->disconnectedLabel->setVisible(true);

	QPixmap greenBall(":/images/greenBall.png");
	mUi->connectedLabel->setPixmap(greenBall);
	mUi->connectedLabel->setVisible(false);

	QPixmap blueBall(":/images/blueBall.png");
	mUi->connectingLabel->setPixmap(blueBall);
	mUi->connectingLabel->setVisible(false);
}

bool GamepadForm::eventFilter(QObject *obj, QEvent *event)
{
	Q_UNUSED(obj)

	int resultingPowerX1 = 0;
	int resultingPowerY1 = 0;
	int resultingPowerX2 = 0;
	int resultingPowerY2 = 0;

	// Handle key press event
	if(event->type() == QEvent::KeyPress) {
		int pressedKey = (dynamic_cast<QKeyEvent *> (event))->key();
		if (controlButtonsHash.keys().contains(pressedKey))
			setButtonChecked(pressedKey, true);

		mPressedKeys += pressedKey;

		// Handle W A S D buttons
		resultingPowerX1 = (mPressedKeys.contains(Qt::Key_D) ? 100 : 0) + (mPressedKeys.contains(Qt::Key_A) ? -100 : 0);
		resultingPowerY1 = (mPressedKeys.contains(Qt::Key_S) ? -100 : 0) + (mPressedKeys.contains(Qt::Key_W) ? 100 : 0);
		// Handle arrow buttons
		resultingPowerX2 = (mPressedKeys.contains(Qt::Key_Right) ? 100 : 0) + (mPressedKeys.contains(Qt::Key_Left) ? -100 : 0);
		resultingPowerY2 = (mPressedKeys.contains(Qt::Key_Down) ? -100 : 0) + (mPressedKeys.contains(Qt::Key_Up) ? 100 : 0);

		if (resultingPowerX1 != 0 || resultingPowerY1 != 0) {
			onPadPressed(QString("pad 1 %1 %2").arg(resultingPowerX1).arg(resultingPowerY1));
		} else if (resultingPowerX2 != 0 || resultingPowerY2 != 0) {
			onPadPressed(QString("pad 2 %1 %2").arg(resultingPowerX2).arg(resultingPowerY2));
		}

		// Handle 1 2 3 4 5 buttons
		QMap<int, QPushButton *> digits = {
			{Qt::Key_1, mUi->button1}
			, {Qt::Key_2, mUi->button2}
			, {Qt::Key_3, mUi->button3}
			, {Qt::Key_4, mUi->button4}
			, {Qt::Key_5, mUi->button5}
		};

		for (auto key : digits.keys()) {
			if (mPressedKeys.contains(key)) {
				digits[key]->pressed();
			}
		}
	} else if(event->type() == QEvent::KeyRelease) {

		// Handle key release event
		QSet<int> pad1 = {Qt::Key_W, Qt::Key_A, Qt::Key_S, Qt::Key_D};
		QSet<int> pad2 = {Qt::Key_Left, Qt::Key_Right, Qt::Key_Up, Qt::Key_Down};

		auto releasedKey = (dynamic_cast<QKeyEvent*>(event))->key();
		if (controlButtonsHash.keys().contains(releasedKey))
			setButtonChecked(releasedKey, false);
		mPressedKeys -= releasedKey;

		if (pad1.contains(releasedKey)) {
			onPadReleased(1);
		} else if (pad2.contains(releasedKey)){
			onPadReleased(2);
		}
	}

	return false;
}

void GamepadForm::onButtonPressed(int buttonId)
{
	// Checking that we are still connected, just in case.
	if (!connectionManager.isConnected()) {
		return;
	}

	emit commandReceived(QString("btn %1\n").arg(buttonId));
}

void GamepadForm::onPadPressed(const QString &action)
{
	// Here we send "pad <padId> <x> <y>" command.
	if (!connectionManager.isConnected()) {
		return;
	}

	emit commandReceived(QString(action + "\n"));
}

void GamepadForm::onPadReleased(int padId)
{
	// Here we send "pad <padId> up" command.
	if (!connectionManager.isConnected()) {
		return;
	}

	emit commandReceived(QString("pad %1 up\n").arg(padId));
}

void GamepadForm::openConnectDialog()
{
	mMyNewConnectForm = new ConnectForm(&connectionManager);
	mMyNewConnectForm->show();

	connect(mMyNewConnectForm, SIGNAL(dataReceived()),
			&connectionManager, SLOT(connectToHost()));
	connect(mMyNewConnectForm, SIGNAL(dataReceived()),
			this, SLOT(startVideoStream()));
}

void GamepadForm::exit()
{
	qApp->exit();
}

void GamepadForm::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		mUi->retranslateUi(this);

		retranslate();
	}

	QWidget::changeEvent(event);
}

void GamepadForm::handlePadRelease(QWidget *w)
{
	onPadReleased(mPadHashtable[w][0]);
	QPushButton *padButton = dynamic_cast<QPushButton *> (w);
	padButton->setChecked(false);
}

void GamepadForm::handlePadPress(QWidget *w)
{
	auto vec = mPadHashtable[w];
	onPadPressed(QString("pad %1 %2 %3").arg(vec[0]).arg(vec[1]).arg(vec[2]));
}

void GamepadForm::handleDigitPress(QWidget *w)
{
	onButtonPressed(mDigitHashtable[w]);
}

void GamepadForm::handleDigitRelease(QWidget *w)
{
	QPushButton *digitButton = dynamic_cast<QPushButton *> (w);
	digitButton->setChecked(false);
}

void GamepadForm::retranslate()
{
	mConnectionMenu->setTitle(tr("&Connection"));
	mLanguageMenu->setTitle(tr("&Language"));
	mConnectAction->setText(tr("&Connect"));
	mExitAction->setText(tr("&Exit"));

	mRussianLanguageAction->setText(tr("&Russian"));
	mEnglishLanguageAction->setText(tr("&English"));
	mFrenchLanguageAction->setText(tr("&French"));
	mGermanLanguageAction->setText(tr("&German"));

	mAboutAction->setText(tr("&About"));

}

void GamepadForm::changeLanguage(const QString &language)
{
	mTranslator = new QTranslator(this);
	mTranslator->load(language);
	qApp->installTranslator(mTranslator);

	mUi->retranslateUi(this);
}

void GamepadForm::about()
{
	const QString title = tr("About TRIK Gamepad");
	const QString about =  tr("TRIK Gamepad 1.1.2\n\nThis is desktop gamepad for TRIK robots.");
	QMessageBox::about(this, title, about);
}
