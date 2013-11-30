#include <iostream>
#include <vector>
#include <cmath>

using namespace std;

#define PI 3.14159265

double currX=0;
double currY=0;
double currZ=0;

void move(double x, double y, double z)
{
	if(z!=0 && (x!=0 || y!=0))
		cout << "-------------------------------------------------------" << endl << "CONFLICT: unsafe movement in xz, yz, or xyz space. Separate xy and z movement" << endl << "-------------------------------------------------------" << endl;
	cout << "G00 X" << x << " Y" << y << " Z" << z << ";" << endl;
	currX=x;
	currY=y;
	currZ=z;
}

void cut(double x, double y, double z, double speed)
{
	cout << "G01 X" << x << " Y" << y << " Z" << z << " F" << speed << ";" << endl;
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
	currX+=dx;
	currY+=dy;
	currZ+=dz;
}

void arc(double endx, double endy, double endz, double deltaCenterX, double deltaCenterY, double deltaCenterZ, double speed, bool clockwise)
{
	if(clockwise)
		cout << "G02 ";
	else
		cout << "G03 ";
	cout << "X" << endx << " Y" << endy << " Z" << endz << " I";
	if(deltaCenterX>0)
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
		cout << "-" << deltaCenterZ;
	cout << " F" << speed << ";" << endl;
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

/*
void arcAngle(double centerx, double centery, double theta, double speed)
{
	double d=sqrt((currX-centerx)*(currX-centerx) + (currY-centery)*(currY-centery));
	double y2=(sin(theta*PI/180.0)+centery)/d;
	double x2=(cos(theta*PI/180.0)+centerx)/d;
	arc(x2,y2,currZ,centerx-currX,centery-currY,0,speed,((theta>0)?(false):(true)) );
}

void arcAngleRelative(double deltacenterx, double deltacentery, double deltacenterz, double theta, double speed)
{
	double d=sqrt((deltacenterx*deltacenterx) + (deltacentery*deltacentery));
	double y2=(sin(theta*PI/180.0)+(deltacentery+currY))/d;
	double x2=(cos(theta*PI/180.0)+(deltacenterx+currX))/d;
	arc(x2,y2,currZ,deltacenterx,centery-currY,0,speed,((theta>0)?(false):(true)) );
}*/

int main()
{
	
}

