#include "media.h"
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QMediaDevices>
#include <QSoundEffect>
#include <QPropertyAnimation>
#include <QCoreApplication>
#include <QFile>
#include <QUrl>

Media::Media(QObject* parent) : QObject(parent) {
    auto* e = new QSoundEffect(this);
    e->setSource(QUrl("qrc:/sfx/accelerate.wav"));
    e->setLoopCount(1);
    e->setVolume(1.0);
    m_driveSfx.push_back(e);

    m_nitroSfx = new QSoundEffect(this);
    m_nitroSfx->setSource(QUrl("qrc:/sfx/nitro.wav"));
    m_nitroSfx->setLoopCount(1);
    m_nitroSfx->setVolume(1.0);
    m_gameOverOut = new QAudioOutput(this);
    m_gameOverOut->setVolume(1.0);
    m_gameOver = new QMediaPlayer(this);
    m_gameOver->setAudioOutput(m_gameOverOut);
    m_gameOver->setSource(QUrl("qrc:/sfx/gameOver.mp3"));

    const char* coinPath = "qrc:/sfx/coin.mp3";
    for (int i = 0; i < 3; ++i) {
        auto* out = new QAudioOutput(this);
        out->setVolume(1.5);
        auto* p = new QMediaPlayer(this);
        p->setAudioOutput(out);
        p->setSource(QUrl(coinPath));
        m_coinOuts.push_back(out);
        m_coinPlayers.push_back(p);
    }

    const char* fuelPath = "qrc:/sfx/fuel.mp3";
    for (int i = 0; i < 3; ++i) {
        auto* out = new QAudioOutput(this);
        out->setVolume(2.5);
        auto* p = new QMediaPlayer(this);
        p->setAudioOutput(out);
        p->setSource(QUrl(fuelPath));
        m_fuelOuts.push_back(out);
        m_fuelPlayers.push_back(p);
    }
}

Media::~Media() {}

void Media::setupBgm() {
    m_bgmOut = new QAudioOutput(QMediaDevices::defaultAudioOutput(), this);
    m_bgmOut->setVolume(0.35);

    m_bgm = new QMediaPlayer(this);
    m_bgm->setAudioOutput(m_bgmOut);

    bool qrcOk = QFile(":/audio/bgm.mp3").exists();
    QUrl source = qrcOk ? QUrl("qrc:/audio/bgm.mp3")
                        : QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/assets/audio/bgm.mp3");
    m_bgm->setSource(source);
    m_bgm->setLoops(QMediaPlayer::Infinite);
}

void Media::setBgmVolume(qreal v) {
    if (m_bgmOut) m_bgmOut->setVolume(v);
}

void Media::playBgm() {
    if (m_bgm) m_bgm->play();
}

void Media::stopBgm() {
    if (m_bgm) m_bgm->stop();
}

void Media::startAccelLoop() {
    if (m_driveSfx.isEmpty()) return;
    auto* e = m_driveSfx[0];
    if (m_accelFade) { m_accelFade->stop(); delete m_accelFade; m_accelFade = nullptr; }
    e->setLoopCount(QSoundEffect::Infinite);
    e->setVolume(1.0);
    if (!e->isPlaying()) e->play();
}

void Media::stopAccelLoop() {
    if (m_driveSfx.isEmpty()) return;
    auto* e = m_driveSfx[0];
    if (m_accelFade) { m_accelFade->stop(); delete m_accelFade; m_accelFade = nullptr; }
    m_accelFade = new QPropertyAnimation(e, "volume", this);
    m_accelFade->setStartValue(e->volume());
    m_accelFade->setEndValue(0.0);
    m_accelFade->setDuration(250);
    connect(m_accelFade, &QPropertyAnimation::finished, this, [this, e]{
        e->stop();
        e->setLoopCount(1);
        e->setVolume(1.0);
    });
    m_accelFade->start();
}

void Media::playNitroOnce() {
    if (!m_nitroSfx) return;
    m_nitroSfx->stop();
    m_nitroSfx->setLoopCount(1);
    m_nitroSfx->setVolume(1.0);
    m_nitroSfx->play();
}

void Media::coinPickup() {
    if (m_coinPlayers.isEmpty()) return;
    m_nextCoinPl = (m_nextCoinPl + 1) % m_coinPlayers.size();
    auto* p = m_coinPlayers[m_nextCoinPl];
    p->setPosition(0);
    p->play();
}

void Media::fuelPickup() {
    if (m_fuelPlayers.isEmpty()) return;
    m_nextFuelPl = (m_nextFuelPl + 1) % m_fuelPlayers.size();
    auto* p = m_fuelPlayers[m_nextFuelPl];
    p->setPosition(0);
    p->play();
}

void Media::playGameOverOnce() {
    if (!m_gameOver) return;
    m_gameOver->setPosition(0);
    m_gameOver->play();
}
