#include <cstdlib>
#include <fstream>
#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <string>

using namespace std;

#define PI 3.141592
#define LINESTEP 7
#define ARCSTEP 7
#define ZMIN -20
#define ZMAX 20

int mainWindow=0;
int WIDTH=0;
int HEIGHT=0;

double currX=0;
double currY=0;
double currZ=0;
int ctr=0;
int curDrawing=0;
double progress;

void move(double x, double y, double z);
void cut(double x, double y, double z, double speed);
void cutRelative(double dx, double dy, double dz, double speed);
void cutarc(double endx, double endy, double endz, double deltaCenterX, double deltaCenterY, double deltaCenterZ, double speed, bool clockwise);
void setUnits(bool sane);
void rectangleRelative(double dx2, double dy2, double dz2, double dx3, double dy3, double dz3, double dx4, double dy4, double dz4, double speed);
void rectangleAbsolute(double x2, double y2, double z2, double x3, double y3, double z3, double x4, double y4, double z4, double speed); 
void stepwiseDepthRectangle(double x2,double y2, double x3, double y3, double x4, double y4, double zstep, int numsteps, double speed);
void stepwiseDepthRectangleRelative(double x2,double y2, double x3, double y3, double x4, double y4, double zstep, int numsteps, double speed);
double getArcAngleRad(double startx, double starty, double startz, double endx, double endy, double endz, double deltaCenterX, double deltaCenterY, double deltaCenterZ, bool clockwise);

void GCodeProgram();

class point3d
{
public:
    double x;
    double y;
    double z;
    point3d(double px, double py, double pz)
    {
        x=px;
        y=py;
        z=pz;
    }
};

class line
{
public:
    point3d start;
    point3d end;
    int when;
    bool cut;
    line(point3d s, point3d e, int w, bool CUT) : start(s), end(e)
    {
        when=w;
        cut=CUT;
    }
};

class arc
{
public:
    point3d start;
    point3d center;
    double theta;
    double endZ;
    int when;
    arc(point3d s, point3d c, double t, int w, double eZ) : start(s), center(c)
    {
        theta=t;
        when=w;
        endZ=eZ;
    }
};

vector<line> queueLines;
vector<arc> queueArcs;

class RGB
{
public:
    unsigned char r;
    unsigned char g;
    unsigned char b;
    RGB(unsigned char pr, unsigned char pg, unsigned char pb)
    {
        r=pr;
        g=pg;
        b=pb;
    }
};

struct ARGB
{
    unsigned char a;
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

void renderScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
            // Set the camera
    gluLookAt(1.0f, 1.0f, 1.0f,
              1.0f, 1.0f, 0.0f,
              0.0f, 1.0f, 0.0f);

