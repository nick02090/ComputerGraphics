#define _USE_MATH_DEFINES

#include <cstdio>
#include <glm/glm.hpp>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <cmath>
#include <math.h>
#include <algorithm>
#include <ctime>
#include <glut.h>
#include "PerlinNoise.h"
using namespace std;

//*********************************************************************************
//  Razredi potrebni za iscrtavanje tijela.
//  Vertex3D - predstavlja vrh/tocku u prostoru s koordinatama x, y, z
//  Polygon3D - predstavlja poligon u prostoru s vrhovima v1, v2, v3
//  Object3D - predstavlja objekt unutar 3D scene
//*********************************************************************************

class Vertex3D
{
public:
    Vertex3D(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}

    glm::vec3 get() { return glm::vec3(x, y, z); }
    double x, y, z;
};

class Polygon3D
{
public:
    Polygon3D(Vertex3D _v1, Vertex3D _v2, Vertex3D _v3) : v1(_v1), v2(_v2), v3(_v3) {}

    Vertex3D v1, v2, v3;
    glm::vec3 normal()
    {
        glm::vec3 edge1 = v2.get() - v1.get();
        glm::vec3 edge2 = v3.get() - v1.get();
        glm::vec3 normal = glm::cross(edge1, edge2);
        return glm::normalize(normal);
    }
};

class Object3D
{
public:
    Object3D(string _name) : name(_name)
    { 
        offset = glm::vec3(0.0, 0.0, 0.0);
        rotation = glm::vec4(0.0, 0.0, 0.0, 0.0);
        center = glm::vec3(0.0, 0.0, 0.0);
    }

    Object3D(Object3D* other)
    {
        this->name = string(other->name);
        this->offset = glm::vec3(other->offset);
        this->rotation = glm::vec4(other->rotation);
        this->vertices = vector<Vertex3D>(other->vertices);
        this->polygons = vector<Polygon3D>(other->polygons);
        this->center = glm::vec3(center);
    }

    string name;
    vector<Vertex3D> vertices;
    vector<Polygon3D> polygons;
    glm::vec3 offset;
    glm::vec4 rotation;
    glm::vec3 center;
};

struct ObjectData
{
    string filePath;
    string name;
};

class Particle
{
public:
    Particle(Vertex3D _position, glm::vec3 _color, double _size) : position(_position), color(_color), size(_size)
    {
        age = 1.0;
        angle = 0;
        axis = glm::vec3(0.0, 0.0, 0.0);
    }
    virtual void render() = 0;

    Vertex3D position;
    glm::vec3 color;
    glm::vec3 axis;
    double angle;
    double size;
    double age;
};

class EngineParticle : public Particle
{
public:
    EngineParticle(Vertex3D _position, glm::vec3 _color, double _size) : Particle(_position, _color, _size) {}

    void render() {
        glColor3f(color.x, color.y, color.z);
        glTranslatef(position.x, position.y, position.z);
        glutSolidSphere(size, 5, 5);
    }
};

class RocketSmashParticle : public Particle
{
public:
    RocketSmashParticle(Vertex3D _position, glm::vec3 _color, double _size) : Particle(_position, _color, _size) {}

    void render()
    {
        glColor3f(color.x, color.y, color.z);
        glTranslatef(position.x, position.y, position.z);
        glutSolidSphere(size, 5, 5);
    }
};

struct Timer
{
    int current_time;
    int previous_time;
    int start_time;
};

//*********************************************************************************
//	Pokazivac na glavni prozor i pocetna velicina.
//*********************************************************************************

GLuint window;
GLuint width = 927, height = 522;

vector<ObjectData> objectsData;

Object3D *character;

int max_missiles = 4;
int missile_counter = max_missiles;
Timer missile_timer = Timer{0, 0};

const int MAX_HEALTH = 100;
int health = MAX_HEALTH;
bool bCharacterHit;
Timer character_timer = Timer{ 0, 0 };
bool bEnemyHit;
Timer enemy_hit_timer = Timer{ 0, 0 };

Object3D* enemy;
Timer enemies_timer = Timer{ 0, 0 };
int enemies_defeated = 0; 
Timer enemy_timer;
int enemy_side;
bool bShowEnemy = false;
Object3D* newEnemy;
glm::vec4 enemyTilt(0.0, 0.0, 0.0, 0.0);

Object3D* enemyRocket;
vector<Object3D*> enemyRockets;
vector<vector<RocketSmashParticle>> enemyRocketsParticles;
vector<Timer> enemy_rockets_timer;
vector<glm::vec3> enemyRocketsSmashPosition;
vector<glm::vec4> enemyRocketsSmashRotation;
vector<glm::vec3> enemyMissilePosition;
vector<glm::vec4> enemyMissileRotation;

Vertex3D* character_engine_center;
double engine_radius;
vector<EngineParticle> engine;
Timer engine_timer = Timer{ 0, 0 };
bool bNoMissilesError = false;
Timer missile_error_timer = Timer{ 0, 0 };
vector<glm::vec3> missilePosition;
vector<glm::vec4> missileRotation;

Object3D* rocket;
vector<Object3D*> rockets;
vector<vector<RocketSmashParticle>> rocketsParticles;
vector<Timer> rockets_timer;
vector<glm::vec3> rocketsSmashPosition;
vector<glm::vec4> rocketsSmashRotation;

const int TERRAIN_WIDTH = 30;
const int TERRAIN_HEIGHT = 15;
double flying_speed = 0.0;

Vertex3D ociste(0.0, 2.3, -9.2);

bool bPaused = false;
bool bGameOver = false;

int fps_print = 0;
int fps = 0;
Timer fps_timer = Timer{ 0, 0 };

//*********************************************************************************
//	Function Prototypes.
//*********************************************************************************

void myDisplay();
void myIdle();
void myKeyboard(unsigned char theKey, int mouseX, int mouseY);
void myReshape(int w, int h);
void updatePerspective();

