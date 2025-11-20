#include "scoreboard.h"
#include <QPainter>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPalette>
#include <QSysInfo>
#include <QSettings>
#include <algorithm>

LeaderboardWidget::LeaderboardWidget(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(0, 0, 0, 210));
    setPalette(pal);

    setFocusPolicy(Qt::StrongFocus);
    hide();
}

void LeaderboardWidget::setEntries(const QVector<LeaderboardEntry>& entries)
{
    m_entries = entries;
    update();
}

void LeaderboardWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const QRect panel = rect().adjusted(width() * 0.1, height() * 0.1, -width() * 0.1, -height() * 0.1);

    // Panel background
    p.fillRect(panel, QColor(28, 24, 36, 240));
    p.setPen(QColor(80, 70, 100));
    p.drawRect(panel.adjusted(0, 0, -1, -1));

    // Title (pixel-style)
    QFont titleFont;
    titleFont.setFamily("Monospace");
    titleFont.setBold(true);
    titleFont.setPixelSize(24);
    titleFont.setStyleStrategy(QFont::NoAntialias);
    p.setFont(titleFont);
    p.setPen(QColor(230, 230, 240));

    const QString title = QStringLiteral("SCOREBOARD");
    QFontMetrics tfm(titleFont);
    int titleW = tfm.horizontalAdvance(title);
    int titleX = panel.center().x() - titleW / 2;
    int titleY = panel.top() + tfm.ascent() + 16;
    p.drawText(titleX, titleY, title);

    // Columns
    QFont headerFont;
    headerFont.setFamily("Monospace");
    headerFont.setBold(true);
    headerFont.setPixelSize(14);
    headerFont.setStyleStrategy(QFont::NoAntialias);

    QFont rowFont = headerFont;
    rowFont.setBold(false);

    const int topMargin = 60;
    const int rowHeight = 26;

    int colStageX = panel.left() + 40;
    int colScoreX = panel.right() - 160;

    // Header row
    p.setFont(headerFont);
    p.setPen(QColor(200, 200, 210));
    int headerY = panel.top() + topMargin;
    p.drawText(colStageX, headerY, QStringLiteral("STAGE"));
    p.drawText(colScoreX, headerY, QStringLiteral("BEST SCORE"));


    // Separator line
    p.setPen(QColor(80, 70, 100));
    p.drawLine(panel.left() + 20, headerY + 6,
               panel.right() - 20, headerY + 6);

    // Rows
    p.setFont(rowFont);
    p.setPen(QColor(230, 230, 240));

    if (m_entries.isEmpty()) {
        const QString msg = QStringLiteral("No scores yet. Play a game to create a record.");
        QFontMetrics rfm(rowFont);
        int w = rfm.horizontalAdvance(msg);
        int x = panel.center().x() - w / 2;
        int y = headerY + rowHeight * 2;
        p.drawText(x, y, msg);
        return;
    }

    int y = headerY + rowHeight;
    for (int i = 0; i < m_entries.size(); ++i) {
        const LeaderboardEntry& e = m_entries[i];

        // Alternate row background
        if (i % 2 == 0) {
            QRect rowRect(panel.left() + 10, y - rowHeight + 6, panel.width() - 20, rowHeight);
            p.fillRect(rowRect, QColor(40, 34, 50, 160));
        }

        p.setPen(QColor(220, 220, 230));
        p.drawText(colStageX, y, e.stageName);
        p.drawText(colScoreX, y, QString::number(e.score));

        y += rowHeight;
        if (y > panel.bottom() - 20) break;
    }

    // Hint footer
    QFont hintFont;
    hintFont.setFamily("Monospace");
    hintFont.setPixelSize(10);
    hintFont.setStyleStrategy(QFont::NoAntialias);
    p.setFont(hintFont);
    p.setPen(QColor(170, 170, 190));
    const QString hint = QStringLiteral("[L] or [Esc] to close");
    QFontMetrics hfm(hintFont);
    int hw = hfm.horizontalAdvance(hint);
    int hx = panel.center().x() - hw / 2;
    int hy = panel.bottom() - 12;
    p.drawText(hx, hy, hint);
}

void LeaderboardWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_S || event->key() == Qt::Key_Escape || event->key() == Qt::Key_L) {
        hide();
        emit closed();
        return;
    }
    QWidget::keyPressEvent(event);
}

void LeaderboardWidget::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    hide();
    emit closed();
}

LeaderboardManager::LeaderboardManager(QObject* parent)
    : QObject(parent)
{
    loadFromSettings();
}

QString LeaderboardManager::deviceId() const
{
    QByteArray id = QSysInfo::machineUniqueId();
    if (!id.isEmpty())
        return QString::fromLatin1(id.toHex());

    QString host = QSysInfo::machineHostName();
    if (!host.isEmpty())
        return host;

    return QStringLiteral("UNKNOWN_DEVICE");
}

void LeaderboardManager::submitScore(const QString& stageName, int score)
{
    const QString user = deviceId();

    // Update local entries: keep only best score per (stage, user)
    bool found = false;
    for (auto &e : m_entries) {
        if (e.stageName == stageName && e.userName == user) {
            if (score > e.score)
                e.score = score;
            found = true;
            break;
        }
    }

    if (!found) {
        LeaderboardEntry e;
        e.stageName = stageName;
        e.userName  = user;
        e.score     = score;
        m_entries.push_back(e);
    }

    std::sort(m_entries.begin(), m_entries.end(),
              [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
                  if (a.stageName == b.stageName)
                      return a.score > b.score;
                  return a.stageName < b.stageName;
              });

    saveToSettings();
    emit leaderboardUpdated(m_entries);
}

void LeaderboardManager::refreshLeaderboard()
{
    emit leaderboardUpdated(m_entries);
}

void LeaderboardManager::loadFromSettings()
{
    m_entries.clear();

    QSettings s("JU", "F1PixelGrid");
    int n = s.beginReadArray("leaderboard");
    for (int i = 0; i < n; ++i) {
        s.setArrayIndex(i);
        LeaderboardEntry e;
        e.stageName = s.value("stage").toString();
        e.userName  = s.value("user").toString();
        e.score     = s.value("score").toInt();
        if (!e.stageName.isEmpty())
            m_entries.push_back(e);
    }
    s.endArray();

    // Ensure deterministic ordering
    std::sort(m_entries.begin(), m_entries.end(),
              [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
                  if (a.stageName == b.stageName)
                      return a.score > b.score;
                  return a.stageName < b.stageName;
              });
}

void LeaderboardManager::saveToSettings() const
{
    QSettings s("JU", "F1PixelGrid");
    s.beginWriteArray("leaderboard");
    for (int i = 0; i < m_entries.size(); ++i) {
        s.setArrayIndex(i);
        s.setValue("stage", m_entries[i].stageName);
        s.setValue("user",  m_entries[i].userName);
        s.setValue("score", m_entries[i].score);
    }
    s.endArray();
    s.sync();
}
