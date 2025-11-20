#include "media.h"

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QMediaDevices>
#include <QSoundEffect>
#include <QPropertyAnimation>
#include <QCoreApplication>
#include <QFile>
#include <QUrl>

namespace {

// Try to load a stage-specific BGM either from qrc:/audio or from
// applicationDirPath()/assets/audio
QUrl pickBgmUrl(const QString& alias, const QString& fileName)
{
    // 1) qrc-based path, e.g. qrc:/audio/bgm_nightlife.mp3
    const QString qrcPath = QString(":/audio/%1").arg(alias);
    if (QFile::exists(qrcPath)) {
        return QUrl(QStringLiteral("qrc:/audio/") + alias);
    }

    // 2) filesystem-based path, e.g. <app>/assets/audio/bgm_nightlife.mp3
    const QString fsPath =
        QCoreApplication::applicationDirPath() + "/assets/audio/" + fileName;
    if (QFile::exists(fsPath)) {
        return QUrl::fromLocalFile(fsPath);
    }

    // Not found
    return {};
}

// Fallback BGM if no stage-specific one is found
QUrl defaultBgmUrl()
{
    // 1) qrc-based default
    const QString qrcPath = QStringLiteral(":/audio/bgm.mp3");
    if (QFile::exists(qrcPath)) {
        return QUrl(QStringLiteral("qrc:/audio/bgm.mp3"));
    }

    // 2) filesystem-based default
    const QString fsPath =
        QCoreApplication::applicationDirPath() + "/assets/audio/bgm.mp3";
    if (QFile::exists(fsPath)) {
        return QUrl::fromLocalFile(fsPath);
    }

    // Nothing found
    return {};
}

} // namespace

Media::Media(QObject* parent)
    : QObject(parent)
{
    // --- Driving / accelerate loop SFX ---
    auto* e = new QSoundEffect(this);
    e->setSource(QUrl(QStringLiteral("qrc:/sfx/accelerate.wav")));
    e->setLoopCount(1);
    e->setVolume(0.35);
    m_driveSfx.push_back(e);

    // --- Nitro SFX ---
    m_nitroSfx = new QSoundEffect(this);
    m_nitroSfx->setSource(QUrl(QStringLiteral("qrc:/sfx/nitro.wav")));
    m_nitroSfx->setLoopCount(1);
    m_nitroSfx->setVolume(0.35);

    // --- Game over SFX ---
    m_gameOverOut = new QAudioOutput(this);
    m_gameOverOut->setVolume(0.35);
    m_gameOver = new QMediaPlayer(this);
    m_gameOver->setAudioOutput(m_gameOverOut);
    m_gameOver->setSource(QUrl(QStringLiteral("qrc:/sfx/gameOver.mp3")));

    // --- Coin SFX pool ---
    const char* coinPath = "qrc:/sfx/coin.mp3";
    for (int i = 0; i < 3; ++i) {
        auto* out = new QAudioOutput(this);
        out->setVolume(0.35);
        auto* p = new QMediaPlayer(this);
        p->setAudioOutput(out);
        p->setSource(QUrl(coinPath));
        m_coinOuts.push_back(out);
        m_coinPlayers.push_back(p);
    }

    // --- Fuel SFX pool ---
    const char* fuelPath = "qrc:/sfx/fuel.mp3";
    for (int i = 0; i < 3; ++i) {
        auto* out = new QAudioOutput(this);
        out->setVolume(0.35);
        auto* p = new QMediaPlayer(this);
        p->setAudioOutput(out);
        p->setSource(QUrl(fuelPath));
        m_fuelOuts.push_back(out);
        m_fuelPlayers.push_back(p);
    }
}

Media::~Media() = default;

// -----------------------------------------------------------------------------
// Background music setup / control
// -----------------------------------------------------------------------------
void Media::setupBgm()
{
    // Output device for BGM
    m_bgmOut = new QAudioOutput(this);
    m_bgmOut->setVolume(1); // initial volume, you later override via setBgmVolume()

    // Player for BGM
    m_bgm = new QMediaPlayer(this);
    m_bgm->setAudioOutput(m_bgmOut);
    m_bgm->setLoops(QMediaPlayer::Infinite);

    // Default source at startup (intro / menu)
    const QUrl src = defaultBgmUrl();
    if (!src.isEmpty()) {
        m_bgm->setSource(src);
    }
}

void Media::setBgmVolume(qreal v)
{
    if (m_bgmOut) {
        m_bgmOut->setVolume(v);
    }
}

void Media::playBgm()
{
    if (m_bgm) {
        m_bgm->play();
    }
}

void Media::stopBgm()
{
    if (m_bgm) {
        m_bgm->stop();
    }
}

