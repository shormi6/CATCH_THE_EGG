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

