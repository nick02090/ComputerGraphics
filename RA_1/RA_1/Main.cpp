#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <cmath>
#include <glut.h>
using namespace std;

//*********************************************************************************
//  Razredi potrebni za iscrtavanje tijela.
//  Vertex - predstavlja vrh/toèku u prostoru s koordinatama x, y, z
//  Polygon - predstavlja poligon u prostoru s vrhovima v1, v2, v3
//  Trajectory - predstavlja putanju po kojoj se tijelo animira
//*********************************************************************************

class Vertex
{
public:
    Vertex(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}

    double get_x() { return x; }
    double get_y() { return y; }
    double get_z() { return z; }

private:
    double x, y, z;
};

class Polygon
{
public:
    Polygon(Vertex _v1, Vertex _v2, Vertex _v3) : v1(_v1), v2(_v2), v3(_v3) {}
    Vertex get_v1() { return v1; }
    Vertex get_v2() { return v2; }
    Vertex get_v3() { return v3; }

private:
    Vertex v1, v2, v3;
};

class Trajectory
{
public:
    Trajectory(vector<Vertex> _R) : R(_R)
    {
        path = calculatePath();
    }

    vector<Vertex> get_path() { return path; }

    vector<Vertex> get_R() { return R; }

    //raèuna Pi(t)
    // Pi(t) = T * 1/6 * B * R
    Vertex calculateP(double t, int i)
    {
        glm::vec4 T_vec = glm::vec4(pow(t, 3), pow(t, 2), t, 1);
        glm::mat3x4 R_mat = glm::mat3x4(
            glm::vec4(R[i - 1].get_x(), R[i].get_x(), R[i + 1].get_x(), R[i + 2].get_x()),
            glm::vec4(R[i - 1].get_y(), R[i].get_y(), R[i + 1].get_y(), R[i + 2].get_y()),
            glm::vec4(R[i - 1].get_z(), R[i].get_z(), R[i + 1].get_z(), R[i + 2].get_z()));
        glm::vec3 p = T_vec * 0.1666666f * B * R_mat;
        return Vertex(p.x, p.y, p.z);
    }

    //raèuna Pi(t)'
    // Pi(t)' = T * 1/2 * B' * R
    Vertex calculateDp(double t, int i)
    {
        glm::vec3 T_vec = glm::vec3(pow(t, 2), t, 1);
        glm::mat3x4 R_mat = glm::mat3x4(
            glm::vec4(R[i - 1].get_x(), R[i].get_x(), R[i + 1].get_x(), R[i + 2].get_x()),
            glm::vec4(R[i - 1].get_y(), R[i].get_y(), R[i + 1].get_y(), R[i + 2].get_y()),
            glm::vec4(R[i - 1].get_z(), R[i].get_z(), R[i + 1].get_z(), R[i + 2].get_z()));
        glm::vec3 dp = T_vec * 0.5f * dB * R_mat;
        return Vertex(dp.x, dp.y, dp.z);
    }

    //raèuna Pi(t)''
    // Pi(t)'' = T * B'' * R
    Vertex calculateDDp(double t, int i)
    {
        glm::vec2 T_vec = glm::vec2(t, 1);
        glm::mat3x4 R_vec = glm::mat3x4(
            glm::vec4(R[i - 1].get_x(), R[i].get_x(), R[i + 1].get_x(), R[i + 2].get_x()),
            glm::vec4(R[i - 1].get_y(), R[i].get_y(), R[i + 1].get_y(), R[i + 2].get_y()),
            glm::vec4(R[i - 1].get_z(), R[i].get_z(), R[i + 1].get_z(), R[i + 2].get_z()));
        glm::vec3 ddp = T_vec * ddB * R_vec;
        return Vertex(ddp.x, ddp.y, ddp.z);
    }

private:
    vector<Vertex> calculatePath()
    {
        vector<Vertex> path;
        for (int i = 1; i < R.size() - 2; i++)
        {
            int counter = 0;
            for (double t = .0; t < 1.; t += .01)
            {
                path.push_back(calculateP(t, i));
                counter++;
            }
        }
        return path;
    }

