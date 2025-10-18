#include <GL/freeglut.h>
#include <cmath>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

#ifdef HAVE_SDL_MIXER
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#endif

using namespace std;

// ---------------- CONFIG ----------------
const int WINDOW_W = 1150;
const int WINDOW_H = 720;
const float PLAY_AREA_TOP = 560.0f;
const float BASKET_Y = 100.0f;
const float INITIAL_TIME_SEC = 60.0f; // changed to 60s

const float EGG_BASE_SPEED = 150.0f;
float BASE_SPAWN_INTERVAL = 0.72f;

const float SLOW_DURATION = 6.0f;
const float BIG_DURATION = 7.5f;
const float SLOW_FACTOR = 0.45f;
const float BIG_FACTOR = 1.6f;

const char* HIGHSCORE_FILE = "highscores.txt";
const int MAX_HIGHSCORES = 12;

const char* MUSIC_FILE = "bg_music.ogg";
const char* SFX_CATCH = "sfx_catch.wav";
const char* SFX_POOP = "sfx_poop.wav";
const char* SFX_PERK = "sfx_perk.wav";

// ---------------- UTIL ----------------
static const float PI_F = 3.14159265358979323846f;
float randf(float a, float b) { return a + (b-a) * (rand() / (float)RAND_MAX); }
template<typename T> T clamp_val(T v, T lo, T hi) { return (v < lo ? lo : (v > hi ? hi : v)); }

void set2DProjection() {
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0, WINDOW_W, 0, WINDOW_H);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

void drawText(float x, float y, const string &s) {
    glRasterPos2f(x, y);
    for (char c : s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}
void drawTextBig(float x, float y, const string &s) {
    glRasterPos2f(x, y);
    for (char c : s) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
}
void drawTextSmall(float x, float y, const string &s) {
    glRasterPos2f(x, y);
    for (char c : s) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, c);
}

void drawCircle(float cx, float cy, float r, int segments=28) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i=0;i<=segments;i++){
        float a = (2.0f*PI_F*i)/segments;
        glVertex2f(cx + cosf(a)*r, cy + sinf(a)*r);
    }
    glEnd();
}
void drawEllipse(float cx, float cy, float rx, float ry, int segs=28) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i=0;i<=segs;i++){
        float a = (2.0f*PI_F*i)/segs;
        glVertex2f(cx + cosf(a)*rx, cy + sinf(a)*ry);
    }
    glEnd();
}
void roundedRect(float x1, float y1, float x2, float y2, float r) {
    // central rect
    glBegin(GL_QUADS);
    glVertex2f(x1+r, y1); glVertex2f(x2-r, y1); glVertex2f(x2-r, y2); glVertex2f(x1+r, y2);
    glEnd();
    // left and right rects to fill
    glBegin(GL_QUADS);
    glVertex2f(x1, y1+r); glVertex2f(x1+r, y1+r); glVertex2f(x1+r, y2-r); glVertex2f(x1, y2-r);
    glVertex2f(x2-r, y1+r); glVertex2f(x2, y1+r); glVertex2f(x2, y2-r); glVertex2f(x2-r, y2-r);
    glEnd();
    // corners
    drawEllipse(x1+r, y1+r, r, r);
    drawEllipse(x2-r, y1+r, r, r);
    drawEllipse(x2-r, y2-r, r, r);
    drawEllipse(x1+r, y2-r, r, r);
}

// ---------------- AUDIO (optional) ----------------
#ifdef HAVE_SDL_MIXER
static Mix_Music* bgMusic = nullptr;
static Mix_Chunk* sfxCatch = nullptr;
static Mix_Chunk* sfxPoop = nullptr;
static Mix_Chunk* sfxPerk = nullptr;
bool initAudio() {
    if (SDL_Init(SDL_INIT_AUDIO) != 0) { cerr << "SDL_Init audio failed: " << SDL_GetError() << '\n'; return false; }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) { cerr << "Mix_OpenAudio failed: " << Mix_GetError() << '\n'; return false; }
    bgMusic = Mix_LoadMUS(MUSIC_FILE);
    sfxCatch = Mix_LoadWAV(SFX_CATCH);
    sfxPoop = Mix_LoadWAV(SFX_POOP);
    sfxPerk = Mix_LoadWAV(SFX_PERK);
    if (!bgMusic) cerr << "Warning: bg music load failed: " << Mix_GetError() << '\n';
    if (!sfxCatch) cerr << "Warning: sfx catch load failed: " << Mix_GetError() << '\n';
    if (!sfxPoop) cerr << "Warning: sfx poop load failed: " << Mix_GetError() << '\n';
    if (!sfxPerk) cerr << "Warning: sfx perk load failed: " << Mix_GetError() << '\n';
    return true;
}
void playMusic() { if (bgMusic) Mix_PlayMusic(bgMusic, -1); }
void playSfxCatch() { if (sfxCatch) Mix_PlayChannel(-1, sfxCatch, 0); }
void playSfxPoop() { if (sfxPoop) Mix_PlayChannel(-1, sfxPoop, 0); }
void playSfxPerk() { if (sfxPerk) Mix_PlayChannel(-1, sfxPerk, 0); }
void closeAudio() { if (bgMusic) Mix_FreeMusic(bgMusic); if (sfxCatch) Mix_FreeChunk(sfxCatch); if (sfxPoop) Mix_FreeChunk(sfxPoop); if (sfxPerk) Mix_FreeChunk(sfxPerk); Mix_CloseAudio(); SDL_Quit(); }
#else
bool initAudio(){ return false; }
void playMusic() {}
void playSfxCatch() {}
void playSfxPoop() {}
void playSfxPerk() {}
void closeAudio() {}
#endif

