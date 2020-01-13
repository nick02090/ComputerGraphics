#include <cstdio>
#include <glm/glm.hpp>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <glut.h>
using namespace std;

//*********************************************************************************
//  Razredi potrebni za iscrtavanje tijela.
//  Vertex - predstavlja vrh/toèku u prostoru s koordinatama x, y, z
//  Polygon - predstavlja poligon u prostoru s vrhovima v1, v2, v3
//  Material - predstavlja materijal pojedinog tijela
//  Object3D - predstavlja objekt unutar 3D scene
//*********************************************************************************

class Vertex
{
public:
    Vertex(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}

    glm::vec3 get() { return glm::vec3(x, y, z); }
    double x, y, z;
};

class MyPolygon
{
public:
    MyPolygon(Vertex _v1, Vertex _v2, Vertex _v3) : v1(_v1), v2(_v2), v3(_v3) {}

    Vertex v1, v2, v3;
    glm::vec3 normal()
    {
        glm::vec3 edge1 = v2.get() - v1.get();
        glm::vec3 edge2 = v3.get() - v1.get();
        glm::vec3 normal = glm::cross(edge1, edge2);
        return glm::normalize(normal);
    }
};

class Material
{
public:
    Material(string _name) : name(_name) {}

    string name;  //naziv
    glm::vec3 Ka; //ambient
    glm::vec3 Kd; //difuse
    glm::vec3 Ks; //specular
    double d;     //roughness
};

class Object3D
{
public:
    Object3D(string _name) : name(_name) {}

    string name;
    vector<MyPolygon> polygons;
    Material* material;
};

class Particle
{
public:
    Particle(Vertex _position, glm::vec3 _color, double _size) : position(_position), color(_color), size(_size)
    {
        age = 1.0;
        angle = 0;
        axis = glm::vec3(0.0, 0.0, 0.0);
    };
    virtual void render() = 0;

    Vertex position;
    glm::vec3 color;
    glm::vec3 axis;
    double angle;
    double size;
    double age;
};

class FireParticle : public Particle
{
public:
    FireParticle(Vertex _position, glm::vec3 _color, double _size) : Particle(_position, _color, _size)
    {
        b_grey = false;
    }

    void render()
    {
        glColor3f(color.x, color.y, color.z);
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glRotatef(angle, axis.x, axis.y, axis.z);
        glutSolidSphere(size, 5, 5);
        glPopMatrix();
    }

    bool b_grey;
};

class SnowParticle : public Particle
{
public:
    SnowParticle(Vertex _position, glm::vec3 _color, double _size) : Particle(_position, _color, _size) {}

    void render()
    {
        glColor3f(color.x, color.y, color.z);
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glRotatef(angle, axis.x, axis.y, axis.z);
        glutSolidSphere(size, 5, 5);
        glPopMatrix();
    }
};

//*********************************************************************************
//  Globalne varijable
//*********************************************************************************

vector<Vertex> vertices;
vector<Vertex> normals;
vector<glm::vec2> textures;
vector<Object3D> objects;
vector<Material*> materials;
vector<FireParticle> fire;
vector<SnowParticle> snow;

//*********************************************************************************
//	Pokazivac na glavni prozor i pocetna velicina.
//*********************************************************************************

static const string sceneFilePath = "woods.obj";
static const string materialFilePath = "woods.mtl";

GLuint window;
GLuint width = 927, height = 522;

Vertex O(-8.0, 4.7, -4.2);
Vertex I(-4.6, -27.2, -10.7);

glm::vec3 light = glm::vec3(1.0, 1.0, 1.0);
glm::vec3 ambientLight = glm::vec3(0.5, 0.5, 0.5);

bool b_renderSnow = false;
Vertex fire_starting_point = Vertex(-1.75, 0.2, -1.0);

