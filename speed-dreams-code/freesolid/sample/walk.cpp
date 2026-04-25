/*
  This sample should be a bit more complex than the others,
  it's intention is to demonstrate how someone might use
  FreeSOLID to do collision detection between a character
  (represented by an ellipsoid) and the world (represented by a complex
  shape).
  The basis for this code is the teapot.cpp sample to take advantage of the
  .vtr "loading code" for the World Mesh

  NOTE: THIS SAMPLE IS NOT FUNCTIONAL YET, IT COMPILES AND RUNS,
  BUT IT DOES NOT YET PERFORMS IT'S INTENDED FUNCTIONALITY
*/

#include <stdio.h>
#include <fstream>
#include <SOLID/solid.h>

#include <assert.h>

#include <3D/Point.h>
#include <3D/Quaternion.h>
#include <vector>

#include "SDL.h"
#include "SDL_opengl.h"

#define SPACE_SIZE 5
#define NUM_ITER 10000

typedef vector<Point> PointList;
typedef vector<int> IndexList;

typedef struct MyObject {
  int id;
} MyObject;

typedef struct Player
{
  int id;
  float pos[3];
  float radii[3];
} Player;

bool LoadWorld();
void DrawWorld();
void DrawEllipsoid(float xradius, float yradius, float zradius);
void DrawBox(float width, float height, float depth);
void Step(float step);

int main(int argc, char *argv[]);



//----------------------
// Globals
//----------------------
MyObject object2;
MyObject object3;
DtShapeRef world;
DtShapeRef ellipsoid;
DtShapeRef box;
PointList points;
IndexList indexes;
GLUquadricObj *sphereqobj;
Player player;
float gravityvel = 0.0f;
const float gravity = -9.8f;
const float stepsize = 0.016;
float boxsize[3]={20.0f,20.0f,5.0f};

void CollideCallback(void * client_data, DtObjectRef obj1, DtObjectRef obj2,
		     const DtCollData *coll_data)
{
  if(coll_data!=NULL)
    {
//       fprintf(stdout,"Collided Point 1 %f,%f,%f\nPoint 2 %f,%f,%f\nNormal %f,%f,%f\n",
// 	      coll_data->point1[X],coll_data->point1[Y],coll_data->point1[Z],
// 	      coll_data->point1[X],coll_data->point1[Y],coll_data->point1[Z],
// 	      coll_data->normal[X],coll_data->normal[Y],coll_data->normal[Z]);

      player.pos[X]-=coll_data->normal[X];
      player.pos[Y]-=coll_data->normal[Y];
      player.pos[Z]-=coll_data->normal[Z];
    }
}

int main(int argc, char *argv[])
{
  player.id = 1;
  player.pos[X]=0.0f;
  player.pos[Y]=0.0f;
  player.pos[Z]=40.0f;
  player.radii[X]=5.0f;
  player.radii[Y]=5.0f;
  player.radii[Z]=10.0f;
  object2.id = 2;

  if (!LoadWorld()) return -1;
  ellipsoid = dtEllipsoid(player.radii[0],player.radii[1],player.radii[2]);
  //ellipsoid = dtSphere(player.radii[2]);
  box = dtBox(boxsize[0],boxsize[1],boxsize[2]);
  dtCreateObject(&player, ellipsoid);
  dtCreateObject(&object2, world);
  dtCreateObject(&object3, box);

  //dtDisableCaching();

  dtSetDefaultResponse(CollideCallback, DT_SMART_RESPONSE, stdout);

  int col_count = 0;
  Quaternion q;
  bool bRunning = true;
  SDL_Event event;

  if(SDL_Init(SDL_INIT_VIDEO)<0) // try to initialize Video System
    {
      fprintf( stderr, "Video initialization failed: %s\n",SDL_GetError()); // if it failed, find out why
      return -1;
    }

  SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 ); // Set red frame buffer size
  SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 ); // Set green frame buffer size
  SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 ); // Set blue frame buffer size
  SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 ); // Set depth frame buffer size
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER,1); // Tell SDL we want Double buffering

  if(SDL_SetVideoMode(800,600,0,SDL_OPENGL) == 0 )
    {
      fprintf(stderr, "Video mode set failed: %s\n",SDL_GetError()); // why did it failed?
      SDL_Quit(); // Release whatever SDL grabbed during SDL_Init
      return -1;
    }

  glViewport(0,0,800,600); // set the view port to take the whole screen
  glMatrixMode(GL_PROJECTION); // switch to the Projection Matrix
  glLoadIdentity(); // reset Projection Matrix
  gluPerspective(45.0f,800.0f/600.0f, 0.1f ,1000.0f); // set our view to perspective
  glMatrixMode(GL_MODELVIEW); // switch to the Model View Matrix
  glLoadIdentity(); // reset Model View Matrix
  glClearColor(0.0f,0.0f,0.0f,0.0f); // Set our Clear Color to Black
  glShadeModel(GL_SMOOTH); // Enable Smooth Shading
  glClearDepth(1.0f); // Set the depth buffer
  glEnable(GL_DEPTH_TEST); // Enables Depth Testing
  glDepthFunc(GL_LEQUAL); // Type of depth test
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // Use really nice perspective calculations
  sphereqobj = gluNewQuadric();

  dtDisableCaching();
  dtSelectObject(&object3);
  dtLoadIdentity();
  while(bRunning)
    {
      if(SDL_PollEvent(&event))
	{
	  switch(event.type)
	    {
            case SDL_QUIT: // test to see if the user closed the app
	      bRunning = false;
	      break;
            case SDL_KEYDOWN: /* Handle a KEYDOWN event */
	      if(event.key.keysym.sym==SDLK_ESCAPE)
                bRunning = false; // quit if ESCAPE is pressed
	      break;
	    }
	}
      Step(stepsize);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glLoadIdentity();
      gluLookAt(100.0f,100.0f,100.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f);
      glPushMatrix();
      DrawWorld();
      glPopMatrix();
      glPushMatrix();
      glTranslatef(player.pos[X],player.pos[Y],player.pos[Z]);
      DrawEllipsoid(player.radii[X],player.radii[Y],player.radii[Z]);
      glPopMatrix();
      DrawBox(boxsize[0],boxsize[1],boxsize[2]);
      SDL_GL_SwapBuffers();
    }
  gluDeleteQuadric(sphereqobj);
  dtDeleteObject(&player);
  dtDeleteObject(&object2);
  dtDeleteShape(world);
  return 0;
}