// ---------------- GAME OBJECTS ----------------
enum EggType { EGG_NORMAL, EGG_BLUE, EGG_GOLD, EGG_POOP, EGG_PERK_TIME, EGG_PERK_SLOW, EGG_PERK_BIG };

struct Egg {
    float x,y;
    float vx, vy;
    EggType type;
    float rx, ry;
    bool active;
    Egg(float X, float Y, float VX, float VY, EggType T) : x(X), y(Y), vx(VX), vy(VY), type(T), active(true) {
        if (type==EGG_GOLD) { rx = 12; ry = 17; }
        else if (type==EGG_BLUE) { rx = 11; ry = 15; }
        else if (type==EGG_POOP) { rx = 14; ry = 7; }
        else if (type==EGG_PERK_TIME || type==EGG_PERK_SLOW || type==EGG_PERK_BIG) { rx=12; ry=12; }
        else { rx = 10; ry = 14; }
    }
    void update(float dt, float globalFactor) {
        x += vx * dt;
        y -= vy * dt * globalFactor;
        if (x < 10) { x = 10; vx = fabs(vx); }
        if (x > WINDOW_W-10) { x = WINDOW_W-10; vx = -fabs(vx); }
        if (y < -60) active = false;
    }
    void drawShape() const {
        if (type == EGG_POOP) {
            glColor3f(0.25f,0.18f,0.08f);
            drawCircle(x, y, rx*0.9f, 20);
            drawCircle(x - 8, y + 6, rx*0.45f, 18);
            drawCircle(x + 8, y + 6, rx*0.5f, 18);
            return;
        }
        switch(type) {
            case EGG_NORMAL: glColor3f(1.0f,1.0f,1.0f); break;
            case EGG_BLUE: glColor3f(0.25f,0.45f,0.95f); break;
            case EGG_GOLD: glColor3f(1.0f,0.85f,0.15f); break;
            case EGG_PERK_TIME: glColor3f(0.6f,1.0f,0.6f); break;
            case EGG_PERK_SLOW: glColor3f(0.6f,1.0f,1.0f); break;
            case EGG_PERK_BIG: glColor3f(1.0f,0.75f,0.9f); break;
            default: glColor3f(1,1,1); break;
        }
        int layers = 6;
        for (int i=0;i<layers;i++){
            float t = i / (float)layers;
            float layerY = y - ry*(t*0.7f);
            float layerRx = rx * (1.0f - t*0.28f);
            float layerRy = ry * (1.0f - t*0.12f);
            drawEllipse(x, layerY, layerRx, layerRy);
        }
        glColor3f(1,1,1);
        drawEllipse(x - rx*0.25f, y + ry*0.35f, rx*0.2f, ry*0.12f);
    }
};

struct Basket {
    float x; float width; float height;
    Basket(): x(WINDOW_W*0.5f), width(160), height(28) {}
    void draw() const {
        // wicker base
        float left = x - width*0.5f;
        float right = x + width*0.5f;
        float top = BASKET_Y + height;
        float bot = BASKET_Y;
        // outer shell
        glColor3f(0.58f,0.36f,0.18f);
        roundedRect(left, bot, right, top, 10.0f);
        // inner shading
        glColor3f(0.8f,0.66f,0.45f);
        glBegin(GL_QUADS);
        glVertex2f(left+6, bot+6); glVertex2f(right-6, bot+6); glVertex2f(right-6, top-6); glVertex2f(left+6, top-6);
        glEnd();
        // woven lines (horizontal)
        glColor3f(0.48f,0.32f,0.16f);
        int rows = 6;
        for (int i=0;i<=rows;i++){
            float yy = bot + 6 + i*( (top-6) - (bot+6) ) / (float)rows;
            glBegin(GL_LINE_STRIP);
            for (float px = left+8; px <= right-8; px += 6.0f){
                float offset = sinf(px*0.08f + i*0.6f) * 1.8f;
                glVertex2f(px, yy + offset);
            }
            glEnd();
        }
        // woven vertical short lines
        for (float px = left+12; px < right-12; px += 12.0f){
            glBegin(GL_LINES);
            glVertex2f(px, bot+8); glVertex2f(px, top-8);
            glEnd();
        }
        // handle (arched)
        glLineWidth(5.0f);
        glColor3f(0.42f,0.25f,0.10f);
        glBegin(GL_LINE_STRIP);
        float hx = x; float hy = top + 6;
        for (int i=0;i<=40;i++){
            float a = PI_F * (float)i / 40.0f; // 0..pi
            float rx = width*0.55f;
            float ry = 28.0f;
            glVertex2f(hx + cosf(a - PI_F)*rx*0.9f, hy + sinf(a - PI_F)*ry);
        }
        glEnd();
        glLineWidth(1.0f);
    }
};

struct Cloud { float x,y,scale,speed; };
struct Bird { float x,y,vx; float wingPhase; float size; int dir; };
struct Tree { float x,y,scale; };
struct Chicken { float x; float vx; float phase; };

// ---------------- HIGHSCORES ----------------
struct ScoreEntry { string name; int score; };
vector<ScoreEntry> readHighScores() {
    vector<ScoreEntry> v;
    ifstream in(HIGHSCORE_FILE);
    if (!in.is_open()) return v;
    string name; int s;
    while (in >> name >> s) v.push_back({name,s});
    in.close();
    sort(v.begin(), v.end(), [](const ScoreEntry &a, const ScoreEntry &b){ return a.score > b.score; });
    if (v.size() > MAX_HIGHSCORES) v.resize(MAX_HIGHSCORES);
    return v;
}
void writeHighScores(const vector<ScoreEntry> &v) {
    ofstream out(HIGHSCORE_FILE);
    for (size_t i=0;i<v.size() && i<(size_t)MAX_HIGHSCORES;i++) out << v[i].name << ' ' << v[i].score << '\n';
}