double snow_polygon_size = 6;
double snow_polygon_height = 5.0;
Vertex snow_v1 = Vertex(fire_starting_point.x + snow_polygon_size, snow_polygon_height, fire_starting_point.z + snow_polygon_size);
Vertex snow_v2 = Vertex(fire_starting_point.x + snow_polygon_size, snow_polygon_height, fire_starting_point.z - snow_polygon_size);
Vertex snow_v3 = Vertex(fire_starting_point.x - snow_polygon_size, snow_polygon_height, fire_starting_point.z - snow_polygon_size);
Vertex snow_v4 = Vertex(fire_starting_point.x - snow_polygon_size, snow_polygon_height, fire_starting_point.z + snow_polygon_size);
glm::vec3 snow_color = glm::vec3(1.0, 0.98, 0.98);
int number_of_snow_particles = 10;

//*********************************************************************************
//	Function Prototypes.
//*********************************************************************************

void myDisplay();
void myIdle();
void myKeyboard(unsigned char theKey, int mouseX, int mouseY);
void readSceneFile(string filePath);
void readMaterialFile(string filePath);
void myReshape(int width, int height);
void updatePerspective();
void renderObjects();
void renderObject(Object3D object);
void renderFire();
void fireParticles();
void renderSnow();
void snowParticles();
double clamp(double x, double lower, double upper);
double f();

//*********************************************************************************
//	Glavni program.
//*********************************************************************************

int main(int argc, char** argv)
{
    srand(time(NULL));

    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(100, 100);
    glutInit(&argc, argv);

    window = glutCreateWindow("Snow/Fire");
    glutDisplayFunc(myDisplay);
    glutKeyboardFunc(myKeyboard);
    glutReshapeFunc(myReshape);

    readMaterialFile(materialFilePath);
    readSceneFile(sceneFilePath);

    glutIdleFunc(myIdle);
    glutMainLoop();
    return 0;
}

//*********************************************************************************
//	Osvjezavanje prikaza. (nakon preklapanja prozora)
//*********************************************************************************

