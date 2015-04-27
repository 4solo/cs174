#include "../CS174a template/Color.h"
#include "../CS174a template/Utilities.h"
#include "../CS174a template/Shapes.h"
std::stack<mat4> mvstack;
#include <queue>
int g_width = 800, g_height = 800;
float zoom = 1;

int animate = 0, recording = 0, basis_to_display = -1;
double TIME = 0;

const unsigned X = 0, Y = 1, Z = 2;

vec4 eye(45,45, 45, 1), ref(0, 0, 0, 1), up(0, 1, 0, 0);	// The eye point and look-at point.
Color red(1.0f, 0.0f, 0.0f);   //red
Color yellow(1.0f, 1.0f, 0.0f);  //yellow
Color blue(0.0f, 0.0f, 1.0f);  //blue
Color white(1.0f, 1.0f, 1.0f);  //white
Color green(0.000f, 0.502f, 0.000f);//green
Color orange(1.0f, 0.647f, 0.0f);  //orange
mat4	orientation, model_view;
ShapeData cubeData, sphereData, coneData, cylData, sphereData2;				// Structs that hold the Vertex Array Object index and number of vertices of each shape.
GLuint	texture_cube, texture_earth;
GLint   uModelView, uProjection, uView,
uAmbient, uDiffuse, uSpecular, uLightPos, uShininess,
uTex, uEnableTex;

void init()
{
#ifdef EMSCRIPTEN
	GLuint program = LoadShaders("vshader.glsl", "fshader.glsl");								// Load shaders and use the resulting shader program
	TgaImage coolImage("challenge.tga");
	TgaImage earthImage("earth.tga");

#else
	GLuint program = LoadShaders("../my code/vshader.glsl", "../my code/fshader.glsl");		// Load shaders and use the resulting shader program
	TgaImage coolImage("../my code/challenge.tga");
	TgaImage earthImage("../my code/earth.tga");
#endif
	glUseProgram(program);

	generateCube(program, &cubeData);		// Generate vertex arrays for geometric shapes
	generateSphere(program, &sphereData);
	generateCone(program, &coneData);
	generateCylinder(program, &cylData);
	generateSphere2(program, &sphereData2);

	uModelView = glGetUniformLocation(program, "ModelView");
	uProjection = glGetUniformLocation(program, "Projection");
	uView = glGetUniformLocation(program, "View");
	uAmbient = glGetUniformLocation(program, "AmbientProduct");
	uDiffuse = glGetUniformLocation(program, "DiffuseProduct");
	uSpecular = glGetUniformLocation(program, "SpecularProduct");
	uLightPos = glGetUniformLocation(program, "LightPosition");
	uShininess = glGetUniformLocation(program, "Shininess");
	uTex = glGetUniformLocation(program, "Tex");
	uEnableTex = glGetUniformLocation(program, "EnableTex");

	glUniform4f(uAmbient, 0.2, 0.2, 0.2, 1);
	glUniform4f(uDiffuse, 0.6, 0.6, 0.6, 1);
	glUniform4f(uSpecular, 0.2, 0.2, 0.2, 1);
	glUniform4f(uLightPos, 15.0, 15.0, 30.0, 0);
	glUniform1f(uShininess, 100);

	glEnable(GL_DEPTH_TEST);

	glGenTextures(1, &texture_cube);
	glBindTexture(GL_TEXTURE_2D, texture_cube);

	glTexImage2D(GL_TEXTURE_2D, 0, 4, coolImage.width, coolImage.height, 0,
		(coolImage.byteCount == 3) ? GL_BGR : GL_BGRA,
		GL_UNSIGNED_BYTE, coolImage.data);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);


	glGenTextures(1, &texture_earth);
	glBindTexture(GL_TEXTURE_2D, texture_earth);

	glTexImage2D(GL_TEXTURE_2D, 0, 4, earthImage.width, earthImage.height, 0,
		(earthImage.byteCount == 3) ? GL_BGR : GL_BGRA,
		GL_UNSIGNED_BYTE, earthImage.data);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glUniform1i(uTex, 0);	// Set texture sampler variable to texture unit 0

	glEnable(GL_DEPTH_TEST);
}

struct color{ color(float r, float g, float b) : r(r), g(g), b(b) {} float r, g, b; };
std::stack<color> colors;
void set_color(float r, float g, float b)
{
	colors.push(color(r, g, b));

	float ambient = 0.2, diffuse = 0.6, specular = 0.2;
	glUniform4f(uAmbient, ambient*r, ambient*g, ambient*b, 1);
	glUniform4f(uDiffuse, diffuse*r, diffuse*g, diffuse*b, 1);
	glUniform4f(uSpecular, specular*r, specular*g, specular*b, 1);
}
void Set_color(Color a)
{
	set_color(a.first(), a.second(), a.third());
}

int mouseButton = -1, prevZoomCoord = 0;
vec2 anchor;
void myPassiveMotionCallBack(int x, int y) { anchor = vec2(2. * x / g_width - 1, -2. * y / g_height + 1); }

void myMouseCallBack(int button, int state, int x, int y)	// start or end mouse interaction
{
	mouseButton = button;

	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
		mouseButton = -1;
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
		prevZoomCoord = -2. * y / g_height + 1;

	glutPostRedisplay();
}

