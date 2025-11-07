#pragma once
#include <QObject>
#include <QAudioDevice>
#include <QVector>

class QAudioOutput;
class QMediaPlayer;
class QSoundEffect;
class QPropertyAnimation;

class Media : public QObject {
    Q_OBJECT
public:
    explicit Media(QObject* parent=nullptr);
    ~Media();

    void setupBgm();
    void setBgmVolume(qreal v);
    void playBgm();
    void stopBgm();

    void startAccelLoop();
    void stopAccelLoop();
    void playNitroOnce();

    void coinPickup();
    void fuelPickup();

private:
    QAudioOutput* m_bgmOut = nullptr;
    QMediaPlayer* m_bgm    = nullptr;

    QVector<QSoundEffect*> m_driveSfx;
    QPropertyAnimation* m_accelFade = nullptr;

    QSoundEffect* m_nitroSfx = nullptr;

    QVector<QMediaPlayer*> m_coinPlayers;
    QVector<QAudioOutput*> m_coinOuts;
    int m_nextCoinPl = -1;

    QVector<QMediaPlayer*> m_fuelPlayers;
    QVector<QAudioOutput*> m_fuelOuts;
    int m_nextFuelPl = -1;
};
