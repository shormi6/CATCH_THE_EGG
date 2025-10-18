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