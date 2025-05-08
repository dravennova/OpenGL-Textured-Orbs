#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#define _USE_MATH_DEFINES
#include <cmath>

#include <iostream>
#include "shader.h"
#include "shaderprogram.h"
#include <../../../CImg-3.3.6/CImg.h>
using namespace cimg_library;

using namespace std;

//opengl functions and how to use them: https://learn.microsoft.com/en-us/windows/win32/opengl/gl-functions


/*=================================================================================================
	DOMAIN
=================================================================================================*/

// Window dimensions
const int InitWindowWidth  = 800;
const int InitWindowHeight = 800;
int WindowWidth  = InitWindowWidth;
int WindowHeight = InitWindowHeight;

// Last mouse cursor position
int LastMousePosX = 0;
int LastMousePosY = 0;

// Arrays that track which keys are currently pressed
bool key_states[256];
bool key_special_states[256];
bool mouse_states[8];

// Other parameters
bool draw_wireframe = false;

/*=================================================================================================
	SHADERS & TRANSFORMATIONS
=================================================================================================*/

ShaderProgram PassthroughShader;
ShaderProgram PerspectiveShader;

glm::mat4 PerspProjectionMatrix( 1.0f );
glm::mat4 PerspViewMatrix( 1.0f );
glm::mat4 PerspModelMatrix( 1.0f );

float perspZoom = 1.0f, perspSensitivity = 0.35f;
float perspRotationX = 0.0f, perspRotationY = 0.0f;

/*=================================================================================================
	OBJECTS
=================================================================================================*/

//VAO -> the object "as a whole", the collection of buffers that make up its data
//VBOs -> the individual buffers/arrays with data, for ex: one for coordinates, one for color, etc.

GLuint axis_VAO;
GLuint axis_VBO[2];

float axis_vertices[] = {
	//x axis
	-1.0f,  0.0f,  0.0f, 1.0f,
	1.0f,  0.0f,  0.0f, 1.0f,
	//y axis
	0.0f, -1.0f,  0.0f, 1.0f,
	0.0f,  1.0f,  0.0f, 1.0f,
	//z axis
	0.0f,  0.0f, -1.0f, 1.0f,
	0.0f,  0.0f,  1.0f, 1.0f
};

float axis_colors[] = {
	//x axis
	1.0f, 0.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 0.0f, 1.0f,
	//y axis
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	//z axis
	0.0f, 0.0f, 1.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 1.0f
};

class Planet
{
public:
	float radius;
	float distance;
	float orbit;
	float orbitSpeed;
	float axisAnimate;

	Planet(float planetRadius, float planetDistance, float planetOrbit, float planetOrbitSpeed, float planetAxisAnimate) {
		radius = planetRadius;
		distance = planetDistance;
		orbit = planetOrbit;
		orbitSpeed = planetOrbitSpeed;
		axisAnimate = planetAxisAnimate;
	}
};

Planet donut3(5.0, 0, 0, 0, 0);				
Planet donut1(1.0, 7, 0, 4.74, 0);		
Planet snail(1.5, 11, 0, 3.50, 0);
Planet pokeball(2.0, 16, 0, 2.98, 0);		

int planetTurning = 0;
int planetOrbit = 0;
int camera = 0;

GLuint texturePlanet1, texturePlanet2, texturePlanet3, texturePlanet4, textureStars;

GLuint loadTexture(const std::string& filename)
{
	CImg<unsigned char> texture;
	texture.load(filename.c_str());

	int size = texture.width() * texture.height();
	unsigned char* data = new unsigned char[3 * size];

	// Extract RGB components and store them in the data array
	for (int i = 0; i < size; i++)
	{
		data[3 * i + 0] = texture.data()[0 * size + i]; // red
		data[3 * i + 1] = texture.data()[1 * size + i]; // green
		data[3 * i + 2] = texture.data()[2 * size + i]; // blue
	}

	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);

	// Use the data array containing RGB components
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture.width(), texture.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	delete[] data; // Free allocated memory

	return textureId;
}


