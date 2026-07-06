#include "MainWindow.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
QString formatVideoSettings(const VideoSettings &settings) {
    return QStringLiteral("%1 / %2 FPS / %3")
        .arg(settings.resolution, QString::number(settings.frameRate), settings.codec);
}
}

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent) {
    buildUi();
    wireSignals();
    sessionPollTimer_.setInterval(3000);
    setConnectedState(false);
    updateStatus(QStringLiteral("Ready. Configure the server and press Connect."));
}

MainWindow::~MainWindow() {
    videoWorker_.stopStreaming();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    videoWorker_.stopStreaming();
    if (!sessionId_.isEmpty()) {
        apiClient_.deleteSession(baseUrl(), sessionId_, []() {}, [](const QString &) {});
    }
    QWidget::closeEvent(event);
}

void MainWindow::buildUi() {
    setWindowTitle(QStringLiteral("Camera Client"));
    resize(1280, 800);

    auto *rootLayout = new QVBoxLayout(this);

    auto *connectionGroup = new QGroupBox(QStringLiteral("Server Control"), this);
    auto *connectionLayout = new QGridLayout(connectionGroup);

    serverHostEdit_ = new QLineEdit(QStringLiteral("127.0.0.1"), connectionGroup);
    controlPortSpin_ = new QSpinBox(connectionGroup);
    controlPortSpin_->setRange(1, 65535);
    controlPortSpin_->setValue(8080);
    clientNameEdit_ = new QLineEdit(QStringLiteral("qt-viewer-01"), connectionGroup);

    resolutionCombo_ = new QComboBox(connectionGroup);
    resolutionCombo_->addItems({QStringLiteral("1280x720"), QStringLiteral("1920x1080"), QStringLiteral("3840x2160")});
    resolutionCombo_->setCurrentText(QStringLiteral("1920x1080"));

    frameRateCombo_ = new QComboBox(connectionGroup);
    frameRateCombo_->addItems({QStringLiteral("15"), QStringLiteral("30"), QStringLiteral("60")});
    frameRateCombo_->setCurrentText(QStringLiteral("30"));

    cameraDeviceCombo_ = new QComboBox(connectionGroup);
    cameraDeviceCombo_->setEditable(false);
    cameraDeviceCombo_->addItem(QStringLiteral("(select after refresh)"));

    connectButton_ = new QPushButton(QStringLiteral("Connect"), connectionGroup);
    disconnectButton_ = new QPushButton(QStringLiteral("Disconnect"), connectionGroup);
    applyButton_ = new QPushButton(QStringLiteral("Apply Video Settings"), connectionGroup);
    echoButton_ = new QPushButton(QStringLiteral("Echo Hello"), connectionGroup);
    refreshCameraButton_ = new QPushButton(QStringLiteral("Refresh Cameras"), connectionGroup);
    selectCameraButton_ = new QPushButton(QStringLiteral("Select Camera"), connectionGroup);

    connectionLayout->addWidget(new QLabel(QStringLiteral("Server Host"), connectionGroup), 0, 0);
    connectionLayout->addWidget(serverHostEdit_, 0, 1);
    connectionLayout->addWidget(new QLabel(QStringLiteral("Control Port"), connectionGroup), 0, 2);
    connectionLayout->addWidget(controlPortSpin_, 0, 3);
    connectionLayout->addWidget(new QLabel(QStringLiteral("Client Name"), connectionGroup), 1, 0);
    connectionLayout->addWidget(clientNameEdit_, 1, 1);
    connectionLayout->addWidget(new QLabel(QStringLiteral("Resolution"), connectionGroup), 1, 2);
    connectionLayout->addWidget(resolutionCombo_, 1, 3);
    connectionLayout->addWidget(new QLabel(QStringLiteral("Frame Rate"), connectionGroup), 2, 0);
    connectionLayout->addWidget(frameRateCombo_, 2, 1);
    connectionLayout->addWidget(new QLabel(QStringLiteral("Camera"), connectionGroup), 2, 2);
    connectionLayout->addWidget(cameraDeviceCombo_, 2, 3);
    connectionLayout->addWidget(refreshCameraButton_, 3, 0);
    connectionLayout->addWidget(selectCameraButton_, 3, 1);
    connectionLayout->addWidget(connectButton_, 3, 2);
    connectionLayout->addWidget(disconnectButton_, 3, 3);
    connectionLayout->addWidget(applyButton_, 4, 2);
    connectionLayout->addWidget(echoButton_, 4, 3);

    auto *infoGroup = new QGroupBox(QStringLiteral("Session Status"), this);
    auto *infoLayout = new QFormLayout(infoGroup);
    sessionLabel_ = new QLabel(QStringLiteral("Not connected"), infoGroup);
    streamLabel_ = new QLabel(QStringLiteral("No stream"), infoGroup);
    statusLabel_ = new QLabel(QStringLiteral("Idle"), infoGroup);
    statusLabel_->setWordWrap(true);
    infoLayout->addRow(QStringLiteral("Session"), sessionLabel_);
    infoLayout->addRow(QStringLiteral("Video"), streamLabel_);
    infoLayout->addRow(QStringLiteral("Status"), statusLabel_);

    videoLabel_ = new QLabel(QStringLiteral("No video"), this);
    videoLabel_->setMinimumSize(960, 540);
    videoLabel_->setAlignment(Qt::AlignCenter);
    videoLabel_->setStyleSheet(QStringLiteral("background-color: #0f172a; color: #e2e8f0; border: 1px solid #334155;"));

    rootLayout->addWidget(connectionGroup);
    rootLayout->addWidget(infoGroup);
    rootLayout->addWidget(videoLabel_, 1);
}

