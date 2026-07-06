#pragma once

#include "ServerApiClient.h"
#include "VideoWorker.h"

#include <QTimer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QImage;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
QT_END_NAMESPACE

class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void wireSignals();
    void setConnectedState(bool connected);
    void updateStatus(const QString &message);
    void updateVideoFrame(const QImage &image);
    void connectToServer();
    void disconnectFromServer();
    void applyVideoSettings();
    void sendEcho();
    void pollSessionStatus();
    void refreshCameraDevices();
    void selectCameraDevice(std::function<void()> onSuccess = {});
    void createSession();
    void stopStreamingAndResetUi();
    QUrl baseUrl() const;
    QUrl fallbackRtspUrl() const;
    VideoSettings selectedVideoSettings() const;
    QString selectedResolution() const;

    ServerApiClient apiClient_;
    VideoWorker videoWorker_;

    QString sessionId_;
    QUrl rtspUrl_;
    VideoSettings activeVideoSettings_;
    QTimer sessionPollTimer_;

    QLineEdit *serverHostEdit_ = nullptr;
    QSpinBox *controlPortSpin_ = nullptr;
    QLineEdit *clientNameEdit_ = nullptr;
    QComboBox *resolutionCombo_ = nullptr;
    QComboBox *frameRateCombo_ = nullptr;
    QComboBox *cameraDeviceCombo_ = nullptr;
    QPushButton *connectButton_ = nullptr;
    QPushButton *disconnectButton_ = nullptr;
    QPushButton *applyButton_ = nullptr;
    QPushButton *echoButton_ = nullptr;
    QPushButton *refreshCameraButton_ = nullptr;
    QPushButton *selectCameraButton_ = nullptr;
    QLabel *sessionLabel_ = nullptr;
    QLabel *streamLabel_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QLabel *videoLabel_ = nullptr;
};