// anim.cpp version 5.0 -- Template code for drawing an articulated figure.  CS 174A.

#ifdef _WIN32
#pragma comment(lib, "../GL/Win32/freeglut.lib")
#pragma comment(lib, "../GL/Win32/glew32s.lib")
#pragma comment(lib, "../GL/Win32/glew32mxs.lib")
#else
#pragma comment(lib, "../GL/x64/freeglut.lib")
#pragma comment(lib, "../GL/x64/glew32s.lib")
#pragma comment(lib, "../GL/x64/glew32mxs.lib")
#endif

#define _CRT_SECURE_NO_DEPRECATE
#ifndef EMSCRIPTEN
#include <Windows.h>
#define GLEW_STATIC
#include "..\GL\glew.h"
#endif
#define GL_GLEXT_PROTOTYPES
#include "..\GL\freeglut.h"

#include <math.h>
#include <assert.h>
#include <cmath>

#include "../CS174a template/ArcBall.h"
#include "../CS174a template/FrameSaver.h"
#include "../CS174a template/Timer.h"
#include "../CS174a template/Shapes.h"
#include "../CS174a template/mat.h"
#include "../CS174a template/vec.h"
#include "../CS174a template/InitShaders.h"

#ifdef __APPLE__
#define glutInitContextVersion(a,b)
#define glutInitContextProfile(a)
#define glewExperimental int glewExperimentalAPPLE
#define glewInit()
#endif

FrameSaver FrSaver ;
Timer TM ;

BallData *Arcball = NULL ;
int Width = 800, Height = 800 ;
float Zoom = 1 ;

int Animate = 0, Recording = 0 ;

const int STRLEN = 100;
typedef char STR[STRLEN];

#define X 0
#define Y 1
#define Z 2

GLuint texture_cube, texture_earth;

// Structs that hold the Vertex Array Object index and number of vertices of each shape.
ShapeData cubeData, sphereData, coneData, cylData;

mat4         model_view;
GLint        uModelView, uProjection, uView,
			 uAmbient, uDiffuse, uSpecular, uLightPos, uShininess,
			 uTex, uEnableTex;

// The eye point and look-at point.
// Currently unused. Use to control a camera with LookAt().
Angel::vec4 eye(0, 0.0, 50.0,1.0);
Angel::vec4 ref(0.0, 0.0, 0.0,1.0);
Angel::vec4 up(0.0,1.0,0.0,0.0);

double TIME = 0.0 ;


void instructions() 
{
    printf("Press:\n");
    printf("  s to save the image\n");
    printf("  r to restore the original view.\n") ;
    printf("  0 to set it to the zero state.\n") ;
    printf("  a to toggle the animation.\n") ;
    printf("  m to toggle frame dumping.\n") ;
    printf("  q to quit.\n");
}

void drawCylinder(void)	//render a solid cylinder oriented along the Z axis; bases are of radius 1, placed at Z = 0, and at Z = 1.
{
    glUniformMatrix4fv( uModelView, 1, GL_FALSE, transpose(model_view) );
    glBindVertexArray( cylData.vao );
    glDrawArrays( GL_TRIANGLES, 0, cylData.numVertices );
}

void drawCone(void)	//render a solid cone oriented along the Z axis; bases are of radius 1, placed at Z = 0, and at Z = 1.
{
    glUniformMatrix4fv( uModelView, 1, GL_FALSE, transpose(model_view) );
    glBindVertexArray( coneData.vao );
    glDrawArrays( GL_TRIANGLES, 0, coneData.numVertices );
}

void drawCube(void)		// draw a cube with dimensions 1,1,1 centered around the origin.
{
    glBindTexture( GL_TEXTURE_2D, texture_cube );
    glUniformMatrix4fv( uModelView, 1, GL_FALSE, transpose(model_view) );
    glBindVertexArray( cubeData.vao );
    glDrawArrays( GL_TRIANGLES, 0, cubeData.numVertices );
}

void drawSphere(void)	// draw a sphere with radius 1 centered around the origin.
{
    glBindTexture( GL_TEXTURE_2D, texture_earth);
    glUniformMatrix4fv( uModelView, 1, GL_FALSE, transpose(model_view) );
    glBindVertexArray( sphereData.vao );
    glDrawArrays( GL_TRIANGLES, 0, sphereData.numVertices );
}