void MainWindow::wireSignals() {
    connect(connectButton_, &QPushButton::clicked, this, &MainWindow::connectToServer);
    connect(disconnectButton_, &QPushButton::clicked, this, &MainWindow::disconnectFromServer);
    connect(applyButton_, &QPushButton::clicked, this, &MainWindow::applyVideoSettings);
    connect(echoButton_, &QPushButton::clicked, this, &MainWindow::sendEcho);
    connect(refreshCameraButton_, &QPushButton::clicked, this, &MainWindow::refreshCameraDevices);
    connect(selectCameraButton_, &QPushButton::clicked, this, [this]() {
        selectCameraDevice();
    });
    connect(&sessionPollTimer_, &QTimer::timeout, this, &MainWindow::pollSessionStatus);

    connect(&videoWorker_, &VideoWorker::frameReady, this, &MainWindow::updateVideoFrame);
    connect(&videoWorker_, &VideoWorker::statusChanged, this, [this](const QString &message) {
        updateStatus(message);
    });
    connect(&videoWorker_, &VideoWorker::streamError, this, [this](const QString &message) {
        updateStatus(message);
        videoLabel_->setText(QStringLiteral("Stream interrupted. Waiting for reconnect..."));
    });
}

void MainWindow::setConnectedState(bool connected) {
    connectButton_->setEnabled(!connected);
    disconnectButton_->setEnabled(connected);
    applyButton_->setEnabled(connected);
    echoButton_->setEnabled(connected);
    refreshCameraButton_->setEnabled(true);
    selectCameraButton_->setEnabled(true);
    cameraDeviceCombo_->setEnabled(true);
}

void MainWindow::updateStatus(const QString &message) {
    statusLabel_->setText(message);
}