Object3D* loadObject(string filePath, string name);
void drawObject(Object3D* object, glm::vec3 color);

void drawCharacter();
void moveObject(Object3D* object, glm::vec3 direction);
void rotateObject(Object3D* object, glm::vec4 rotation);

void drawGUI();

void updateTerrain();
void drawTerrain();

void engineParticles();
void updateMissiles();
Vertex3D* centerByVertices(Object3D* object);

void rocketSmashParticles();
void drawRocketSmash();
void drawEnemyRocketSmash();
void enemyRocketSmashParticles();

void drawEnemies();
void enemiesAI();

double doubleRandomGen();

void checkCollisions();

//*********************************************************************************
//	Glavni program.
//*********************************************************************************

int main(int argc, char** argv)
{
    srand(time(NULL));

    character = loadObject("f16.obj", "f16");

    rocket = loadObject("rocket.obj", "rocket");

    //missile = loadObject("missile.obj", "missile");

    enemy = new Object3D(character);
    enemy->name = "enemy";
    enemy->offset = glm::vec3(0.0, 0.0, 10.0);
    enemy->rotation = glm::vec4(180, 0.0, 1.0, 0.0);

    enemyRocket = new Object3D(rocket);
    enemyRocket->offset = glm::vec3(0.0, 0.0, 0.0);
    //enemyRocket->rotation = glm::vec4(180, 0.0, 1.0, 0.0);

    Object3D *character_engine = loadObject("f16_engine.obj", "f16_engine");
    character_engine_center = centerByVertices(character_engine);
    glm::vec3 diff = character_engine->vertices.front().get() - character_engine_center->get();
    engine_radius = sqrt(pow(diff.x, 2) + pow(diff.y, 2) + pow(diff.z, 2));

    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(100, 100);
    glutInit(&argc, argv);

    window = glutCreateWindow("Aeroplane");
    glutDisplayFunc(myDisplay);
    glutKeyboardFunc(myKeyboard);
    glutReshapeFunc(myReshape);

    glutIdleFunc(myIdle);
    glutMainLoop();
    return 0;
}

//*********************************************************************************
//	Osvjezavanje prikaza. (nakon preklapanja prozora)
//*********************************************************************************