// this function gets caled for any keypresses
void myKey(unsigned char key, int x, int y)
{
    float time ;
    switch (key) {
        case 'q':
        case 27:
            exit(0); 
        case 's':
            FrSaver.DumpPPM(Width,Height) ;
            break;
        case 'r':
			Ball_Init(Arcball);					// reset arcball
			Ball_Place(Arcball,qOne,0.75);
            glutPostRedisplay() ;
            break ;
        case 'a': // toggle animation
            Animate = 1 - Animate ;
            // reset the timer to point to the current time		
            time = TM.GetElapsedTime() ;
            TM.Reset() ;
            // printf("Elapsed time %f\n", time) ;
            break ;
        case '0':
            //reset your object
            break ;
        case 'm':
            if( Recording == 1 )
            {
                printf("Frame recording disabled.\n") ;
                Recording = 0 ;
            }
            else
            {
                printf("Frame recording enabled.\n") ;
                Recording = 1  ;
            }
            FrSaver.Toggle(Width);
            break ;
        case 'h':
        case '?':
            instructions();
            break;
    }
    glutPostRedisplay() ;
}

// Performs most of the OpenGL intialization -- change these with care, if you must.
void myinit(void)		
{

#ifndef EMSCRIPTEN
    GLuint program = InitShader( "../my code/vshader.glsl", "../my code/fshader.glsl" );		// Load shaders and use the resulting shader program
#else
	GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );								// Load shaders and use the resulting shader program
#endif
    glUseProgram(program);		

    // Generate vertex arrays for geometric shapes
    generateCube(program, &cubeData);
    generateSphere(program, &sphereData);
    generateCone(program, &coneData);
    generateCylinder(program, &cylData);

    uModelView  = glGetUniformLocation( program, "ModelView"  );
    uProjection = glGetUniformLocation( program, "Projection" );
    uView       = glGetUniformLocation( program, "View"       );

    glClearColor( 0.1, 0.1, 0.2, 1.0 ); // dark blue background

    uAmbient   = glGetUniformLocation( program, "AmbientProduct"  );
    uDiffuse   = glGetUniformLocation( program, "DiffuseProduct"  );
    uSpecular  = glGetUniformLocation( program, "SpecularProduct" );
    uLightPos  = glGetUniformLocation( program, "LightPosition"   );
    uShininess = glGetUniformLocation( program, "Shininess"       );
    uTex       = glGetUniformLocation( program, "Tex"             );
    uEnableTex = glGetUniformLocation( program, "EnableTex"       );

    glUniform4f(uAmbient,    0.2f,  0.2f,  0.2f, 1.0f);
    glUniform4f(uDiffuse,    0.6f,  0.6f,  0.6f, 1.0f);
    glUniform4f(uSpecular,   0.2f,  0.2f,  0.2f, 1.0f);
    glUniform4f(uLightPos,  15.0f, 15.0f, 30.0f, 0.0f);
    glUniform1f(uShininess, 100.0f);

    glEnable(GL_DEPTH_TEST);
 
    Arcball = new BallData;
    Ball_Init(Arcball);
    Ball_Place(Arcball,qOne,0.75);
}

void set_colour(float r, float g, float b)		// sets all material properties to the given colour
{
    float ambient  = 0.2f, diffuse  = 0.6f, specular = 0.2f;
    glUniform4f(uAmbient,  ambient*r,  ambient*g,  ambient*b,  1.0f);
    glUniform4f(uDiffuse,  diffuse*r,  diffuse*g,  diffuse*b,  1.0f);
    glUniform4f(uSpecular, specular*r, specular*g, specular*b, 1.0f);
}

