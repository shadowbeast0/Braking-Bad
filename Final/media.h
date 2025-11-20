#pragma once

#include <QObject>
#include <QVector>

class QAudioOutput;
class QMediaPlayer;
class QSoundEffect;
class QPropertyAnimation;

class Media : public QObject {
    Q_OBJECT
public:
    explicit Media(QObject* parent = nullptr);
    ~Media();

    // Background music
    void setupBgm();
    void setBgmVolume(qreal v);
    void playBgm();
    void stopBgm();

    // Per-stage BGM (now also supports NIGHTLIFE)
    void setStageBgm(int levelIndex);

    // Engine / driving SFX
    void startAccelLoop();
    void stopAccelLoop();

    // Nitro SFX
    void playNitroOnce();

    // Pickups
    void coinPickup();
    void fuelPickup();

    // Game over SFX
    void playGameOverOnce();

private:
    // BGM
    QAudioOutput* m_bgmOut = nullptr;
    QMediaPlayer* m_bgm    = nullptr;

    // Driving loop SFX
    QVector<QSoundEffect*> m_driveSfx;
    QPropertyAnimation*    m_accelFade = nullptr;

    // Nitro SFX
    QSoundEffect* m_nitroSfx = nullptr;

    // Coin SFX pool
    QVector<QMediaPlayer*> m_coinPlayers;
    QVector<QAudioOutput*> m_coinOuts;
    int m_nextCoinPl = -1;

    // Fuel SFX pool
    QVector<QMediaPlayer*> m_fuelPlayers;
    QVector<QAudioOutput*> m_fuelOuts;
    int m_nextFuelPl = -1;

    // Game over SFX
    QAudioOutput* m_gameOverOut = nullptr;
    QMediaPlayer* m_gameOver    = nullptr;
};