// ---------------- STATE ----------------
enum GameState { STATE_MENU, STATE_PLAYING, STATE_PAUSED, STATE_GAMEOVER, STATE_HIGHSCORE, STATE_HELP, STATE_ENTER_NAME };
GameState gState = STATE_MENU;

vector<Egg> eggs; Basket basket;
vector<Cloud> clouds; vector<Bird> birds; vector<Tree> trees; vector<Chicken> chickens;

float windX = 0.0f; float windChangeTimer = 0.0f;
float spawnTimer = 0.0f; float spawnInterval = BASE_SPAWN_INTERVAL;

int scoreVal = 0; float remainingTime = INITIAL_TIME_SEC;
float slowTimer = 0.0f; float bigTimer = 0.0f; float globalSpeedFactor = 1.0f;
bool moveLeft=false, moveRight=false;
int lastTimeMs = 0; int menuIndex = 0;
string nameInputBuffer; float dayTime = 9.0f; bool isDay() { return dayTime >= 6.0f && dayTime < 18.0f; }

// egg counters
int cntNormal=0, cntBlue=0, cntGold=0, cntPoop=0, cntPerkTime=0, cntPerkSlow=0, cntPerkBig=0;

// Buttons
struct Rect { float x1,y1,x2,y2; bool contains(float x,float y) const { return x>=x1 && x<=x2 && y>=y1 && y<=y2; } };
Rect btnStart, btnHigh, btnHelp, btnExit, btnPause, btnExitInGame, btnRestart, btnResume, btnBackHS, btnBackHelp;

// ---------------- GAME LOGIC ----------------
void resetGame() {
    eggs.clear(); basket = Basket(); spawnTimer = 0.0f; spawnInterval = BASE_SPAWN_INTERVAL;
    scoreVal = 0; remainingTime = INITIAL_TIME_SEC; slowTimer = bigTimer = 0.0f; globalSpeedFactor = 1.0f;
    moveLeft = moveRight = false; nameInputBuffer.clear(); windX = randf(-40, 40); windChangeTimer = randf(3.0f, 7.0f);
    chickens.clear();
    cntNormal = cntBlue = cntGold = cntPoop = cntPerkTime = cntPerkSlow = cntPerkBig = 0;
    int nc = 3; float start = 150.0f; float gap = (WINDOW_W - 300.0f)/ (nc-1);
    for (int i=0;i<nc;i++) { Chicken c; c.x = start + i*gap + randf(-20,20); c.vx = randf(30,80) * (randf(0,1)>0.5f?1:-1); c.phase=randf(0,PI_F*2); chickens.push_back(c); }
}

void spawnEgg() {
    float p = randf(0,1); EggType t = EGG_NORMAL;
    if (p < 0.70f) t = EGG_NORMAL; else if (p < 0.88f) t = EGG_BLUE; else if (p < 0.95f) t = EGG_GOLD; else t = EGG_POOP;
    if (randf(0,1) < 0.04f) { float s = randf(0,1); if (s < 0.33f) t = EGG_PERK_TIME; else if (s < 0.66f) t = EGG_PERK_SLOW; else t = EGG_PERK_BIG; }
    int ci = rand() % max(1, (int)chickens.size()); float spiderX = chickens[ci].x + randf(-14,14);
    float spawnX = clamp_val(spiderX, 30.0f, (float)WINDOW_W-30.0f); float startY = PLAY_AREA_TOP + 18.0f;
    float speed = EGG_BASE_SPEED * randf(0.88f,1.14f); float vx = windX * randf(0.5f,1.0f) + randf(-25,25);
    eggs.emplace_back(spawnX, startY, vx, speed, t);
}

void applyPerk(EggType t) {
    if (t==EGG_PERK_TIME) remainingTime += 8.0f;
    if (t==EGG_PERK_SLOW) slowTimer = SLOW_DURATION;
    if (t==EGG_PERK_BIG) bigTimer = BIG_DURATION;
}
void playSfxCatchSafe() { playSfxCatch(); } void playSfxPoopSafe() { playSfxPoop(); } void playSfxPerkSafe() { playSfxPerk(); }

void countEggCaught(EggType t) {
    switch(t){
        case EGG_NORMAL: cntNormal++; break;
        case EGG_BLUE: cntBlue++; break;
        case EGG_GOLD: cntGold++; break;
        case EGG_POOP: cntPoop++; break;
        case EGG_PERK_TIME: cntPerkTime++; break;
        case EGG_PERK_SLOW: cntPerkSlow++; break;
        case EGG_PERK_BIG: cntPerkBig++; break;
    }
}

void catchEggAt(int idx) {
    if (idx<0 || idx >= (int)eggs.size()) return; Egg &e = eggs[idx]; if (!e.active) return;
    // increment counters for display & stats
    countEggCaught(e.type);

    if (e.type==EGG_NORMAL) { scoreVal += 1; playSfxCatchSafe(); } else if (e.type==EGG_BLUE) { scoreVal += 5; playSfxCatchSafe(); }
    else if (e.type==EGG_GOLD) { scoreVal += 10; playSfxCatchSafe(); } else if (e.type==EGG_POOP) { scoreVal -= 10; if (scoreVal<0) scoreVal=0; playSfxPoopSafe(); }
    else { applyPerk(e.type); playSfxPerkSafe(); }
    e.active = false;
}

