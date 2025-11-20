// prop.cpp
#include "prop.h"
#include <cmath>
#include <algorithm>
#include <vector>

PropSystem::PropSystem() {}

void PropSystem::clear() {
    m_props.clear();
}

void PropSystem::prune(int minWorldX) {
    auto it = std::remove_if(m_props.begin(), m_props.end(), [minWorldX](const Prop& p){
        return p.wx < minWorldX - 500;
    });
    m_props.erase(it, m_props.end());
}

void PropSystem::maybeSpawnProp(int worldX, int groundGy, int levelIndex, float slope, std::mt19937& rng) {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::uniform_int_distribution<int> varDist(0, 6);
    std::uniform_int_distribution<int> flipDist(0, 1);

    if (!m_props.isEmpty()) {
        int minSpacing = 120;
        if (levelIndex == 5) minSpacing = 30; // Keep dense for city feel

        if (std::abs(worldX - m_props.last().wx) < minSpacing) {
            return;
        }
    }

    float chance = dist(rng);
    int wy = groundGy * Constants::PIXEL_SIZE;

    if (levelIndex == 0) {
        // MEADOW
        if (chance < 0.03f)      m_props.append({worldX, wy, PropType::Tree,    varDist(rng), (bool)flipDist(rng)});
        else if (chance < 0.06f) m_props.append({worldX, wy, PropType::Rock,    varDist(rng), (bool)flipDist(rng)});
        else if (chance < 0.12f) m_props.append({worldX, wy, PropType::Flower,  varDist(rng), false});
        else if (chance < 0.14f) m_props.append({worldX, wy, PropType::Mushroom,varDist(rng), false});
    }
    else if (levelIndex == 1) {
        // DESERT
        if (chance < 0.02f) {
            if (std::abs(slope) < 0.15f) {
                bool camelNearby = false;
                for(const auto& p : m_props) {
                    if (p.type == PropType::Camel && std::abs(p.wx - worldX) < 3000) {
                        camelNearby = true;
                        break;
                    }
                }
                if (!camelNearby) {
                    m_props.append({worldX, wy, PropType::Camel, varDist(rng), (bool)flipDist(rng)});
                }
            }
        }
        else if (chance < 0.06f) {
            m_props.append({worldX, wy, PropType::Cactus, varDist(rng), (bool)flipDist(rng)});
        }
        else if (chance > 0.998f) {
            bool exists = false;
            for(const auto& p : m_props) {
                if (p.type == PropType::Tumbleweed && std::abs(p.wx - worldX) < 1000) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                m_props.append({worldX, wy, PropType::Tumbleweed, varDist(rng), (bool)flipDist(rng)});
            }
        }
    }
    else if (levelIndex == 2) {
        // TUNDRA
        if (chance < 0.018f) {
            m_props.append({worldX, wy, PropType::Penguin, varDist(rng), (bool)flipDist(rng)});
        }
        else if (chance < 0.048f) {
            if (std::abs(slope) < 0.15f) {
                bool iglooNearby = false;
                for (const auto& p : m_props) {
                    if (p.type == PropType::Igloo && std::abs(p.wx - worldX) < 2500) {
                        iglooNearby = true;
                        break;
                    }
                }
                if (!iglooNearby) {
                    m_props.append({worldX, wy, PropType::Igloo, varDist(rng), false});
                }
            }
        }
        else if (chance < 0.06f) {
            m_props.append({worldX, wy, PropType::Snowman, varDist(rng), (bool)flipDist(rng)});
        }
        else if (chance < 0.08f) {
            m_props.append({worldX, wy, PropType::IceSpike, varDist(rng), (bool)flipDist(rng)});
        }
    }
    else if (levelIndex == 3) {
        // LUNAR
        if (chance < 0.009f) {
            int liftCells = 50 + (varDist(rng) * 2);
            int skyWy = wy - (liftCells * Constants::PIXEL_SIZE);
            m_props.append({worldX, skyWy, PropType::UFO, varDist(rng), false});
        }
    }
    else if (levelIndex == 4) {
        // MARTIAN
        if (chance < 0.015f) {
            if (std::abs(slope) < 0.25f) {
                m_props.append({worldX, wy, PropType::Rover, varDist(rng), (bool)flipDist(rng)});
            }
        }
        else if (chance > 0.998f) {
            m_props.append({worldX, wy, PropType::Alien, varDist(rng), false});
        }
    }
    else if (levelIndex == 5) {
        // NIGHTLIFE
        if (chance < 0.5f) {
            m_props.append({worldX, wy, PropType::Building, varDist(rng), false});
        }

        float lampChance = dist(rng);
        if (lampChance < 0.1f) {
            m_props.append({worldX + 15, wy, PropType::StreetLamp, varDist(rng), false});
        }
    }
}