/*********************************************************
**********************************************************
**********************************************************

    MAKE YOUR CHANGES AND ADDITIONS HERE

    Add other procedures if you like.

**********************************************************
**********************************************************
**********************************************************/
void display(void)
{
    // Clear the screen with the background colour (set in myinit)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    model_view = mat4(1.0f);
    
    model_view *= Translate(0.0f, 0.0f, -15.0f);
    HMatrix r;
    Ball_Value(Arcball,r);

    mat4 mat_arcball_rot(
        r[0][0], r[0][1], r[0][2], r[0][3],
        r[1][0], r[1][1], r[1][2], r[1][3],
        r[2][0], r[2][1], r[2][2], r[2][3],
        r[3][0], r[3][1], r[3][2], r[3][3]);
    model_view *= mat_arcball_rot;
        
    glUniformMatrix4fv( uView, 1, GL_FALSE, transpose(model_view) );

    model_view *= Scale(Zoom);

    // Draw Something
    set_colour(0.8f, 0.8f, 0.8f);
    drawSphere();

    model_view *= Translate(3.0f, 0.0f, 0.0f);

    model_view *= Scale(3.0f, 3.0f, 3.0f);

    set_colour(0.8f, 0.0f, 0.8f);
    drawCube();

    model_view *= Scale(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f);
    model_view *= Translate(3.0f, 0.0f, 0.0f);
    set_colour(0.0f, 1.0f, 0.0f);
    drawCone();

    model_view *= Translate(-9.0f, 0.0f, 0.0f);
    set_colour(1.0f, 1.0f, 0.0f);
    drawCylinder();

    glutSwapBuffers();
    if(Recording == 1)
        FrSaver.DumpPPM(Width, Height);
}

void myReshape(int w, int h)		//handles the window being resized 
{
    glViewport(0, 0, Width = w, Height = h);		
    mat4 projection = Perspective(50.0f, (float)w/(float)h, 1.0f, 1000.0f);
    glUniformMatrix4fv( uProjection, 1, GL_FALSE, transpose(projection) );
}

void myMouseCB(int button, int state, int x, int y)	// start or end mouse interaction
{
    ArcBall_mouseButton = button ;
    if( button == GLUT_LEFT_BUTTON && state == GLUT_DOWN )
    {
        HVect arcball_coords;
        arcball_coords.x = 2.0*(float)x/(float)Width-1.0;
        arcball_coords.y = -2.0*(float)y/(float)Height+1.0;
        Ball_Mouse(Arcball, arcball_coords) ;
        Ball_Update(Arcball);
        Ball_BeginDrag(Arcball);

    }
    if( button == GLUT_LEFT_BUTTON && state == GLUT_UP )
    {
        Ball_EndDrag(Arcball);
        ArcBall_mouseButton = -1 ;
    }
    if( button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN )
    {
        ArcBall_PrevZoomCoord = y ;
    }

    // Tell the system to redraw the window
    glutPostRedisplay() ;
}

// interaction (mouse motion)
void myMotionCB(int x, int y)
{
    if( ArcBall_mouseButton == GLUT_LEFT_BUTTON )
    {
        HVect arcball_coords;
        arcball_coords.x = 2.0*(float)x/(float)Width - 1.0 ;
        arcball_coords.y = -2.0*(float)y/(float)Height + 1.0 ;
        Ball_Mouse(Arcball,arcball_coords);
        Ball_Update(Arcball);
        glutPostRedisplay() ;
    }
    else if( ArcBall_mouseButton == GLUT_RIGHT_BUTTON )
    {
        if( y - ArcBall_PrevZoomCoord > 0 )
            Zoom  = Zoom * 1.03 ;
        else 
            Zoom  = Zoom * 0.97 ;
        ArcBall_PrevZoomCoord = y ;
        glutPostRedisplay() ;
    }
}


void idleCB(void)
{
    if( Animate == 1 )
    {
        // TM.Reset() ; // commenting out this will make the time run from 0
        // leaving 'Time' counts the time interval between successive calls to idleCB
        if( Recording == 0 )
            TIME = TM.GetElapsedTime() ;
        else
            TIME += 0.033 ; // save at 30 frames per second.
        
        eye.x = 20*sin(TIME);
        eye.z = 20*cos(TIME);
        
        printf("TIME %f\n", TIME) ;
        glutPostRedisplay() ; 
    }
}

int main(int argc, char** argv)		// calls initialization, then hands over control to the event handler, which calls display() whenever the screen needs to be redrawn
{
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition (0, 0);
    glutInitWindowSize(Width,Height);
    glutCreateWindow(argv[0]);
    printf("GL version %s\n", glGetString(GL_VERSION));
#ifndef EMSCRIPTEN
    glewExperimental = GL_TRUE;
    glewInit();
#endif

    myinit();

    glutIdleFunc(idleCB) ;
    glutReshapeFunc (myReshape);
    glutKeyboardFunc( myKey );
    glutMouseFunc(myMouseCB) ;
    glutMotionFunc(myMotionCB) ;
    instructions();

    glutDisplayFunc(display);
    glutMainLoop();
}