void myMotionCallBack(int x, int y)
{
	vec2 arcball_coords(2. * x / g_width - 1, -2. * y / g_height + 1);

	if (mouseButton == GLUT_LEFT_BUTTON)
	{
		orientation = RotateX(-10 * (arcball_coords.y - anchor.y)) * orientation;
		orientation = RotateY(10 * (arcball_coords.x - anchor.x)) * orientation;
	}

	if (mouseButton == GLUT_RIGHT_BUTTON)
		zoom *= 1 + .1 * (arcball_coords.y - anchor.y);
	glutPostRedisplay();
}

void idleCallBack(void)
{
	if (!animate) return;
	double prev_time = TIME;
	TIME = TM.GetElapsedTime();
	if (prev_time == 0) TM.Reset();
	glutPostRedisplay();
}

void drawCylinder()	//render a solid cylinder oriented along the Z axis; bases are of radius 1, placed at Z = 0, and at Z = 1.
{
	glUniformMatrix4fv(uModelView, 1, GL_FALSE, transpose(model_view));
	glBindVertexArray(cylData.vao);
	glDrawArrays(GL_TRIANGLES, 0, cylData.numVertices);
}

void drawCone()	//render a solid cone oriented along the Z axis; bases are of radius 1, placed at Z = 0, and at Z = 1.
{
	glUniformMatrix4fv(uModelView, 1, GL_FALSE, transpose(model_view));
	glBindVertexArray(coneData.vao);
	glDrawArrays(GL_TRIANGLES, 0, coneData.numVertices);
}

void drawTextCube()		// draw a cube with dimensions 1,1,1 centered around the origin.
{
	glBindTexture(GL_TEXTURE_2D, texture_cube);
	glUniform1i(uEnableTex, 1);
	glUniformMatrix4fv(uModelView, 1, GL_FALSE, transpose(model_view));
	glBindVertexArray(cubeData.vao);
	glDrawArrays(GL_TRIANGLES, 0, cubeData.numVertices);
	glUniform1i(uEnableTex, 0);
}
void drawCube()		// draw a cube with dimensions 1,1,1 centered around the origin.
{
	glBindTexture(GL_TEXTURE_2D, texture_cube);
	glUniformMatrix4fv(uModelView, 1, GL_FALSE, transpose(model_view));
	glBindVertexArray(cubeData.vao);
	glDrawArrays(GL_TRIANGLES, 0, cubeData.numVertices);
}
void drawSphere2()	// draw a sphere with radius 1 centered around the origin.
{
	glBindTexture(GL_TEXTURE_2D, texture_earth);
	//glUniform1i(uEnableTex, 1);
	glUniformMatrix4fv(uModelView, 1, GL_FALSE, transpose(model_view));
	glBindVertexArray(sphereData2.vao);
	glDrawArrays(GL_TRIANGLES, 0, sphereData2.numVertices);
	//glUniform1i(uEnableTex, 0);
}

void drawSphere()	// draw a sphere with radius 1 centered around the origin.
{
	glBindTexture(GL_TEXTURE_2D, texture_earth);
	//glUniform1i(uEnableTex, 1);
	glUniformMatrix4fv(uModelView, 1, GL_FALSE, transpose(model_view));
	glBindVertexArray(sphereData.vao);
	glDrawArrays(GL_TRIANGLES, 0, sphereData.numVertices);
	//glUniform1i(uEnableTex, 0);
}

int basis_id = 0;

void drawOneAxis()
{
	mat4 origin = model_view;
	model_view *= Translate(0, 0, 4);
	model_view *= Scale(.25) * Scale(1, 1, -1);
	drawCone();
	model_view = origin;
	model_view *= Translate(1, 1, .5);
	model_view *= Scale(.1, .1, 1);
	drawCube();
	model_view = origin;
	model_view *= Translate(1, 0, .5);
	model_view *= Scale(.1, .1, 1);
	drawCube();
	model_view = origin;
	model_view *= Translate(0, 1, .5);
	model_view *= Scale(.1, .1, 1);
	drawCube();
	model_view = origin;
	model_view *= Translate(0, 0, 2);
	model_view *= Scale(.1) * Scale(1, 1, 20);
	drawCylinder();
	model_view = origin;
}

void drawAxes(int selected)
{
	if (basis_to_display != selected)
		return;
	mat4 given_basis = model_view;
	model_view *= Scale(.25);
	drawSphere();
	model_view = given_basis;
	set_color(0, 0, 1);
	drawOneAxis();
	model_view *= RotateX(-90);
	model_view *= Scale(1, -1, 1);
	set_color(1, 1, 1);
	drawOneAxis();
	model_view = given_basis;
	model_view *= RotateY(90);
	model_view *= Scale(-1, 1, 1);
	set_color(1, 0, 0);
	drawOneAxis();
	model_view = given_basis;

	colors.pop();
	colors.pop();
	colors.pop();
	set_color(colors.top().r, colors.top().g, colors.top().b);
}

void drawGround(){
	mvstack.push(model_view);
	set_color(0.867, 0.627, 0.867);
	//model_view *= Translate(0, -10, 0);									
	drawAxes(basis_id++);
	model_view *= Scale(10, 10, 1);									
	drawAxes(basis_id++);
	drawTextCube();
	model_view = mvstack.top(); 
	mvstack.pop();								
	drawAxes(basis_id++);
}
void drawShapes()
{
	mvstack.push(model_view);

	model_view *= Translate(0, 3, 0);									drawAxes(basis_id++);
	model_view *= Scale(3, 3, 3);									drawAxes(basis_id++);
	set_color(.8, .0, .8);
	drawCube();

	model_view *= Scale(1 / 3.0f, 1 / 3.0f, 1 / 3.0f);						drawAxes(basis_id++);
	model_view *= Translate(0, 3, 0);									drawAxes(basis_id++);
	set_color(0, 1, 0);
	drawCone();

	model_view *= Translate(0, -9, 0);									drawAxes(basis_id++);
	set_color(1, 1, 0);
	drawCylinder();

	model_view = mvstack.top(); mvstack.pop();								drawAxes(basis_id++);

	model_view *= Scale(1 / 3.0f, 1 / 3.0f, 1 / 3.0f);						drawAxes(basis_id++);

	drawGround();
}