    vector<Vertex> R;
    vector<Vertex> path;
    glm::mat4 B = glm::mat4(
        glm::vec4(-1, 3, -3, 1),
        glm::vec4(3, -6, 0, 4),
        glm::vec4(-3, 3, 3, 1),
        glm::vec4(1, 0, 0, 0));
    glm::mat4x3 dB = glm::mat4x3(
        glm::vec3(-1, 2, -1),
        glm::vec3(3, -4, 0),
        glm::vec3(-3, 2, 1),
        glm::vec3(1, 0, 0));
    glm::mat4x2 ddB = glm::mat4x2(
        glm::vec2(-1, 1),
        glm::vec2(3, -2),
        glm::vec2(-3, 1),
        glm::vec2(1, 0));
};

//*********************************************************************************
//  vertices - lista svih uèitanih vrhova/toèaka
//  polygons - lista svih uèitanih poligona
//  trajectory - pokazivaè na putanju animacije
//*********************************************************************************

vector<Vertex> vertices;
vector<Polygon> polygons;

Trajectory* trajectory;

//služe za usporedbu kako bi se odredila brzina animacije
double current_time = 0.0;
double previous_time = 0.0;

//brzina animacije (veæi broj oznaèava sporiju animaciju)
int speed = 1;

//  t - vrijeme trajanja segmenta (0 <= t <= 1)
//  i - trenutni segment koji se izvodi
double t = 0.;
int i = 1;

//središte objekta koji se animira
glm::vec3 center;

//zastavica aktivne animacije
bool is_stopped = false;

//zastavica metode rotacije (glm/dcm)
bool is_dcm = true;

//*********************************************************************************
//	Pokazivac na glavni prozor i pocetna velicina.
//*********************************************************************************

GLuint window;
GLuint width = 700, height = 500;

//*********************************************************************************
//	Function Prototypes.
//*********************************************************************************

void myDisplay();
void myIdle();
void myKeyboard(unsigned char theKey, int mouseX, int mouseY);
void readObjFile(string filePath);
void readTrajectoryFile(string filePath);
void drawObject(glm::mat3 R);
void drawTrajectory();

//*********************************************************************************
//	Glavni program.
//*********************************************************************************

int main(int argc, char** argv)
{
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(100, 100);
    glutInit(&argc, argv);

    window = glutCreateWindow("Animacija");
    if (is_dcm)
        gluLookAt(0, 0.5, 1, 0, 20, 50, 0, 5, 5);
    else
    {
        glMatrixMode(GL_PROJECTION);
    }
    glutDisplayFunc(myDisplay);
    glutKeyboardFunc(myKeyboard);

    readObjFile("747.obj");
    readTrajectoryFile("putanja.txt");

    glutIdleFunc(myIdle);
    glutMainLoop();
    return 0;
}

void myIdle()
{
    if (is_stopped)
    {
        return;
    }
    current_time+=0.01f;
    double diff = current_time - previous_time;
    if (diff > speed)
    {
        myDisplay();
        current_time = 0;
        cout << "sad" << endl;
    }
}

//*********************************************************************************
//	Osvjezavanje prikaza. (nakon preklapanja prozora)
//*********************************************************************************

