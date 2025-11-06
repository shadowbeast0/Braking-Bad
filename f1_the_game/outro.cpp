#include "outro.h"
#include "intro.h"
#include "constants.h"
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QMouseEvent>
#include <QMap>
#include <algorithm>

namespace {

constexpr int CHAR_ADV = 7;

inline int textHeightCells(int scale){ return 7*scale; }

inline void plotGridPixel(QPainter& p, int cell, int gx, int gy, const QColor& c) {
    p.fillRect(gx*cell, gy*cell, cell, cell, c);
}

inline int textWidthCells(const QString& s, int scale){
    return s.isEmpty()?0:((int)s.size()-1)*CHAR_ADV*scale + 5*scale;
}

inline int fitTextScale(int wCells, int hCells, const QString& s){
    int wcap = std::max(1, (wCells-4)/std::max(1,textWidthCells(s,1)));
    int hcap = std::max(1, (hCells-2)/7);
    return std::clamp(std::min(wcap,hcap),1,18);
}

void drawPixelText(QPainter& p, const QString& s, int cell, int gx, int gy, int scale, const QColor& c, bool bold){
    auto plot=[&](int x,int y,const QColor& col){ plotGridPixel(p,cell,gx+x,gy+y,col); };
    for(int i=0;i<s.size();++i){
        const QChar ch=s.at(i).toUpper();
        const auto rows = ::font_map.contains(ch) ? ::font_map.value(ch) : ::font_map.value(QChar(' '));
        int base=i*CHAR_ADV*scale;
        for(int ry=0;ry<7;++ry){
            uint8_t row=rows[ry];
            for(int rx=0;rx<5;++rx){
                if(row&(1<<(4-rx))){
                    for(int sy=0;sy<scale;++sy){
                        for(int sx=0;sx<scale;++sx){
                            if(bold){
                                plot(base+rx*scale+sx-1,ry*scale+sy,QColor(20,20,22));
                                plot(base+rx*scale+sx+1,ry*scale+sy,QColor(20,20,22));
                                plot(base+rx*scale+sx,ry*scale+sy-1,QColor(20,20,22));
                                plot(base+rx*scale+sx,ry*scale+sy+1,QColor(20,20,22));
                            }
                            plot(base+rx*scale+sx,ry*scale+sy,c);
                        }
                    }
                }
            }
        }
    }
}
}

OutroScreen::OutroScreen(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    QPalette pal=palette();
    pal.setColor(QPalette::Window, QColor(15,12,20,230));
    setPalette(pal);

    m_exitBtn=new QPushButton("Exit",this);
    m_exitBtn->setCursor(Qt::PointingHandCursor);
    connect(m_exitBtn,&QPushButton::clicked,this,[this]{ emit exitRequested(); });
    m_exitBtn->hide();

    if(parent){
        int w = int(parent->width()  * 0.60);
        int h = int(parent->height() * 0.50);
        resize(w, h);
        centerInParent();
    }
    m_cell=std::max(4,std::min(width(),height())/90);
    setFocusPolicy(Qt::StrongFocus);
    setVisible(true);
    raise();
}

void OutroScreen::centerInParent(){
    if(!parentWidget()) return;
    move(parentWidget()->width()/2 - width()/2, parentWidget()->height()/2 - height()/2);
}

void OutroScreen::setStats(int coinCount, int nitroCount, int score, double distanceMeters){
    m_coins  = coinCount;
    m_nitros = nitroCount;
    m_score  = score;
    m_distanceM = distanceMeters;
    update();
}

