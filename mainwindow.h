#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QVector>
#include <QPoint>
#include <QSet>
#include <QTimer>
#include <QResizeEvent>
#include <QKeyEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

inline uint qHash(const QPoint &p, uint seed = 0) {
    return ::qHash((quint64(p.x()) << 32) ^ quint64(p.y()), seed);
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onCanvasPressed(QPoint pos, Qt::MouseButton button);
    void onCanvasMoved(QPoint pos, Qt::MouseButtons buttons);
    void onCanvasReleased(QPoint pos, Qt::MouseButton button);
    void on_spinGridSize_valueChanged(int val);
    void on_btnDrawGrid_clicked();
    void on_btnEditMode_clicked();
    void on_btnStartGame_clicked();
    void on_btnScrollLeft_clicked();
    void on_btnScrollRight_clicked();
    void onGameTick();

private:
    Ui::MainWindow *ui;

    int gridSize = 20;
    QPixmap canvas;
    double viewOffsetX = 0.0;

    void drawGrid();
    void drawTerrainAndMarkers();
    void drawCar();
    QPoint viewToWorldCell(const QPoint &inView) const;
    QRect worldCellToScreenRect(const QPoint &wcell) const;
    void fillLineBresenham(const QPoint &from, const QPoint &to);
    void eraseLineBresenham(const QPoint &from, const QPoint &to);

    void recomputeStartEnd();
    void jumpOnePixel();  // jumps 1.5 cells

    // ---- Slope + rotation helpers ----
    bool columnTopY(int x, int &yTop) const;
    double groundSlopeAngleRad() const; // angle of surface under car (radians)
    void updateRotation(double slopeAngleRad, double stepDt); // uses substep dt

    bool editMode = true;
    bool eraserMode = false;
    QSet<QPoint> terrain;

    static constexpr double CAR_SIZE = 2.0;
    double carX = 2.0;
    double carY = 0.0;
    double vx = 0.0;
    double vy = 0.0;

    // Orientation (degrees) and angular velocity (deg/s)
    double carAngleDeg = 0.0;
    double angVelDeg = 0.0;

    bool keyAccel = false;
    bool keyBrake = false;

    // ===== Physics tuning =====
    const double dt = 1.0 / 120.0;        // base simulation step
    const int    physSubsteps = 2;         // internal micro-steps per tick

    // Gravity (stronger so it stays planted)
    const double g  = 90.0;                // cells/s^2 downward

    // Engine power + top speed (calm but climbs)
    const double engineAcc = 105.0;        // throttle strength on ground
    const double maxVx     = 110.0;        // clamp horizontal speed

    // Friction & air drag
    const double rollFriction = 12.0;
    const double airDragX     = 0.020;     // horizontal air drag
    const double airDragY     = 0.080;     // vertical air drag

    // Micro-step climbing resolution & limits
    const double stepInc = 0.02;           // finer steps for smoother climbs
    const double stepMax = 1.80;           // base max vertical “pop” per horizontal move
    const double minJumpSpeed = 60.0;      // faster motion allows a bit more step
    const double eps = 1e-4;

    // Landing & slope feel
    const double landingVelThreshold = 12.0;
    const double landingBoostFactor  = 0.20;  // reduced so it doesn't launch forward
    const double slopeAccelFactor    = 0.18;  // gravity feel along slope (scaled)

    // Rotation feel
    const double torqueAccel = 220.0; // deg/s^2 (anticlockwise on throttle)
    const double torqueBrake = 160.0; // deg/s^2 (clockwise on brake)
    const double alignSpring = 12.0;  // ground alignment spring (deg/s^2 per deg error)
    const double angDamp     = 3.5;   // angular damping (1/s)

    // Smoother control & hill-climb helpers
    const double engineAccAir       = 60.0;  // mild air control
    const double groundStick        = 60.0;  // pushes car into ground over crests
    const double stepMaxUphillBoost = 2.5;   // extra micro-step when accelerating uphill
    const double throttleRampUp     = 0.60;  // seconds to reach full power
    const double throttleRampDown   = 0.30;  // seconds to decay when released
    const double groundSnapMax      = 0.60;  // auto-drop distance to “stick” to stairs

    // === NEW: momentum-aware stair params ===
    const double airStepFactor      = 0.85;  // when airborne-but-falling, allow 85% of stepMax
    const double stepBonusPerSpeed  = 0.006; // extra step (cells) per |vx| (cells/s)
    const double stepBonusMax       = 0.90;  // clamp extra step from speed
    const double blockedVxDampen    = 0.35;  // keep ~35% of vx if blocked (don’t kill momentum)

    QTimer gameTimer;

    void resetEverything();
    void startGame();
    void stopGame(const QString &msg = QString());
    void simulate();
    void applyForces(double slopeAngleRad, double stepDt);
    void moveHorizontal(double dx);
    void moveVertical(double dy);
    void snapToGroundIfClose();
    bool overlaps(const QRectF& r) const;
    bool colAt(int i, int j) const;

    bool onGround = false;

    bool carIntersectsFinish() const;

    int colsVisible() const;
    int rowsVisible() const;

    bool dragging = false;
    Qt::MouseButton dragButton = Qt::NoButton;

    bool paintingStroke = false;
    bool hasPath = false;
    QPoint pathStart;
    QPoint pathEnd;
    QPoint lastPaintCell;
    QPoint lastDrawnCell;
};

#endif // MAINWINDOW_H