bool LoadWorld()
{
  world = dtNewComplexShape();
  ////////////////////////////////////////////////////////////////////////////////

  fprintf(stdout, "Loading the World Model...");


  ifstream arg_s("world.vrt");

  // Quit if not found.

   if(!arg_s) { cout << "?????" <<endl;return false;}

  // if no vertextree is declared, skip the lines
   char ch;
  arg_s >> ch;
  assert(ch == '[');


  Point point;

  // Extract Points
  do
  {
    arg_s>>point[0]>>point[1]>>point[2]>>ch;
    points.push_back(point);
  }
  while (ch == ',');
  assert(ch == ']');
  dtVertexBase(&points[0]);

  arg_s >> ch;
  assert(ch == '[');
  // extract indexes
  do
  {
    int index;
    dtBegin(DT_POLYGON);
    do
    {
      arg_s >> index >> ch;
      if (index >= 0)
	  {
	    dtVertexIndex(index);
	    indexes.push_back(index);
	  }
    }
    while (index >= 0);
    dtEnd();
  }
  while (ch == ',');
  assert(ch == ']');

  //--------------------------------------------------------
  fprintf(stdout, "done.\n");
  fprintf(stdout, "Building hierarchy..."); fflush(stdout);
  dtEndComplexShape();
  fprintf(stdout, "done.\n");
  return true;
}

void DrawEllipsoid(float xradius, float yradius, float zradius)
{
  glColor3ub(255,255,255);
  glScalef(xradius,yradius,zradius);
  gluSphere(sphereqobj, 1.0, 30, 30);
}
void DrawBox(float width, float height, float depth)
{
  float wdiv2=width/2;
  float hdiv2=height/2;
  float ddiv2=depth/2;
  glPushMatrix();
  glBegin(GL_LINES);
  {
    glVertex3f(wdiv2,hdiv2,ddiv2);
    glVertex3f(-wdiv2,hdiv2,ddiv2);

    glVertex3f(-wdiv2,-hdiv2,ddiv2);
    glVertex3f(wdiv2,-hdiv2,ddiv2);

    glVertex3f(wdiv2,hdiv2,ddiv2);
    glVertex3f(wdiv2,-hdiv2,ddiv2);

    glVertex3f(-wdiv2,-hdiv2,ddiv2);
    glVertex3f(-wdiv2,hdiv2,ddiv2);

    glVertex3f(wdiv2,hdiv2,-ddiv2);
    glVertex3f(-wdiv2,hdiv2,-ddiv2);

    glVertex3f(-wdiv2,-hdiv2,-ddiv2);
    glVertex3f(wdiv2,-hdiv2,-ddiv2);

    glVertex3f(wdiv2,hdiv2,-ddiv2);
    glVertex3f(wdiv2,-hdiv2,-ddiv2);

    glVertex3f(-wdiv2,-hdiv2,-ddiv2);
    glVertex3f(-wdiv2,hdiv2,-ddiv2);

    glVertex3f(-wdiv2,-hdiv2,ddiv2);
    glVertex3f(-wdiv2,-hdiv2,-ddiv2);

    glVertex3f(wdiv2,-hdiv2,ddiv2);
    glVertex3f(wdiv2,-hdiv2,-ddiv2);

    glVertex3f(wdiv2,hdiv2,-ddiv2);
    glVertex3f(wdiv2,hdiv2,ddiv2);

    glVertex3f(-wdiv2,hdiv2,ddiv2);
    glVertex3f(-wdiv2,hdiv2,-ddiv2);
  }
  glEnd();
  glPopMatrix();

}
void DrawWorld()
{
  glColor3ub(255,0,0);
  glBegin(GL_TRIANGLES);
    {
      for(IndexList::iterator i = indexes.begin();i!=indexes.end();++i)
	{
	  glVertex3f(points[*i][X],points[*i][Y],points[*i][Z]);
	}
    }
  glEnd();
}

void Step(float step)
{
  gravityvel+=gravity*step;
  player.pos[Z]+=(gravityvel*step);
  dtSelectObject(&player);
  dtLoadIdentity();
  dtTranslate(player.pos[X],player.pos[Y],player.pos[Z]);
  dtTest();
}