void drawPlanets()
{
	set_color(.8, .0, .0);	//model sun
	mvstack.push(model_view);
	model_view *= Scale(3);													drawAxes(basis_id++);
	drawSphere();
	model_view = mvstack.top(); mvstack.pop();								drawAxes(basis_id++);

	set_color(.0, .0, .8);	//model earth
	model_view *= RotateY(10 * TIME);									drawAxes(basis_id++);
	model_view *= Translate(15, 5 * sin(30 * DegreesToRadians*TIME), 0);	drawAxes(basis_id++);
	mvstack.push(model_view);
	model_view *= RotateY(300 * TIME);										drawAxes(basis_id++);
	drawCube();
	model_view = mvstack.top(); mvstack.pop();								drawAxes(basis_id++);

	set_color(.8, .0, .8);	//model moon
	model_view *= RotateY(30 * TIME);									drawAxes(basis_id++);
	model_view *= Translate(2, 0, 0);										drawAxes(basis_id++);
	model_view *= Scale(0.2);												drawAxes(basis_id++);
	drawCylinder();

}

void drawMidterm()
{
	mvstack.push(model_view);
	mvstack.push(model_view);
	model_view *= Translate(-1, 0, 0);									drawAxes(basis_id++);
	model_view *= Scale(2, 1, 1);									drawAxes(basis_id++);
	drawCube();
	model_view = mvstack.top(); mvstack.pop();								drawAxes(basis_id++);

	model_view *= Scale(2, 1, 1);									drawAxes(basis_id++);
	model_view *= Translate(1, 0, 0);									drawAxes(basis_id++);
	drawCube();


	model_view *= Translate(0, 2, 0);									drawAxes(basis_id++);
	model_view *= RotateZ(90 + 360 * TIME);							drawAxes(basis_id++);
	drawCube();
	model_view = mvstack.top(); mvstack.pop();								drawAxes(basis_id++);
}
ColorTwo edge1(green,red);
ColorTwo edge2(green,white);
ColorTwo edge3(orange,white);
ColorTwo edge4(red,yellow);
ColorTwo edge5(yellow,orange);
ColorTwo edge6(blue,yellow);
ColorTwo edge7(green,orange);
ColorTwo edge8(red,blue);
ColorTwo edge9(blue,white);
ColorTwo edge10(green,yellow);
ColorTwo edge11(red,white);
ColorTwo edge12(blue,orange);
ColorTwo edgetemp;
ColorThree corner1(white, red, green);
ColorThree corner2(yellow, red, blue);
ColorThree corner3(white, blue, orange);
ColorThree corner4(orange, blue, yellow);
ColorThree corner5(white, red, blue);
ColorThree corner6(orange, green, white);
ColorThree corner7(yellow, orange, green);
ColorThree corner8(red, yellow, green);
ColorThree cornertemp;