void PropSystem::draw(QPainter& p, int camX, int camY, int screenW, int screenH, const QHash<int,int>& heightMap) {
    int camGX = camX / Constants::PIXEL_SIZE;
    int camGY = camY / Constants::PIXEL_SIZE;

    auto drawProp = [&](const Prop& prop) {
        if (prop.wx < camX - 200 || prop.wx > camX + screenW + 200) return;

        int gx = (prop.wx / Constants::PIXEL_SIZE) - camGX;
        int gy = (prop.wy / Constants::PIXEL_SIZE) + camGY;
        int worldGX = prop.wx / Constants::PIXEL_SIZE;

        switch (prop.type) {
        case PropType::Tree:       drawTree(p, gx, gy, worldGX, prop.wx, prop.wy, prop.variant, heightMap); break;
        case PropType::Rock:       drawRock(p, gx, gy, prop.variant); break;
        case PropType::Flower:     drawFlower(p, gx, gy, prop.variant); break;
        case PropType::Mushroom:   drawMushroom(p, gx, gy, prop.variant); break;
        case PropType::Cactus:     drawCactus(p, gx, gy, prop.variant); break;
        case PropType::Tumbleweed: drawTumbleweed(p, gx, gy, prop.variant); break;
        case PropType::Camel:      drawCamel(p, gx, gy, prop.variant, prop.flipped); break;
        case PropType::Igloo:      drawIgloo(p, gx, gy, worldGX, prop.variant, heightMap); break;
        case PropType::Penguin:    drawPenguin(p, gx, gy, prop.variant, prop.flipped); break;
        case PropType::Snowman:    drawSnowman(p, gx, gy, prop.variant); break;
        case PropType::IceSpike:   drawIceSpike(p, gx, gy, prop.variant); break;
        case PropType::UFO:        drawUFO(p, gx, gy, prop.variant); break;
        case PropType::Rover:      drawRover(p, gx, gy, worldGX, prop.variant, prop.flipped, heightMap); break;
        case PropType::Alien:      drawAlien(p, gx, gy, prop.variant); break;
        case PropType::Building:   drawBuilding(p, gx, gy, worldGX, prop.variant, heightMap); break;
        case PropType::StreetLamp: drawStreetLamp(p, gx, gy, worldGX, prop.variant, heightMap); break;
        }
    };

    // PASS 1: Draw Buildings FIRST
    for (const Prop& prop : m_props) {
        if (prop.type == PropType::Building) drawProp(prop);
    }

    // PASS 2: Draw Everything Else
    for (const Prop& prop : m_props) {
        if (prop.type != PropType::Building) drawProp(prop);
    }
}

void PropSystem::plot(QPainter& p, int gx, int gy, const QColor& c) {
    p.fillRect(gx * Constants::PIXEL_SIZE, gy * Constants::PIXEL_SIZE,
               Constants::PIXEL_SIZE, Constants::PIXEL_SIZE, c);
}

// === PROPS IMPLEMENTATION ===