void updateGame(float dt) {
    if (gState != STATE_PLAYING) return;
    remainingTime -= dt; if (remainingTime <= 0.0f) { remainingTime = 0.0f; gState = STATE_GAMEOVER; }
    if (slowTimer > 0.0f) slowTimer -= dt; if (slowTimer < 0) slowTimer = 0;
    if (bigTimer > 0.0f) bigTimer -= dt; if (bigTimer < 0) bigTimer = 0;
    globalSpeedFactor = 1.0f; if (slowTimer > 0.0f) globalSpeedFactor *= SLOW_FACTOR;
    basket.width = 160.0f * (bigTimer>0.0f ? BIG_FACTOR : 1.0f);
    float mv = 420.0f * dt;
    if (moveLeft) basket.x -= mv; if (moveRight) basket.x += mv;
    basket.x = max(basket.width*0.5f, min((float)WINDOW_W - basket.width*0.5f, basket.x));
    for (auto &c : chickens) { c.x += c.vx * dt; c.phase += dt*4.0f; if (c.x < 120) { c.x = 120; c.vx = fabs(c.vx); } if (c.x > WINDOW_W-120) { c.x = WINDOW_W-120; c.vx = -fabs(c.vx); } }
    windChangeTimer -= dt; if (windChangeTimer <= 0.0f) { windX = randf(-90,90); windChangeTimer = randf(3.0f, 8.0f); }
    // airflow: influence existing eggs' vx gradually (visual wind gusts affect vx)
    float gustStrength = windX * 0.06f; // smaller per-frame gentle nudge
    for (auto &e : eggs) { e.vx += gustStrength * dt * 8.0f; }
    spawnTimer -= dt;
    if (spawnTimer <= 0) { spawnEgg(); spawnInterval = BASE_SPAWN_INTERVAL * randf(0.92f,1.08f); spawnTimer = spawnInterval; }
    for (auto &e : eggs) if (e.active) e.update(dt, globalSpeedFactor);
    for (int i=(int)eggs.size()-1;i>=0;i--) { Egg &e = eggs[i]; if (!e.active) continue;
        if (e.y - e.ry <= BASKET_Y + basket.height && e.y >= BASKET_Y - 6) {
            if (e.x >= basket.x - basket.width*0.5f && e.x <= basket.x + basket.width*0.5f) {
                if (e.type==EGG_POOP) { countEggCaught(e.type); scoreVal -= 10; if (scoreVal<0) scoreVal=0; playSfxPoopSafe(); e.active=false; }
                else if (e.type==EGG_NORMAL || e.type==EGG_BLUE || e.type==EGG_GOLD) catchEggAt(i);
                else { applyPerk(e.type); playSfxPerkSafe(); e.active=false; }
            }
        }
    }
    eggs.erase(remove_if(eggs.begin(), eggs.end(), [](const Egg &e){ return !e.active; }), eggs.end());

    // clouds and birds moved in timerFunc to keep consistent visuals
    dayTime += dt * 0.017f; if (dayTime >= 24.0f) dayTime -= 24.0f;
    BASE_SPAWN_INTERVAL = max(0.36f, BASE_SPAWN_INTERVAL - dt*0.0009f);
}