void myDisplay()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(.58, .76, .8, 1);

    drawTrajectory();

    //tangenta (y os)
    glColor3f(.0, 1., .0);
    Vertex tangent = trajectory->calculateDp(t, i);
    glm::vec3 tangent_vec = glm::normalize(glm::vec3(tangent.get_x(), tangent.get_y(), tangent.get_z()));
    Vertex tangent_start = trajectory->calculateP(t, i);
    glm::vec3 tangent_start_vec = glm::vec3(tangent_start.get_x(), tangent_start.get_y(), tangent_start.get_z());
    glm::vec3 tangent_end_vec = tangent_start_vec + 5.f * tangent_vec;
    glBegin(GL_LINES);
    glVertex3f(tangent_start_vec.x / 25.f, tangent_start_vec.y / 25.f, tangent_start_vec.z / 25.f);
    glVertex3f(tangent_end_vec.x / 25.f, tangent_end_vec.y / 25.f, tangent_end_vec.z / 25.f);
    glEnd();

    //normala (z os)
    glColor3f(.0, .0, 1.0);
    Vertex ddp = trajectory->calculateDDp(t, i);
    glm::vec3 normal_vec = glm::normalize(glm::cross(tangent_vec, glm::vec3(ddp.get_x(), ddp.get_y(), ddp.get_z())));
    Vertex normal_start = trajectory->calculateP(t, i);
    glm::vec3 normal_start_vec = glm::vec3(normal_start.get_x(), normal_start.get_y(), normal_start.get_z());
    glm::vec3 normal_end_vec = normal_start_vec + 5.f * normal_vec;
    glBegin(GL_LINES);
    glVertex3f(normal_start_vec.x / 25.f, normal_start_vec.y / 25.f, normal_start_vec.z / 25.f);
    glVertex3f(normal_end_vec.x / 25.f, normal_end_vec.y / 25.f, normal_end_vec.z / 25.f);
    glEnd();

    //binormala (x os)
    glColor3f(1., .0, .0);
    glm::vec3 binormal_vec = glm::normalize(glm::cross(tangent_vec, normal_vec));
    Vertex binormal_start = trajectory->calculateP(t, i);
    glm::vec3 binormal_start_vec = glm::vec3(binormal_start.get_x(), binormal_start.get_y(), binormal_start.get_z());
    glm::vec3 binormal_end_vec = binormal_start_vec + 5.f * binormal_vec;
    glBegin(GL_LINES);
    glVertex3f(binormal_start_vec.x / 25.f, binormal_start_vec.y / 25.f, binormal_start_vec.z / 25.f);
    glVertex3f(binormal_end_vec.x / 25.f, binormal_end_vec.y / 25.f, binormal_end_vec.z / 25.f);
    glEnd();

    //rotacijska matrica (DCM)
    glm::mat3 rotation_mat = glm::mat3(tangent_vec, normal_vec, binormal_vec);

    drawObject(rotation_mat);

    t += 0.1;
    if (t >= 1.)
    {
        i++;
        t = 0.1;
    }

    if (i >= trajectory->get_R().size()-2)
    {
        i = 1;
        t = 0.;
    }

    glFlush();
}

//*********************************************************************************
//	Tastatura tipke.
//*********************************************************************************

void myKeyboard(unsigned char theKey, int mouseX, int mouseY)
{
    switch (theKey)
    {
    case 'w':
        cout << "Gasenje programa..." << endl;
        exit(1);
        break;
    case ' ':
        is_stopped = !is_stopped;
        if (is_stopped)
            cout << "Zaustavljanje animacije..." << endl;
        else
            cout << "Pokretanje animacije..." << endl;
        break;
    case 'i':
        if (speed - 10000 <= 0) {
            if (speed - 1000 <= 0) {
                if (speed - 5 <= 0) {
                    if (speed - 0.0001 <= 0) {
                        break;
                    }
                    speed = 0.0001f;
                    break;
                }
                speed -= 5;
                break;
            }
            speed -= 1000;
            break;
        }
        speed -= 10000;
        break;
    case 'd':
        speed += 10000;
        break;

    default:
        break;
    }
    cout << speed << endl;
}

void readTrajectoryFile(string filePath)
{
    ifstream file(filePath);
    string line;
    if (!file)
    {
        cerr << "Problem prilikom uèitavanja datoteke " << filePath << endl;
        exit(1);
    }
    vector<Vertex> path;
    while (getline(file, line))
    {
        double x, y, z;
        istringstream iss(line);
        iss >> x >> y >> z;
        Vertex v(x, y, z);
        path.push_back(v);
    }
    trajectory = new Trajectory(path);
}