void myDisplay()
{
    glClearColor(+0.11f, +0.124f, +0.16f, +1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawTerrain();
    drawEnemies();
    drawRocketSmash();
    drawCharacter();
    drawEnemyRocketSmash();
    drawGUI();

    glutSwapBuffers();
    glutPostRedisplay();
}

void updateMissiles() {
    if (missile_counter < max_missiles) {
        missile_counter++;
    }
}

void myIdle()
{
    if (bPaused)
    {
        return;
    }

    if (health <= 0) {
        health = 0;
        bGameOver = true;
    }

    if (bGameOver)
    {
        return;
    }

    // FPS counter
    fps++;
    fps_timer.current_time = glutGet(GLUT_ELAPSED_TIME);
    int diff = fps_timer.current_time - fps_timer.previous_time;
    if (diff > 1000)
    {
        fps_timer.previous_time = fps_timer.current_time;
        fps_print = fps;
        fps = 0;
    }

    // cheking for collisions
    checkCollisions();

    // change character color on collision
    if (bCharacterHit)
    {
        character_timer.current_time = glutGet(GLUT_ELAPSED_TIME);
        int diff = character_timer.current_time - character_timer.previous_time;
        if (diff > 0.5 * 1000)
        {
            bCharacterHit = false;
        }
    }
    if (bEnemyHit)
    {
        enemy_hit_timer.current_time = glutGet(GLUT_ELAPSED_TIME);
        int diff = enemy_hit_timer.current_time - enemy_hit_timer.previous_time;
        if (diff > 0.5 * 1000)
        {
            bEnemyHit = false;
        }
    }

    // terrain update
    updateTerrain();

    // character tilt
    glm::vec4 currentRotation = character->rotation;
    double angle = currentRotation.x;
    if (currentRotation.x > 0.1) {
        angle -= 0.1;
    }
    else if (currentRotation.x < -0.1) {
        angle += 0.1;
    }
    rotateObject(character, glm::vec4(angle, 0.0, 0.0, 1.0));

    // missiles refill
    missile_timer.current_time = glutGet(GLUT_ELAPSED_TIME);
    diff = missile_timer.current_time - missile_timer.previous_time;
    if (diff > 5 * 1000)
    {
        updateMissiles();
        missile_timer.previous_time = missile_timer.current_time;
    }
    if (bNoMissilesError)
    {
        if (missile_counter > 0)
        {
            bNoMissilesError = false;
        } else
        {
            missile_error_timer.current_time = glutGet(GLUT_ELAPSED_TIME);
            diff = missile_error_timer.current_time - missile_error_timer.previous_time;
            if (diff > 3 * 1000)
            {
                bNoMissilesError = false;
            }
        }
    }

    // engine particles timer
    engine_timer.current_time = glutGet(GLUT_ELAPSED_TIME);
    diff = engine_timer.current_time - engine_timer.previous_time;
    if (diff > 10)
    {
        engineParticles();
        engine_timer.previous_time = engine_timer.current_time;
    }

    // rockets launch timer
    for (int i = rockets.size() - 1; i >= 0; i--) {
        rockets.at(i)->offset += glm::vec3(0.0, 0.0, 0.3);
        if (rockets.at(i)->offset.z >= 8.0) {
            rockets_timer.push_back(Timer{0, 0, glutGet(GLUT_ELAPSED_TIME)});
            rocketsParticles.push_back(vector<RocketSmashParticle>());
            rocketsSmashPosition.push_back(glm::vec3(missilePosition.at(i).x, missilePosition.at(i).y, missilePosition.at(i).z));
            rocketsSmashRotation.push_back(glm::vec4(missileRotation.at(i).x, missileRotation.at(i).y, missileRotation.at(i).z, missileRotation.at(i).w));
            rockets.erase(rockets.begin() + i);
            missilePosition.erase(missilePosition.begin() + i);
            missileRotation.erase(missileRotation.begin() + i);
        }
    }
    for (int i = rockets_timer.size() - 1; i >= 0; i--)
    {
        rockets_timer.at(i).current_time = glutGet(GLUT_ELAPSED_TIME);
        diff = rockets_timer.at(i).current_time - rockets_timer.at(i).previous_time;
        if (diff > 10)
        {
            rocketSmashParticles();
            rockets_timer.at(i).previous_time = rockets_timer.at(i).current_time;
        }
        if (rockets_timer.at(i).current_time - rockets_timer.at(i).start_time > 2 * 1000)
        {
            rockets_timer.erase(rockets_timer.begin() + i);
            rocketsParticles.erase(rocketsParticles.begin() + i);
            rocketsSmashPosition.erase(rocketsSmashPosition.begin() + i);
            rocketsSmashRotation.erase(rocketsSmashRotation.begin() + i);
        }
    }

    // enemies rockets launch timer
    for (int i = enemyRockets.size() - 1; i >= 0; i--) {
        enemyRockets.at(i)->offset += glm::vec3(0.0, 0.0, 0.3);
        if (enemyRockets.at(i)->offset.z >= 8.0) {
            enemy_rockets_timer.push_back(Timer{ 0, 0, glutGet(GLUT_ELAPSED_TIME) });
            enemyRocketsParticles.push_back(vector<RocketSmashParticle>());
            enemyRocketsSmashPosition.push_back(glm::vec3(enemyMissilePosition.at(i).x, enemyMissilePosition.at(i).y, enemyMissilePosition.at(i).z));
            enemyRocketsSmashRotation.push_back(glm::vec4(enemyMissileRotation.at(i).x, enemyMissileRotation.at(i).y, enemyMissileRotation.at(i).z, enemyMissileRotation.at(i).w));
            enemyRockets.erase(enemyRockets.begin() + i);
            enemyMissilePosition.erase(enemyMissilePosition.begin() + i);
            enemyMissileRotation.erase(enemyMissileRotation.begin() + i);
        }
    }
    for (int i = enemy_rockets_timer.size() - 1; i >= 0; i--)
    {
        enemy_rockets_timer.at(i).current_time = glutGet(GLUT_ELAPSED_TIME);
        diff = enemy_rockets_timer.at(i).current_time - enemy_rockets_timer.at(i).previous_time;
        if (diff > 10)
        {
            enemyRocketSmashParticles();
            enemy_rockets_timer.at(i).previous_time = enemy_rockets_timer.at(i).current_time;
        }
        if (enemy_rockets_timer.at(i).current_time - enemy_rockets_timer.at(i).start_time > 2 * 1000)
        {
            enemy_rockets_timer.erase(enemy_rockets_timer.begin() + i);
            enemyRocketsParticles.erase(enemyRocketsParticles.begin() + i);
            enemyRocketsSmashPosition.erase(enemyRocketsSmashPosition.begin() + i);
            enemyRocketsSmashRotation.erase(enemyRocketsSmashRotation.begin() + i);
        }
    }

    // enemies show
    if (!bShowEnemy)
    {
        enemies_timer.current_time = glutGet(GLUT_ELAPSED_TIME);
        diff = enemies_timer.current_time - enemies_timer.previous_time;
        if (diff > 3 * 1000)
        {
            // showing position
            newEnemy = new Object3D(enemy);
            glm::vec3 currentOffset = newEnemy->offset;
            double x = -5.0 + doubleRandomGen() * (4.5 + 5.0);
            double y = -1.0 + doubleRandomGen() * (3.0 + 1.0);
            newEnemy->offset = glm::vec3(x, y, currentOffset.z);

            int side = 1 + (rand() % (4 - 1 + 1));

            bShowEnemy = true;
            enemy_side = side;
            enemy_timer = Timer{ glutGet(GLUT_ELAPSED_TIME), glutGet(GLUT_ELAPSED_TIME) };
            enemies_timer.previous_time = enemies_timer.current_time;
        }
    }
    else {
        glm::vec4 currentRotation = enemyTilt;
        double angle = currentRotation.x;
        if (currentRotation.x > 0.1) {
            angle -= 0.5;
        }
        else if (currentRotation.x < -0.1) {
            angle += 0.5;
        }
        enemyTilt = glm::vec4(angle, 0.0, 0.0, 1.0);
        if (newEnemy->offset.z <= -5.0)
        {
            bShowEnemy = false;
            enemies_timer.previous_time = glutGet(GLUT_ELAPSED_TIME);
        }
    }
    enemy_timer.current_time = glutGet(GLUT_ELAPSED_TIME);
    enemiesAI();

    myDisplay();
}

void myReshape(int w, int h)
{
    width = w;
    height = h;
    glViewport(0, 0, width, height);
    updatePerspective();
}

void updatePerspective()
{
    glMatrixMode(GL_PROJECTION); // aktivirana matrica projekcije
    glLoadIdentity();
    gluPerspective(45.0, (float)width / height, 0.5, 80.0); // kut pogleda, x/y, prednja i straznja ravnina odsjecanja
    glMatrixMode(GL_MODELVIEW);                             // aktivirana matrica modela
    glLoadIdentity();
    gluLookAt(ociste.x, ociste.y, ociste.z, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0); // ociste x,y,z; glediste x,y,z; up vektor x,y,z
}

Object3D *loadObject(string filePath, string name)
{
    ifstream file(filePath);
    string line;
    if (!file)
    {
        cerr << "Problem prilikom otvaranja datoteke " << filePath << endl;
        exit(1);
    }
    Object3D* object = new Object3D(name);
    while (getline(file, line)) {
        // v => vertex
        if (line.compare(0, 2, "v ") == 0)
        {
            double x, y, z;
            char c;
            istringstream iss(line);
            iss >> c >> x >> y >> z;
            Vertex3D v(x, y, z);
            object->vertices.push_back(v);
        }
        // vt => vertex texture

        // vn => vertex normal

        // f => face/polygon
        else if (line.compare(0, 2, "f ") == 0)
        {
            string V1, V2, V3;
            char c;
            istringstream iss(line);
            iss >> c >> V1 >> V2 >> V3;
            int indexV1 = stoi(V1.substr(0, V1.find("//")));
            int indexV2 = stoi(V2.substr(0, V2.find("//")));
            int indexV3 = stoi(V3.substr(0, V3.find("//")));
            Vertex3D v1 = object->vertices[indexV1 - 1];
            Vertex3D v2 = object->vertices[indexV2 - 1];
            Vertex3D v3 = object->vertices[indexV3 - 1];
            Polygon3D p(v1, v2, v3);
            object->polygons.push_back(p);
        }
    }
    return object;
}

void drawObject(Object3D* object, glm::vec3 color)
{
    for (auto& polygon : object->polygons)
    {
        glPushMatrix();
        glColor3f(color.x, color.y, color.z);
        glTranslatef(object->offset.x, object->offset.y, object->offset.z);
        glRotatef(object->rotation.x, object->rotation.y, object->rotation.z, object->rotation.w);
        glBegin(GL_LINE_STRIP);
        glVertex3f(polygon.v1.x, polygon.v1.y, polygon.v1.z);
        glVertex3f(polygon.v2.x, polygon.v2.y, polygon.v2.z);
        glVertex3f(polygon.v3.x, polygon.v3.y, polygon.v3.z);
        glEnd();
        glPopMatrix();
    }
}

void drawCharacter()
{
    // draw polygons
    glm::vec3 color(1.0, 1.0, 1.0);
    if (bCharacterHit)
    {
        color = glm::vec3(1.0, 0.0, 0.0);
    }
    if (bEnemyHit)
    {
        color = glm::vec3(1.0, 1.0, 0.0);
    }
    drawObject(character, color);

    // aim
    glPushMatrix();
    glTranslatef(character->offset.x, character->offset.y, character->offset.z);
    glRotatef(character->rotation.x, character->rotation.y, character->rotation.z, character->rotation.w);
    glColor3f(1.0, 0.0, 0.0);
    glTranslatef(character_engine_center->x, character_engine_center->y, character_engine_center->z + 10);
    glutSolidSphere(0.05, 5, 5);
    glPopMatrix();

    // collision sphere
    glm::vec3 center(0.0, 0.0, 0.0);
    double angle = character->rotation.x;
    center += character->offset;
    center += character_engine_center->get();
    center += glm::vec3(0.0, 0.0, 1.5);
    center += glm::vec3(angle / 25.0, 0.0, 0.0);
    character->center = center;
    
    // draws characters collision sphere
    /*glPushMatrix();
    glTranslatef(center.x, center.y, center.z);
    glutSolidSphere(1, 10, 10);
    glPopMatrix();*/

    // engine particles
    for (auto& ep : engine) {
        glPushMatrix();
        glTranslatef(character->offset.x, character->offset.y, character->offset.z);
        glRotatef(character->rotation.x, character->rotation.y, character->rotation.z, character->rotation.w);
        ep.render();
        glPopMatrix();
    }

    // rocket if fired
    for (int i = rockets.size() - 1; i >= 0; i--)
    {
        for (auto& polygon : rockets.at(i)->polygons)
        {
            glPushMatrix();
            glTranslatef(missilePosition.at(i).x, missilePosition.at(i).y, missilePosition.at(i).z);
            glRotatef(missileRotation.at(i).x, missileRotation.at(i).y, missileRotation.at(i).z, missileRotation.at(i).w);
            glColor3f(0.5, 0.0, 0.5);
            glTranslatef(rockets.at(i)->offset.x, rockets.at(i)->offset.y, rockets.at(i)->offset.z);
            glRotatef(rockets.at(i)->rotation.x, rockets.at(i)->rotation.y, rockets.at(i)->rotation.z, rockets.at(i)->rotation.w);
            glBegin(GL_LINE_STRIP);
            glVertex3f(polygon.v1.x, polygon.v1.y, polygon.v1.z);
            glVertex3f(polygon.v2.x, polygon.v2.y, polygon.v2.z);
            glVertex3f(polygon.v3.x, polygon.v3.y, polygon.v3.z);
            glEnd();
            glPopMatrix();
        }

        // collision sphere
        glm::vec3 center(0.0, 0.0, 0.0);
        double angle = missileRotation.at(i).x;
        center += missilePosition.at(i);
        center += rockets.at(i)->offset;
        center += glm::vec3(0.0, -2.0, 1.25);
        center += glm::vec3(angle / 25.0, 0.0, 0.0);
        rockets.at(i)->center = center;

        // draws characters collision sphere
        /*glPushMatrix();
        glTranslatef(center.x, center.y, center.z);
        glutSolidSphere(0.2, 10, 10);
        glPopMatrix();*/
    }
}

void drawGUI()
{
    // missiles GUI
    glm::vec3 color = glm::vec3(1.0, 0.0, 1.0);
    string time = "";
    if (missile_counter == 0.0) {
        color = glm::vec3(0.8, 0.8, 0.8);
    }
    if (missile_counter < max_missiles) {
        time = " (" + to_string(5 - (missile_timer.current_time - missile_timer.previous_time) / 1000) + "s)";
    }
    glColor3f(color.x, color.y, color.z);    // needs to be called before RasterPos
    glRasterPos2f(7.5, -4.3);
    string s = "Missiles: " + to_string(missile_counter) + "/" + to_string(max_missiles) + time;
    void* font = GLUT_BITMAP_9_BY_15;
    for (string::iterator i = s.begin(); i != s.end(); ++i)
    {
        char c = *i;
        glutBitmapCharacter(font, c);
    }

    // health GUI
    color = glm::vec3(0.0, 1.0, 0.0);
    if (health <= 50)
    {
        color = glm::vec3(1.0, 1.0, 0.0);
    }
    if (health <= 25)
    {
        color = glm::vec3(1.0, 0.0, 0.0);
    }
    glColor3f(color.x, color.y, color.z);
    glRasterPos2f(-5.7, -4.3);
    s = "Health: " + to_string(health) + "%";
    for (string::iterator i = s.begin(); i != s.end(); ++i)
    {
        char c = *i;
        glutBitmapCharacter(font, c);
    }

    // missiles error GUI
    if (bNoMissilesError)
    {
        glColor3f(1.0, 0.0, 0.0);
        glRasterPos2f(1.25, 3.4);
        s = "No missiles available!";
        for (string::iterator i = s.begin(); i != s.end(); ++i)
        {
            char c = *i;
            glutBitmapCharacter(font, c);
        }
    }

    // enemies hit GUI
    glColor3f(1.0, 1.0, 0.0);
    glRasterPos2f(6.2, 3.4);
    s = "Hits: " + to_string(enemies_defeated);
    for (string::iterator i = s.begin(); i != s.end(); ++i)
    {
        char c = *i;
        glutBitmapCharacter(font, c);
    }

    // pause text
    if (bPaused)
    {
        glColor3f(1.0, 1.0, 0.0);
        glRasterPos2f(0.5, 1.5);
        s = "PAUSE";
        font = GLUT_BITMAP_HELVETICA_18;
        for (string::iterator i = s.begin(); i != s.end(); ++i)
        {
            char c = *i;
            glutBitmapCharacter(font, c);
        }

        glColor3f(1.0, 0.0, 0.0);
        glRasterPos2f(1.0, 1.0);
        s = "Esc - quit game";
        font = GLUT_BITMAP_9_BY_15;
        for (string::iterator i = s.begin(); i != s.end(); ++i)
        {
            char c = *i;
            glutBitmapCharacter(font, c);
        }

        glColor3f(0.0, 1.0, 0.0);
        glRasterPos2f(1.8, 0.5);
        s = "SPACEBAR - continue playing";
        font = GLUT_BITMAP_9_BY_15;
        for (string::iterator i = s.begin(); i != s.end(); ++i)
        {
            char c = *i;
            glutBitmapCharacter(font, c);
        }
    }

    // game over
    if (bGameOver)
    {
        glColor3f(1.0, 0.0, 0.0);
        glRasterPos2f(0.8, 1.5);
        s = "GAME OVER";
        font = GLUT_BITMAP_HELVETICA_18;
        for (string::iterator i = s.begin(); i != s.end(); ++i)
        {
            char c = *i;
            glutBitmapCharacter(font, c);
        }

        glColor3f(0.8, 0.8, 0.8);
        glRasterPos2f(0.65, 1.0);
        s = "Press Esc.";
        font = GLUT_BITMAP_9_BY_15;
        for (string::iterator i = s.begin(); i != s.end(); ++i)
        {
            char c = *i;
            glutBitmapCharacter(font, c);
        }
    }

    // FPS GUI
    glColor3f(0.0, 1.0, 0.0);
    glRasterPos2f(-5.3, 3.4);
    s = "FPS: " + to_string(fps_print);
    font = GLUT_BITMAP_9_BY_15;
    for (string::iterator i = s.begin(); i != s.end(); ++i)
    {
        char c = *i;
        glutBitmapCharacter(font, c);
    }
}

void drawTerrain()
{
    vector<Vertex3D> vertices;
    vector<Polygon3D> polygons;

    double z_off = flying_speed;
    for (int z = 0; z <= TERRAIN_HEIGHT; z++)
    {
        double x_off = 0.0;
        for (int x = 0; x <= TERRAIN_WIDTH; x++)
        {
            PerlinNoise pn;
            double noise = pn.noise(x_off, z_off, 0.0) * 8;
            vertices.push_back(Vertex3D(x, noise, z));
            x_off+=0.15;
        }
        z_off+=0.15;
    }

    int vert = 0;
    for (int z = 0; z < TERRAIN_HEIGHT; z++)
    {
        for (int x = 0; x < TERRAIN_WIDTH; x++)
        {
            polygons.push_back(Polygon3D(vertices.at(vert + 0), vertices.at(vert + TERRAIN_WIDTH + 1), vertices.at(vert + 1)));
            polygons.push_back(Polygon3D(vertices.at(vert + 1), vertices.at(vert + TERRAIN_WIDTH + 1), vertices.at(vert + TERRAIN_WIDTH + 2)));
            vert++;
        }
        vert++;
    }

    Object3D* terrain = new Object3D("terrain");
    terrain->vertices = vertices;
    terrain->polygons = polygons;
    terrain->offset = glm::vec3(-15.0, -10.0, -3.0);

    drawObject(terrain, glm::vec3(0.0, 0.0, 1.0));
    delete terrain;
}

void updateTerrain()
{
    flying_speed += 0.05;
}

void moveObject(Object3D* object, glm::vec3 direction)
{
    object->offset += direction;
}

void rotateObject(Object3D* object, glm::vec4 rotation)
{
    object->rotation = rotation;
}

Vertex3D* centerByVertices(Object3D* object)
{
    glm::vec3 center(0.0, 0.0, 0.0);
    for (auto& vertex : object->vertices)
    {
        center += vertex.get();
    }
    center /= object->vertices.size();
    return new Vertex3D(center.x, center.y, center.z);
}

void engineParticles()
{
    // create new particle
    Vertex3D position(character_engine_center->x, character_engine_center->y, character_engine_center->z);
    EngineParticle ep(position, glm::vec3(0.0, 0.0, 0.0), 0.05);
    engine.push_back(ep);

    // edit existing particles
    for (int i = engine.size() - 1; i >= 0; i--) {
        // transition
        Vertex3D current_position = engine.at(i).position;
        double x = -0.05 + doubleRandomGen() * (0.05 + 0.05);
        double y = -0.05 + doubleRandomGen() * (0.05 + 0.05);
        engine.at(i).position = Vertex3D(current_position.x + x, current_position.y + y, current_position.z - 0.01);

        // coloring
        if (engine.at(i).age < 0.6) {
            engine.at(i).color = glm::vec3(1.0, 0.0, 0.0);
        }
        if (engine.at(i).age < 0.3)
        {
            engine.at(i).color = glm::vec3(0.1, 0.1, 0.1);
        }

        // killing
        engine.at(i).age -= 0.07;
        if (engine.at(i).age <= 0.0) {
            engine.erase(engine.begin() + i);
        }
    }
}

void rocketSmashParticles()
{
    for (int j = rocketsParticles.size() - 1; j >= 0; j--)
    {
        // create new particle
        Vertex3D position(character_engine_center->x, character_engine_center->y, character_engine_center->z + 10.0);
        RocketSmashParticle rsp(position, glm::vec3(0.0, 0.0, 0.0), 0.05);
        rocketsParticles.at(j).push_back(rsp);

        // edit existing particles
        for (int i = rocketsParticles.at(j).size() - 1; i >= 0; i--) {
            // transition
            Vertex3D current_position = rocketsParticles.at(j).at(i).position;
            double x = -0.07 + doubleRandomGen() * (0.07 + 0.07);
            double y = -0.07 + doubleRandomGen() * (0.07 + 0.07);
            double z = -0.01 + doubleRandomGen() * (0.01 + 0.01);
            rocketsParticles.at(j).at(i).position = Vertex3D(current_position.x + x, current_position.y + y, current_position.z + z);

            // coloring
            if (rocketsParticles.at(j).at(i).age < 0.6) {
                rocketsParticles.at(j).at(i).color = glm::vec3(1.0, 0.0, 0.0);
            }
            if (rocketsParticles.at(j).at(i).age < 0.3)
            {
                rocketsParticles.at(j).at(i).color = glm::vec3(0.1, 0.1, 0.1);
            }

            // killing
            rocketsParticles.at(j).at(i).age -= 0.01;
            if (rocketsParticles.at(j).at(i).age <= 0.0) {
                rocketsParticles.at(j).erase(rocketsParticles.at(j).begin() + i);
            }
        }
    }
}

void drawRocketSmash()
{
    for (int i = rocketsParticles.size() - 1; i >= 0; i--) {
        for (auto& rocketParticle : rocketsParticles.at(i))
        {
            glPushMatrix();
            glTranslatef(rocketsSmashPosition.at(i).x, rocketsSmashPosition.at(i).y, rocketsSmashPosition.at(i).z);
            glRotatef(rocketsSmashRotation.at(i).x, rocketsSmashRotation.at(i).y, rocketsSmashRotation.at(i).z, rocketsSmashRotation.at(i).w);
            rocketParticle.render();
            glPopMatrix();
        }
    }
}

void drawEnemyRocketSmash()
{
    for (int i = enemyRocketsParticles.size() - 1; i >= 0; i--) {
        for (auto& enemyRocketParticle : enemyRocketsParticles.at(i))
        {
            glPushMatrix();
            glTranslatef(enemyRocketsSmashPosition.at(i).x, enemyRocketsSmashPosition.at(i).y, enemyRocketsSmashPosition.at(i).z);
            glRotatef(enemyRocketsSmashRotation.at(i).x, enemyRocketsSmashRotation.at(i).y, enemyRocketsSmashRotation.at(i).z, enemyRocketsSmashRotation.at(i).w);
            enemyRocketParticle.render();
            glPopMatrix();
        }
    }
}

void enemyRocketSmashParticles()
{
    for (int j = enemyRocketsParticles.size() - 1; j >= 0; j--)
    {
        // create new particle
        Vertex3D position(enemy->offset.x, enemy->offset.y, enemy->offset.z);
        RocketSmashParticle rsp(position, glm::vec3(0.0, 0.0, 0.0), 0.05);
        enemyRocketsParticles.at(j).push_back(rsp);

        // edit existing particles
        for (int i = enemyRocketsParticles.at(j).size() - 1; i >= 0; i--) {
            // transition
            Vertex3D current_position = enemyRocketsParticles.at(j).at(i).position;
            double x = -0.07 + doubleRandomGen() * (0.07 + 0.07);
            double y = -0.07 + doubleRandomGen() * (0.07 + 0.07);
            double z = -0.01 + doubleRandomGen() * (0.01 + 0.01);
            enemyRocketsParticles.at(j).at(i).position = Vertex3D(current_position.x + x, current_position.y + y, current_position.z + z);

            // coloring
            if (enemyRocketsParticles.at(j).at(i).age < 0.6) {
                enemyRocketsParticles.at(j).at(i).color = glm::vec3(1.0, 0.0, 0.0);
            }
            if (enemyRocketsParticles.at(j).at(i).age < 0.3)
            {
                enemyRocketsParticles.at(j).at(i).color = glm::vec3(0.1, 0.1, 0.1);
            }

            // killing
            enemyRocketsParticles.at(j).at(i).age -= 0.01;
            if (enemyRocketsParticles.at(j).at(i).age <= 0.0) {
                enemyRocketsParticles.at(j).erase(enemyRocketsParticles.at(j).begin() + i);
            }
        }
    }
}

void drawEnemies()
{
    if (bShowEnemy) {
        glm::vec3 color = glm::vec3(0.0, 0.0, 0.0);
        for (auto& polygon : newEnemy->polygons)
        {
            glPushMatrix();
            glColor3f(color.x, color.y, color.z);
            glTranslatef(newEnemy->offset.x, newEnemy->offset.y, newEnemy->offset.z);
            glRotatef(newEnemy->rotation.x, newEnemy->rotation.y, newEnemy->rotation.z, newEnemy->rotation.w);
            glRotatef(enemyTilt.x, enemyTilt.y, enemyTilt.z, enemyTilt.w);
            glBegin(GL_LINE_STRIP);
            glVertex3f(polygon.v1.x, polygon.v1.y, polygon.v1.z);
            glVertex3f(polygon.v2.x, polygon.v2.y, polygon.v2.z);
            glVertex3f(polygon.v3.x, polygon.v3.y, polygon.v3.z);
            glEnd();
            glPopMatrix();
        }

        // collision sphere
        glm::vec3 center(0.0, 0.0, 0.0);
        center += newEnemy->offset;
        center += enemy->offset;
        center += glm::vec3(0.0, -1.5, -10.0);
        newEnemy->center = center;

        // draws characters collision sphere
        /*glPushMatrix();
        glTranslatef(center.x, center.y, center.z);
        glutSolidSphere(1, 10, 10);
        glPopMatrix();*/

        for (int i = enemyRockets.size() - 1; i >= 0; i--)
        {
            for (auto& polygon : enemyRockets.at(i)->polygons)
            {
                glPushMatrix();
                glTranslatef(enemyMissilePosition.at(i).x, enemyMissilePosition.at(i).y, enemyMissilePosition.at(i).z);
                glRotatef(enemyMissileRotation.at(i).x, enemyMissileRotation.at(i).y, enemyMissileRotation.at(i).z, enemyMissileRotation.at(i).w);
                glColor3f(0.5, 0.0, 0.5);
                glTranslatef(enemyRockets.at(i)->offset.x, enemyRockets.at(i)->offset.y, enemyRockets.at(i)->offset.z);
                glRotatef(enemyRockets.at(i)->rotation.x, enemyRockets.at(i)->rotation.y, enemyRockets.at(i)->rotation.z, enemyRockets.at(i)->rotation.w);
                glBegin(GL_LINE_STRIP);
                glVertex3f(polygon.v1.x, polygon.v1.y, polygon.v1.z);
                glVertex3f(polygon.v2.x, polygon.v2.y, polygon.v2.z);
                glVertex3f(polygon.v3.x, polygon.v3.y, polygon.v3.z);
                glEnd();
                glPopMatrix();
            }

            // collision sphere
            glm::vec3 center(0.0, 0.0, 0.0);
            center += enemyMissilePosition.at(i);
            center -= enemyRockets.at(i)->offset;
            center += glm::vec3(0.0, -2.0, -1.25);
            enemyRockets.at(i)->center = center;

            // draws characters collision sphere
            /*glPushMatrix();
            glTranslatef(center.x, center.y, center.z);
            glutSolidSphere(0.2, 10, 10);
            glPopMatrix();*/
        }
    }
}

void enemiesAI()
{
    if (bShowEnemy)
    {
        // pick another move
        int diff = enemy_timer.current_time - enemy_timer.previous_time;
        if (diff > 1000)
        {
            int side = 1 + (rand() % (4 - 1 + 1));
            enemy_side = side;
            enemy_timer.previous_time = enemy_timer.current_time;
        }

        // animate move
        glm::vec3 currentPosition = newEnemy->offset;
        int side = enemy_side;
        if (side == 1)
        {
            // move up
            if (currentPosition.y + 0.05 < 3.0) {
                moveObject(newEnemy, glm::vec3(0.0, 0.05, 0.0));
            }
        }
        else if (side == 2)
        {
            // move down
            if (currentPosition.y - 0.05 > -1.0) {
                moveObject(newEnemy, glm::vec3(0.0, -0.05, 0.0));
            }
        }
        else if (side == 3)
        {
            // move left
            if (currentPosition.x - 0.05 > -5.0) {
                moveObject(newEnemy, glm::vec3(-0.05, 0.0, 0.0));
                glm::vec4 currentTilt = enemyTilt;
                double angle = currentTilt.x - 1.0;
                enemyTilt = glm::vec4(angle, 0.0, 0.0, 1.0);
            }
        }
        else if (side == 4)
        {
            // move right
            if (currentPosition.x + 0.05 > 4.5) {
                moveObject(newEnemy, glm::vec3(0.05, 0.0, 0.0));
                glm::vec4 currentTilt = enemyTilt;
                double angle = currentTilt.x + 1.0;
                enemyTilt = glm::vec4(angle, 0.0, 0.0, 1.0);
            }
        }
        moveObject(newEnemy, glm::vec3(0.0, 0.0, -0.1));

        // shoot
        if (diff > 1000)
        {
            double shoot = 0.0 + doubleRandomGen() * (1.0 - 0.0);
            if (shoot >= 0.5)
            {
                Object3D* newEnemyRocket = new Object3D(enemyRocket);
                enemyRockets.push_back(new Object3D(newEnemyRocket));
                enemyMissilePosition.push_back(glm::vec3(newEnemy->offset));
                enemyMissileRotation.push_back(glm::vec4(newEnemy->rotation));
            }
        }
    }
}

double doubleRandomGen()
{
    return (double)rand() / RAND_MAX;
}

//*********************************************************************************
//	Tastatura tipke.
//*********************************************************************************

void myKeyboard(unsigned char theKey, int mouseX, int mouseY)
{
    if (bGameOver)
    {
        switch (theKey)
        {
        case 27:
            cout << "Turning off..." << endl;
            exit(1);
            break;
        default:
            break;
        }
        return;
    }

    switch (theKey)
    {
    case 27:
        if (bPaused)
        {
            cout << "Turning off..." << endl;
            exit(1);
        }
        bPaused = !bPaused;
        break;
    case 'w':
        if (character->offset.y < 3.0) {
            moveObject(character, glm::vec3(0.0f, 0.2f, +0.0f));
        }
        break;

    case 's':
        if (character->offset.y > -1) {
            moveObject(character, glm::vec3(0.0f, -0.2f, +0.0f));
        }
        break;

    case 'a':
        if (character->offset.x < 4.5) {
            moveObject(character, glm::vec3(0.2f, 0.0f, +0.0f));
            glm::vec4 currentRotation = character->rotation;
            double angle = currentRotation.x + 1.0;
            rotateObject(character, glm::vec4(angle, 0.0, 0.0, 1.0));
        }
        break;

    case 'd':
        if (character->offset.x > -5.0) {
            moveObject(character, glm::vec3(-0.2f, 0.0f, +0.0f));
            glm::vec4 currentRotation = character->rotation;
            double angle = currentRotation.x - 1.0;
            rotateObject(character, glm::vec4(angle, 0.0, 0.0, 1.0));
        }
        break;

    case ' ':
        if (bPaused)
        {
            bPaused = false;
            break;
        }
        if (missile_counter == max_missiles) {
            missile_timer.previous_time = missile_timer.current_time;
        }
        if (missile_counter > 0) {
            rockets.push_back(new Object3D(rocket));
            missilePosition.push_back(glm::vec3(character->offset));
            missileRotation.push_back(glm::vec4(character->rotation));
            missile_counter--;
        }
        else {
            missile_error_timer.previous_time = glutGet(GLUT_ELAPSED_TIME);
            bNoMissilesError = true;
        }
        break;

    case 'l':
        ociste.x = ociste.x + 0.1;
        break;

    case 'k':
        ociste.x = ociste.x - 0.1;
        break;

    case 'i':
        ociste.y = ociste.y + 0.1;
        break;

    case 'o':
        ociste.y = ociste.y - 0.1;
        break;

    case 'n':
        ociste.z = ociste.z + 0.1;
        break;

    case 'm':
        ociste.z = ociste.z - 0.1;
        break;

    case 'r':
        ociste.x = 0.0;
        ociste.y = 2.3;
        ociste.z = -9.2;
        break;

    default:
        break;
    }
    updatePerspective();
    glutPostRedisplay();
}

void checkCollisions()
{
    // character - enemy collision
    if (bShowEnemy)
    {
        glm::vec3 character_position = character->center;
        glm::vec3 enemy_position = newEnemy->center;
        glm::vec3 diff = enemy_position - character_position;
        double distance = sqrt(pow(diff.x, 2) + pow(diff.y, 2) + pow(diff.z, 2));
        if (distance <= 2.0) // 2.0 = character_collision_sphere.radius (1.0) + enemy_collision_sphere.radius (1.0)
        {
            health -= 20;
            bCharacterHit = true; 
            character_timer.previous_time = glutGet(GLUT_ELAPSED_TIME);
            bShowEnemy = false;
            enemies_timer.previous_time = glutGet(GLUT_ELAPSED_TIME);
        }
    }

    // enemy - rocket collision
    if (bShowEnemy)
    {
        glm::vec3 enemy_position = newEnemy->center;
        for (int i = rockets.size() - 1; i >= 0; i--)
        {
            glm::vec3 rocket_position = rockets.at(i)->center;
            glm::vec3 diff = rocket_position - enemy_position;
            double distance = sqrt(pow(diff.x, 2) + pow(diff.y, 2) + pow(diff.z, 2));
            if (distance <= 1.2) // 1.2 = enemy_collision_sphere.radius (1.0) + rocket_collision_sphere.radius (0.2)
            {
                bShowEnemy = false;
                bEnemyHit = true;
                enemy_hit_timer.previous_time = glutGet(GLUT_ELAPSED_TIME);
                enemies_timer.previous_time = glutGet(GLUT_ELAPSED_TIME);
                enemies_defeated++;
                rockets_timer.push_back(Timer{ 0, 0, glutGet(GLUT_ELAPSED_TIME) });
                rocketsParticles.push_back(vector<RocketSmashParticle>());
                rocketsSmashPosition.push_back(glm::vec3(missilePosition.at(i).x, missilePosition.at(i).y, missilePosition.at(i).z));
                rocketsSmashRotation.push_back(glm::vec4(missileRotation.at(i).x, missileRotation.at(i).y, missileRotation.at(i).z, missileRotation.at(i).w));
                rockets.erase(rockets.begin() + i);
                missilePosition.erase(missilePosition.begin() + i);
                missileRotation.erase(missileRotation.begin() + i);
            }
        }
    }

    // character - enemy_rocket collision
    if (bShowEnemy)
    {
        glm::vec3 character_position = character->center;
        for (int i = enemyRockets.size() - 1; i >= 0; i--)
        {
            glm::vec3 enemy_rocket_position = enemyRockets.at(i)->center;
            glm::vec3 diff = enemy_rocket_position - character_position;
            double distance = sqrt(pow(diff.x, 2) + pow(diff.y, 2) + pow(diff.z, 2));
            if (distance <= 1.2) // 1.2 = enemy_collision_sphere.radius (1.0) + rocket_collision_sphere.radius (0.2)
            {
                health -= 10;
                bCharacterHit = true;
                character_timer.previous_time = glutGet(GLUT_ELAPSED_TIME);
                enemy_rockets_timer.push_back(Timer{ 0, 0, glutGet(GLUT_ELAPSED_TIME) });
                enemyRocketsParticles.push_back(vector<RocketSmashParticle>());
                enemyRocketsSmashPosition.push_back(glm::vec3(enemyMissilePosition.at(i).x, enemyMissilePosition.at(i).y, enemyMissilePosition.at(i).z));
                enemyRocketsSmashRotation.push_back(glm::vec4(enemyMissileRotation.at(i).x, enemyMissileRotation.at(i).y, enemyMissileRotation.at(i).z, enemyMissileRotation.at(i).w));
                enemyRockets.erase(enemyRockets.begin() + i);
                enemyMissilePosition.erase(enemyMissilePosition.begin() + i);
                enemyMissileRotation.erase(enemyMissileRotation.begin() + i);
            }
        }
    }
}