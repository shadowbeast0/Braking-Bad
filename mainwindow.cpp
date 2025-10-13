#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "my_label.h"

#include <QPainter>
#include <QPen>
#include <QColor>
#include <QTransform>
#include <algorithm>
#include <cmath>
#include <limits>

static inline double rad2deg(double r){ return r * (180.0 / M_PI); }
static inline double deg2rad(double d){ return d * (M_PI / 180.0); }
static inline double clamp(double v, double lo, double hi){ return std::max(lo, std::min(v, hi)); }
static inline double normAngleDeg(double a){
    while (a > 180.0) a -= 360.0;
    while (a < -180.0) a += 360.0;
    return a;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFocusPolicy(Qt::StrongFocus);
    ui->frame->setFocusPolicy(Qt::StrongFocus);

    connect(ui->frame, &my_label::mousePressedAt,  this, &MainWindow::onCanvasPressed);
    connect(ui->frame, &my_label::mouseMovedAt,    this, &MainWindow::onCanvasMoved);
    connect(ui->frame, &my_label::mouseReleasedAt, this, &MainWindow::onCanvasReleased);

    connect(&gameTimer, &QTimer::timeout, this, &MainWindow::onGameTick);
    gameTimer.setTimerType(Qt::PreciseTimer);
    gameTimer.setInterval(int(dt * 1000.0));

    gridSize = ui->spinGridSize->value();
    drawGrid();
    setFocus(Qt::ActiveWindowFocusReason);
    ui->lblInfo->setText(
        "Edit Mode: LMB draw, RMB erase. Press E for Eraser Mode (LMB erases). "
        "Start/Finish = leftmost/rightmost drawn columns. In Game: D/A, Space = 1.5-cell jump."
        );
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::drawGrid()
{
    const QSize area = ui->frame->size();
    if (area.isEmpty()) return;

    canvas = QPixmap(area);
    canvas.fill(Qt::white);

    {
        QPainter p(&canvas);
        QPen pen(QColor(200, 200, 200, 80));
        p.setPen(pen);

        int roundedOffsetX = int(std::round(viewOffsetX * gridSize));
        for (int x = -(roundedOffsetX % gridSize); x <= canvas.width(); x += gridSize)
            p.drawLine(x, 0, x, canvas.height());
        for (int y = 0; y <= canvas.height(); y += gridSize)
            p.drawLine(0, y, canvas.width(), y);
    }

    drawTerrainAndMarkers();
    drawCar();
    ui->frame->setPixmap(canvas);
}

void MainWindow::drawTerrainAndMarkers()
{
    if (canvas.isNull()) return;

    QPainter p(&canvas);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);

    const int vx0 = int(std::floor(viewOffsetX));
    const int vx1 = vx0 + colsVisible();
    const int vy0 = 0;
    const int vy1 = rowsVisible() - 1;

    for (const QPoint &cell : terrain) {
        if (cell.x() < vx0 || cell.x() > vx1 || cell.y() < vy0 || cell.y() > vy1) continue;
        p.drawRect(worldCellToScreenRect(cell));
    }

    if (hasPath) {
        if (pathStart.x() >= vx0 && pathStart.x() <= vx1 && pathStart.y() >= vy0 && pathStart.y() <= vy1) {
            p.setBrush(QColor(255, 200, 0, 180));
            p.drawRect(worldCellToScreenRect(pathStart));
        }
        if (pathEnd.x() >= vx0 && pathEnd.x() <= vx1 && pathEnd.y() >= vy0 && pathEnd.y() <= vy1) {
            p.setBrush(Qt::green);
            p.drawRect(worldCellToScreenRect(pathEnd));
        }
    }
}

void MainWindow::drawCar()
{
    if (!gameTimer.isActive()) return;

    QPainter p(&canvas);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 255, 120));

    const double sx = (carX - viewOffsetX) * gridSize;
    const double sy =  carY * gridSize;
    const double w  =  CAR_SIZE * gridSize;
    const double h  =  CAR_SIZE * gridSize;

    const double cx = sx + 0.5 * w;
    const double cy = sy + 0.5 * h;

    p.translate(cx, cy);
    p.rotate(carAngleDeg);
    p.drawRect(QRectF(-0.5 * w, -0.5 * h, w, h));
    p.resetTransform();
}

QPoint MainWindow::viewToWorldCell(const QPoint &inView) const
{
    double gx = double(inView.x()) / gridSize + viewOffsetX;
    int gy = inView.y() / gridSize;
    return QPoint(int(std::floor(gx)), gy);
}