void drawCenter1(Color a)  //top center
{
	mvstack.push(model_view);  
	model_view *= Translate(0, 3, 0);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a);
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawCorner1(ColorThree a)//top left corner
{
	mvstack.push(model_view);  //origin
	model_view *= Translate(-2, 3,-2);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbot());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(-3, 2, -2);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.left());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(-2, 2, -3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.right());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawCorner2(ColorThree a)//top right corner
{
	mvstack.push(model_view);  //origin
	model_view *= Translate(2, 3, -2);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbot());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(2, 2, -3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.left());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(3, 2, -2);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.right());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawCorner3(ColorThree a)//top right corner
{
	mvstack.push(model_view);  //origin
	model_view *= Translate(2, 3, 2);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbot());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(2, 2, 3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.left());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(3, 2, 2);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.right());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawCorner4(ColorThree a)//top right corner
{
	mvstack.push(model_view);  //origin
	model_view *= Translate(-2, 3, 2);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbot());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(-3, 2, 2);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.left());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(-2, 2, 3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.right());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawEdge1(ColorTwo a)
{
	mvstack.push(model_view);  //origin
	model_view *= Translate(0, 3, -2);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbotandleft());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(0, 2, -3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.noupbotandright());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawEdge2(ColorTwo a)
{
	mvstack.push(model_view);  //origin
	model_view *= Translate(2, 3, 0);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbotandleft());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(3, 2, 0);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.noupbotandright());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawEdge3(ColorTwo a)
{
	mvstack.push(model_view);  //origin
	model_view *= Translate(0, 3, 2);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbotandleft());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(0, 2, 3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.noupbotandright());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawEdge4(ColorTwo a)
{
	mvstack.push(model_view);  //origin
	model_view *= Translate(-2, 3, 0);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbotandleft());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(-3, 2, 0);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.noupbotandright());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawCenter2(Color a)  //back center
{
	mvstack.push(model_view);
	model_view *= Translate(0, 0, -3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a);
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawCenter3(Color a)  //right center
{
	mvstack.push(model_view);
	model_view *= Translate(3, 0, 0);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a);
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawCenter4(Color a)  //front center
{
	mvstack.push(model_view);
	model_view *= Translate(0, 0, 3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a);
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawCenter5(Color a)  //left center
{
	mvstack.push(model_view);
	model_view *= Translate(-3, 0, 0);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a);
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawEdge5(ColorTwo a)
{

	mvstack.push(model_view);  //origin
	model_view *= Translate(-3, 0, -2);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.upbotandleft());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(-2, 0, -3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.noupbotandright());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

}
void drawEdge6(ColorTwo a)
{

	mvstack.push(model_view);  //origin
	model_view *= Translate(2, 0, -3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.upbotandleft());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(3, 0, -2);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.noupbotandright());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

}
void drawEdge7(ColorTwo a)
{

	mvstack.push(model_view);  //origin
	model_view *= Translate(2, 0, 3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.upbotandleft());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(3, 0, 2);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.noupbotandright());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

}
void drawEdge8(ColorTwo a)
{

	
	mvstack.push(model_view);  //origin
	model_view *= Translate(-3, 0, 2);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.upbotandleft());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(-2, 0, 3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.noupbotandright());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();


}
void drawCenter6(Color a)
{
	mvstack.push(model_view);
	model_view *= Translate(0, -3, 0);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a);
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawCorner5(ColorThree a)
{
	mvstack.push(model_view);  //origin
	model_view *= Translate(-2, -3, -2);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbot());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(-3, -2, -2);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.left());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(-2, -2, -3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.right());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	
}
void drawCorner6(ColorThree a)
{
	mvstack.push(model_view);  //origin
	model_view *= Translate(2, -3, -2);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbot());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(2, -2, -3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.left());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(3, -2, -2);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.right());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawCorner7(ColorThree a)
{
	mvstack.push(model_view);  //origin
	model_view *= Translate(2, -3, 2);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbot());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(2, -2, 3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.left());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(3, -2, 2);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.right());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawCorner8(ColorThree a)
{
	mvstack.push(model_view);  //origin
	model_view *= Translate(-2, -3, 2);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbot());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(-3, -2, 2);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.left());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(-2, -2, 3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.right());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawEdge9(ColorTwo a)
{


	mvstack.push(model_view);  //origin
	model_view *= Translate(0, -3, -2);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbotandleft());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(0, -2, -3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.noupbotandright());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();


}
void drawEdge10(ColorTwo a)
{


	mvstack.push(model_view);  //origin
	model_view *= Translate(2, -3, 0);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbotandleft());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(3, -2, 0);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.noupbotandright());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();


}
void drawEdge11(ColorTwo a)
{


	mvstack.push(model_view);  //origin
	model_view *= Translate(0, -3, 2);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbotandleft());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(0, -2, 3);
	model_view *= Scale(1.9, 1.9, 0.1);
	Set_color(a.noupbotandright());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();
}
void drawEdge12(ColorTwo a)
{


	mvstack.push(model_view);  //origin
	model_view *= Translate(-2, -3, 0);
	model_view *= Scale(1.9, 0.1, 1.9);
	Set_color(a.upbotandleft());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();

	mvstack.push(model_view);  //origin
	model_view *= Translate(-3, -2, 0);
	model_view *= Scale(0.1, 1.9, 1.9);
	Set_color(a.noupbotandright());
	drawCube();
	model_view = mvstack.top();
	mvstack.pop();


}
void RotateTop(float t,int lefttoright) // left to right
{
	mvstack.push(model_view);
	if (lefttoright)
	{
		model_view *= RotateY(90 * t);
	}
	else
	{
		model_view *= RotateY(-90 * t);
	}
	drawCorner1(corner1);
	drawCorner2(corner2);
	drawCorner3(corner3);
	drawCorner4(corner4);
	drawEdge1(edge1);
	drawEdge2(edge2);
	drawEdge3(edge3);
	drawEdge4(edge4);
	drawCenter1(blue);
	model_view = mvstack.top();
	mvstack.pop();
	drawCenter2(orange);
	drawCenter3(yellow);
	drawCenter4(red);
	drawCenter5(white);
	drawEdge5(edge5);
	drawEdge6(edge6);
	drawEdge7(edge7);
	drawEdge8(edge8);
	drawCenter6(green);
	drawCorner5(corner5);
	drawCorner6(corner6);
	drawCorner7(corner7);
	drawCorner8(corner8);
	drawEdge9(edge9);
	drawEdge10(edge10);
	drawEdge11(edge11);
	drawEdge12(edge12);
}
void RotateLeft(float t, int uptobot)// up to bot
{
	mvstack.push(model_view);
	if (uptobot)
	{
		model_view *= RotateX(90 * t);
	}
	else
	{
		model_view *= RotateX(-90 * t);
	}
	drawCorner1(corner1);
	drawCorner4(corner4);
	drawEdge4(edge4);
	drawEdge5(edge5);
	drawEdge8(edge8); 
	drawCenter5(white);
	drawCorner5(corner5);
	drawEdge12(edge12);
	drawCorner8(corner8);
	model_view = mvstack.top();
	mvstack.pop();
	drawCenter2(orange);
	drawCenter3(yellow);
	drawCenter4(red);	
	drawEdge6(edge6);
	drawEdge7(edge7);
	drawCorner2(corner2);
	drawCorner3(corner3);
	drawEdge1(edge1);
	drawEdge2(edge2);
	drawEdge3(edge3);
	drawCenter1(blue);
	drawCenter6(green);	
	drawCorner6(corner6);
	drawCorner7(corner7);	
	drawEdge9(edge9);
	drawEdge10(edge10);
	drawEdge11(edge11);	
}
void RotateBot(float t, int lefttoright) // left to right
{
	mvstack.push(model_view);
	if (lefttoright)
	{
		model_view *= RotateY(90 * t);
	}
	else
	{
		model_view *= RotateY(-90 * t);
	}
	drawCenter6(green);
	drawCorner5(corner5);
	drawCorner6(corner6);
	drawCorner7(corner7);
	drawCorner8(corner8);
	drawEdge9(edge9);
	drawEdge10(edge10);
	drawEdge11(edge11);
	drawEdge12(edge12);
	model_view = mvstack.top();
	mvstack.pop();
	drawCenter2(orange);
	drawCenter3(yellow);
	drawCenter4(red);
	drawCenter5(white);
	drawEdge5(edge5);
	drawEdge6(edge6);
	drawEdge7(edge7);
	drawEdge8(edge8);
	drawCorner1(corner1);
	drawCorner2(corner2);
	drawCorner3(corner3);
	drawCorner4(corner4);
	drawEdge1(edge1);
	drawEdge2(edge2);
	drawEdge3(edge3);
	drawEdge4(edge4);
	drawCenter1(blue);
	
}
void RotateRight(float t,int uptobot) // up to bot
{
	mvstack.push(model_view);
	if (uptobot)
	{
		model_view *= RotateX(90 * t);
	}
	else
	{
		model_view *= RotateX(-90 * t);
	}
	drawCorner2(corner2);
	drawEdge2(edge2);
	drawCorner3(corner3);
	drawEdge6(edge6);
	drawEdge7(edge7);
	drawCenter3(yellow);
	drawCorner6(corner6);
	drawCorner7(corner7);
	drawEdge10(edge10);
	model_view = mvstack.top();
	mvstack.pop();
	drawCenter2(orange);
	drawCenter4(red);
	drawEdge1(edge1);	
	drawEdge3(edge3);
	drawCenter1(blue);
	drawCenter6(green);
	drawEdge9(edge9);
	drawEdge11(edge11);
	drawCorner1(corner1);
	drawCorner4(corner4);
	drawEdge4(edge4);
	drawEdge5(edge5);
	drawEdge8(edge8);
	drawCenter5(white);
	drawCorner5(corner5);
	drawEdge12(edge12);
	drawCorner8(corner8);
}
void drawInCube()
{	
	mvstack.push(model_view);
	drawCorner1(corner1);
	drawCorner2(corner2);
	drawCorner3(corner3);
	drawCorner4(corner4);
	drawEdge1(edge1);
	drawEdge2(edge2);
	drawEdge3(edge3);
	drawEdge4(edge4);
	drawCenter1(blue);
	model_view = mvstack.top();
	mvstack.pop();
	drawCenter2(orange);
	drawCenter3(yellow);
	drawCenter4(red);
	drawCenter5(white);
	drawEdge5(edge5);
	drawEdge6(edge6);
	drawEdge7(edge7);
	drawEdge8(edge8);
	drawCenter6(green);
	drawCorner5(corner5);
	drawCorner6(corner6);
	drawCorner7(corner7);
	drawCorner8(corner8);
	drawEdge9(edge9);
	drawEdge10(edge10);
	drawEdge11(edge11);
	drawEdge12(edge12);
}
void ChangeColorRotateRight(int bot)
{
	if (bot == 0)
	{
		cornertemp.setupbot(corner6.upbot());
		cornertemp.setleft(corner6.left());
		cornertemp.setright(corner6.right());
		corner6.setleft(corner2.upbot());
		corner6.setupbot(corner2.left());
		corner6.setright(corner2.right());
		corner2.setleft(corner3.upbot());
		corner2.setupbot(corner3.left());
		corner2.setright(corner3.right());
		corner3.setleft(corner7.upbot());
		corner3.setupbot(corner7.left());
		corner3.setright(corner7.right());
		corner7.setleft(cornertemp.upbot());
		corner7.setupbot(cornertemp.left());
		corner7.setright(cornertemp.right());
		edgetemp.setupbotandleft(edge6.upbotandleft());
		edgetemp.setnoupbotandright(edge6.noupbotandright());
		edge6.setupbotandleft(edge2.upbotandleft());
		edge6.setnoupbotandright(edge2.noupbotandright());
		edge2.setupbotandleft(edge7.upbotandleft());
		edge2.setnoupbotandright(edge7.noupbotandright());
		edge7.setupbotandleft(edge10.upbotandleft());
		edge7.setnoupbotandright(edge10.noupbotandright());
		edge10.setupbotandleft(edgetemp.upbotandleft());
		edge10.setnoupbotandright(edgetemp.noupbotandright());
	}
	else
	{
		cornertemp.setupbot(corner6.upbot());
		cornertemp.setleft(corner6.left());
		cornertemp.setright(corner6.right());
		corner6.setleft(corner7.upbot());
		corner6.setupbot(corner7.left());
		corner6.setright(corner7.right());
		corner7.setleft(corner3.upbot());
		corner7.setupbot(corner3.left());
		corner7.setright(corner3.right());
		corner3.setleft(corner2.upbot());
		corner3.setupbot(corner2.left());
		corner3.setright(corner2.right());
		corner2.setleft(cornertemp.upbot());
		corner2.setupbot(cornertemp.left());
		corner2.setright(cornertemp.right());
		edgetemp.setupbotandleft(edge6.upbotandleft());
		edgetemp.setnoupbotandright(edge6.noupbotandright());
		edge6.setupbotandleft(edge10.upbotandleft());
		edge6.setnoupbotandright(edge10.noupbotandright());
		edge10.setupbotandleft(edge7.upbotandleft());
		edge10.setnoupbotandright(edge7.noupbotandright());
		edge7.setupbotandleft(edge2.upbotandleft());
		edge7.setnoupbotandright(edge2.noupbotandright());
		edge2.setupbotandleft(edgetemp.upbotandleft());
		edge2.setnoupbotandright(edgetemp.noupbotandright());
	}
}
void ChangeColorRotateTop(int right)
{
	if (!right)
	{
		cornertemp.setupbot(corner1.upbot());
		cornertemp.setleft(corner1.left());
		cornertemp.setright(corner1.right());
		corner1.setleft(corner4.right());
		corner1.setupbot(corner4.upbot());
		corner1.setright(corner4.left());
		corner4.setleft(corner3.left());
		corner4.setupbot(corner3.upbot());
		corner4.setright(corner3.right());
		corner3.setleft(corner2.right());
		corner3.setupbot(corner2.upbot());
		corner3.setright(corner2.left());
		corner2.setleft(cornertemp.left());
		corner2.setupbot(cornertemp.upbot());
		corner2.setright(cornertemp.right());
		edgetemp.setupbotandleft(edge1.upbotandleft());
		edgetemp.setnoupbotandright(edge1.noupbotandright());
		edge1.setupbotandleft(edge4.upbotandleft());
		edge1.setnoupbotandright(edge4.noupbotandright());
		edge4.setupbotandleft(edge3.upbotandleft());
		edge4.setnoupbotandright(edge3.noupbotandright());
		edge3.setupbotandleft(edge2.upbotandleft());
		edge3.setnoupbotandright(edge2.noupbotandright());
		edge2.setupbotandleft(edgetemp.upbotandleft());
		edge2.setnoupbotandright(edgetemp.noupbotandright());
	}
}
void ChangeColorRotateLeft(int bot)
{
	if (bot)
	{
		cornertemp.setupbot(corner5.upbot());
		cornertemp.setleft(corner5.left());
		cornertemp.setright(corner5.right());
		corner5.setleft(corner8.left());
		corner5.setupbot(corner8.right());
		corner5.setright(corner8.upbot());
		corner8.setleft(corner4.left());
		corner8.setupbot(corner4.right());
		corner8.setright(corner4.upbot());
		corner4.setleft(corner1.left());
		corner4.setupbot(corner1.right());
		corner4.setright(corner1.upbot());
		corner1.setleft(cornertemp.left());
		corner1.setupbot(cornertemp.right());
		corner1.setright(cornertemp.upbot());
		edgetemp.setupbotandleft(edge4.upbotandleft());
		edgetemp.setnoupbotandright(edge4.noupbotandright());
		edge4.setupbotandleft(edge5.noupbotandright());
		edge4.setnoupbotandright(edge5.upbotandleft());
		edge5.setupbotandleft(edge12.noupbotandright());
		edge5.setnoupbotandright(edge12.upbotandleft());
		edge12.setupbotandleft(edge8.noupbotandright());
		edge12.setnoupbotandright(edge8.upbotandleft());
		edge8.setupbotandleft(edgetemp.noupbotandright());
		edge8.setnoupbotandright(edgetemp.upbotandleft());
	}
}
void ChangeColorRotateBot(int right)
{
	if (!right)
	{
		cornertemp.setupbot(corner5.upbot());
		cornertemp.setleft(corner5.left());
		cornertemp.setright(corner5.right());
		corner5.setleft(corner8.right());
		corner5.setupbot(corner8.upbot());
		corner5.setright(corner8.left());
		corner8.setleft(corner7.left());
		corner8.setupbot(corner7.upbot());
		corner8.setright(corner7.right());
		corner7.setleft(corner6.right());
		corner7.setupbot(corner6.upbot());
		corner7.setright(corner6.left());
		corner6.setleft(cornertemp.left());
		corner6.setupbot(cornertemp.upbot());
		corner6.setright(cornertemp.right());
		edgetemp.setupbotandleft(edge9.upbotandleft());
		edgetemp.setnoupbotandright(edge9.noupbotandright());
		edge9.setupbotandleft(edge12.upbotandleft());
		edge9.setnoupbotandright(edge12.noupbotandright());
		edge12.setupbotandleft(edge11.upbotandleft());
		edge12.setnoupbotandright(edge11.noupbotandright());
		edge11.setupbotandleft(edge10.upbotandleft());
		edge11.setnoupbotandright(edge10.noupbotandright());
		edge10.setupbotandleft(edgetemp.upbotandleft());
		edge10.setnoupbotandright(edgetemp.noupbotandright());	
	}
	else
	{
		cornertemp.setupbot(corner5.upbot());
		cornertemp.setleft(corner5.left());
		cornertemp.setright(corner5.right());
		corner5.setleft(corner6.left());
		corner5.setupbot(corner6.upbot());
		corner5.setright(corner6.right());
		corner6.setleft(corner7.right());
		corner6.setupbot(corner7.upbot());
		corner6.setright(corner7.left());
		corner7.setleft(corner8.left());
		corner7.setupbot(corner8.upbot());
		corner7.setright(corner8.right());
		corner8.setleft(cornertemp.right());
		corner8.setupbot(cornertemp.upbot());
		corner8.setright(cornertemp.left());
		edgetemp.setupbotandleft(edge9.upbotandleft());
		edgetemp.setnoupbotandright(edge9.noupbotandright());
		edge9.setupbotandleft(edge10.upbotandleft());
		edge9.setnoupbotandright(edge10.noupbotandright());
		edge10.setupbotandleft(edge11.upbotandleft());
		edge10.setnoupbotandright(edge11.noupbotandright());
		edge11.setupbotandleft(edge12.upbotandleft());
		edge11.setnoupbotandright(edge12.noupbotandright());
		edge12.setupbotandleft(edgetemp.upbotandleft());
		edge12.setnoupbotandright(edgetemp.noupbotandright());
	}
}
void CalculateFrameRate()
{
	static float framesPerSecond = 0.0f;       // This will store our fps
	static float lastTime = 0.0f;       // This will hold the time from the last frame
	float currentTime = GetTickCount() * 0.001f;
	++framesPerSecond;
	if (currentTime - lastTime > 1.0f)
	{
		lastTime = currentTime;
		std::cout << "Current Frames Per Second: " << int(framesPerSecond) << std::endl;
		framesPerSecond = 0;
	}
}
vec4 unRotatedPoint = eye;
vec4 unRotatedPoint1(15, 15, 15);
int run = 0;
void display(void)
{
	basis_id = 0;
	glClearColor(.1, .1, .2, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	set_color(.6, .6, .6);

	model_view = LookAt(eye, ref, up);
	float rotationBeginTime = 0;
	float timeToRotate = 5;
	float rotationSceneTime = TIME - rotationBeginTime;
	//if (rotationSceneTime > 0 && rotationSceneTime < timeToRotate)
		//eye = RotateY(360 / timeToRotate * rotationSceneTime) * unRotatedPoint;
	float time1;
	
	model_view *= orientation;
	model_view *= Scale(zoom);	
	CalculateFrameRate();
	/*if (0 <= TIME&&TIME < 5)
	{
		mvstack.push(model_view);		
		model_view *= Translate(0, -5, 0);
		model_view *= Translate(0, TIME*7, 0);
		model_view *= Scale(10, 10, 10);
		set_color(0.753, 0.753, 0.753);
		drawSphere2();
		model_view *= Translate(0, 5, 0);
		set_color(0, 0, 0);
		drawSphere();
		model_view = mvstack.top();
		mvstack.pop();

		mvstack.push(model_view);	
		model_view *= Translate(0, -5, 0);
		model_view *= Scale(13, 0.1, 13);
		set_color(0.545, 0.271, 0.075);
		drawSphere();
		model_view = mvstack.top();
		mvstack.pop();
		
		drawInCube();
	}
	else if (5<=TIME &&TIME<10)
	{
		if (run == 0)
		{
			eye = unRotatedPoint1;
			run = 1;
		}
		time1 = TIME;
		eye = RotateY(360 / 5 * time1) * unRotatedPoint1;
		eye = RotateZ(360 / 5 * time1) * unRotatedPoint1;
		eye = RotateX(360 / 5 * time1) * unRotatedPoint1;	
		drawInCube();
	}
	else if (10<=TIME && TIME<11) //1
	{
		time1 = TIME - 10;
		RotateRight(time1,0); //right up
	}
	else if (11 <= TIME&&TIME <12) //2
	{
		time1 = TIME - 11;
		if (run == 1)
		{
			ChangeColorRotateRight(0); //rightup
			run = 0;
		}
		RotateLeft(time1,1);  //left bot
	}
	else if (12 <= TIME&&TIME < 13) //3
	{
		if (run == 0)
		{
			ChangeColorRotateLeft(1);  //left bot
			run = 1;
		}
		time1 = TIME - 12;
		RotateBot(time1,0); //Bot left
	}
	else if (13 <= TIME&&TIME < 14) //4
	{
		if (run == 1)
		{
			ChangeColorRotateBot(0); //Bot left
			run = 0;
		}
		time1 = TIME - 13;
		RotateTop(time1, 0);//top left
	}
	else if (14 <= TIME&&TIME < 15) //5
	{
		if (run == 0)
		{
			ChangeColorRotateTop(0); //top left
			run = 1;
		}
		time1 = TIME - 14;
		RotateLeft(time1, 1);  //left bot
	}
	else if (15 <= TIME&&TIME < 16) //6
	{
		if (run == 1)
		{
			ChangeColorRotateLeft(1);  //left bot
			run = 0;
		}
		time1 = TIME - 15;
		RotateLeft(time1, 1);
	}
	else if (16<= TIME&&TIME < 17) //7
	{
		if (run == 0)
		{
			ChangeColorRotateLeft(1);  //left bot
			run = 1;
		}
		time1 = TIME - 16;
		RotateTop(time1, 0); //top left
	}
	else if (17 <= TIME&&TIME < 18) //8
	{
		if (run == 1)
		{
			ChangeColorRotateTop(0); //top left
			run = 0;
		}
		time1 = TIME - 17;
		RotateRight(time1, 1); //right bot
	}
	else if (18 <= TIME&&TIME < 19) //9
	{
		if (run == 0)
		{
			ChangeColorRotateRight(1); //right bot 
			run = 1;
		}
		time1 = TIME - 18;
		RotateBot(time1, 0); //bot left
	}
	else if (19 <= TIME&&TIME < 20) //10
	{
		if (run == 1)
		{
			ChangeColorRotateBot(0); //Bot left
			run = 0;
		}
		time1 = TIME - 19;
		RotateTop(time1, 0);  //top left
	}
	else if (20 <= TIME&&TIME < 21) //11
	{
		if (run == 0)
		{
			ChangeColorRotateTop(0); //top left
			run = 1;
		}
		time1 = TIME - 20;
		RotateLeft(time1, 1); //left bot
	}
	else if (21 <= TIME&&TIME < 22) //12
	{
		if (run == 1)
		{
			ChangeColorRotateLeft(1);  //left bot
			run = 0;
		}
		time1 = TIME - 21;
		RotateTop(time1, 0); //top left
	}
	else if (22 <= TIME&&TIME < 23) //13
	{
		if (run == 0)
		{
			ChangeColorRotateTop(0); //top left
			run = 1;
		}
		time1 = TIME - 22;
		RotateRight(time1, 1); //right bot
	}
	else if (23 <= TIME&&TIME < 24) //14
	{
		if (run == 1)
		{
			ChangeColorRotateRight(1); //right bot 
			run = 0;
		}
		time1 = TIME - 23;
		RotateLeft(time1, 1); //left bot
	}
	else if (24 <= TIME&&TIME < 25) //15
	{
		if (run == 0)
		{
			ChangeColorRotateLeft(1);  //left bot
			run = 1;
		}
		time1 = TIME - 24;
		RotateTop(time1, 0); //top left
	}
	else if (25 <= TIME&&TIME < 26) //16
	{
		if (run == 1)
		{
			ChangeColorRotateTop(0); //top left
			run = 0;
		}
		time1 = TIME - 25;
		RotateRight(time1, 1); //right bot
	}
	else if (26 <= TIME&&TIME < 27) //17
	{
		if (run == 0)
		{
			ChangeColorRotateRight(1); //right bot 
			run = 1;
		}
		time1 = TIME - 26;
		RotateBot(time1, 1);  //bot right
	}
	else if (27 <= TIME&&TIME < 28) //18
	{
		if (run == 1)
		{
			ChangeColorRotateBot(1); //Bot right
			run = 0;
		}
		time1 = TIME - 27;
		RotateLeft(time1, 1); //left bot
	}
	else if (28 <= TIME&&TIME < 29) //19
	{
		if (run == 0)
		{
			ChangeColorRotateLeft(1);  //left bot
			run = 1;
		}
		time1 = TIME - 28;
		RotateTop(time1, 0);  //top left
	}
	else if (29 <= TIME&&TIME < 37) 
	{
		if (run == 1)
		{
			ChangeColorRotateTop(0); //top left
			run = 0;
		}
		time1 = TIME - 29;
		eye = RotateY(360 / 5 * time1) * unRotatedPoint1;
		eye = RotateZ(120 / 5 * time1) * unRotatedPoint1;
		eye = RotateX(180 / 5 * time1) * unRotatedPoint1;
		drawInCube();
	}
	else if (37 <= TIME&&TIME < 45)
	{
		if (run == 0)
		{
			eye = unRotatedPoint;
			vec4 neweye(-45, -45,-30, 1);
			eye = eye + neweye;
			run = 1;
		}
		drawGround();
	}*/
	drawMidterm();
	glutSwapBuffers();
}

void myReshape(int w, int h)	// Handles window sizing and resizing.
{
	mat4 projection = Perspective(50, (float)w / h, 1, 1000);
	glUniformMatrix4fv(uProjection, 1, GL_FALSE, transpose(projection));
	glViewport(0, 0, g_width = w, g_height = h);
}

void instructions() {
	std::cout << "Press:" << '\n' <<
		"  r to restore the original view." << '\n' <<
		"  0 to restore the original state." << '\n' <<
		"  a to toggle the animation." << '\n' <<
		"  b to show the next basis's axes." << '\n' <<
		"  B to show the previous basis's axes." << '\n' <<
		"  q to quit." << '\n';
}

void myKey(unsigned char key, int x, int y)
{
	switch (key) {
	case 'q':   case 27:				// 27 = esc key
		exit(0);
	case 'b':
		std::cout << "Basis: " << ++basis_to_display << '\n';
		break;
	case 'B':
		std::cout << "Basis: " << --basis_to_display << '\n';
		break;
	case 'a':							// toggle animation           		
		if (animate) std::cout << "Elapsed time " << TIME << '\n';
		animate = 1 - animate;
		break;
	case '0':							// Add code to reset your object here.
		TIME = 0;	TM.Reset();
	case 'r':
		orientation = mat4();
		break;
	}
	glutPostRedisplay();
}

int main()
{
	char title[] = "Title";
	int argcount = 1;	 char* title_ptr = title;
	glutInit(&argcount, &title_ptr);
	glutInitWindowPosition(230, 70);
	glutInitWindowSize(g_width, g_height);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutCreateWindow(title);
#if !defined(__APPLE__) && !defined(EMSCRIPTEN)
	glewExperimental = GL_TRUE;
	glewInit();
#endif
	std::cout << "GL version " << glGetString(GL_VERSION) << '\n';
	instructions();
	init();

	glutDisplayFunc(display);
	glutIdleFunc(idleCallBack);
	glutReshapeFunc(myReshape);
	glutKeyboardFunc(myKey);
	glutMouseFunc(myMouseCallBack);
	glutMotionFunc(myMotionCallBack);
	glutPassiveMotionFunc(myPassiveMotionCallBack);

	glutMainLoop();
}