void myDisplay()
{
    glClearColor(+0.06f, +0.084f, +0.11f, +1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderObjects();

    /*glColor3f(1.0, 0.98, 0.98);
    glBegin(GL_POLYGON);
    glVertex3f(snow_v1.x, snow_v1.y, snow_v1.z);
    glVertex3f(snow_v2.x, snow_v2.y, snow_v2.z);
    glVertex3f(snow_v3.x, snow_v3.y, snow_v3.z);
    glVertex3f(snow_v4.x, snow_v4.y, snow_v4.z);
    glEnd();*/

    renderFire();
    renderSnow();

    glutSwapBuffers();
}

int current_time = 0;
int previous_time = 0;

void myIdle()
{
    current_time = glutGet(GLUT_ELAPSED_TIME);
    int diff = current_time - previous_time;
    if (diff > 100)
    {
        fireParticles();

        snowParticles();

        myDisplay();
        previous_time = current_time;
    }
}

// random double generator
double f()
{
    return (double)rand() / RAND_MAX;
}

void fireParticles()
{
    // create new particle
    Vertex position = fire_starting_point;
    glm::vec3 color = glm::vec3(1.0, 1.0, 0.0);
    double size = 0.05;
    FireParticle fp(position, color, size);
    // cout << "dodajem novu" << endl;
    fire.push_back(fp);

    // edit existing particles
    for (int i = fire.size() - 1; i >= 0; i--)
    {
        // transition
        Vertex current_position = fire.at(i).position;
        double x = -0.05 + f() * (0.05 + 0.05);
        double z = -0.05 + f() * (0.05 + 0.05);
        fire.at(i).position = Vertex(current_position.x + x, clamp(current_position.y + 0.01, 0.2, 1.2), current_position.z + z);

        // rotation
        double x_axis = 0.0 + f() * (1.0 - 0.0);
        double y_axis = 0.0 + f() * (1.0 - 0.0);
        double z_axis = 0.0 + f() * (1.0 - 0.0);
        fire.at(i).axis = glm::vec3(x_axis, y_axis, z_axis);
        double angle = 0.0 + f() * (360.0 - 0.0);
        fire.at(i).angle = angle;

        // coloring
        Vertex new_position = fire.at(i).position;
        // if too away from start
        if (abs(abs(new_position.x) - abs(fire_starting_point.x)) >= 0.5 || abs(abs(new_position.z) - abs(fire_starting_point.z)) >= 0.5)
        {
            // if already grey -> lower color brigthness near to black color
            if (fire.at(i).b_grey)
            {
                glm::vec3 current_color = fire.at(i).color;
                fire.at(i).color = glm::vec3(current_color.x - 0.04, current_color.x - 0.04, current_color.x - 0.04);
            }
            // else -> set initial grey color
            else
            {
                fire.at(i).b_grey = true;
                fire.at(i).color = glm::vec3(0.3689, 0.3689, 0.3689);
            }
            // if too far away -> kill
            if (abs(abs(new_position.x) - abs(fire_starting_point.x)) >= 0.7 || abs(abs(new_position.z) - abs(fire_starting_point.z)) >= 0.7)
            {
                fire.erase(fire.begin() + i);
                continue;
            }
        }
        // else -> lower color to specific brightness
        else
        {
            // was too far away, now in range again -> turn black
            if (fire.at(i).b_grey)
            {
                fire.at(i).color = glm::vec3(0.0, 0.0, 0.0);
            }
            glm::vec3 current_color = fire.at(i).color;
            fire.at(i).color = glm::vec3(current_color.x, clamp(current_color.y - 0.01, 0.0, 1.0), current_color.z);
        }

        // killing
        fire.at(i).age -= 0.005;
        if (fire.at(i).age <= 0.0)
        {
            // cout << "brisem" << endl;
            fire.erase(fire.begin() + i);
        }
        Vertex fp_position = fire.at(i).position;
        double fp_radius = fire.at(i).size;
        for (auto& sp : snow)
        {
            Vertex sp_position = sp.position;
            double sp_radius = sp.size;
            double distance = sqrt(pow(sp_position.x - fp_position.x, 2) + pow(sp_position.y - fp_position.y, 2) + pow(sp_position.z - fp_position.z, 2));
            if (distance <= fp_radius + sp_radius)
            {
                fire.erase(fire.begin() + i);
            }
        }
    }
}

void snowParticles()
{
    // create new particles
    if (b_renderSnow)
    {
        int n = 0 + (rand() % (number_of_snow_particles - 0 + 1));
        for (int i = 0; i < n; i++)
        {
            double x_min = snow_v4.x;
            double x_max = snow_v1.x;
            double x = x_min + f() * (x_max - x_min);
            double z_min = snow_v3.z;
            double z_max = snow_v4.z;
            double z = z_min + f() * (z_max - z_min);
            Vertex position = Vertex(x, snow_polygon_height, z);
            double size_min = 0.001;
            double size_max = 0.05;
            double size = size_min + f() * (size_max - size_min);
            SnowParticle sp(position, snow_color, size);
            // cout << "dodajem novu" << endl;
            snow.push_back(sp);
        }
    }

    // edit existing particles
    for (int i = snow.size() - 1; i >= 0; i--)
    {
        // transition
        Vertex current_position = snow.at(i).position;
        double x = -0.001 + f() * (0.001 + 0.001);
        double z = -0.001 + f() * (0.001 + 0.001);
        snow.at(i).position = Vertex(current_position.x + x, clamp(current_position.y - 0.05, 0.0, snow_polygon_height), current_position.z + z);

        // sizing
        if (snow.at(i).position.y <= 1.5)
        {
            double size_min = 0.01;
            double size_max = snow.at(i).size;
            snow.at(i).size = size_min + f() * (size_max - size_min);
        }

        // rotation
        double x_axis = 0.0 + f() * (1.0 - 0.0);
        double y_axis = 0.0 + f() * (1.0 - 0.0);
        double z_axis = 0.0 + f() * (1.0 - 0.0);
        snow.at(i).axis = glm::vec3(x_axis, y_axis, z_axis);
        double angle = 0.0 + f() * (360.0 - 0.0);
        snow.at(i).angle = angle;

        // killing
        snow.at(i).age -= 0.005;
        if (snow.at(i).age <= 0.0)
        {
            // cout << "brisem" << endl;
            snow.erase(snow.begin() + i);
        }
    }
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
    gluLookAt(O.x, O.y, O.z, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0); // O x,y,z; glediste x,y,z; up vektor x,y,z
}

//*********************************************************************************
//	Tastatura tipke.
//*********************************************************************************

void myKeyboard(unsigned char theKey, int mouseX, int mouseY)
{
    switch (theKey)
    {
    case 27:
        cout << "Gasenje programa..." << endl;
        exit(1);
        break;
    case 'l':
        O.x = O.x + 0.1;
        break;

    case 'k':
        O.x = O.x - 0.1;
        break;

    case 'i':
        O.y = O.y + 0.1;
        break;

    case 'o':
        O.y = O.y - 0.1;
        break;

    case 'n':
        O.z = O.z + 0.1;
        break;

    case 'm':
        O.z = O.z - 0.1;
        break;

    case 'r':
        O.x = -8.0;
        O.y = 4.7;
        O.z = -4.2;
        break;

    case 'q':
        I.x = I.x + 0.1;
        break;

    case 'w':
        I.x = I.x - 0.1;
        break;

    case 'a':
        I.y = I.y + 0.1;
        break;

    case 's':
        I.y = I.y - 0.1;
        break;

    case 'y':
        I.z = I.z + 0.1;
        break;

    case 'x':
        I.z = I.z - 0.1;
        break;

    case 'e':
        I.x = -4.6;
        I.y = -27.2;
        I.z = -10.7;
        break;

    case 'g':
        b_renderSnow = !b_renderSnow;
        if (b_renderSnow)
        {
            cout << "Crtanje snijega..." << endl;
        }
        else
        {
            cout << "Prestanak snijega..." << endl;
        }
        break;

    case '+':
        if (b_renderSnow && number_of_snow_particles < 100)
        {
            number_of_snow_particles += 10;
        }
        break;

    case '-':
        if (b_renderSnow && number_of_snow_particles > 10)
        {
            number_of_snow_particles -= 10;
        }
        break;

    default:
        break;
    }
    // cout << O.x << " " << O.y << " " << O.z << endl;
    // cout << I.x << " " << I.y << " " << I.z << endl;
    updatePerspective();
    glutPostRedisplay();
}

void readSceneFile(string filePath)
{
    ifstream file(filePath);
    string line;
    string previous_line = "NULL";
    if (!file)
    {
        cerr << "Problem prilikom uèitavanja datoteke " << filePath << endl;
        exit(1);
    }
    while (true)
    {
        if (line.compare(0, 1, "") == 0 && previous_line.compare(0, 1, "o") == 0)
        {
            break;
        }
        if (previous_line.compare("NULL") == 0)
        {
            if (!getline(file, line))
            {
                break;
            }
        }
        else
        {
            line = previous_line;
        }
        //linija poèinje s o => novi objekt
        if (line.compare(0, 1, "o") == 0)
        {
            string name;
            char c;
            istringstream iss(line);
            iss >> c >> name;
            Object3D object(name);
            vector<MyPolygon> obj_polygons;
            while (getline(file, line))
            {
                // linija poèinje s vn -> normala vrha
                if (line.compare(0, 2, "vn") == 0)
                {
                    double x, y, z;
                    string vn;
                    istringstream iss(line);
                    iss >> vn >> x >> y >> z;
                    Vertex v(x, y, z);
                    normals.push_back(v);
                }
                // linija poèinje s vt -> textura vrha
                else if (line.compare(0, 2, "vt") == 0)
                {
                    double x, y;
                    string vt;
                    istringstream iss(line);
                    iss >> vt >> x >> y;
                    glm::vec2 v(x, y);
                    textures.push_back(v);
                }
                // linija poèinje s v => vrh
                else if (line.compare(0, 1, "v") == 0)
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
                    string V1, V2, V3;
                    char c;
                    istringstream iss(line);
                    iss >> c >> V1 >> V2 >> V3;
                    int indexV1 = stoi(V1.substr(0, V1.find("//")));
                    int indexV2 = stoi(V2.substr(0, V2.find("//")));
                    int indexV3 = stoi(V3.substr(0, V3.find("//")));
                    Vertex v1 = vertices[indexV1 - 1];
                    Vertex v2 = vertices[indexV2 - 1];
                    Vertex v3 = vertices[indexV3 - 1];
                    MyPolygon p(v1, v2, v3);
                    obj_polygons.push_back(p);
                }
                // linija poèinje s usemtl => naziv materijala
                else if (line.compare(0, 6, "usemtl") == 0)
                {
                    string name;
                    string usemtl;
                    istringstream iss(line);
                    iss >> usemtl >> name;
                    auto it = find_if(materials.begin(), materials.end(), [&name](const Material* obj) { return obj->name == name; });

                    if (it != materials.end())
                    {
                        // found element. it is an iterator to the first matching element.
                        // if you really need the index, you can also get it:
                        auto index = distance(materials.begin(), it);
                        object.material = materials[index];
                    }
                }
                // linija poèinje sa o => poèetak novog objekta
                else if (line.compare(0, 1, "o") == 0)
                {
                    previous_line = line;
                    break;
                }
                else
                {
                    continue;
                }
            }
            object.polygons = obj_polygons;
            objects.push_back(object);
        }
    }
    // for (auto &obj : objects)
    // {
    //     cout << obj.name << endl;
    //     cout << obj.material->name << endl;
    //     cout << endl;
    // }
}