// ---------------- RENDER ----------------
void drawSky() {
    float r,g,b; if (isDay()) { r = 0.53f; g = 0.80f; b = 0.98f; } else { r = 0.05f; g = 0.08f; b = 0.2f; }
    glBegin(GL_QUADS); glColor3f(r,g,b); glVertex2f(0,0); glColor3f(r,g,b); glVertex2f(WINDOW_W,0);
    glColor3f(r*0.35f, g*0.45f, b*0.7f); glVertex2f(WINDOW_W, WINDOW_H); glColor3f(r*0.35f, g*0.45f, b*0.7f); glVertex2f(0, WINDOW_H);
    glEnd();
}
void drawSun() {
    float cx = WINDOW_W - 120; float cy = WINDOW_H - 120; float r = 48;
    for (int i=0;i<6;i++){ float alpha = isDay() ? 0.18f + 0.1f*(6-i)/6.0f : 0.06f; glColor4f(1.0f, 0.95f - i*0.02f, 0.2f + i*0.01f, alpha); drawCircle(cx, cy, r*(1.0f - i*0.12f)); }
}
void drawCloud(const Cloud &c) {
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // bigger soft layered cloud with subtle shading
    float baseX = c.x;
    float baseY = c.y;
    float s = c.scale;
    // shadow layer behind
    glColor4f(0.92f,0.92f,0.95f,0.45f);
    drawEllipse(baseX + 22*s, baseY-6*s, 120*s, 28*s);
    // mid blobs
    glColor4f(1,1,1,0.98f);
    drawCircle(baseX + 0*s, baseY + 0*s, 38.0f*s, 32);
    drawCircle(baseX + 48.0f*s, baseY + 12.0f*s, 42.0f*s, 32);
    drawCircle(baseX + 94.0f*s, baseY + 4.0f*s, 48.0f*s, 36);
    drawCircle(baseX + 150.0f*s, baseY - 8.0f*s, 32.0f*s, 28);
    // subtle bottom soft ellipse
    glColor4f(0.98f,0.98f,1.0f,0.55f);
    drawEllipse(baseX+68*s, baseY-10*s, 120*s, 20*s);
    glDisable(GL_BLEND);
}
void drawBird(const Bird &b) {
    // nicer bird: small rounded body, tail, beak, wings animated
    float wf = sinf(b.wingPhase) * 18.0f;
    float sx = b.size;
    // body
    glColor3f(0.12f,0.12f,0.12f);
    drawEllipse(b.x, b.y, 12.0f*sx, 8.0f*sx);
    // head
    drawCircle(b.x + (b.dir>0?8.0f*sx:-8.0f*sx), b.y + 3.0f*sx, 6.0f*sx, 18);
    // beak
    glColor3f(1.0f,0.7f,0.2f);
    if (b.dir>0) {
        glBegin(GL_TRIANGLES); glVertex2f(b.x + 12.0f*sx, b.y + 3.0f*sx); glVertex2f(b.x + 20.0f*sx, b.y + 6.0f*sx); glVertex2f(b.x + 20.0f*sx, b.y + 0.0f*sx); glEnd();
    } else {
        glBegin(GL_TRIANGLES); glVertex2f(b.x - 12.0f*sx, b.y + 3.0f*sx); glVertex2f(b.x - 20.0f*sx, b.y + 6.0f*sx); glVertex2f(b.x - 20.0f*sx, b.y + 0.0f*sx); glEnd();
    }
    // tail
    glColor3f(0.10f,0.10f,0.12f);
    if (b.dir>0) {
        glBegin(GL_TRIANGLES); glVertex2f(b.x - 10.0f*sx, b.y); glVertex2f(b.x - 20.0f*sx, b.y + 6.0f*sx); glVertex2f(b.x - 20.0f*sx, b.y - 6.0f*sx); glEnd();
    } else {
        glBegin(GL_TRIANGLES); glVertex2f(b.x + 10.0f*sx, b.y); glVertex2f(b.x + 20.0f*sx, b.y + 6.0f*sx); glVertex2f(b.x + 20.0f*sx, b.y - 6.0f*sx); glEnd();
    }
    // wing (flapping)
    glPushMatrix();
    glTranslatef(b.x, b.y, 0);
    glRotatef(b.dir>0 ? wf : -wf, 0,0,1);
    glColor3f(0.18f,0.18f,0.18f);
    drawEllipse(2.0f*sx, 0, 14.0f*sx, 6.0f*sx);
    glPopMatrix();
}
void drawTree(const Tree &t) {
    glColor3f(0.45f,0.25f,0.08f);
    glBegin(GL_QUADS); glVertex2f(t.x - 8*t.scale, t.y); glVertex2f(t.x + 8*t.scale, t.y); glVertex2f(t.x + 8*t.scale, t.y + 60*t.scale); glVertex2f(t.x - 8*t.scale, t.y + 60*t.scale); glEnd();
    glColor3f(0.10f,0.48f,0.12f);
    drawCircle(t.x, t.y + 95*t.scale, 46*t.scale);
    drawCircle(t.x - 36*t.scale, t.y + 74*t.scale, 36*t.scale);
    drawCircle(t.x + 36*t.scale, t.y + 74*t.scale, 36*t.scale);
}
void drawChickenOnGround(float cx, float cy, float phase, bool big=false) {
    glPushMatrix(); glTranslatef(cx,cy,0);
    float bodyRx = big ? 34.0f : 22.0f; float bodyRy = big ? 26.0f : 16.0f;
    // body base
    glColor3f(1.0f,0.95f,0.85f); drawEllipse(0, 12, bodyRx, bodyRy);
    // feather pattern - layered colored blobs for more realistic look
    glColor3f(1.0f,0.88f,0.7f); drawEllipse(-6, 10, bodyRx*0.55f, bodyRy*0.6f);
    glColor3f(0.95f,0.75f,0.45f); drawEllipse(6, 8, bodyRx*0.42f, bodyRy*0.5f);
    // head
    glColor3f(1.0f,0.95f,0.85f); drawCircle(bodyRx*0.6f, 26, big?14:10);
    // beak
    glColor3f(1.0f,0.6f,0.0f); glBegin(GL_TRIANGLES); glVertex2f(bodyRx*0.6f+14,26); glVertex2f(bodyRx*0.6f+24,24); glVertex2f(bodyRx*0.6f+14,22); glEnd();
    // comb
    glColor3f(0.9f,0.15f,0.15f); drawCircle(bodyRx*0.6f,34, big?6:4); drawCircle(bodyRx*0.6f+8,36,big?6:4);
    // eye
    glColor3f(0,0,0); drawCircle(bodyRx*0.6f+4,28, big?2.2f:1.5f);
    glPopMatrix();
}

void drawEggOnGround(float x, float y, EggType t) { Egg tmp(x,y,0,0,t); tmp.drawShape(); }

void drawStickAndChickens() {
    glColor3f(0.45f,0.27f,0.12f);
    glBegin(GL_QUADS);
    glVertex2f(60, PLAY_AREA_TOP + 6); glVertex2f(WINDOW_W-60, PLAY_AREA_TOP + 6); glVertex2f(WINDOW_W-60, PLAY_AREA_TOP - 6); glVertex2f(60, PLAY_AREA_TOP - 6);
    glEnd();
    for (auto &c : chickens) drawChickenOnGround(c.x, PLAY_AREA_TOP + 18, c.phase, true); // big perched chickens
}

void drawUI() {
    glColor3f(0.06f,0.06f,0.06f);
    ostringstream ss; ss << "Catch The Egg"; drawText(WINDOW_W*0.5f - 60, WINDOW_H - 32, ss.str());
    ss.str(""); ss.clear(); ss << "Score: " << scoreVal; drawText(20, WINDOW_H - 40, ss.str());
    ss.str(""); ss.clear(); ss << "Time: " << (int)ceil(remainingTime) << "s"; drawText(20, WINDOW_H - 68, ss.str());


}