void PropSystem::drawBuilding(QPainter& p, int gx, int gy, int worldGX, int variant, const QHash<int,int>& heightMap) {
    // Dark building body colors
    QColor bDark(10, 10, 18);
    QColor bFrame(40, 40, 60);

    std::vector<QColor> neons = {
        QColor(30, 180, 180), QColor(180, 0, 90), QColor(120, 0, 200),
        QColor(50, 180, 40),  QColor(200, 180, 40), QColor(200, 80, 40),
        QColor(80, 100, 180)
    };
    QColor neon = neons[(variant + worldGX) % neons.size()];

    int h = 30 + (variant * 4);
    int w = 32 + (variant % 3) * 8;

    if (variant == 5) { w = 25; h = 80; }
    if (variant == 3) { w = 36; h = 100; }

    int leftRel = -w/2;
    int rightRel = w/2;
    int topScreenY = gy - h;

    // Iterate columns
    for (int dx = leftRel; dx <= rightRel; dx++) {
        int currentWX = worldGX + dx;
        int currentScreenX = gx + dx;

        // Lookup heightmap with default value heuristic
        int centerGroundY = heightMap.value(worldGX, 0);
        int groundYAtCol = heightMap.value(currentWX, centerGroundY);
        int groundScreenY = gy + (groundYAtCol - centerGroundY);

        // OPTIMIZATION: Draw Building Body as one solid rect per column
        int columnHeight = groundScreenY - topScreenY;
        if (columnHeight > 0) {
            bool isSideEdge = (dx == leftRel || dx == rightRel);
            p.fillRect(currentScreenX * Constants::PIXEL_SIZE,
                       topScreenY * Constants::PIXEL_SIZE,
                       Constants::PIXEL_SIZE,
                       columnHeight * Constants::PIXEL_SIZE,
                       isSideEdge ? bFrame : bDark);
        }

        // Draw Top Edge
        plot(p, currentScreenX, topScreenY, bFrame);

        // OPTIMIZATION: Draw Windows with Stride 4
        // Fix: Use relative Y coordinate to prevent flickering/crawling lights
        if (dx > leftRel && dx < rightRel) {
            int yStart = topScreenY + 2;

            for (int y = yStart; y < groundScreenY - 1; y += 4) {
                bool windowExists = false;

                // Determine position relative to the building top, not the screen
                int relY = y - topScreenY;

                if (variant == 5) { // Mall
                    windowExists = true;
                } else if (variant == 3) { // Casino
                    if ((dx % 4) == 0) windowExists = true;
                } else { // Standard Grid
                    // Use relY so the pattern is fixed to the building geometry
                    if ((dx % 3 == 0) && ((dx + relY) % 3 != 0)) windowExists = true;
                }

                if (windowExists) {
                    // Draw 2-pixel high window to match the stride
                    p.fillRect(currentScreenX * Constants::PIXEL_SIZE,
                               y * Constants::PIXEL_SIZE,
                               Constants::PIXEL_SIZE,
                               2 * Constants::PIXEL_SIZE,
                               neon);
                }
            }
        }
    }

    // Antennas (Static)
    if (variant == 3 || variant == 5) {
        for(int i=1; i<=10; i++){
            plot(p, gx, topScreenY - i, bFrame);
        }
    }
}

void PropSystem::drawStreetLamp(QPainter& p, int gx, int gy, int worldGX, int variant, const QHash<int,int>& heightMap) {
    QColor pole(100, 100, 110);
    QColor light(255, 255, 220);

    if (variant % 3 == 1) light = QColor(200, 240, 255);
    else if (variant % 3 == 2) light = QColor(255, 200, 180);

    int h = 24;
    int groundScreenY = gy;

    // Optimization: Draw Pole as one rect
    p.fillRect(gx * Constants::PIXEL_SIZE, (groundScreenY - h) * Constants::PIXEL_SIZE,
               Constants::PIXEL_SIZE, h * Constants::PIXEL_SIZE, pole);

    // Top
    plot(p, gx + 1, groundScreenY - h, pole);
    plot(p, gx + 2, groundScreenY - h, pole);
    // Light
    plot(p, gx + 2, groundScreenY - h + 1, light);
    // Glow
    plot(p, gx + 1, groundScreenY - h + 2, light);
    plot(p, gx + 3, groundScreenY - h + 2, light);
    plot(p, gx + 2, groundScreenY - h + 2, light);
}

// === Existing Prop Implementations (Unchanged) ===