float positionLight[] = { 0.0, 0.0, -75.0, 1.0 }; //position of light
static float positionAngle = 360; // half angle of light
float positionDirection[] = { 1.0, 0.0, 0.0 }; //direction of light
static float positionExpo = 0.5; //	intesity of light

void setup(void)
{
	glClearColor(0.0, 0.0, 0.0, 0.0); // set scene to black
	glEnable(GL_DEPTH_TEST); //enables depth in scene and includes objects, do not run without it

	//textures
	glEnable(GL_NORMALIZE); //enables texture and color for objects
	glEnable(GL_COLOR_MATERIAL);
	

	//lighting
	glEnable(GL_LIGHTING);	//turns on lighting
	
	float lightDifAndSpec[] = { 1.0, 1.0, 1.0, 1.0 }; //creates diffuse and spec light, since all 1, light will equally diffuse onto object color
	float globAmb[] = { 0.5, 0.5, 0.5, 1.0 };// ambient light globally that is small weak light

	//
	
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDifAndSpec); //set diffuse properties to first light in array index 0
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightDifAndSpec);	//same thing but for spec

	glEnable(GL_LIGHT0);  //enable this light source

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globAmb); //set ambient light to globAmb
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);  //set light model to viewers position for lighting calculations
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE); // tells opengl to use both ambient and diffuse on colors material
}



void orbit(void)
{
	glPushMatrix();
	glColor3ub(255, 255, 255); // White color

	// calculate points on the orbit circle for each object
	float radius1 = donut1.distance;
	float radius2 = snail.distance;
	float radius3 = pokeball.distance;
	const int numPoints = 100; // Adjust for desired smoothness

	// draw orbit for first object
	glBegin(GL_LINES);
	for (int i = 0; i < numPoints; ++i) {
		float angle = 2 * M_PI * (float)i / (numPoints - 1);
		glVertex3f(radius1 * cos(angle), 0.0f, radius1 * sin(angle)); // swap X and Z for orbit on X-axis
	}
	glEnd();

	// draw orbit for second object (similar logic, swap X and Z)
	glBegin(GL_LINES);
	for (int i = 0; i < numPoints; ++i) {
		float angle = 2 * M_PI * (float)i / (numPoints - 1);
		glVertex3f(radius2 * cos(angle), 0.0f, radius2 * sin(angle));
	}
	glEnd();

	// draw orbit for third object (similar logic, swap X and Z)
	glBegin(GL_LINES);
	for (int i = 0; i < numPoints; ++i) {
		float angle = 2 * M_PI * (float)i / (numPoints - 1);
		glVertex3f(radius3 * cos(angle), 0.0f, radius3 * sin(angle));
	}
	glEnd();

	glPopMatrix();
}