// Menu buttons layout (made larger for clickability)
void layoutMenuButtons() {
    float cx = WINDOW_W*0.5f; float by = WINDOW_H*0.5f - 40; float bw = 360, bh = 56; // larger buttons
    btnStart = { cx - bw*0.5f, by + 110, cx + bw*0.5f, by + 110 + bh };
    btnHigh  = { cx - bw*0.5f, by + 30, cx + bw*0.5f, by + 30 + bh };
    btnHelp  = { cx - bw*0.5f, by - 50, cx + bw*0.5f, by - 50 + bh };
    btnExit  = { cx - bw*0.5f, by - 130, cx + bw*0.5f, by - 130 + bh };
    // in-game buttons (pause, restart, resume, exit)
    float smallW = 110, smallH = 40;
    btnPause = { WINDOW_W - 240, WINDOW_H - 64, WINDOW_W - 240 + smallW, WINDOW_H - 64 + smallH };
    btnRestart = { WINDOW_W - 360, WINDOW_H - 64, WINDOW_W - 360 + smallW, WINDOW_H - 64 + smallH };
    btnResume = { WINDOW_W - 120, WINDOW_H - 64, WINDOW_W - 120 + smallW, WINDOW_H - 64 + smallH };
    btnExitInGame = { WINDOW_W - 120, WINDOW_H - 64 - 48, WINDOW_W - 120 + smallW, WINDOW_H - 64 - 48 + smallH };
    // back buttons
    btnBackHS = { 40, WINDOW_H - 76, 40 + 120, WINDOW_H - 76 + 44 };
    btnBackHelp = { 40, WINDOW_H - 76, 40 + 120, WINDOW_H - 76 + 44 };
}

void drawButton(const Rect &r, const string &label, bool hover=false) {
    if (hover) glColor3f(0.98f,0.68f,0.25f); else glColor3f(0.92f,0.55f,0.16f);
    roundedRect(r.x1, r.y1, r.x2, r.y2, 10.0f);
    glColor3f(0,0,0);
    float tx = (r.x1 + r.x2)/2 - (label.size()*6.0f);
    float ty = (r.y1 + r.y2)/2 - 6;
    drawText(tx, ty, label);
}

// ---------------- RENDER SCENE ----------------
void renderScene();

// ---------------- INPUT ----------------
void saveHighscoreWithName(const string &name) {
    auto hs = readHighScores();
    ScoreEntry e; e.name = name; e.score = scoreVal;
    hs.push_back(e);
    sort(hs.begin(), hs.end(), [](const ScoreEntry &a, const ScoreEntry &b){ return a.score > b.score; });
    writeHighScores(hs);
}

void startGameFromMenu() { resetGame(); gState = STATE_PLAYING; playMusic(); }

void keyboardDown(unsigned char key, int x, int y) {
    if (gState == STATE_MENU) {
        if (key == 13) startGameFromMenu();
        if (key=='w' || key=='W') menuIndex = max(0, menuIndex-1);
        if (key=='s' || key=='S') menuIndex = min(3, menuIndex+1);
    }
    else if (gState == STATE_PLAYING) {
        if (key=='a' || key=='A') moveLeft=true;
        if (key=='d' || key=='D') moveRight=true;
        if (key=='p' || key=='P') { gState = STATE_PAUSED; }
        if (key==27) { gState = STATE_MENU; }
    }
    else if (gState == STATE_PAUSED) {
        if (key=='p' || key=='P') { gState = STATE_PLAYING; }
        if (key==27) { gState = STATE_MENU; }
    }
    else if (gState == STATE_GAMEOVER) {
        if (key==13) { nameInputBuffer = ""; gState = STATE_ENTER_NAME; }
        if (key==27) { gState = STATE_MENU; }
    }
    else if (gState == STATE_ENTER_NAME) {
        if (key==13) { string nm = nameInputBuffer; nm.erase(remove_if(nm.begin(), nm.end(), [](char c){ return isspace((unsigned char)c); }), nm.end()); if (nm.empty()) nm = "Player"; saveHighscoreWithName(nm); gState = STATE_MENU; }
        else if (key==27) { gState = STATE_MENU; }
        else if (key==8 || key==127) { if (!nameInputBuffer.empty()) nameInputBuffer.pop_back(); }
        else { if (key >= 33 && key <= 126 && key != ' ') { if (nameInputBuffer.size() < 12) nameInputBuffer.push_back((char)key); } }
    }
    else if (gState == STATE_HIGHSCORE || gState == STATE_HELP) {
        if (key==27 || key==8) gState = STATE_MENU;
    }
}
void keyboardUp(unsigned char key, int x, int y) {
    if (key=='a' || key=='A') moveLeft=false;
    if (key=='d' || key=='D') moveRight=false;
}
void specialDown(int key, int x, int y) {
    if (gState==STATE_MENU) { if (key==GLUT_KEY_UP) menuIndex = max(0, menuIndex-1); if (key==GLUT_KEY_DOWN) menuIndex = min(3, menuIndex+1); }
    if (gState==STATE_PLAYING) { if (key==GLUT_KEY_LEFT) moveLeft=true; if (key==GLUT_KEY_RIGHT) moveRight=true; }
}
void specialUp(int key, int x, int y) { if (key==GLUT_KEY_LEFT) moveLeft=false; if (key==GLUT_KEY_RIGHT) moveRight=false; }

void passiveMouse(int mx, int my) {
    if (gState == STATE_PLAYING) {
        basket.x = (float)mx; if (basket.x < basket.width*0.5f) basket.x = basket.width*0.5f; if (basket.x > WINDOW_W - basket.width*0.5f) basket.x = WINDOW_W - basket.width*0.5f;
    }
}