void readMaterialFile(string filePath)
{
    ifstream file(filePath);
    string line;
    if (!file)
    {
        cerr << "Problem prilikom uèitavanja datoteke " << filePath << endl;
        exit(1);
    }
    while (getline(file, line))
    {
        //linija poèinje s newmtl => novi materijal
        if (line.compare(0, 6, "newmtl") == 0)
        {
            string name;
            string newmtl;
            istringstream iss(line);
            iss >> newmtl >> name;
            Material* material = new Material(name);
            while (getline(file, line))
            {
                // linija poèinje s Ka => ambijentalna komponenta
                if (line.compare(0, 2, "Ka") == 0)
                {
                    double x, y, z;
                    string Ka;
                    istringstream iss(line);
                    iss >> Ka >> x >> y >> z;
                    glm::vec3 ka(x, y, z);
                    material->Ka = ka;
                }
                // linija poèinje s Kd => difuzna komponenta
                else if (line.compare(0, 2, "Kd") == 0)
                {
                    double x, y, z;
                    string Kd;
                    istringstream iss(line);
                    iss >> Kd >> x >> y >> z;
                    glm::vec3 kd(x, y, z);
                    material->Kd = kd;
                }
                // linija poèinje s Ks => spekularna komponenta
                else if (line.compare(0, 2, "Ks") == 0)
                {
                    double x, y, z;
                    string Ks;
                    istringstream iss(line);
                    iss >> Ks >> x >> y >> z;
                    glm::vec3 ks(x, y, z);
                    material->Ks = ks;
                }
                // linija poèinje s d => indeks hrapavosti
                else if (line.compare(0, 1, "d") == 0)
                {
                    double n;
                    char d;
                    istringstream iss(line);
                    iss >> d >> n;
                    material->d = n;
                }
                // linija poèinje s illum => kraj opisa objekta
                else if (line.compare(0, 5, "illum") == 0)
                {
                    break;
                }
                else
                {
                    continue;
                }
            }
            materials.push_back(material);
        }
    }
    // for (auto &mat : materials)
    // {
    //     cout << mat->name << endl;
    //     cout << "Ka: " << mat->Ka.x << " " << mat->Ka.y << " " << mat->Ka.z << endl;
    //     cout << "Kd: " << mat->Kd.x << " " << mat->Kd.y << " " << mat->Kd.z << endl;
    //     cout << "Ks: " << mat->Ks.x << " " << mat->Ks.y << " " << mat->Ks.z << endl;
    //     cout << "d: " << mat->d << endl;
    //     cout << endl;
    // }
}