void drawScene(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	//clears color and depth buffer to draw new scene
	glLoadIdentity(); //load identity matrix to how objects will be translated, rotated or scaled before projecting onto screen


	if (camera == 0)gluLookAt(0.0, 30.0, 10.0,0.0, 0.0, 0.0, 0.0, 1.0, 0.0);  //sets angles of camera
	if (camera == 1)gluLookAt(0.0, 0.0, 30.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	if (planetOrbit == 1) //if this is checked to be 1 then calls onto orbit and draws path
	{
		orbit();
	}

	GLUquadric *quadric; // handles drawing four objects via quadric for drawins spheres
	quadric = gluNewQuadric();

	//donut3
	glPushMatrix(); // pushes new matrix into stack into modelview matrix
	glRotatef(donut3.orbit, 0.0, 1.0, 0.0);//applies a rotation around the Y-axis (because of 1.0 in the second parameter) by donut3.orbit degrees.
	glTranslatef(donut3.distance, 0.0, 0.0); //distance from origin
	
	glPushMatrix();	//temp scope for transfomrations around its rotation around its axis
	glRotatef(donut3.axisAnimate, 0.0, 1.0, 0.0); //rotates again but to itself
	glRotatef(90.0, 1.0, 0.0, 0.0);// rotate 90 degrees on the x axis for texture to be correct
	glEnable(GL_TEXTURE_2D);  // enables texture for drawing texture
	glBindTexture(GL_TEXTURE_2D, texturePlanet1); //binds texture 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);	 // set texture to nearest neighbor 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);	//do it again
	gluQuadricTexture(quadric, 1);	 //texture to this quadric
	gluSphere(quadric, donut3.radius, 20.0, 20.0);	 //creates a quadric based on radius
	glDisable(GL_TEXTURE_2D);  //disables drawing after drawing donut.3
	glPopMatrix();	//pop
	glPopMatrix();	//pop

	//donut1
	glPushMatrix();
	glRotatef(donut1.orbit, 0.0, 1.0, 0.0);
	glTranslatef(donut1.distance, 0.0, 0.0);
	
	glPushMatrix();
	glRotatef(donut1.axisAnimate, 0.0, 1.0, 0.0);
	glRotatef(90.0, 1.0, 0.0, 0.0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texturePlanet2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gluQuadricTexture(quadric, 1);
	gluSphere(quadric, donut1.radius, 20.0, 20.0);
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
	glPopMatrix();

	//snail
	glPushMatrix();
	glRotatef(snail.orbit, 0.0, 1.0, 0.0);
	glTranslatef(snail.distance, 0.0, 0.0);
	
	glPushMatrix();
	glRotatef(snail.axisAnimate, 0.0, 1.0, 0.0);
	glRotatef(90.0, 1.0, 0.0, 0.0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texturePlanet3);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gluQuadricTexture(quadric, 1);
	gluSphere(quadric, snail.radius, 20.0, 20.0);
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
	glPopMatrix();


	//pokeball
	glPushMatrix();
	glRotatef(pokeball.orbit, 0.0, 1.0, 0.0);
	glTranslatef(pokeball.distance, 0.0, 0.0);
	
	glPushMatrix();
	glRotatef(pokeball.axisAnimate, 0.0, 1.0, 0.0);
	glRotatef(90.0, 1.0, 0.0, 0.0);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texturePlanet4);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gluQuadricTexture(quadric, 1);
	gluSphere(quadric, pokeball.radius, 20.0, 20.0);
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
	glPopMatrix();
	

	glPushMatrix();
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureStars);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBegin(GL_POLYGON);
	glTexCoord2f(-1.0, 0.0); glVertex3f(-200, -200, -100);
	glTexCoord2f(2.0, 0.0); glVertex3f(200, -200, -100);
	glTexCoord2f(2.0, 2.0); glVertex3f(200, 200, -100);
	glTexCoord2f(-1.0, 2.0); glVertex3f(-200, 200, -100);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, textureStars);
	glBegin(GL_POLYGON);
	glTexCoord2f(0.0, 0.0); glVertex3f(-200, -83, 200);
	glTexCoord2f(8.0, 0.0); glVertex3f(200, -83, 200);
	glTexCoord2f(8.0, 8.0); glVertex3f(200, -83, -200);
	glTexCoord2f(0.0, 8.0); glVertex3f(-200, -83, -200);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();

	glutSwapBuffers();
}



void resize(int w, int h)					 
{
	glViewport(0, 0, w, h);//sets new window width and height after resizing window 

	glMatrixMode(GL_PROJECTION); //switches to project matrix to define 3d objects onto the 2d screen

	glLoadIdentity(); //resets project matrix to identiy matrix for clean starting point for new projection

	glFrustum(-5.0, 5.0, -5.0, 5.0, 5.0, 200.0); // defines perspective project using new aspect ratio

	glMatrixMode(GL_MODELVIEW); //after resizing switches back to the posotion of camera or objects in scene
}