    glutSwapBuffers();
}
void makeRect(float x, float y, float width, float height, RGB color)
{
    glColor3f((float)color.r/255.0, (float)color.g/255.0, (float)color.b/255.0);
    glBegin(GL_QUADS);
        glVertex3f((float)x*2.0/WIDTH,2.0-(float)y*2.0/HEIGHT,0);
        glVertex3f((float)(x+width)*2.0/WIDTH,2.0-(float)y*2.0/HEIGHT,0);
        glVertex3f((float)(x+width)*2.0/WIDTH,2.0-(float)(y+height)*2.0/HEIGHT,0);
        glVertex3f((float)x*2.0/WIDTH,2.0-(float)(y+height)*2.0/HEIGHT,0);
    glEnd();
}
void makeEllipse(float x, float y, float radiusx, float radiusy, RGB color)
{
    glColor3f((float)color.r/255.0, (float)color.g/255.0, (float)color.b/255.0);
    glBegin(GL_POLYGON);
        for(double i = 0; i < 2 * PI; i += PI / 48) //The 6 is the smoothness, basically. Higher number makes it smoother.
            glVertex3f((float)(cos(i)*radiusx+x)*2.0/WIDTH, 2.0-(float)(sin(i)*radiusy+y)*2.0/HEIGHT, 0.0);
    glEnd();
}
void makeArc(float x, float y, float radiusx, float radiusy, float starttheta, float theta, RGB color)
{
    glColor3f((float)color.r/255.0, (float)color.g/255.0, (float)color.b/255.0);
    glBegin(GL_LINE_STRIP);
        for(double i = starttheta; i < theta; i += PI / 48) //The 48 is the smoothness, basically. Higher number makes it smoother.
            glVertex3f((float)(cos(i)*radiusx+x)*2.0/WIDTH, 2.0-(float)(sin(i)*radiusy+y)*2.0/HEIGHT, 0.0);
    glEnd();
}
void makeArc(float x, float y, float radiusx, float radiusy, float starttheta, float theta, RGB color1, RGB color2)
{
    RGB colorDiff = RGB(color1.r-color2.r,color1.g-color2.g,color1.b-color2.b);
    glBegin(GL_LINE_STRIP);
        glColor3f((float)color1.r/255.0, (float)color1.g/255.0, (float)color1.b/255.0);
        for(double i = starttheta; i < theta; i += (theta-starttheta) / 48.0) //The 48 is the smoothness, basically. Higher number makes it smoother.
        {
            glColor3f((float)color1.r/255.0+(double)colorDiff.r*(double)(i+1)/48.0, (float)color1.g/255.0+(double)colorDiff.g*(double)(i+1)/48.0, (float)color1.b/255.0+(double)colorDiff.b*(double)(i+1)/48.0);
            glVertex3f((float)(cos(i)*radiusx+x)*2.0/WIDTH, 2.0-(float)(sin(i)*radiusy+y)*2.0/HEIGHT, 0.0);
            //cout << (float)(cos(i)*radiusx+x) << " " << (float)(sin(i)*radiusy+y) << endl;
        }
    glEnd();
}
void makeRect(float x, float y, float width, float height, ARGB color)
{
    glColor4f(color.r/255.0, color.g/255.0, color.b/255.0, color.a/255.0); //rgba
    glBegin(GL_QUADS);
        glVertex3f((float)x*2.0/WIDTH,2.0-(float)y*2.0/HEIGHT,0);
        glVertex3f((float)(x+width)*2.0/WIDTH,2.0-(float)y*2.0/HEIGHT,0);
        glVertex3f((float)(x+width)*2.0/WIDTH,2.0-(float)(y+height)*2.0/HEIGHT,0);
        glVertex3f((float)x*2.0/WIDTH,2.0-(float)(y+height)*2.0/HEIGHT,0);
    glEnd();
}
void makeEllipse(float x, float y, float radiusx, float radiusy, ARGB color)
{
    glColor4f((float)color.r/255.0, (float)color.g/255.0, (float)color.b/255.0, color.a/255.0);
    glBegin(GL_POLYGON);
        for(double i = 0; i < 2 * PI; i += PI / 48) //The 6 is the smoothness, basically. Higher number makes it smoother.
            glVertex3f((float)(cos(i)*radiusx+x)*2.0/WIDTH, 2.0-(float)(sin(i)*radiusy+y)*2.0/HEIGHT, 0.0);
    glEnd();
}
void makeArc(float x, float y, float radiusx, float radiusy, float theta, ARGB color) //radiusx is the length of the radius. so is radiusy. They can be different if you want to cut an ellipse, but I don't think GCode does that.
{
    glColor4f((float)color.r/255.0, (float)color.g/255.0, (float)color.b/255.0, color.a/255.0);
    glBegin(GL_POLYGON);
        for(double i = 0; i < theta; i += PI / 48) //The 6 is the smoothness, basically. Higher number makes it smoother.
            glVertex3f((float)(cos(i)*radiusx+x)*2.0/WIDTH, 2.0-(float)(sin(i)*radiusy+y)*2.0/HEIGHT, 0.0);
    glEnd();
}
void makeRect(float x, float y, float width, float height, RGB color, int winwidth, int winheight)
{
    glColor3f((float)color.r/255.0, (float)color.g/255.0, (float)color.b/255.0);
    glBegin(GL_QUADS);
        glVertex3f((float)x*2.0/winwidth,2.0-(float)y*2.0/winheight,0);
        glVertex3f((float)(x+width)*2.0/winwidth,2.0-(float)y*2.0/winheight,0);
        glVertex3f((float)(x+width)*2.0/winwidth,2.0-(float)(y+height)*2.0/winheight,0);
        glVertex3f((float)x*2.0/winwidth,2.0-(float)(y+height)*2.0/winheight,0);
    glEnd();
}
void drawLine(float x, float y, float fx, float fy, RGB color)
{
    glColor3f((float)color.r/255.0, (float)color.g/255.0, (float)color.b/255.0);
    glBegin(GL_LINES);
        glVertex3f((float)x*2.0/WIDTH,2.0-(float)y*2.0/HEIGHT,0);
        glVertex3f((float)fx*2.0/WIDTH,2.0-(float)fy*2.0/HEIGHT,0);
    glEnd();
}
void drawLine(float x, float y, float fx, float fy, RGB color1, RGB color2)
{
    glBegin(GL_LINES);
        glColor3f((float)color1.r/255.0, (float)color1.g/255.0, (float)color1.b/255.0);
        glVertex3f((float)x*2.0/WIDTH,2.0-(float)y*2.0/HEIGHT,0);
        glColor3f((float)color2.r/255.0, (float)color2.g/255.0, (float)color2.b/255.0);
        glVertex3f((float)fx*2.0/WIDTH,2.0-(float)fy*2.0/HEIGHT,0);
    glEnd();
}
void drawEmptyRect(float x, float y, float width, float height, RGB color)
{
    glColor3f(color.r/255.0, color.g/255.0, color.b/255.0);
    glBegin(GL_LINE_LOOP);
        glVertex3f((float)x*2.0/WIDTH,2.0-(float)y*2.0/HEIGHT,0);
        glVertex3f((float)(x+width)*2.0/WIDTH,2.0-(float)y*2.0/HEIGHT,0);
        glVertex3f((float)(x+width)*2.0/WIDTH,2.0-(float)(y+height)*2.0/HEIGHT,0);
        glVertex3f((float)x*2.0/WIDTH,2.0-(float)(y+height)*2.0/HEIGHT,0);
    glEnd();
}
void drawEmptyRect(float x, float y, float width, float height, RGB color, int winwidth, int winheight)
{
    glColor3f(color.r/255.0, color.g/255.0, color.b/255.0);
    glBegin(GL_LINE_LOOP);
        glVertex3f((float)x*2.0/winwidth,2.0-(float)y*2.0/winheight,0);
        glVertex3f((float)(x+width)*2.0/winwidth,2.0-(float)y*2.0/winheight,0);
        glVertex3f((float)(x+width)*2.0/winwidth,2.0-(float)(y+height)*2.0/winheight,0);
        glVertex3f((float)x*2.0/winwidth,2.0-(float)(y+height)*2.0/winheight,0);
    glEnd();
}