void PropSystem::drawTree(QPainter& p, int gx, int gy, int worldGX, int wx, int wy, int variant, const QHash<int,int>& heightMap) {
    QColor cTrunk(184, 115, 51); QColor cTrunkDark(100, 50, 20); QColor cHole(80, 40, 10);
    QColor cLeafBase(46, 184, 46); QColor cLeafLight(154, 235, 90); QColor cLeafDark(20, 110, 35);
    int trunkW = 6; int trunkH = 30 + (variant * 2);
    int centerGroundWorldY = heightMap.value(worldGX, 0); int camYOffset = gy - centerGroundWorldY;
    int peakScreenY = 999999; int halfTrunk = trunkW / 2;
    for(int dx = -halfTrunk; dx < halfTrunk; dx++) { int wgx = worldGX + dx; if(heightMap.contains(wgx)) { int sGY = heightMap.value(wgx) + camYOffset; if(sGY < peakScreenY) peakScreenY = sGY; } }
    if(peakScreenY == 999999) peakScreenY = gy;
    int effectiveBaseY = peakScreenY - 1;
    for(int dx = -halfTrunk + 1; dx < halfTrunk; dx++) { int wgx = worldGX + dx; int groundScreenY = gy; if(heightMap.contains(wgx)) groundScreenY = heightMap.value(wgx) + camYOffset; int trunkTopY = effectiveBaseY - trunkH;
        for(int y = trunkTopY; y <= groundScreenY; y++) {
            bool isBorder = (dx == -halfTrunk+1 || dx == halfTrunk-1); bool isShadow = (dx == -1 && (effectiveBaseY - y) > 0 && (effectiveBaseY - y) % 3 == 0); bool isLight  = (dx == 1  && (effectiveBaseY - y) > 0 && (effectiveBaseY - y) % 4 == 0);
            bool isRootFill = (y > effectiveBaseY); QColor c = cTrunk; if (isRootFill) c = cTrunkDark; else if (isBorder) c = cTrunkDark; else if (isShadow) c = cTrunkDark; else if (isLight)  c = cTrunk.lighter(110);
            plot(p, gx + dx, y, c);
        }
    }
    plot(p, gx, effectiveBaseY - trunkH/2, cHole); plot(p, gx, effectiveBaseY - trunkH/2 - 1, cHole);
    int folBot = effectiveBaseY - trunkH + 2;
    std::vector<int> rows = { 26, 28, 30, 30, 28, 26, 22, 20, 22, 24, 24, 22, 20, 16, 14, 18, 20, 18, 16, 14, 12, 10, 14, 16, 14, 12, 10, 8, 6, 8, 6, 4, 2 };
    for(int i=0; i<rows.size(); i++) { int w = rows[i]; if (variant % 2 == 0) w += 2; int py = folBot - i; int startX = gx - w/2; int endX = gx + w/2;
        for(int px = startX; px <= endX; px++) { int snX = (wx / Constants::PIXEL_SIZE) + (px - gx); int snY = (wy / Constants::PIXEL_SIZE) - i; int pat = (snX * 17 + snY * 13 + variant * 7) % 100; int lightThresh = 50; int shadowThresh = 15;
            if (px < gx) { lightThresh -= 15; shadowThresh -= 10; } else if (px > gx) { lightThresh += 25; shadowThresh += 20; }
            QColor c = cLeafBase; if (pat > lightThresh) c = cLeafLight; else if (pat < shadowThresh) c = cLeafDark; if (px == startX || px == endX || i == rows.size()-1) { c = cLeafDark; }
            plot(p, px, py, c);
        }
    }
}