QRect MainWindow::worldCellToScreenRect(const QPoint &wcell) const
{
    return QRect(
        int(std::round((wcell.x() - viewOffsetX) * gridSize)),
        (wcell.y()) * gridSize,
        gridSize,
        gridSize
        );
}

int MainWindow::colsVisible() const { return std::max(1, canvas.width() / gridSize); }
int MainWindow::rowsVisible() const { return std::max(1, canvas.height() / gridSize); }

void MainWindow::fillLineBresenham(const QPoint &from, const QPoint &to)
{
    int x0 = from.x(), y0 = from.y();
    int x1 = to.x(), y1 = to.y();

    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        terrain.insert(QPoint(x0, y0));
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

void MainWindow::eraseLineBresenham(const QPoint &from, const QPoint &to)
{
    int x0 = from.x(), y0 = from.y();
    int x1 = to.x(), y1 = to.y();

    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        terrain.remove(QPoint(x0, y0));
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

void MainWindow::onCanvasPressed(QPoint pos, Qt::MouseButton button)
{
    if (!editMode) return;
    setFocus(Qt::ActiveWindowFocusReason);

    QPoint w = viewToWorldCell(pos);

    if (button == Qt::LeftButton) {
        dragging = true;
        dragButton = Qt::LeftButton;
        lastDrawnCell = w;

        if (eraserMode) {
            terrain.remove(w);
            recomputeStartEnd();
            drawGrid();
            return;
        }

        paintingStroke = true;
        pathStart = w; // temporary; real start/end are recomputed globally
        lastPaintCell = w;
        terrain.insert(w);
        recomputeStartEnd();
        drawGrid();
    } else if (button == Qt::RightButton) {
        dragging = true;
        dragButton = Qt::RightButton;
        lastDrawnCell = w;
        terrain.remove(w);
        recomputeStartEnd();
        drawGrid();
    }
}

void MainWindow::onCanvasMoved(QPoint pos, Qt::MouseButtons buttons)
{
    if (!editMode) return;
    if (!dragging) return;

    QPoint w = viewToWorldCell(pos);

    if (dragButton == Qt::LeftButton && (buttons & Qt::LeftButton)) {
        if (w != lastDrawnCell) {
            if (eraserMode) {
                eraseLineBresenham(lastDrawnCell, w);
            } else {
                fillLineBresenham(lastDrawnCell, w);
                lastPaintCell = w;
            }
            lastDrawnCell = w;
            recomputeStartEnd();
            drawGrid();
        }
    } else if (dragButton == Qt::RightButton && (buttons & Qt::RightButton)) {
        if (w != lastDrawnCell) {
            eraseLineBresenham(lastDrawnCell, w);
            lastDrawnCell = w;
            recomputeStartEnd();
            drawGrid();
        }
    }
}

void MainWindow::onCanvasReleased(QPoint, Qt::MouseButton)
{
    paintingStroke = false;
    dragging = false;
    dragButton = Qt::NoButton;
}

void MainWindow::on_spinGridSize_valueChanged(int val)
{
    gridSize = std::max(5, val);
    drawGrid();
}

void MainWindow::on_btnDrawGrid_clicked()
{
    resetEverything();
    drawGrid();
}

void MainWindow::on_btnEditMode_clicked()
{
    stopGame();
    editMode = true;
    setFocus(Qt::ActiveWindowFocusReason);
    ui->lblInfo->setText(
        eraserMode
            ? "Edit Mode (ERASER): LMB erases, RMB also erases. Start/Finish = leftmost/rightmost columns."
            : "Edit Mode: LMB draw, RMB erase. Press E for Eraser Mode. Start/Finish = leftmost/rightmost columns."
        );
    drawGrid();
}

void MainWindow::on_btnStartGame_clicked()
{
    startGame();
}

void MainWindow::on_btnScrollLeft_clicked()
{
    if (!editMode) return;
    viewOffsetX -= std::max(1, colsVisible() / 2);
    drawGrid();
}

void MainWindow::on_btnScrollRight_clicked()
{
    if (!editMode) return;
    viewOffsetX += std::max(1, colsVisible() / 2);
    drawGrid();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    drawGrid();
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if (editMode) {
        if (e->key() == Qt::Key_Left)  { viewOffsetX -= 2; drawGrid(); }
        if (e->key() == Qt::Key_Right) { viewOffsetX += 2; drawGrid(); }
        if (!e->isAutoRepeat() && e->key() == Qt::Key_E) {
            eraserMode = !eraserMode;
            ui->lblInfo->setText(
                eraserMode
                    ? "Edit Mode (ERASER): LMB erases, RMB also erases. Start/Finish = leftmost/rightmost columns."
                    : "Edit Mode: LMB draw, RMB erase. Press E for Eraser Mode. Start/Finish = leftmost/rightmost columns."
                );
        }
        return;
    }

    // Game mode
    if (e->isAutoRepeat()) return;
    if (e->key() == Qt::Key_D) keyAccel = true;
    if (e->key() == Qt::Key_A) keyBrake = true;
    if (e->key() == Qt::Key_Space) {
        if (onGround) {
            jumpOnePixel();   // 1.5-cell jump
            onGround = false; // block stacking in same frame
            drawGrid();
        }
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
    if (e->isAutoRepeat()) return;
    if (e->key() == Qt::Key_D) keyAccel = false;
    if (e->key() == Qt::Key_A) keyBrake = false;
}

void MainWindow::resetEverything()
{
    stopGame();
    terrain.clear();
    editMode = true;
    eraserMode = false;
    viewOffsetX = 0.0;
    carX = 2.0; carY = 0.0; vx = 0.0; vy = 0.0;
    carAngleDeg = 0.0; angVelDeg = 0.0;
    hasPath = false;
    paintingStroke = false;
    setFocus(Qt::ActiveWindowFocusReason);
    ui->lblInfo->setText("Reset. Edit Mode: LMB draw, RMB erase. Press E for Eraser Mode.");
}

void MainWindow::startGame()
{
    editMode = false;
    keyAccel = keyBrake = false;

    if (hasPath) {
        carX = pathStart.x() + 0.05;
        carY = pathStart.y() - CAR_SIZE - 0.001;
        viewOffsetX = carX - colsVisible() / 4.0;
    }

    vx = 0.0;
    vy = 0.0;
    carAngleDeg = 0.0;
    angVelDeg = 0.0;

    setFocus(Qt::ActiveWindowFocusReason);
    gameTimer.start();
    ui->lblInfo->setText("Game: D accelerate, A brake, Space = 1.5-cell jump.");
}

void MainWindow::stopGame(const QString &msg)
{
    if (gameTimer.isActive()) gameTimer.stop();
    if (!msg.isEmpty()) ui->lblInfo->setText(msg);
}

void MainWindow::onGameTick()
{
    simulate();

    double targetOffset = carX - colsVisible() / 3.0;
    viewOffsetX = viewOffsetX * 0.95 + targetOffset * 0.05;

    if (carIntersectsFinish()) stopGame("Finish reached!");
    drawGrid();
}

void MainWindow::simulate()
{
    const double subDt = dt / double(physSubsteps);

    for (int s = 0; s < physSubsteps; ++s) {
        double slopeAngleRad = groundSlopeAngleRad();

        // Forces & integration for this sub-step
        applyForces(slopeAngleRad, subDt);
        onGround = false;
        moveHorizontal(vx * subDt);
        moveVertical(vy * subDt);

        // Keep contact on jagged stairs when we're very close
        if (!onGround) snapToGroundIfClose();

        // Update orientation after collision resolution of this sub-step
        updateRotation(slopeAngleRad, subDt);
    }
}

// ============ uphill-by-acceleration + air drag + soft-start ================
void MainWindow::applyForces(double slopeAngleRad, double stepDt)
{
    double ax = 0.0;   // world-x acceleration
    double ay = 0.0;   // world-y acceleration term to add with gravity

    // Raw input in [-1, +1]
    const double rawDrive =
        (keyAccel ? 1.0 : 0.0) +
        (keyBrake ? -1.0 : 0.0);

    // Soft-start ramp so speed doesn't spike immediately on key down
    static double throttleGain = 0.0;
    if (std::abs(rawDrive) > 1e-6) {
        throttleGain = std::min(1.0, throttleGain + stepDt / throttleRampUp);
    } else {
        throttleGain = std::max(0.0, throttleGain - stepDt / throttleRampDown);
    }
    const double drive = rawDrive * (0.35 + 0.65 * throttleGain); // start ~35%, ease to 100%

    if (onGround) {
        const double ct = std::cos(slopeAngleRad);
        const double st = std::sin(slopeAngleRad);

        // Push along the slope (tangent). Key for climbing hills by throttle.
        ax += drive * engineAcc * ct;
        ay += drive * engineAcc * st;

        // Downhill pull feel
        ax += slopeAccelFactor * (-g * st);

        // Stick to ground at crests
        ay -= groundStick;
    } else {
        // Mild air control
        ax += drive * engineAccAir;
    }

    // Rolling friction when coasting on ground
    if (!keyAccel && !keyBrake && onGround) {
        double s = (vx > 0) ? -1.0 : (vx < 0 ? 1.0 : 0.0);
        ax += s * rollFriction;
        if (std::abs(vx) < 0.5 * rollFriction * stepDt) vx = 0.0;
    }

    // Air drag (horizontal + vertical)
    ax += -airDragX * vx;
    ay += -airDragY * vy;

    // Integrate horizontal
    vx += ax * stepDt;
    vx = clamp(vx, -maxVx, maxVx);

    // Gravity (down is +g). Add our vertical term as well.
    vy += (g + ay) * stepDt;
}

bool MainWindow::colAt(int i, int j) const
{
    return terrain.contains(QPoint(i, j));
}

bool MainWindow::overlaps(const QRectF& r) const
{
    int i0 = int(std::floor(r.left()));
    int i1 = int(std::floor(r.right() - 1e-6));
    int j0 = int(std::floor(r.top()));
    int j1 = int(std::floor(r.bottom() - 1e-6));
    for (int i = i0; i <= i1; ++i)
        for (int j = j0; j <= j1; ++j)
            if (colAt(i, j)) return true;
    return false;
}

// ====== momentum-aware stair controller (airborne pre-step + speed bonus) ====
void MainWindow::moveHorizontal(double dx)
{
    if (dx == 0.0) return;

    const double targetX = carX + dx;

    // quick forward try
    QRectF forward(targetX, carY, CAR_SIZE, CAR_SIZE);
    if (!overlaps(forward)) { carX = targetX; return; }

    // allow stepping when grounded OR when falling/landing (vy >= 0)
    const bool canStep = onGround || (vy >= 0.0);

    // dynamic step budget: base +/- situation + momentum bonus
    double stepBudget = stepMax * (onGround ? 1.0 : (canStep ? airStepFactor : 0.0));
    stepBudget = std::max(0.0, stepBudget);
    const double speedBonus = std::min(stepBonusMax, std::abs(vx) * stepBonusPerSpeed);
    stepBudget += speedBonus;

    // if we truly can't step now (e.g., mid-jump upwards), just block softly
    if (stepBudget <= 0.0 && overlaps(forward)) { vx *= blockedVxDampen; return; }

    // 1) classic "raise THEN forward"
    for (double s = stepInc; s <= stepBudget + 1e-9; s += stepInc) {
        QRectF up(carX, carY - s, CAR_SIZE, CAR_SIZE);
        if (overlaps(up)) continue;

        QRectF upF(targetX, carY - s, CAR_SIZE, CAR_SIZE);
        if (!overlaps(upF)) {
            carY -= s;
            carX  = targetX;
            onGround = true; // we effectively climbed onto a step
            return;
        }
    }

    // 2) slice-walk: advance in tiny chunks, auto-raising each chunk
    const double sign = (dx > 0.0) ? 1.0 : -1.0;
    const double slice = std::max(stepInc, std::min(std::abs(dx), 0.20)); // <= 0.20 cell
    double moved = 0.0;
    double tryX = carX, tryY = carY;
    bool blocked = false;

    while (moved + 1e-9 < std::abs(dx)) {
        const double advance = std::min(slice, std::abs(dx) - moved);
        const double nx = tryX + sign * advance;

        QRectF nextF(nx, tryY, CAR_SIZE, CAR_SIZE);
        if (!overlaps(nextF)) {
            tryX = nx;
            moved += advance;
            continue;
        }

        // auto-raise this slice within the current momentum-aware budget
        bool climbed = false;
        for (double s = stepInc; s <= stepBudget + 1e-9; s += stepInc) {
            QRectF upStep(nx, tryY - s, CAR_SIZE, CAR_SIZE);
            if (!overlaps(upStep)) {
                tryX = nx;
                tryY -= s;
                moved += advance;
                climbed = true;
                break;
            }
        }
        if (!climbed) { blocked = true; break; }
    }

    carX = tryX;
    carY = tryY;
    if (blocked) vx *= blockedVxDampen; // soften, don't nuke momentum
}

void MainWindow::moveVertical(double dy)
{
    if (dy == 0.0) return;

    double targetY = carY + dy;
    QRectF test(carX, targetY, CAR_SIZE, CAR_SIZE);

    if (overlaps(test)) {
        if (dy > 0) {
            if (vy > landingVelThreshold) {
                // small forward shove on hard landings to keep momentum feel
                vx += vy * landingBoostFactor;
            }
            carY = std::floor(targetY + CAR_SIZE) - CAR_SIZE - eps;
            vy = 0.0;
            onGround = true;
        } else {
            carY = std::floor(targetY) + 1 + eps;
            vy = 0.0;
        }
    } else {
        carY = targetY;
    }

    if (carY < 0.0 && dy < 0) {
        carY = 0.0;
        vy = 0.0;
    }
}

// tiny auto-drop to “stick” to the ground on jagged steps
void MainWindow::snapToGroundIfClose()
{
    if (onGround || vy < 0.0) return; // only when falling / near ground
    for (double s = stepInc; s <= groundSnapMax + 1e-9; s += stepInc) {
        double ytest = carY + s;
        QRectF probe(carX, ytest, CAR_SIZE, CAR_SIZE);
        if (overlaps(probe)) {
            carY = std::floor(ytest + CAR_SIZE) - CAR_SIZE - eps;
            vy = 0.0;
            onGround = true;
            return;
        }
    }
}

bool MainWindow::carIntersectsFinish() const
{
    if (!hasPath) return false;
    QRectF car(carX, carY, CAR_SIZE, CAR_SIZE);
    QRectF goal(pathEnd.x(), pathEnd.y(), 1.0, 1.0);
    return car.intersects(goal);
}

void MainWindow::recomputeStartEnd()
{
    if (terrain.isEmpty()) {
        hasPath = false;
        return;
    }

    int minX = std::numeric_limits<int>::max();
    int maxX = std::numeric_limits<int>::min();
    int minY_at_minX = std::numeric_limits<int>::max();
    int minY_at_maxX = std::numeric_limits<int>::max();

    for (const QPoint& c : terrain) {
        if (c.x() < minX) { minX = c.x(); minY_at_minX = c.y(); }
        else if (c.x() == minX && c.y() < minY_at_minX) { minY_at_minX = c.y(); }

        if (c.x() > maxX) { maxX = c.x(); minY_at_maxX = c.y(); }
        else if (c.x() == maxX && c.y() < minY_at_maxX) { minY_at_maxX = c.y(); }
    }

    pathStart = QPoint(minX, minY_at_minX);
    pathEnd   = QPoint(maxX, minY_at_maxX);
    hasPath = true;
}

bool MainWindow::columnTopY(int x, int &yTop) const
{
    bool found = false;
    int best = std::numeric_limits<int>::max();
    for (const QPoint &c : terrain) {
        if (c.x() == x && c.y() < best) {
            best = c.y();
            found = true;
        }
    }
    if (found) yTop = best;
    return found;
}

double MainWindow::groundSlopeAngleRad() const
{
    // sample the columns under left and right edges of the car
    int iL = int(std::floor(carX));
    int iR = int(std::floor(carX + CAR_SIZE - 1e-6));
    if (iR == iL) iR = iL + 1;

    int yL, yR;
    bool fL = columnTopY(iL, yL);
    bool fR = columnTopY(iR, yR);
    if (!fL || !fR) return 0.0; // flat if unknown

    double dy = double(yR - yL);
    double dx = double(iR - iL);
    return std::atan2(dy, dx);
}

void MainWindow::updateRotation(double slopeAngleRad, double stepDt)
{
    // Torques from player input (visual feedback & cresting help)
    if (keyAccel) angVelDeg -= torqueAccel * stepDt; // anticlockwise
    if (keyBrake) angVelDeg += torqueBrake * stepDt; // clockwise

    // Align to ground slope when on ground (soft spring towards slope)
    if (onGround) {
        double targetDeg = rad2deg(slopeAngleRad);
        double err = normAngleDeg(targetDeg - carAngleDeg);
        angVelDeg += alignSpring * err * stepDt;
    }

    // Damping and integrate
    angVelDeg *= (1.0 - angDamp * stepDt);
    carAngleDeg += angVelDeg * stepDt;
    carAngleDeg = normAngleDeg(carAngleDeg);
}

// Jump UP by 1.5 grid cells if space is free; only allowed when onGround.
void MainWindow::jumpOnePixel()
{
    const double dy_world = 1.5;                // 1.5 cells
    const double targetY  = carY - dy_world;

    QRectF test(carX, targetY, CAR_SIZE, CAR_SIZE);
    if (!overlaps(test)) {
        carY = targetY;
        onGround = false; // airborne immediately
    }
}