void OutroScreen::paintEvent(QPaintEvent*){
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing,false);
    p.setRenderHint(QPainter::TextAntialiasing,false);

    const int cell = m_cell;
    const int pad = cell;

    const int gw = (width()-2*pad)/cell;
    const int gh = (height()-2*pad)/cell;
    const int panelX = pad;
    const int panelY = pad;
    const QRect panel(panelX,panelY,gw*cell,gh*cell);

    p.fillRect(panel,QColor(28,24,36));
    p.setPen(QColor(65,60,80));
    p.drawRect(panel.adjusted(0,0,-1,-1));

    const int pgx=panelX/cell;
    const int pgy=panelY/cell;
    auto GX=[&](int c){ return pgx+c; };
    auto GY=[&](int c){ return pgy+c; };

    const QString title="GAME OVER";
    const int titleScale = fitTextScale(gw-10, 9, title);
    const int titleW = textWidthCells(title,titleScale);
    const int tGX = GX((gw-titleW)/2);
    const int tGY = GY(std::max(2, gh/10));
    drawPixelText(p,title,cell,tGX,tGY,titleScale,QColor(230,230,240),false);

    const int titleH = textHeightCells(titleScale);
    const int topGap = std::max(12, gh/18);
    const int rowGap = std::max(10, gh/24);

    const int contentTop = tGY + titleH + topGap;

    int iconR = std::clamp(gw/50, 2, 5);
    int gapL  = std::max(8, gw/40);
    int leftGX = GX(std::max(6, gw/12));

    QString scoreStr = QString("SCORE: %1").arg(m_score);
    QString distStr  = QString("DIST: %1 m").arg(QString::number(m_distanceM,'f',1));
    QString flipsStr = QString("FLIPS: x%1").arg(m_flips);

    int numScale    = std::max(1, titleScale/2);
    int rightScale  = std::max(1, titleScale/2);

    auto leftEndGX = [&](const QString& s, int baseGX){
        return baseGX + gapL + textWidthCells(s, numScale);
    };
    auto rightStartGX = [&](const QString& s){
        int rightPadCells = std::clamp(gw/10, 12, 30);
        int w = textWidthCells(s, rightScale);
        return GX(gw - rightPadCells - w);
    };

    const int rightRowH = textHeightCells(rightScale);
    const int row1Y = contentTop;
    const int row2Y = row1Y + rightRowH + rowGap;
    const int row3Y = row2Y + rightRowH + rowGap;

    int iter=12;
    while (iter--){
        int sGX = rightStartGX(scoreStr);
        int dGX = rightStartGX(distStr);
        int fGX = rightStartGX(flipsStr);
        int l1End = leftEndGX(QString("x%1").arg(m_coins), leftGX + iconR);
        int l2End = leftEndGX(QString("x%1").arg(m_nitros), leftGX + iconR);
        bool overlap = (l1End + 2 >= sGX) || (l2End + 2 >= dGX) || (l2End + 2 >= fGX);
        if (!overlap) break;
        if (rightScale > 1) --rightScale;
        if (numScale   > 1) --numScale;
    }

    drawPixelCoin(p,leftGX,row1Y+3,iconR);
    drawPixelText(p,QString("x%1").arg(m_coins),cell,leftGX+iconR+std::max(8, gw/40),row1Y,numScale,QColor(230,230,240),false);

    drawPixelFlame(p,leftGX,row2Y+3,iconR*2);
    drawPixelText(p,QString("x%1").arg(m_nitros),cell,leftGX+iconR+std::max(8, gw/40),row2Y,numScale,QColor(230,230,240),false);

    drawPixelText(p, scoreStr, cell, rightStartGX(scoreStr), row1Y, rightScale, QColor(230,230,240), false);
    drawPixelText(p, distStr,  cell, rightStartGX(distStr),  row2Y, rightScale, QColor(230,230,240), false);
    drawPixelText(p, flipsStr, cell, rightStartGX(flipsStr), row3Y, rightScale, QColor(230,230,240), false);

    const int gwBtn = gw;
    int btnWCells = std::min(std::max(int(gwBtn/6.5), 24), gwBtn - 12);
    int btnHCells = std::max(9, gh/10);
    int bGX = GX((gwBtn - btnWCells)/2);
    int bGY = GY(gh - btnHCells - std::max(3, gh/40));
    QRect rExit(bGX*cell, bGY*cell, btnWCells*cell, btnHCells*cell);
    m_btnExitRect = rExit;

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255,0,0));
    p.drawRect(rExit);

    const QString sExit="EXIT";
    const int innerW=btnWCells-6;
    const int innerH=btnHCells-4;
    const int bsW=std::max(1, innerW/std::max(1,textWidthCells(sExit,1)));
    const int bsH=std::max(1, innerH/7);
    const int bs=std::clamp(std::min(bsW,bsH),1,12);
    const int labelW=textWidthCells(sExit,bs);
    const int txGX=bGX+(btnWCells-labelW)/2;
    const int tyGY=bGY+(btnHCells-7*bs)/2;
    drawPixelText(p,sExit,cell,txGX,tyGY,bs,QColor(255,255,255),false);
}

void OutroScreen::resizeEvent(QResizeEvent*){
    m_cell=std::max(4,std::min(width(),height())/90);
    centerInParent();
    update();
}

void OutroScreen::mousePressEvent(QMouseEvent* e){
    if(m_btnExitRect.contains(e->pos())){ emit exitRequested(); return; }
    QWidget::mousePressEvent(e);
}

void OutroScreen::drawPixelCoin(QPainter& p, int gx, int gy, int rCells){
    auto plot=[&](int x,int y,const QColor& c){
        p.fillRect(x*m_cell,y*m_cell,m_cell,m_cell,c);
    };

    int x0=gx,y0=gy,r=rCells,x=0,y=r,d=1-r;

    auto span=[&](int cy,int xl,int xr,const QColor& c){
        for(int xx=xl;xx<=xr;++xx)
            plot(xx,cy,c);
    };

    QColor c1(195,140,40),c2(250,204,77),hl(255,255,220);
    while(y>=x){
        span(y0+y,x0-x,x0+x,c1);
        span(y0-y,x0-x,x0+x,c1);
        span(y0+x,x0-y,x0+y,c1);
        span(y0-x,x0-y,x0+y,c1);
        ++x;
        if(d<0) d+=2*x+1;
        else { --y; d+=2*(x-y)+1; }
    }

    for(int yy=-r+1; yy<=r-1; ++yy){
        int rad=int(std::floor(std::sqrt(double(r*r-yy*yy))));
        span(y0+yy,x0-rad+1,x0+rad-1,c2);
    }

    plot(x0-1,y0-r+1,hl);
}

void OutroScreen::drawPixelFlame(QPainter& p, int gx, int gy, int lenCells){
    auto plot=[&](int x,int y,const QColor& c){
        p.fillRect(x*m_cell,y*m_cell,m_cell,m_cell,c);
    };
    QColor cOuter(255,100,35),cMid(255,160,45),cCore(255,240,120);
    int x0=gx,y0=gy;

    for(int i=0;i<lenCells;++i){
        int cx=x0-i, cy=y0;

        int w=std::max(0,2-i/2);
        for(int j=-w;j<=w;++j) plot(cx,cy+j,cOuter);

        int w2=std::max(0,w-1);
        for(int j=-w2;j<=w2;++j) plot(cx,cy+j,cMid);

        if(w2>=0) plot(cx,cy,cCore);
    }
}

void OutroScreen::setFlips(int flips) {
    m_flips = std::max(0, flips);
    update();
}