void animate(int n) {
	if (planetTurning) 
	{
		donut1.orbit += donut1.orbitSpeed; //based on orbit speed values it keeps updating to move along the circle path
		snail.orbit += snail.orbitSpeed;
		pokeball.orbit += pokeball.orbitSpeed;
		
		if (donut1.orbit, snail.orbit, pokeball.orbit > 360.0)	// if it exceeds the 360 degrees then take away 360 to keep it in rotation
		{
			donut1.orbit, snail.orbit, pokeball.orbit-= 360.0;
		}

		donut1.axisAnimate += 10.0;	//same thing happens for when the balls turn themselves in orbit
		snail.axisAnimate += 10.0;
		pokeball.axisAnimate += 10.0;
		
		if (donut1.axisAnimate, snail.axisAnimate, pokeball.axisAnimate > 360.0) {
			donut1.axisAnimate, snail.axisAnimate, pokeball.axisAnimate -= 360.0;
		}
		glutPostRedisplay();
		glutTimerFunc(30, animate, 1);	//keeps updating after some time if true
	}
}



/*=================================================================================================
	HELPER FUNCTIONS
=================================================================================================*/

void window_to_scene( int wx, int wy, float& sx, float& sy )
{
	sx = ( 2.0f * (float)wx / WindowWidth ) - 1.0f;
	sy = 1.0f - ( 2.0f * (float)wy / WindowHeight );
}

/*=================================================================================================
	SHADERS
=================================================================================================*/

void CreateTransformationMatrices( void )
{
	// PROJECTION MATRIX
	PerspProjectionMatrix = glm::perspective<float>( glm::radians( 60.0f ), (float)WindowWidth / (float)WindowHeight, 0.01f, 1000.0f );

	// VIEW MATRIX
	glm::vec3 eye   ( 0.0, 0.0, 2.0 );
	glm::vec3 center( 0.0, 0.0, 0.0 );
	glm::vec3 up    ( 0.0, 1.0, 0.0 );

	PerspViewMatrix = glm::lookAt( eye, center, up );

	// MODEL MATRIX
	PerspModelMatrix = glm::mat4( 1.0 );
	PerspModelMatrix = glm::rotate( PerspModelMatrix, glm::radians( perspRotationX ), glm::vec3( 1.0, 0.0, 0.0 ) );
	PerspModelMatrix = glm::rotate( PerspModelMatrix, glm::radians( perspRotationY ), glm::vec3( 0.0, 1.0, 0.0 ) );
	PerspModelMatrix = glm::scale( PerspModelMatrix, glm::vec3( perspZoom ) );
}

void CreateShaders( void )
{
	// Renders without any transformations
	PassthroughShader.Create( "./shaders/simple.vert", "./shaders/simple.frag" );

	// Renders using perspective projection
	PerspectiveShader.Create( "./shaders/persp.vert", "./shaders/persp.frag" );

	//
	// Additional shaders would be defined here
	//
}

/*=================================================================================================
	BUFFERS
=================================================================================================*/

void CreateAxisBuffers( void )
{
	glGenVertexArrays( 1, &axis_VAO ); //generate 1 new VAO, its ID is returned in axis_VAO
	glBindVertexArray( axis_VAO ); //bind the VAO so the subsequent commands modify it

	glGenBuffers( 2, &axis_VBO[0] ); //generate 2 buffers for data, their IDs are returned to the axis_VBO array

	// first buffer: vertex coordinates
	glBindBuffer( GL_ARRAY_BUFFER, axis_VBO[0] ); //bind the first buffer using its ID
	glBufferData( GL_ARRAY_BUFFER, sizeof( axis_vertices ), axis_vertices, GL_STATIC_DRAW ); //send coordinate array to the GPU
	glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), (void*)0 ); //let GPU know this is attribute 0, made up of 4 floats
	glEnableVertexAttribArray( 0 );

	// second buffer: colors
	glBindBuffer( GL_ARRAY_BUFFER, axis_VBO[1] ); //bind the second buffer using its ID
	glBufferData( GL_ARRAY_BUFFER, sizeof( axis_colors ), axis_colors, GL_STATIC_DRAW ); //send color array to the GPU
	glVertexAttribPointer( 1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), (void*)0 ); //let GPU know this is attribute 1, made up of 4 floats
	glEnableVertexAttribArray( 1 );

	glBindVertexArray( 0 ); //unbind when done

	//NOTE: You will probably not use an array for your own objects, as you will need to be
	//      able to dynamically resize the number of vertices. Remember that the sizeof()
	//      operator will not give an accurate answer on an entire vector. Instead, you will
	//      have to do a calculation such as sizeof(v[0]) * v.size().
}