void mouseClick(int button, int state, int mx, int my) {
    float wy = WINDOW_H - my; float wx = mx;
    layoutMenuButtons();
    if (gState == STATE_MENU && button==GLUT_LEFT_BUTTON && state==GLUT_DOWN) {
        if (btnStart.contains(wx,wy)) { startGameFromMenu(); return; }
        if (btnHigh.contains(wx,wy)) { gState = STATE_HIGHSCORE; return; }
        if (btnHelp.contains(wx,wy)) { gState = STATE_HELP; return; }
        if (btnExit.contains(wx,wy)) { exit(0); return; }
    }
    else if ((gState == STATE_PLAYING) && button==GLUT_LEFT_BUTTON && state==GLUT_DOWN) {
        if (btnPause.contains(wx,wy)) { gState = STATE_PAUSED; return; }
        if (btnExitInGame.contains(wx,wy)) { gState = STATE_MENU; return; }
    }
    else if ((gState == STATE_PAUSED) && button==GLUT_LEFT_BUTTON && state==GLUT_DOWN) {
        // in paused overlay we have Resume and Restart and Exit
        if (btnResume.contains(wx,wy)) { gState = STATE_PLAYING; return; }
        if (btnRestart.contains(wx,wy)) { resetGame(); gState = STATE_PLAYING; return; }
        if (btnExitInGame.contains(wx,wy)) { gState = STATE_MENU; return; }
    }
    else if (gState == STATE_HIGHSCORE && button==GLUT_LEFT_BUTTON && state==GLUT_DOWN) {
        if (btnBackHS.contains(wx,wy)) { gState = STATE_MENU; return; }
        // also clicking anywhere else returns to menu (compat)
        if ( !btnBackHS.contains(wx,wy) ) { /* nothing */ }
    }
    else if (gState == STATE_HELP && button==GLUT_LEFT_BUTTON && state==GLUT_DOWN) {
        if (btnBackHelp.contains(wx,wy)) { gState = STATE_MENU; return; }
    }
}

// ---------------- TIMER ----------------
void timerFunc(int val);
void timerFunc(int val) {
    int now = glutGet(GLUT_ELAPSED_TIME); float dt = (now - lastTimeMs) / 1000.0f; if (dt > 0.09f) dt = 0.09f; lastTimeMs = now;
    if (gState == STATE_PLAYING) updateGame(dt);

    // move clouds (gentle) and wrap
    for (auto &c : clouds) { c.x += c.speed * dt * 0.6f; if (c.x - 260 > WINDOW_W) c.x = -360 - randf(0,120); if (c.x + 260 < -200) c.x = WINDOW_W + randf(0,120); }

    // move birds; they have direction in struct: set vx accordingly
    for (auto &b : birds) {
        b.x += b.vx * dt;
        b.wingPhase += dt * 8.0f;
        // wrap depending on direction
        if (b.vx > 0) {
            if (b.x > WINDOW_W + 120) {
                b.x = -120 - randf(0,60);
                b.y = randf(WINDOW_H*0.55f, WINDOW_H*0.9f);
                b.vx = randf(80.0f, 140.0f);
                b.dir = 1;
            }
        } else {
            if (b.x < -160) {
                b.x = WINDOW_W + randf(0,60);
                b.y = randf(WINDOW_H*0.55f, WINDOW_H*0.9f);
                b.vx = -randf(80.0f, 140.0f);
                b.dir = -1;
            }
        }
    }

    glutPostRedisplay(); glutTimerFunc(16, timerFunc, 0);
}

// ---------------- SETUP ----------------
void setupScene() {
    clouds.clear(); birds.clear(); trees.clear();
    for (int i=0;i<7;i++) clouds.push_back({ randf(-400, WINDOW_W+200), randf(WINDOW_H*0.55f, WINDOW_H*0.85f), randf(0.9f,1.4f), randf(8.0f, 36.0f) });
    for (int i=0;i<6;i++) {
        float dir = randf(0,1) > 0.5f ? 1.0f : -1.0f;
        float vx = (dir>0 ? randf(80.0f,140.0f) : -randf(80.0f,140.0f));
        Bird b; b.x = randf(0, WINDOW_W); b.y = randf(WINDOW_H*0.6f, WINDOW_H*0.9f); b.vx = vx; b.wingPhase = randf(0,6.28f); b.size = randf(0.9f,1.3f); b.dir = vx>0?1:-1;
        birds.push_back(b);
    }
    for (int i=0;i<6;i++) trees.push_back({ 80 + i*170.0f + randf(-40,40), 12.0f, randf(0.95f,1.25f) });
    layoutMenuButtons();
}