void renderBitmapString(float x,float y,float z,void *font,char *string)
{
    char *c;
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.0,0.0,0.0);
    glRasterPos3f((float)x*2.0/WIDTH,2.0-(float)y*2.0/HEIGHT,(float)z);
    for (c=string; *c != '\0'; c++)
    {
        glutBitmapCharacter(font, *c);
    }
    glEnable(GL_TEXTURE_2D);
}
void renderBitmapString(float x,float y,float z,void *font,char *string,RGB color)
{
    char *c;
    glDisable(GL_TEXTURE_2D);
    glColor3f(color.r/255.0, color.g/255.0, color.b/255.0);
    glRasterPos3f((float)x*2.0/WIDTH,2.0-(float)y*2.0/HEIGHT,(float)z);
    for (c=string; *c != '\0'; c++)
    {
            glutBitmapCharacter(font, *c);
    }
    glEnable(GL_TEXTURE_2D);
}
void renderBitmapString(float x,float y,float z,void *font,char *string, int winwidth, int winheight)
{
    char *c;
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.0,0.0,0.0);
    glRasterPos3f((float)x*2.0/winwidth,2.0-(float)y*2.0/winheight,(float)z);
    int numnew=0;
    for (c=string; *c != '\0'; c++)
    {
        if(*c=='\n')
        {
            numnew++;
            glRasterPos3f((float)x*2.0/winwidth,2.0-(float)(y+numnew*20)*2/winheight,(float)z);
        }
        glutBitmapCharacter(font, *c);
    }
    glEnable(GL_TEXTURE_2D);
}