//
//void CreateMyOwnObject( void ) ...
//

/*=================================================================================================
	CALLBACKS
=================================================================================================*/

//-----------------------------------------------------------------------------
// CALLBACK DOCUMENTATION
// https://www.opengl.org/resources/libraries/glut/spec3/node45.html
// http://freeglut.sourceforge.net/docs/api.php#WindowCallback
//-----------------------------------------------------------------------------

void idle_func()
{
	//uncomment below to repeatedly draw new frames
	glutPostRedisplay();
}

void reshape_func( int width, int height )
{
	WindowWidth  = width;
	WindowHeight = height;

	glViewport( 0, 0, width, height );
	glutPostRedisplay();
}

void keyboard_func( unsigned char key, int x, int y )
{
	key_states[ key ] = true;

	switch( key )
	{
		case 'w':
		{
			draw_wireframe = !draw_wireframe;
			if( draw_wireframe == true )
				std::cout << "Wireframes on.\n";
			else
				std::cout << "Wireframes off.\n";
			break;
		}

		// Exit on escape key press
		case '\x1B':
		{
			exit( EXIT_SUCCESS );
			break;
		}
		case' ':
		{
			if (planetTurning)
			{
				planetTurning = 0;
				break;
			}
			else
			{
				planetTurning = 1;
				animate(1);
				break;
			}
		}
		case'q':
		{
			if (planetOrbit)
			{
				planetOrbit = 0;
				glutPostRedisplay();
				break;
			}
			else
			{
				planetOrbit = 1;
				glutPostRedisplay();
				break;
			}
		}
		case '1':
		{
			camera = 0;
			glutPostRedisplay();
			break;
		}
		case'2':
		{
			camera = 1;
			glutPostRedisplay();
			break;
		}
	}
}

void key_released( unsigned char key, int x, int y )
{
	key_states[ key ] = false;
}

void key_special_pressed( int key, int x, int y )
{
	key_special_states[ key ] = true;
}

void key_special_released( int key, int x, int y )
{
	key_special_states[ key ] = false;
}

void mouse_func( int button, int state, int x, int y )
{
	// Key 0: left button
	// Key 1: middle button
	// Key 2: right button
	// Key 3: scroll up
	// Key 4: scroll down

	if( x < 0 || x > WindowWidth || y < 0 || y > WindowHeight )
		return;

	float px, py;
	window_to_scene( x, y, px, py );

	if( button == 3 )
	{
		perspZoom += 0.03f;
	}
	else if( button == 4 )
	{
		if( perspZoom - 0.03f > 0.0f )
			perspZoom -= 0.03f;
	}

	mouse_states[ button ] = ( state == GLUT_DOWN );

	LastMousePosX = x;
	LastMousePosY = y;
}

void passive_motion_func( int x, int y )
{
	if( x < 0 || x > WindowWidth || y < 0 || y > WindowHeight )
		return;

	float px, py;
	window_to_scene( x, y, px, py );

	LastMousePosX = x;
	LastMousePosY = y;
}

void active_motion_func( int x, int y )
{
	if( x < 0 || x > WindowWidth || y < 0 || y > WindowHeight )
		return;

	float px, py;
	window_to_scene( x, y, px, py );

	if( mouse_states[0] == true )
	{
		perspRotationY += ( x - LastMousePosX ) * perspSensitivity;
		perspRotationX += ( y - LastMousePosY ) * perspSensitivity;
	}

	LastMousePosX = x;
	LastMousePosY = y;
}