// -----------------------------------------------------------------------------
// Per-stage BGM (including NIGHTLIFE)
// -----------------------------------------------------------------------------
void Media::setStageBgm(int levelIndex)
{
    if (!m_bgm) {
        return;
    }

    QUrl src;

    // For each level, try to use a dedicated BGM if present.
    // If that fails (file missing), we will fall back to defaultBgmUrl().

    switch (levelIndex) {
    case 0: // MEADOW
        src = pickBgmUrl(QStringLiteral("bgm_meadow.mp3"),
                         QStringLiteral("bgm_meadow.mp3"));
        break;
    case 1: // DESERT
        src = pickBgmUrl(QStringLiteral("bgm_desert.mp3"),
                         QStringLiteral("bgm_desert.mp3"));
        break;
    case 2: // TUNDRA
        src = pickBgmUrl(QStringLiteral("bgm_tundra.mp3"),
                         QStringLiteral("bgm_tundra.mp3"));
        break;
    case 3: // LUNAR
        src = pickBgmUrl(QStringLiteral("bgm_lunar.mp3"),
                         QStringLiteral("bgm_lunar.mp3"));
        break;
    case 4: // MARTIAN
        src = pickBgmUrl(QStringLiteral("bgm_martian.mp3"),
                         QStringLiteral("bgm_martian.mp3"));
        break;
    case 5: // NIGHTLIFE (NEW)
        // This is the new Nightlife-only background music.
        // Expected file name: bgm_nightlife.mp3
        // Either as a qrc resource (qrc:/audio/bgm_nightlife.mp3)
        // or as a filesystem asset (<app>/assets/audio/bgm_nightlife.mp3)
        src = pickBgmUrl(QStringLiteral("bgm_nightlife.mp3"),
                         QStringLiteral("bgm_nightlife.mp3"));
        break;
    default:
        break;
    }

    // Fallback to default if we could not resolve a specific track
    if (src.isEmpty()) {
        src = defaultBgmUrl();
    }

    if (!src.isEmpty()) {
        m_bgm->setSource(src);
    }

    // Ensure it actually starts playing after changing source
    m_bgm->play();
}

// -----------------------------------------------------------------------------
// Accelerate / driving loop
// -----------------------------------------------------------------------------
void Media::startAccelLoop()
{
    if (m_driveSfx.isEmpty()) {
        return;
    }
    auto* e = m_driveSfx[0];

    // Stop any existing fade-out
    if (m_accelFade) {
        m_accelFade->stop();
        delete m_accelFade;
        m_accelFade = nullptr;
    }

    e->setLoopCount(QSoundEffect::Infinite);
    e->setVolume(1.0);
    if (!e->isPlaying()) {
        e->play();
    }
}

void Media::stopAccelLoop()
{
    if (m_driveSfx.isEmpty()) {
        return;
    }
    auto* e = m_driveSfx[0];

    if (m_accelFade) {
        m_accelFade->stop();
        delete m_accelFade;
        m_accelFade = nullptr;
    }

    // Fade volume to zero, then stop
    m_accelFade = new QPropertyAnimation(e, "volume", this);
    m_accelFade->setStartValue(e->volume());
    m_accelFade->setEndValue(0.0);
    m_accelFade->setDuration(250);

    connect(m_accelFade, &QPropertyAnimation::finished, this, [this, e] {
        e->stop();
        e->setVolume(0.0);
        if (m_accelFade) {
            m_accelFade->deleteLater();
            m_accelFade = nullptr;
        }
    });

    m_accelFade->start();
}

// -----------------------------------------------------------------------------
// Nitro SFX
// -----------------------------------------------------------------------------
void Media::playNitroOnce()
{
    if (!m_nitroSfx) {
        return;
    }
    m_nitroSfx->stop();
    m_nitroSfx->setLoopCount(1);
    m_nitroSfx->setVolume(0.35);
    m_nitroSfx->play();
}

// -----------------------------------------------------------------------------
// Pickup SFX
// -----------------------------------------------------------------------------
void Media::coinPickup()
{
    if (m_coinPlayers.isEmpty()) {
        return;
    }
    m_nextCoinPl = (m_nextCoinPl + 1) % m_coinPlayers.size();
    auto* p = m_coinPlayers[m_nextCoinPl];
    p->setPosition(0);
    p->play();
}

void Media::fuelPickup()
{
    if (m_fuelPlayers.isEmpty()) {
        return;
    }
    m_nextFuelPl = (m_nextFuelPl + 1) % m_fuelPlayers.size();
    auto* p = m_fuelPlayers[m_nextFuelPl];
    p->setPosition(0);
    p->play();
}

// -----------------------------------------------------------------------------
// Game over SFX
// -----------------------------------------------------------------------------
void Media::playGameOverOnce()
{
    if (!m_gameOver) {
        return;
    }
    m_gameOver->setPosition(0);
    m_gameOver->play();
}