void MainWindow::updateVideoFrame(const QImage &image) {
    const QPixmap pixmap = QPixmap::fromImage(image).scaled(videoLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    videoLabel_->setPixmap(pixmap);
}

void MainWindow::connectToServer() {
    setConnectedState(false);
    const QString selectedDevice = cameraDeviceCombo_->currentText().trimmed();
    if (!selectedDevice.isEmpty() && selectedDevice != QStringLiteral("(select after refresh)")) {
        updateStatus(QStringLiteral("Selecting camera device..."));
        selectCameraDevice([this]() {
            createSession();
        });
        return;
    }

    createSession();
}

void MainWindow::createSession() {
    updateStatus(QStringLiteral("Creating session..."));

    apiClient_.createSession(
        baseUrl(),
        clientNameEdit_->text().trimmed(),
        selectedVideoSettings(),
        [this](const SessionInfo &sessionInfo) {
            sessionId_ = sessionInfo.sessionId;
            rtspUrl_ = sessionInfo.rtspUrl.isEmpty() ? fallbackRtspUrl() : sessionInfo.rtspUrl;
            activeVideoSettings_ = sessionInfo.video;

            sessionLabel_->setText(QStringLiteral("%1 (%2)").arg(sessionId_, sessionInfo.createdAt));
            streamLabel_->setText(QStringLiteral("%1\n%2")
                                      .arg(rtspUrl_.toString(), formatVideoSettings(activeVideoSettings_)));
            updateStatus(QStringLiteral("Session created. Connecting RTSP stream..."));
            setConnectedState(true);
            sessionPollTimer_.start();
            videoWorker_.startStreaming(rtspUrl_.toString());
        },
        [this](const QString &errorMessage) {
            sessionId_.clear();
            rtspUrl_ = QUrl();
            setConnectedState(false);
            sessionLabel_->setText(QStringLiteral("Not connected"));
            streamLabel_->setText(QStringLiteral("No stream"));
            QMessageBox::critical(this, QStringLiteral("Connection Failed"), errorMessage);
            updateStatus(QStringLiteral("Connection failed."));
        });
}

void MainWindow::disconnectFromServer() {
    if (sessionId_.isEmpty()) {
        stopStreamingAndResetUi();
        return;
    }

    updateStatus(QStringLiteral("Disconnecting session..."));
    const QString sessionId = sessionId_;
    sessionPollTimer_.stop();

    apiClient_.deleteSession(
        baseUrl(),
        sessionId,
        [this]() {
            stopStreamingAndResetUi();
            updateStatus(QStringLiteral("Disconnected."));
        },
        [this](const QString &errorMessage) {
            QMessageBox::warning(this, QStringLiteral("Disconnect Failed"), errorMessage);
            stopStreamingAndResetUi();
            updateStatus(QStringLiteral("Disconnected locally after server-side error."));
        });
}

void MainWindow::applyVideoSettings() {
    if (sessionId_.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Not Connected"), QStringLiteral("Connect to the server before changing video settings."));
        return;
    }

    updateStatus(QStringLiteral("Updating video settings..."));
    const VideoSettings requested = selectedVideoSettings();

    apiClient_.updateVideo(
        baseUrl(),
        sessionId_,
        requested,
        [this](const VideoSettings &appliedVideo) {
            activeVideoSettings_ = appliedVideo;
            streamLabel_->setText(QStringLiteral("%1\n%2")
                                      .arg(rtspUrl_.toString(), formatVideoSettings(activeVideoSettings_)));
            videoWorker_.startStreaming(rtspUrl_.toString());
            updateStatus(QStringLiteral("Video settings updated. Stream reconnect requested."));
        },
        [this](const QString &errorMessage) {
            QMessageBox::warning(this, QStringLiteral("Update Failed"), errorMessage);
            updateStatus(QStringLiteral("Video settings update failed."));
        });
}

void MainWindow::sendEcho() {
    if (sessionId_.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Not Connected"), QStringLiteral("Connect to the server before running the echo handshake."));
        return;
    }

    apiClient_.echo(
        baseUrl(),
        sessionId_,
        QStringLiteral("Hello"),
        [this](const QString &message) {
            QMessageBox::information(this, QStringLiteral("Echo Response"), message);
            updateStatus(QStringLiteral("Echo handshake succeeded."));
        },
        [this](const QString &errorMessage) {
            QMessageBox::warning(this, QStringLiteral("Echo Failed"), errorMessage);
            updateStatus(QStringLiteral("Echo handshake failed."));
        });
}

void MainWindow::stopStreamingAndResetUi() {
    sessionPollTimer_.stop();
    sessionId_.clear();
    rtspUrl_ = QUrl();
    activeVideoSettings_ = VideoSettings{};
    videoWorker_.stopStreaming();
    videoLabel_->setPixmap(QPixmap());
    videoLabel_->setText(QStringLiteral("No video"));
    sessionLabel_->setText(QStringLiteral("Not connected"));
    streamLabel_->setText(QStringLiteral("No stream"));
    setConnectedState(false);
}

void MainWindow::pollSessionStatus() {
    if (sessionId_.isEmpty()) {
        sessionPollTimer_.stop();
        return;
    }

    apiClient_.getSessionStatus(
        baseUrl(),
        sessionId_,
        [this](const SessionInfo &sessionInfo) {
            activeVideoSettings_ = sessionInfo.video;
            sessionLabel_->setText(QStringLiteral("%1 (%2)").arg(sessionId_, sessionInfo.state));
            streamLabel_->setText(QStringLiteral("%1\n%2")
                                      .arg(rtspUrl_.toString(), formatVideoSettings(activeVideoSettings_)));

            if (sessionInfo.state.compare(QStringLiteral("CONNECTED"), Qt::CaseInsensitive) != 0) {
                QMessageBox::warning(this, QStringLiteral("Session Closed"), QStringLiteral("Server session state changed to %1.").arg(sessionInfo.state));
                stopStreamingAndResetUi();
                updateStatus(QStringLiteral("Session closed by server."));
                return;
            }

            updateStatus(QStringLiteral("Session active. Streaming view updated."));
        },
        [this](const QString &errorMessage) {
            QMessageBox::warning(this, QStringLiteral("Session Poll Failed"), errorMessage);
            stopStreamingAndResetUi();
            updateStatus(QStringLiteral("Lost server session state."));
        });
}

void MainWindow::refreshCameraDevices() {
    updateStatus(QStringLiteral("Fetching camera devices..."));
    apiClient_.getVideoDevices(
        baseUrl(),
        [this](const QStringList &devices) {
            cameraDeviceCombo_->clear();
            if (devices.isEmpty()) {
                cameraDeviceCombo_->addItem(QStringLiteral("(no camera devices)"));
                updateStatus(QStringLiteral("No camera devices were returned by server."));
                return;
            }

            cameraDeviceCombo_->addItems(devices);
            updateStatus(QStringLiteral("Camera devices updated."));
        },
        [this](const QString &errorMessage) {
            QMessageBox::warning(this, QStringLiteral("Camera Refresh Failed"), errorMessage);
            updateStatus(QStringLiteral("Failed to refresh camera devices."));
        });
}

void MainWindow::selectCameraDevice(std::function<void()> onSuccess) {
    const QString selectedDevice = cameraDeviceCombo_->currentText().trimmed();
    if (selectedDevice.isEmpty() || selectedDevice.startsWith(QStringLiteral("("))) {
        QMessageBox::information(this,
                                 QStringLiteral("Camera Not Selected"),
                                 QStringLiteral("Select a camera device after refreshing the list."));
        return;
    }

    apiClient_.selectCamera(
        baseUrl(),
        selectedDevice,
        [this, selectedDevice, onSuccess = std::move(onSuccess)]() mutable {
            updateStatus(QStringLiteral("Camera selected: %1").arg(selectedDevice));
            if (onSuccess) {
                onSuccess();
            }
        },
        [this](const QString &errorMessage) {
            QMessageBox::warning(this, QStringLiteral("Camera Select Failed"), errorMessage);
            updateStatus(QStringLiteral("Failed to select camera device."));
            setConnectedState(false);
        });
}

QUrl MainWindow::baseUrl() const {
    QUrl url;
    url.setScheme(QStringLiteral("http"));
    url.setHost(serverHostEdit_->text().trimmed());
    url.setPort(controlPortSpin_->value());
    return url;
}

QUrl MainWindow::fallbackRtspUrl() const {
    QUrl url;
    url.setScheme(QStringLiteral("rtsp"));
    url.setHost(serverHostEdit_->text().trimmed());
    url.setPort(8554);
    url.setPath(QStringLiteral("/camera"));
    return url;
}

VideoSettings MainWindow::selectedVideoSettings() const {
    VideoSettings settings;
    settings.resolution = selectedResolution();
    settings.frameRate = frameRateCombo_->currentText().toInt();
    settings.codec = QStringLiteral("H.264");
    return settings;
}

QString MainWindow::selectedResolution() const {
    return resolutionCombo_->currentText();
}