string inttostring(int conv)
{
	string ret;
	if(conv==0)
	{
		ret="0";
		return ret;
	}
	while(conv>=1)
	{
		ret.push_back(char((conv%10)+48));
		conv/=10;
	}
	for(unsigned int i=0; i<ret.size()/2; i++)
		swap(ret[i],ret[ret.size()-1-i]);
	return ret;
}

void timerProc(int arg)
{
    glutTimerFunc(50,timerProc,0);
    
    // Reset transformations
    glLoadIdentity();
    // Set the camera
    gluLookAt(1.0f, 1.0f, 1.0f,
              1.0f, 1.0f, 0.0f,
              0.0f, 1.0f, 0.0f);
    
    RGB black(0,0,0);
    
    if(queueLines.size()>0 && queueLines[0].when==curDrawing)
    {
        double dx=queueLines[0].end.x-queueLines[0].start.x;
        double dy=queueLines[0].end.y-queueLines[0].start.y;
        double dz=queueLines[0].end.z-queueLines[0].start.z;
        double dist=sqrt(dx*dx+dy*dy+dz*dz);
        if(progress+LINESTEP<dist)
        {            
            drawLine(queueLines[0].start.x+dx*(progress/dist), queueLines[0].start.y+dy*(progress/dist), queueLines[0].start.x+dx*((progress+LINESTEP)/dist), queueLines[0].start.y+dy*((progress+LINESTEP)/dist), RGB(0,(queueLines[0].start.z+dz*(progress/dist))/(ZMAX-ZMIN)*155.0+100.0,0), RGB(0,(queueLines[0].start.z+dz*((progress+LINESTEP)/dist))/(ZMAX-ZMIN)*155.0+100.0,0));
            progress+=LINESTEP;
        }
        else
        {
            double add = dist-progress;
            drawLine(queueLines[0].start.x+dx*(progress/dist), queueLines[0].start.y+dy*(progress/dist), queueLines[0].start.x+dx*((progress+add)/dist), queueLines[0].start.y+dy*((progress+add)/dist), RGB(0,(queueLines[0].start.z+dz*(progress/dist))/(ZMAX-ZMIN)*155.0+100.0,0), RGB(0,(queueLines[0].start.z+dz*((progress+add)/dist))/(ZMAX-ZMIN)*155.0+100.0,0));
            queueLines.erase(queueLines.begin());
            curDrawing++;
            progress=0;
        }
    }
    
    
    if(queueArcs.size()>0 && queueArcs[0].when==curDrawing)
    {
        double dx=queueArcs[0].start.x-queueArcs[0].center.x;
        double dy=queueArcs[0].start.y-queueArcs[0].center.y;
        double dz=queueArcs[0].start.z-queueArcs[0].center.z;
        double rad = sqrt(dx*dx+dy*dy); //radius
        double stepTheta = ARCSTEP/rad; //s=r*theta; theta=s/r
        double dist=rad*queueArcs[0].theta; //an approximation
        double alphaFromCenter = atan(dx/dy)+progress/rad;
        if(progress+ARCSTEP<dist)
        {            
            //drawLine(queueLines[0].start.x+dx*(progress/dist), queueLines[0].start.y+dy*(progress/dist), queueLines[0].start.x+dx*((progress+LINESTEP)/dist), queueLines[0].start.y+dy*((progress+LINESTEP)/dist), RGB(0,(queueLines[0].start.z+dz*(progress/dist))/(ZMAX-ZMIN)*155.0+100.0,0), RGB(0,(queueLines[0].start.z+dz*((progress+LINESTEP)/dist))/(ZMAX-ZMIN)*155.0+100.0,0));
            makeArc(/*sin(alphaFromCenter)*rad+*/queueArcs[0].center.x, /*cos(alphaFromCenter)*rad+*/queueArcs[0].center.y,rad,rad,alphaFromCenter,alphaFromCenter+stepTheta,RGB(0,(queueArcs[0].start.z+dz*(progress/dist))/(ZMAX-ZMIN)*155.0+100.0,0), RGB(0,(queueArcs[0].start.z+dz*((progress+ARCSTEP)/dist))/(ZMAX-ZMIN)*155.0+100.0,0));
            progress+=ARCSTEP;
        }
        else
        {
            double add = dist-progress;
            stepTheta=add/rad;
            makeArc(/*sin(alphaFromCenter)*rad+*/queueArcs[0].center.x, /*cos(alphaFromCenter)*rad+*/queueArcs[0].center.y,rad,rad,alphaFromCenter,alphaFromCenter+stepTheta,RGB(0,(queueArcs[0].start.z+dz*(progress/dist))/(ZMAX-ZMIN)*155.0+100.0,0), RGB(0,(queueArcs[0].start.z+dz*((progress+add)/dist))/(ZMAX-ZMIN)*155.0+100.0,0));
            queueArcs.erase(queueArcs.begin());
            curDrawing++;
            progress=0;
        }
    }
    
    glutSwapBuffers();
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(50,30);
    glutInitWindowSize(glutGet(GLUT_SCREEN_WIDTH)-80,glutGet(GLUT_SCREEN_HEIGHT)-60);
    //glutInitWindowSize(700,700);

    mainWindow=glutCreateWindow("Test GCode");

    //glutFullScreen();
    WIDTH=glutGet(GLUT_WINDOW_WIDTH);
    HEIGHT=glutGet(GLUT_WINDOW_HEIGHT);

    glutDisplayFunc(renderScene);
    /*glutMouseFunc(processMouse);
    glutReshapeFunc(changeSize);*/
    //glutMotionFunc(processMouseMove); //this is while mouse button pressed
    //glutPassiveMotionFunc(processPassiveMouseMove); //this is while mouse button is not pressed
    //glutKeyboardFunc(processKeys);

    //initializeGameEngine();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glLineWidth(3.0);

    GCodeProgram();
    
    glutTimerFunc(50,timerProc,0);
    glutMainLoop();
    return 0;
}