void PropSystem::drawRock(QPainter& p, int gx, int gy, int variant) { QColor c(100, 100, 110); QColor highlight(140, 140, 150); int r = 2 + (variant % 2); for(int dy = -r; dy <= 0; dy++) { for(int dx = -r; dx <= r; dx++) { if (dx*dx + (dy*dy)*1.5 <= r*r) { plot(p, gx+dx, gy+dy, (dx<0 && dy<-r/2) ? highlight : c); } } } }
void PropSystem::drawFlower(QPainter& p, int gx, int gy, int variant) { QColor stem(50, 160, 50); QColor petal = (variant % 3 == 0) ? QColor(255, 50, 50) : ((variant % 3 == 1) ? QColor(255, 255, 50) : QColor(100, 100, 255)); plot(p, gx, gy, stem); plot(p, gx, gy-1, stem); plot(p, gx, gy-2, petal); plot(p, gx-1, gy-2, petal); plot(p, gx+1, gy-2, petal); plot(p, gx, gy-3, petal); }
void PropSystem::drawMushroom(QPainter& p, int gx, int gy, int variant) { QColor stalk(220, 220, 210); QColor cap = (variant % 2 == 0) ? QColor(200, 60, 60) : QColor(180, 140, 80); plot(p, gx, gy, stalk); plot(p, gx, gy-1, stalk); plot(p, gx-2, gy-1, cap); plot(p, gx-1, gy-1, cap); plot(p, gx, gy-1, cap); plot(p, gx+1, gy-1, cap); plot(p, gx+2, gy-1, cap); plot(p, gx-1, gy-2, cap); plot(p, gx, gy-2, cap); plot(p, gx+1, gy-2, cap); }
void PropSystem::drawCactus(QPainter& p, int gx, int gy, int variant) { QColor c(40, 150, 40); int h = 10 + variant * 2; for(int y=0; y<h; y++) { plot(p, gx, gy - y, c); plot(p, gx - 1, gy - y, c); plot(p, gx + 1, gy - y, c); } plot(p, gx, gy - h, c); if (variant > 0) { int armY = gy - (h/2); plot(p, gx-2, armY, c); plot(p, gx-3, armY, c); plot(p, gx-2, armY+1, c); plot(p, gx-3, armY+1, c); plot(p, gx-3, armY-1, c); plot(p, gx-4, armY-1, c); plot(p, gx-3, armY-2, c); plot(p, gx-4, armY-2, c); } if (variant > 2) { int armY2 = gy - (h/2) - 2; plot(p, gx+2, armY2, c); plot(p, gx+3, armY2, c); plot(p, gx+2, armY2+1, c); plot(p, gx+3, armY2+1, c); plot(p, gx+3, armY2-1, c); plot(p, gx+4, armY2-1, c); plot(p, gx+3, armY2-2, c); plot(p, gx+4, armY2-2, c); } }
void PropSystem::drawTumbleweed(QPainter& p, int gx, int gy, int variant) { QColor twigDark(100, 80, 50); QColor twigLight(180, 140, 90); int r = 7 + (variant % 3); int cy = gy - r; for(int dy = -r; dy <= r; dy++) { for(int dx = -r; dx <= r; dx++) { double dist = std::sqrt(dx*dx + dy*dy); if (dist <= r) { int lines1 = (dx * 3 + dy * 3 + variant * 11) % 7; int lines2 = (dx * -3 + dy * 4 + variant * 5) % 6; int lines3 = (dx * 5 + dy + variant * 2) % 9; bool isBranch = false; QColor c = twigDark; if (lines1 == 0 || lines2 == 0) isBranch = true; if (lines3 == 0 && dist < r - 2) isBranch = true; if (dist > r - 1.5) { isBranch = true; c = twigDark; } else if (isBranch) { c = twigLight; } int noise = (dx * 97 + dy * 89) % 100; if (isBranch && lines1 != 0 && lines2 != 0 && noise < 20) { isBranch = false; } if (isBranch) { plot(p, gx+dx, cy+dy, c); } } } } }
void PropSystem::drawCamel(QPainter& p, int gx, int gy, int variant, bool flipped) { int d = flipped ? -1 : 1; QColor bodyColor(218, 165, 32); QColor legColor(139, 69, 19); for (int y = 0; y < 8; ++y) plot(p, gx + (4 * d), gy - y, legColor); for (int y = 0; y < 8; ++y) plot(p, gx - (6 * d), gy - y, legColor); for (int y = 1; y < 8; ++y) plot(p, gx + (3 * d), gy - y, bodyColor); for (int y = 1; y < 8; ++y) plot(p, gx - (5 * d), gy - y, bodyColor); for (int x = -7; x <= 5; ++x) { for (int y = 8; y < 14; ++y) { plot(p, gx + (x * d), gy - y, bodyColor); } } bool twoHumps = (variant % 2 == 0); if (twoHumps) { plot(p, gx - (4 * d), gy - 14, bodyColor); plot(p, gx - (3 * d), gy - 14, bodyColor); plot(p, gx - (4 * d), gy - 15, bodyColor); plot(p, gx - (3 * d), gy - 15, bodyColor); plot(p, gx + (1 * d), gy - 14, bodyColor); plot(p, gx + (2 * d), gy - 14, bodyColor); plot(p, gx + (1 * d), gy - 15, bodyColor); plot(p, gx + (2 * d), gy - 15, bodyColor); } else { for(int x = -2; x <= 1; x++) { plot(p, gx + (x * d), gy - 14, bodyColor); plot(p, gx + (x * d), gy - 15, bodyColor); } plot(p, gx - (1 * d), gy - 16, bodyColor); plot(p, gx, gy - 16, bodyColor); } for(int y = 12; y < 18; y++) { plot(p, gx + (6 * d), gy - y, bodyColor); plot(p, gx + (7 * d), gy - y, bodyColor); } plot(p, gx + (6 * d), gy - 18, bodyColor); plot(p, gx + (7 * d), gy - 18, bodyColor); plot(p, gx + (8 * d), gy - 18, bodyColor); plot(p, gx + (6 * d), gy - 19, bodyColor); plot(p, gx + (7 * d), gy - 19, bodyColor); plot(p, gx + (5 * d), gy - 19, legColor); plot(p, gx + (7 * d), gy - 19, legColor); plot(p, gx - (8 * d), gy - 10, legColor); plot(p, gx - (8 * d), gy - 9, bodyColor); }
void PropSystem::drawIgloo(QPainter& p, int gx, int gy, int worldGX, int variant, const QHash<int,int>& heightMap) { QColor ice(220, 230, 255); QColor iceShadow(180, 190, 220); QColor dark(50, 50, 60); int r = 14 + (variant % 3); int centerGroundWorldY = heightMap.value(worldGX, 0); int camYOffset = gy - centerGroundWorldY; int peakScreenY = 999999; for(int dx = -r; dx <= r; dx++) { int wgx = worldGX + dx; if(heightMap.contains(wgx)) { int groundScreenY = heightMap.value(wgx) + camYOffset; if(groundScreenY < peakScreenY) { peakScreenY = groundScreenY; } } } if (peakScreenY == 999999) peakScreenY = gy; for(int dx = -r; dx <= r; dx++) { int wgx = worldGX + dx; int groundScreenY = gy; if(heightMap.contains(wgx)) { groundScreenY = heightMap.value(wgx) + camYOffset; } int h = std::round(std::sqrt(r*r - dx*dx)); int domeTopY = peakScreenY - h; for (int y = domeTopY; y < groundScreenY; y++) { bool isFoundation = (y >= peakScreenY); bool isShadow = (dx > r/3) || (y > peakScreenY - r/4 && !isFoundation); QColor c = (isShadow || isFoundation) ? iceShadow : ice; plot(p, gx + dx, y, c); } } int tunW = 6; int tunH = 8; int tunBaseY = peakScreenY; for(int dx = -tunW; dx <= tunW; dx++) { int wgx = worldGX + dx; int groundScreenY = gy; if(heightMap.contains(wgx)) groundScreenY = heightMap.value(wgx) + camYOffset; int tunTopY = tunBaseY - tunH; for(int y = tunTopY; y < groundScreenY; y++) { plot(p, gx + dx, y, iceShadow); } } for(int dx = -3; dx <= 3; dx++) { int wgx = worldGX + dx; int groundScreenY = gy; if(heightMap.contains(wgx)) groundScreenY = heightMap.value(wgx) + camYOffset; int holeTopY = tunBaseY - (tunH - 2); for(int y = holeTopY; y < groundScreenY; y++) { plot(p, gx + dx, y, dark); } } }
void PropSystem::drawPenguin(QPainter& p, int gx, int gy, int variant, bool flipped) { int d = flipped ? -1 : 1; QColor black(30, 30, 40); QColor white(240, 240, 250); QColor orange(255, 140, 0); plot(p, gx+(1*d), gy, orange); plot(p, gx+(2*d), gy, orange); plot(p, gx-(1*d), gy, orange); for(int y=1; y<9; y++) for(int x=-2; x<=2; x++) plot(p, gx+(x*d), gy-y, black); for(int y=1; y<8; y++) { plot(p, gx+(1*d), gy-y, white); plot(p, gx+(2*d), gy-y, white); } for(int y=9; y<=11; y++) for(int x=-2; x<=2; x++) plot(p, gx+(x*d), gy-y, black); plot(p, gx+(1*d), gy-10, white); plot(p, gx+(3*d), gy-10, orange); plot(p, gx-(1*d), gy-5, black); plot(p, gx-(2*d), gy-4, black); }
void PropSystem::drawSnowman(QPainter& p, int gx, int gy, int variant) { QColor snow(250, 250, 255); QColor carrot(255, 140, 0); QColor stick(80, 60, 40); QColor coal(20, 20, 20); QColor tooth(255, 255, 255); plot(p, gx-2, gy, snow); plot(p, gx-1, gy, snow); plot(p, gx+1, gy, snow); plot(p, gx+2, gy, snow); for(int y=1; y<6; y++) { for(int x=-3; x<=3; x++) plot(p, gx+x, gy-y, snow); } plot(p, gx, gy-2, coal); plot(p, gx, gy-4, coal); for(int y=6; y<9; y++) { for(int x=-2; x<=2; x++) plot(p, gx+x, gy-y, snow); } plot(p, gx, gy-7, coal); for(int y=9; y<16; y++) { for(int x=-2; x<=2; x++) plot(p, gx+x, gy-y, snow); } plot(p, gx-3, gy-10, snow); plot(p, gx+3, gy-10, snow); plot(p, gx-1, gy-13, coal); plot(p, gx+1, gy-13, coal); plot(p, gx, gy-12, carrot); plot(p, gx+1, gy-12, carrot); plot(p, gx+2, gy-11, carrot); plot(p, gx, gy-10, tooth); plot(p, gx, gy-16, stick); plot(p, gx-1, gy-17, stick); plot(p, gx+1, gy-17, stick); plot(p, gx-3, gy-7, stick); plot(p, gx-4, gy-6, stick); plot(p, gx+3, gy-7, stick); plot(p, gx+4, gy-8, stick); }
void PropSystem::drawIceSpike(QPainter& p, int gx, int gy, int variant) { QColor ice(180, 230, 255); int h = 5 + variant * 2; for(int y=0; y<h; y++) { plot(p, gx, gy-y, ice); if(y < h/2) { plot(p, gx-1, gy-y, ice); plot(p, gx+1, gy-y, ice); } } }
void PropSystem::drawUFO(QPainter& p, int gx, int gy, int variant) { QColor metal(150, 150, 160); QColor glass(100, 200, 255); QColor light = (variant % 2 == 0) ? QColor(255, 50, 50) : QColor(50, 255, 50); plot(p, gx, gy-2, glass); plot(p, gx-1, gy-2, glass); plot(p, gx+1, gy-2, glass); plot(p, gx, gy-3, glass); for(int x=-4; x<=4; x++) plot(p, gx+x, gy-1, metal); for(int x=-2; x<=2; x++) plot(p, gx+x, gy, metal); plot(p, gx-3, gy-1, light); plot(p, gx+3, gy-1, light); plot(p, gx, gy, light); }
void PropSystem::drawRover(QPainter& p, int gx, int gy, int worldGX, int variant, bool flipped, const QHash<int,int>& heightMap) { int d = flipped ? -1 : 1; QColor wheelC(30, 30, 35); QColor chassisC(220, 220, 220); QColor detailC(50, 50, 60); QColor lensC(20, 30, 80); QColor gold(200, 170, 50); QColor strutC(40, 40, 50); int centerGroundWorldY = heightMap.value(worldGX, 0); int camYOffset = gy - centerGroundWorldY; int peakScreenY = 999999; for(int dx = -6; dx <= 6; dx++) { int wgx = worldGX + dx; if(heightMap.contains(wgx)) { int sGY = heightMap.value(wgx) + camYOffset; if(sGY < peakScreenY) peakScreenY = sGY; } } if(peakScreenY == 999999) peakScreenY = gy; int chassisBaseY = peakScreenY - 2; auto drawAdaptiveWheel = [&](int offsetX) { int wheelWorldGX = worldGX + offsetX; int wheelScreenX = gx + offsetX; int groundY = peakScreenY + 5; if (heightMap.contains(wheelWorldGX)) { groundY = heightMap.value(wheelWorldGX) + camYOffset; } int wheelY = groundY; for(int y = chassisBaseY; y < wheelY; y++) { plot(p, wheelScreenX, y, strutC); plot(p, wheelScreenX + 1, y, strutC); } plot(p, wheelScreenX, wheelY, wheelC); plot(p, wheelScreenX+1, wheelY, wheelC); plot(p, wheelScreenX, wheelY-1, wheelC); plot(p, wheelScreenX+1, wheelY-1, wheelC); }; drawAdaptiveWheel(-5 * d); drawAdaptiveWheel(-1 * d); drawAdaptiveWheel(5 * d); int bodyY = chassisBaseY - 1; plot(p, gx-(5*d), bodyY, detailC); plot(p, gx-(1*d), bodyY, detailC); plot(p, gx+(5*d), bodyY, detailC); for(int x=-6; x<=6; x++) { plot(p, gx+(x*d), bodyY-1, chassisC); plot(p, gx+(x*d), bodyY-2, chassisC); } plot(p, gx-(5*d), bodyY-3, detailC); plot(p, gx-(6*d), bodyY-3, detailC); plot(p, gx-(5*d), bodyY-4, detailC); int mastX = gx + (4*d); plot(p, mastX, bodyY-3, detailC); plot(p, mastX, bodyY-4, detailC); plot(p, mastX, bodyY-5, detailC); plot(p, mastX+(1*d), bodyY-6, chassisC); plot(p, mastX+(1*d), bodyY-6, lensC); int dishX = gx - (1*d); plot(p, dishX, bodyY-3, detailC); plot(p, dishX-1, bodyY-4, gold); plot(p, dishX, bodyY-4, gold); plot(p, dishX+1, bodyY-4, gold); plot(p, dishX-2, bodyY-5, gold); plot(p, dishX+2, bodyY-5, gold); }
void PropSystem::drawAlien(QPainter& p, int gx, int gy, int variant) { QColor skin(50, 220, 80); QColor dark(30, 150, 50); QColor eyeWhite(255, 255, 255); QColor eyeBlack(0, 0, 0); for(int y=0; y<6; y++) { plot(p, gx, gy-y, skin); plot(p, gx-1, gy-y, skin); plot(p, gx+1, gy-y, skin); } plot(p, gx-2, gy, dark); plot(p, gx+2, gy, dark); if (variant % 2 == 0) { plot(p, gx-2, gy-3, skin); plot(p, gx-3, gy-4, skin); plot(p, gx+2, gy-3, skin); } else { plot(p, gx+2, gy-3, skin); plot(p, gx+3, gy-4, skin); plot(p, gx-2, gy-3, skin); } for(int y=6; y<10; y++) { for(int x=-2; x<=2; x++) plot(p, gx+x, gy-y, skin); } plot(p, gx, gy-10, dark); plot(p, gx, gy-11, dark); plot(p, gx, gy-12, skin); plot(p, gx-1, gy-7, eyeBlack); plot(p, gx-1, gy-8, eyeBlack); plot(p, gx+1, gy-7, eyeBlack); plot(p, gx+1, gy-8, eyeWhite); }