void renderObjects()
{
    for (auto& obj : objects)
    {
        renderObject(obj);
    }
}

double clamp(double x, double lower, double upper)
{
    return min(upper, max(x, lower));
}

glm::vec3 diffuse_specular(glm::vec3 point, glm::vec3 centroid, glm::vec3 kd, glm::vec3 ks, double n)
{
    glm::vec3 color = glm::vec3(0.0, 0.0, 0.0);

    glm::vec3 L = glm::vec3(I.x - point.x, I.y - point.y, I.z - point.z);
    L = glm::normalize(L);
    glm::vec3 N = glm::vec3(point.x - centroid.x, point.y - centroid.y, point.z - centroid.z);
    N = glm::normalize(N);
    double LN = glm::dot(L, N);

    // Difuzno (Ii * kd * (L*N))
    double red = light.x * kd.x * LN;
    double green = light.y * kd.y * LN;
    double blue = light.z * kd.z * LN;
    color = color + glm::vec3(red, green, blue);

    // R = N * (2 * L*N) - L
    glm::vec3 R = glm::vec3(2 * LN * N.x, 2 * LN * N.y, 2 * LN * N.z) - L;
    R = glm::normalize(R);
    glm::vec3 V = glm::vec3(O.x - point.x, O.y - point.y, O.z - point.z);
    V = glm::normalize(V);
    double RV = glm::dot(R, V);

    // Spekularno (Ii * ks * (R*V)^n)
    red = light.x * ks.x * pow(RV, n);
    green = light.y * ks.y * pow(RV, n);
    blue = light.z * ks.z * pow(RV, n);
    color = color + glm::vec3(red, green, blue);

    return color;
}