/*=================================================================================================
	RENDERING
=================================================================================================*/

void display_func( void )
{
	// Clear the contents of the back buffer
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Update transformation matrices
	CreateTransformationMatrices();

	// Choose which shader to user, and send the transformation matrix information to it
	PerspectiveShader.Use();
	PerspectiveShader.SetUniform( "projectionMatrix", glm::value_ptr( PerspProjectionMatrix ), 4, GL_FALSE, 1 );
	PerspectiveShader.SetUniform( "viewMatrix", glm::value_ptr( PerspViewMatrix ), 4, GL_FALSE, 1 );
	PerspectiveShader.SetUniform( "modelMatrix", glm::value_ptr( PerspModelMatrix ), 4, GL_FALSE, 1 );

	// Drawing in wireframe?
	if( draw_wireframe == true )
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	else
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	// Bind the axis Vertex Array Object created earlier, and draw it
	glBindVertexArray( axis_VAO );
	glDrawArrays( GL_LINES, 0, 6 ); // 6 = number of vertices in the object

	//
	// Bind and draw your object here
	//

	drawScene();

	// Unbind when done
	glBindVertexArray( 0 );

	// Swap the front and back buffers
	glutSwapBuffers();
}

/*=================================================================================================
	INIT
=================================================================================================*/

void init( void )
{
	// Print some info
	std::cout << "Vendor:         " << glGetString( GL_VENDOR   ) << "\n";
	std::cout << "Renderer:       " << glGetString( GL_RENDERER ) << "\n";
	std::cout << "OpenGL Version: " << glGetString( GL_VERSION  ) << "\n";
	std::cout << "GLSL Version:   " << glGetString( GL_SHADING_LANGUAGE_VERSION ) << "\n\n";

	// Set OpenGL settings
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f ); // background color
	glEnable( GL_DEPTH_TEST ); // enable depth test
	glEnable( GL_CULL_FACE ); // enable back-face culling

	// Create shaders
	CreateShaders();

	// Create axis buffers
	CreateAxisBuffers();

	//
	// Consider calling a function to create your object here
	//

	std::cout << "Finished initializing...\n\n";
}

/*=================================================================================================
	MAIN
=================================================================================================*/

int main( int argc, char** argv )
{
	// Create and initialize the OpenGL context
	glutInit( &argc, argv );

	glutInitWindowPosition( 100, 100 );
	glutInitWindowSize( InitWindowWidth, InitWindowHeight );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH );

	glutCreateWindow( "CSE-170 Computer Graphics" );

	std::cout << "Vendor:         " << glGetString(GL_VENDOR) << "\n";
	std::cout << "Renderer:       " << glGetString(GL_RENDERER) << "\n";
	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
	std::cout << "GLSL Version:   " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n\n";

	// Initialize GLEW
	GLenum ret = glewInit();
	if( ret != GLEW_OK ) {
		std::cerr << "GLEW initialization error." << std::endl;
		glewGetErrorString( ret );
		return -1;
	}

	// Register callback functions

	glutInitContextVersion(4, 2);
	glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
	glutDisplayFunc(drawScene);
	glutIdleFunc( idle_func );
	glutReshapeFunc( resize );
	glutKeyboardFunc( keyboard_func );
	glutKeyboardUpFunc( key_released );
	glutSpecialFunc( key_special_pressed );
	glutSpecialUpFunc( key_special_released );
	glutMotionFunc( active_motion_func );
	glutPassiveMotionFunc( passive_motion_func );

	textureStars = loadTexture("stars.bmp");
	texturePlanet1 = loadTexture("donut3.bmp");
	texturePlanet2 = loadTexture("donut1.bmp");
	texturePlanet3 = loadTexture("snail.bmp");
	texturePlanet4 = loadTexture("pokeball.bmp");
	glewInit();
	// Do program initialization
	setup();
	glewInit();
	// Enter the main loop
	glutMainLoop();

	return EXIT_SUCCESS;
}