void move(double x, double y, double z)
{
	if(z!=0 && (x!=0 || y!=0))
		cout << "-------------------------------------------------------" << endl << "CONFLICT: unsafe movement in xz, yz, or xyz space. Separate xy and z movement" << endl << "-------------------------------------------------------" << endl;
	cout << "G00 X" << x << " Y" << y << " Z" << z << ";" << endl;
    queueLines.push_back(line(point3d(currX,currY,currZ), point3d(x,y,z), ctr, false));
    ctr++;
	currX=x;
	currY=y;
	currZ=z;
}

void cut(double x, double y, double z, double speed)
{
	cout << "G01 X" << x << " Y" << y << " Z" << z << " F" << speed << ";" << endl;
    queueLines.push_back(line(point3d(currX,currY,currZ), point3d(x,y,z), ctr, true));
    ctr++;
	currX=x;
	currY=y;
	currZ=z;
}

void cutRelative(double dx, double dy, double dz, double speed)
{
	cout << "G01 X";
	if(dx>0)
		cout << "+" << dx;
	else
		cout << "-" << dx;
	cout << " Y";
	if(dy>0)
		cout << "+" << dy;
	else
		cout << "-" << dy;
	cout << " Z";
	if(dz>0)
		cout << "+" << dz;
	else
		cout << "-" << dz;
	cout << " F" << speed << ";" << endl;
    queueLines.push_back(line(point3d(currX,currY,currZ), point3d(currX+dx,currY+dy,currZ+dz), ctr, true));
    ctr++;
	currX+=dx;
	currY+=dy;
	currZ+=dz;
}