void renderObject(Object3D object)
{
    for (auto& poly : object.polygons)
    {
        glBegin(GL_TRIANGLES);

        // Ambijentno (Ia * ka)
        glm::vec3 ka = object.material->Ka;
        double red = ambientLight.x * ka.x;
        double green = ambientLight.y * ka.y;
        double blue = ambientLight.z * ka.z;
        glm::vec3 ambient = glm::vec3(red, green, blue);

        double x = (poly.v1.x + poly.v2.x + poly.v3.x) / 3.0;
        double y = (poly.v1.y + poly.v2.y + poly.v3.y) / 3.0;
        double z = (poly.v1.z + poly.v2.z + poly.v3.z) / 3.0;
        glm::vec3 centroid = glm::vec3(x, y, z);
        glm::vec3 kd = object.material->Kd;
        glm::vec3 ks = object.material->Ks;
        double n = object.material->d;

        glm::vec3 point_1 = glm::vec3(poly.v1.x, poly.v1.y, poly.v1.z);
        glm::vec3 diffuse_1 = diffuse_specular(point_1, centroid, kd, ks, n);
        glm::vec3 color_1 = ambient + diffuse_1;
        glColor3f(clamp(color_1.x, 0.0, 1.0), clamp(color_1.y, 0.0, 1.0), clamp(color_1.z, 0.0, 1.0));
        glVertex3f(poly.v1.x, poly.v1.y, poly.v1.z);

        glm::vec3 point_2 = glm::vec3(poly.v2.x, poly.v2.y, poly.v2.z);
        glm::vec3 diffuse_2 = diffuse_specular(point_2, centroid, kd, ks, n);
        glm::vec3 color_2 = ambient + diffuse_2;
        glColor3f(clamp(color_2.x, 0.0, 1.0), clamp(color_2.y, 0.0, 1.0), clamp(color_2.z, 0.0, 1.0));
        glVertex3f(poly.v2.x, poly.v2.y, poly.v2.z);

        glm::vec3 point_3 = glm::vec3(poly.v3.x, poly.v3.y, poly.v3.z);
        glm::vec3 diffuse_3 = diffuse_specular(point_3, centroid, kd, ks, n);
        glm::vec3 color_3 = ambient + diffuse_3;
        glColor3f(clamp(color_3.x, 0.0, 1.0), clamp(color_3.y, 0.0, 1.0), clamp(color_3.z, 0.0, 1.0));
        glVertex3f(poly.v3.x, poly.v3.y, poly.v3.z);
        glEnd();
    }
}

void renderFire()
{
    for (auto& particle : fire)
    {
        particle.render();
    }
}

void renderSnow()
{
    for (auto& particle : snow)
    {
        particle.render();
    }
}