// ---------------- RENDER SCENE ----------------
void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    set2DProjection();
    drawSky(); drawSun();
    for (auto &c : clouds) drawCloud(c);
    for (auto &b : birds) drawBird(b);
    for (auto &t : trees) drawTree(t);

    if (gState == STATE_MENU) {
        // draw ground
        glColor3f(0.47f,0.78f,0.36f); glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(WINDOW_W,0); glVertex2f(WINDOW_W,160); glVertex2f(0,160); glEnd();


        drawChickenOnGround(320, 180, 0.0f, true); drawChickenOnGround(520, 180, 0.0f, true); drawChickenOnGround(720, 180, 0.0f, true);
        // eggs on ground (one of each)
        drawEggOnGround(220,140,EGG_NORMAL); drawEggOnGround(280,140,EGG_BLUE); drawEggOnGround(340,140,EGG_GOLD); drawEggOnGround(400,140,EGG_POOP);
        // title: large black bold
        glColor3f(0,0,0); drawTextBig(WINDOW_W*0.5f - 160, WINDOW_H - 110, "Catch The Egg");
        // buttons
        layoutMenuButtons();
        drawButton(btnStart, "Start Game"); drawButton(btnHigh, "High Scores"); drawButton(btnHelp, "Help"); drawButton(btnExit, "Exit");
        drawTextSmall(20,20,"Click a button. In-game controls: A/D or Left/Right arrows; mouse to move; 'p' to pause.");
    }
    else if (gState == STATE_PLAYING || gState == STATE_PAUSED || gState == STATE_GAMEOVER) {
        // ground
        glColor3f(0.47f,0.78f,0.36f); glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(WINDOW_W,0); glVertex2f(WINDOW_W,160); glVertex2f(0,160); glEnd();

        // chicken coop on ground (positioned on ground correctly)
        float hx = 60, hy = 160;
        glColor3f(0.80f,0.45f,0.18f); roundedRect(hx, hy, hx+120, hy+80, 6);
        glColor3f(0.5f,0.12f,0.12f); glBegin(GL_TRIANGLES); glVertex2f(hx-10, hy+80); glVertex2f(hx+130, hy+80); glVertex2f(hx+60, hy+120); glEnd();
        // trees & stick & chickens
        for (auto &t : trees) drawTree(t);
        drawStickAndChickens();
        // eggs
        for (auto &e : eggs) e.drawShape();
        // basket
        basket.draw();
        // UI
        drawUI();
        // in-game buttons (Pause, Restart/Resume, Exit)
        layoutMenuButtons();
        // always show Pause and Exit small
        drawButton(btnPause, "Pause");
        drawButton(btnExitInGame, "Exit");
        if (gState == STATE_PAUSED) {
            // overlay
            glColor4f(0,0,0,0.35f); glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(WINDOW_W,0); glVertex2f(WINDOW_W,WINDOW_H); glVertex2f(0,WINDOW_H); glEnd();
            drawButton(btnResume, "Resume");
            drawButton(btnRestart, "Restart");
            drawButton(btnExitInGame, "Exit");
            glColor3f(1,1,1);
            drawTextBig(WINDOW_W*0.5f - 60, WINDOW_H*0.5f + 80, "PAUSED");
        }
        if (gState == STATE_GAMEOVER) {
            glColor3f(0,0,0); drawText(WINDOW_W*0.5f - 140, WINDOW_H*0.5f + 80, "GAME OVER");
            ostringstream s; s << "Your Score: " << scoreVal; drawText(WINDOW_W*0.5f - 60, WINDOW_H*0.5f + 50, s.str());
            drawTextSmall(WINDOW_W*0.5f - 160, WINDOW_H*0.5f + 10, "Press Enter to save score (type name) or Esc to return to menu without saving.");
        }
    }

    if (gState == STATE_HIGHSCORE) {
        glColor3f(0,0,0); drawText(WINDOW_W*0.5f - 60, WINDOW_H - 60, "HIGH SCORES");
        auto hs = readHighScores();
        for (int i=0;i<(int)hs.size();i++) { ostringstream s; s << i+1 << ". " << hs[i].name << " - " << hs[i].score; drawText(WINDOW_W*0.5f - 120, WINDOW_H - 110 - i*28, s.str()); }
        drawButton(btnBackHS, "Back");
        drawTextSmall(20,20, "Press Esc or Back to return to menu.");
    }

    if (gState == STATE_HELP) {
        // centered detailed help about eggs & controls
        glColor3f(0,0,0);
        string lines[] = {
            "HELP - Egg Types & Effects:",
            "Normal Egg  -> +1 point",
            "Blue Egg    -> +5 points",
            "Golden Egg  -> +10 points",
            "Poop        -> -10 points (avoid catching)",
            "Perks: Extra Time, Slow Motion, Bigger Basket",
            "",
            "Controls:",
            "- Move: Left / Right arrows or A / D (or move mouse)",
            "- Pause: 'p' key or Pause button",
            "- Exit to menu: Esc key or Exit button",
            "",
            "Gameplay tips: Catch golden and blue eggs; avoid poop; use perks to gain advantage."
        };
        float startY = WINDOW_H*0.5f + 140;
        for (int i=0;i<(int)(sizeof(lines)/sizeof(lines[0]));i++) {
            drawText(WINDOW_W*0.5f - 320, startY - i*26, lines[i]);
        }
        drawButton(btnBackHelp, "Back");
        drawTextSmall(20,20,"Click Back or press Esc to return to menu.");
    }

    if (gState == STATE_ENTER_NAME) {
        glColor3f(0,0,0); drawText(WINDOW_W*0.5f - 140, WINDOW_H*0.5f + 60, "ENTER NAME FOR HIGHSCORE (no spaces):");
        string s = nameInputBuffer; if (s.empty()) s = "Player";
        drawText(WINDOW_W*0.5f - 60, WINDOW_H*0.5f + 20, s);
        drawTextSmall(WINDOW_W*0.5f - 160, WINDOW_H*0.5f - 10, "Press Enter to confirm, Esc to cancel.");
    }

    glutSwapBuffers();
}

// ---------------- MISC (grass util) ----------------
void drawGrassStrip(float x, float y, float h, float count) {
    for (int i=0;i<(int)count;i++){
        float ox = x + i*6.0f * (randf(0.8f,1.2f));
        float hh = h * randf(0.8f,1.3f);
        glBegin(GL_LINES); glColor3f(0.14f,0.7f,0.18f); glVertex2f(ox, y); glVertex2f(ox + randf(-3,3), y + hh); glEnd();
    }
}

// ---------------- MAIN ----------------
int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WINDOW_W, WINDOW_H);
    glutCreateWindow("Catch The Egg");

    set2DProjection();
    glClearColor(0.9f,0.9f,0.9f,1.0f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef HAVE_SDL_MIXER
    initAudio();
#endif

    glutDisplayFunc(renderScene);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);
    glutPassiveMotionFunc(passiveMouse);
    glutMouseFunc(mouseClick);

    lastTimeMs = glutGet(GLUT_ELAPSED_TIME);
    setupScene();
    resetGame();
    glutTimerFunc(16, timerFunc, 0);
    glutMainLoop();

#ifdef HAVE_SDL_MIXER
    closeAudio();
#endif
    return 0;
}
