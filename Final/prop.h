// prop.h
#ifndef PROP_H
#define PROP_H

#include <QVector>
#include <QColor>
#include <random>
#include <QPainter>
#include <QHash>
#include "constants.h"

enum class PropType {
    Tree, Rock, Flower, Mushroom,
    Cactus, Tumbleweed, Camel,
    Igloo, Penguin, Snowman, IceSpike,
    UFO,
    Rover, Alien,
    // Nightlife Props
    Building, StreetLamp
};

struct Prop {
    int wx;
    int wy;
    PropType type;
    int variant;
    bool flipped;
};

class PropSystem {
public:
    PropSystem();

    void maybeSpawnProp(int worldX, int groundGy, int levelIndex, float slope, std::mt19937& rng);

    void draw(QPainter& p, int camX, int camY, int screenW, int screenH, const QHash<int,int>& heightMap);

    void prune(int minWorldX);
    void clear();

private:
    QVector<Prop> m_props;

    void plot(QPainter& p, int gx, int gy, const QColor& c);

    // Existing props
    void drawTree(QPainter& p, int gx, int gy, int worldGX, int wx, int wy, int variant, const QHash<int,int>& heightMap);
    void drawRock(QPainter& p, int gx, int gy, int variant);
    void drawFlower(QPainter& p, int gx, int gy, int variant);
    void drawMushroom(QPainter& p, int gx, int gy, int variant);
    void drawCactus(QPainter& p, int gx, int gy, int variant);
    void drawTumbleweed(QPainter& p, int gx, int gy, int variant);
    void drawCamel(QPainter& p, int gx, int gy, int variant, bool flipped);
    void drawIgloo(QPainter& p, int gx, int gy, int worldGX, int variant, const QHash<int,int>& heightMap);
    void drawPenguin(QPainter& p, int gx, int gy, int variant, bool flipped);
    void drawSnowman(QPainter& p, int gx, int gy, int variant);
    void drawIceSpike(QPainter& p, int gx, int gy, int variant);
    void drawUFO(QPainter& p, int gx, int gy, int variant);
    void drawRover(QPainter& p, int gx, int gy, int worldGX, int variant, bool flipped, const QHash<int,int>& heightMap);
    void drawAlien(QPainter& p, int gx, int gy, int variant);

    // Nightlife Drawing Functions
    void drawBuilding(QPainter& p, int gx, int gy, int worldGX, int variant, const QHash<int,int>& heightMap);
    void drawStreetLamp(QPainter& p, int gx, int gy, int worldGX, int variant, const QHash<int,int>& heightMap);
};

#endif // PROP_H