void readObjFile(string filePath)
{
    ifstream file(filePath);
    string line;
    if (!file)
    {
        cerr << "Problem prilikom uèitavanja datoteke " << filePath << endl;
        exit(1);
    }
    center = glm::vec3(0, 0, 0);
    while (getline(file, line))
    {
        // linija poèinje s v => vrh
        if (line.compare(0, 1, "v") == 0)
        {
            double x, y, z;
            char c;
            istringstream iss(line);
            iss >> c >> x >> y >> z;
            Vertex v(x, y, z);
            vertices.push_back(v);
        }
        // linija poèinje s f => poligon
        else if (line.compare(0, 1, "f") == 0)
        {
            int indexV1, indexV2, indexV3;
            char c;
            istringstream iss(line);
            iss >> c >> indexV1 >> indexV2 >> indexV3;
            Vertex v1 = vertices[indexV1 - 1];
            Vertex v2 = vertices[indexV2 - 1];
            Vertex v3 = vertices[indexV3 - 1];

            glm::vec3 v1_vec = glm::vec3(v1.get_x(), v1.get_y(), v1.get_z());
            glm::vec3 v2_vec = glm::vec3(v2.get_x(), v2.get_y(), v2.get_z());
            glm::vec3 v3_vec = glm::vec3(v3.get_x(), v3.get_y(), v3.get_z());
            center += v1_vec + v2_vec + v3_vec;

            glm::mat4 rotation = glm::mat4(
                glm::vec4(cos(-80), 0, sin(-80), 0),
                glm::vec4(0, 1, 0, 0),
                glm::vec4(-sin(-80), 0, cos(-80), 0),
                glm::vec4(0, 0, 0, 1));
            glm::vec4 p1 = glm::vec4(v1_vec.x, v1_vec.y, v1_vec.z, 1) * rotation;
            glm::vec4 p2 = glm::vec4(v2_vec.x, v2_vec.y, v2_vec.z, 1) * rotation;
            glm::vec4 p3 = glm::vec4(v3_vec.x, v3_vec.y, v3_vec.z, 1) * rotation;

            Vertex v1_new(p1.x, p1.y, p1.z);
            Vertex v2_new(p2.x, p2.y, p2.z);
            Vertex v3_new(p3.x, p3.y, p3.z);
            Polygon p(v1_new, v2_new, v3_new);
            polygons.push_back(p);
        }
        else
        {
            continue;
        }
    }
    center /= polygons.size();
}

void drawTrajectory()
{
    glBegin(GL_POINTS);
    for (auto& p : trajectory->get_path())
    {
        glColor3f(0.0f, 0.0f, 0.0f);
        glVertex3f(p.get_x() / 25.f, p.get_y() / 25.f, p.get_z() / 25.f);
    }
    glEnd();
}

void drawObject(glm::mat3 R)
{
    if (!is_dcm)
    {
        glm::vec3 tangent_vec = glm::normalize(glm::column(R, 0));
        glm::vec3 start_vec = glm::normalize(glm::vec3(0, 0.89, 0));
        glm::vec3 os = glm::normalize(glm::cross(start_vec, tangent_vec));
        double fi = acos(glm::dot(start_vec, tangent_vec));
        glRotatef((fi * 180) / (atan(1) * 4), os.x, os.y, os.z);
    }

    glColor3f(.99, .42, .0);
    glBegin(GL_TRIANGLES);
    for (auto& p : polygons)
    {
        Vertex position = trajectory->calculateP(t, i);
        glm::vec3 position_vec = glm::vec3(position.get_x(), position.get_y(), position.get_z());

        Vertex v1 = p.get_v1();
        glm::vec3 v1_vec = glm::vec3(v1.get_x(), v1.get_y(), v1.get_z());
        glm::vec3 v1_final = (v1_vec - center) / 5.f + position_vec / 25.f;
        if (is_dcm)
            v1_final = ((v1_vec - center) * glm::inverse(R)) / 5.f + position_vec / 25.f;

        Vertex v2 = p.get_v2();
        glm::vec3 v2_vec = glm::vec3(v2.get_x(), v2.get_y(), v2.get_z());
        glm::vec3 v2_final = (v2_vec - center) / 5.f + position_vec / 25.f;
        if (is_dcm)
            v2_final = ((v2_vec - center) * glm::inverse(R)) / 5.f + position_vec / 25.f;

        Vertex v3 = p.get_v3();
        glm::vec3 v3_vec = glm::vec3(v3.get_x(), v3.get_y(), v3.get_z());
        glm::vec3 v3_final = (v3_vec - center) / 5.f + position_vec / 25.f;
        if (is_dcm)
            v3_final = ((v3_vec - center) * glm::inverse(R)) / 5.f + position_vec / 25.f;

        glVertex3f(v1_final.x, v1_final.y, v1_final.z);
        glVertex3f(v2_final.x, v2_final.y, v2_final.z);
        glVertex3f(v3_final.x, v3_final.y, v3_final.z);
    }
    glEnd();
}