void cutarc(double endx, double endy, double endz, double deltaCenterX, double deltaCenterY, double deltaCenterZ, double speed, bool clockwise)
{
	if(clockwise)
		cout << "G02 ";
	else
		cout << "G03 ";
	cout << "X" << endx << " Y" << endy << " Z" << endz << " I" << deltaCenterX  << " J" << deltaCenterY << " K" << deltaCenterZ;
	/*if(deltaCenterX>0)
		cout << "+" << deltaCenterX;
	else
		cout << "-" << deltaCenterX;
	cout << " J";
	if(deltaCenterY>0)
		cout << "+" << deltaCenterY;
	else
		cout << "-" << deltaCenterY;
	cout << " K";
	if(deltaCenterZ>0)
		cout << "+" << deltaCenterZ;
	else
		cout << "-" << deltaCenterZ;*/
	cout << " F" << speed << ";" << endl;
    
    queueArcs.push_back(arc(point3d(currX,currY,currZ), point3d(currX+deltaCenterX, currY+deltaCenterY, currZ+deltaCenterZ), getArcAngleRad(currX, currY, currZ, endx, endy, endz, deltaCenterZ, deltaCenterY, deltaCenterZ,clockwise), ctr, endz));
    ctr++;
	currX=endx;
	currY=endy;
	currZ=endx;
}

void setUnits(bool sane) //sane==true --> millimeters. Else --> inches.
{
	if(sane)
		cout << "G21" << endl; //millimeters
	else
		cout << "G20" << endl; //inches
}

void rectangleRelative(double dx2, double dy2, double dz2, double dx3, double dy3, double dz3, double dx4, double dy4, double dz4, double speed)
{
	cutRelative(dx2,dy2,dz2,speed);
	cutRelative(dx3,dy3,dz3,speed);
	cutRelative(dx4,dy4,dz4,speed);
	cutRelative(-dx2-dx3-dx4,-dy2-dy3-dy4,-dz2-dz3-dz4,speed);
}

void rectangleAbsolute(double x2, double y2, double z2, double x3, double y3, double z3, double x4, double y4, double z4, double speed) //x1,y1,z1 is starting position: wherever it already exists.
{
	double tempx=currX;
	double tempy=currY;
	double tempz=currZ;
	cut(x2,y2,z2,speed);
	cut(x3,y3,x3,speed);
	cut(x4,y4,z4,speed);
	cut(tempx,tempy,tempz,speed);
}	

void stepwiseDepthRectangle(double x2,double y2, double x3, double y3, double x4, double y4, double zstep, int numsteps, double speed) //STEPS DOWN FIRST, THEN CUTS, THEN STEPS AGAIN
{
	for(int i=0; i<numsteps; i++)
	{	
		cut(currX,currY,currZ+zstep,speed);
		rectangleAbsolute(x2,y2,currZ,x3,y3,currZ,x4,y4,currZ,speed);
	}
}

void stepwiseDepthRectangleRelative(double x2,double y2, double x3, double y3, double x4, double y4, double zstep, int numsteps, double speed)
{
	for(int i=0; i<numsteps; i++)
	{
		cut(0,0,zstep,speed);
		rectangleRelative(x2,y2,0,x3,y3,0,x4,y4,0,speed);
	}
}   

double getArcAngleRad(double startx, double starty, double startz, double endx, double endy, double endz, double deltaCenterX, double deltaCenterY, double deltaCenterZ, bool clockwise) //NOTE: FOR THIS SIM, arcs had sure as hell better be in the xy plane. They can change height, but they can't be oriented in any way in the z plane. Hard to explain what I mean. Basically, it pretends there is no z coordinate involved, but that doesn't mean it has to be strictly planar.
{
    double dSquared = (endx-startx)*(endx-startx) + (endy-starty)*(endy-starty);
    double rSquared = (deltaCenterX)*(deltaCenterX) + (deltaCenterY)*(deltaCenterY);
    int mult = (clockwise)?(-1):1;
    return acos(1-dSquared/(2*rSquared))*mult;
}


void GCodeProgram()
{
    cut(300,300,20,.5);
    cut(400,150,-20,.5);
    cutarc(currX-50,currY-50,-20,0,-50,0,0.5,false);
}