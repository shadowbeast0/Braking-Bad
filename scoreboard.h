#pragma once

#include <QObject>
#include <QWidget>
#include <QVector>
#include <QString>

class QPaintEvent;
class QKeyEvent;
class QMouseEvent;

// One row in the scoreboard (per player, per stage)
struct LeaderboardEntry {
    QString stageName;   // e.g. "MEADOW", "NIGHTLIFE"
    QString userName;    // device id (for storage only; no longer drawn)
    int     score = 0;   // best score on that stage
};

// -----------------------------------------------------------------------------
// Scoreboard overlay widget (UI)
// -----------------------------------------------------------------------------
class LeaderboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit LeaderboardWidget(QWidget* parent = nullptr);

    void setEntries(const QVector<LeaderboardEntry>& entries);

signals:
    void closed();  // emitted when user closes the overlay (L or Esc)

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QVector<LeaderboardEntry> m_entries;
};

// -----------------------------------------------------------------------------
// Scoreboard manager (LOCAL, per-player)
// -----------------------------------------------------------------------------
class LeaderboardManager : public QObject {
    Q_OBJECT
public:
    explicit LeaderboardManager(QObject* parent = nullptr);

    // Call this after each game ends
    void submitScore(const QString& stageName, int score);

    // Call this when opening the scoreboard (press L)
    void refreshLeaderboard();

signals:
    void leaderboardUpdated(const QVector<LeaderboardEntry>& entries);

private:
    QString deviceId() const;
    void    loadFromSettings();
    void    saveToSettings() const;

    QVector<LeaderboardEntry> m_entries;
};
