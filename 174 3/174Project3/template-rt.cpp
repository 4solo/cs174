//
// template-rt.cpp
//

#define _CRT_SECURE_NO_WARNINGS
#include "matm.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

int g_width;
int g_height;

struct Ray
{
	vec4 origin;
	vec4 dir;
};

// TODO: add structs for spheres, lights and anything else you may need.
struct Sphere
{
	mat4 transform;
	mat4 inverse_transform;
	vec4 color;
	vec4 position;
	vec4 scale;
	float K_a, K_d, K_s, K_r, n;
};
struct Light
{
	vec4 position;
	vec4 color;
};

vector<vec4> g_colors;
vector<Sphere> sphere;
vector<Light> light;
float g_left;
float g_right;
float g_top;
float g_bottom;
float g_near;
float b_r;
float b_g;
float b_b;
vec4 Ambient;
string name;
// -------------------------------------------------------------------
// Input file parsing

vec4 toVec4(const string& s1, const string& s2, const string& s3)
{
	stringstream ss(s1 + " " + s2 + " " + s3);
	vec4 result;
	ss >> result.x >> result.y >> result.z;
	result.w = 1.0f;
	return result;
}
float toFloat(const string& s)
{
	stringstream ss(s);
	float f;
	ss >> f;
	return f;
}

void parseLine(const vector<string>& vs)
{
	//TODO: add parsing of NEAR, LEFT, RIGHT, BOTTOM, TOP, SPHERE, LIGHT, BACK, AMBIENT, OUTPUT.
	if (vs[0] == "RES")
	{
		g_width = (int)toFloat(vs[1]);
		g_height = (int)toFloat(vs[2]);
		g_colors.resize(g_width * g_height);
	}
	if (vs[0] == "NEAR") g_near = toFloat(vs[1]);
	if (vs[0] == "LEFT") g_left = toFloat(vs[1]);
	if (vs[0] == "RIGHT") g_right = toFloat(vs[1]);
	if (vs[0] == "BOTTOM") g_bottom = toFloat(vs[1]);
	if (vs[0] == "TOP")  g_top = toFloat(vs[1]);
	if (vs[0] == "SPHERE")
	{
		Sphere sp;
		sp.position = toVec4(vs[2], vs[3], vs[4]);
		sp.scale = toVec4(vs[5], vs[6], vs[7]);
		sp.color = toVec4(vs[8], vs[9], vs[10]);
		sp.K_a = toFloat(vs[11]);
		sp.K_d = toFloat(vs[12]);
		sp.K_s = toFloat(vs[13]);
		sp.K_r = toFloat(vs[14]);
		sp.n = toFloat(vs[15]);
		sp.transform = Translate(sp.position) * Scale(sp.scale.x, sp.scale.y, sp.scale.z)*mat4(1.0);
		//sp.inverse_transform = Scale(1 / sp.scale.x, 1 / sp.scale.y, 1 / sp.scale.z)*Translate(-sp.position)*mat4(1.0);
		InvertMatrix(sp.transform, sp.inverse_transform);
		sphere.push_back(sp);
	}
	if (vs[0] == "LIGHT")
	{
		Light li;
		li.position = toVec4(vs[2], vs[3], vs[4]);
		li.color = toVec4(vs[5], vs[6], vs[7]);
		light.push_back(li);
	}
	if (vs[0] == "BACK")
	{
		b_r = toFloat(vs[1]);
		b_g = toFloat(vs[2]);
		b_b = toFloat(vs[3]);
	}
	if (vs[0] == "AMBIENT")
	{
		Ambient = toVec4(vs[1], vs[2], vs[3]);
		Ambient.w = 0.0f;
	}
	if (vs[0] == "OUTPUT")
	{
		name = vs[1];
	}
}

void loadFile(const char* filename)
{
	ifstream is(filename);
	if (is.fail())
	{
		cout << "Could not open file " << filename << endl;
		exit(1);
	}
	string s;
	vector<string> vs;
	while (!is.eof())
	{
		vs.clear();
		getline(is, s);
		istringstream iss(s);
		while (!iss.eof())
		{
			string sub;
			iss >> sub;
			vs.push_back(sub);
		}
		parseLine(vs);
	}
}


// -------------------------------------------------------------------
// Utilities

void setColor(int ix, int iy, const vec4& color)
{
	int iy2 = g_height - iy - 1; // Invert iy coordinate.
	g_colors[iy2 * g_width + ix] = color;
}


// -------------------------------------------------------------------
// Intersection routine
float tt;
int intersection(const Ray &ray, const int &depth)
{
	// TODO: add your ray-sphere intersection routine here.
	vec4 S = ray.origin;
	vec4 C = ray.dir;
	float t1, t2;
	int size = sphere.size();
	float *t;
	float determinant;
	t = new float[size];
	vec4 S_, C_;
	for (int i = 0; i < size; i++)
	{
		S_ = sphere[i].inverse_transform*S;
		C_ = sphere[i].inverse_transform*C;
		S_.w = 0;
		//cout << dot(S_, C_)*dot(S_, C_) << endl;
		//cout << dot(C_, C_)*(dot(S_, S_) - 1) << endl;
		determinant = dot(S_, C_)*dot(S_, C_) - dot(C_, C_)*(dot(S_, S_) - 1);
		//cout << determinant <<endl;
		if (determinant < 0)
		{
			t[i] = 0;
		}
		else if (determinant == 0)
		{
			t[i] = -dot(S_, C_) / dot(C_, C_);
		}
		else
		{
			t1 = -dot(S_, C_) / dot(C_, C_) + sqrtf(dot(S_, C_)*dot(S_, C_) - dot(C_, C_)*(dot(S_, S_) - 1)) / dot(C_, C_);
			t2 = -dot(S_, C_) / dot(C_, C_) - sqrtf(dot(S_, C_)*dot(S_, C_) - dot(C_, C_)*(dot(S_, S_) - 1)) / dot(C_, C_);
			if (t1 > t2&& t2 > g_near)
				t[i] = t2;
			else
				t[i] = t1;
		}
	}
	int index = 0;
	float temp = t[0];
	for (int i = 1; i < size; i++)
	{
		if (temp <= 1)
		{
			temp = t[i];
			index = i;
		}
		else if (temp>t[i] && t[i] > 1)
		{
			temp = t[i];
			index = i;
		}
		else if (temp > t[i] && t[i] > 0.0001f && depth != 0)
		{
			temp = t[i];
			index = i;
		}
	}
	if (temp <1&& depth==0 )
	{
		return -1;
	}
	else if (temp < 0.001f && depth != 0)
	{
		return -1;
	}
	else if (temp >= 1 && depth == 0)
	{
		tt = t[index];
		return index;
	}
	else if (temp>=0.0001&& depth!=0)
	{
		tt = t[index];
		return index;
	}
}
bool lightintersection(const Ray &ray, const int &index, const int &depth)
{
	vec4 S = ray.origin;
	vec4 C = ray.dir;
	float t1, t2;
	int size = sphere.size();
	float *t;
	float determinant;
	t = new float[size];
	vec4 S_, C_;
	for (int i = 0; i < size; i++)
	{
		if (i == index&&depth != 0)
			continue;
		S_ = sphere[i].inverse_transform*S;
		C_ = sphere[i].inverse_transform*C;
		S_.w = 0;
		//cout << dot(S_, C_)*dot(S_, C_) << endl;
		//cout << dot(C_, C_)*(dot(S_, S_) - 1) << endl;
		determinant = dot(S_, C_)*dot(S_, C_) - dot(C_, C_)*(dot(S_, S_) - 1);
		//cout << determinant <<endl;
		if (determinant < 0)
		{
			t[i] = 0;
		}
		else if (determinant == 0)
		{
			t[i] = -dot(S_, C_) / dot(C_, C_);
		}
		else
		{
			t1 = -dot(S_, C_) / dot(C_, C_) + sqrtf(dot(S_, C_)*dot(S_, C_) - dot(C_, C_)*(dot(S_, S_) - 1)) / dot(C_, C_);
			t2 = -dot(S_, C_) / dot(C_, C_) - sqrtf(dot(S_, C_)*dot(S_, C_) - dot(C_, C_)*(dot(S_, S_) - 1)) / dot(C_, C_);
			if (t1 > t2&& t2 > g_near)
				t[i] = t2;
			else
				t[i] = t1;
		}
	}
	for (int i = 0; i < size; i++)
	{
		if (t[i]<1 && t[i] >= 0.0001)
			return true;
		if (t[i]>g_near && depth==0)
			return true;
	}
	return false;
}

// -------------------------------------------------------------------
// Ray tracing

vec4 trace(const Ray& ray, int depth)
{
	// TODO: implement your ray tracing routine here.
	int index = intersection(ray, depth);
	Ray rayfromhitpoint;
	if (index == -1)
	{
		if (depth != 0)
		{
			return vec4(0, 0, 0, 1.0f);
		}
		else
			return vec4(b_r, b_g, b_b, 1.0f);
	}
	else
	{
		vec4 hitpoint, hitpointnormal;
		vec4 Fcolor(0, 0, 0, 1.0f);
		vec4 alteredPoint;
		vec4 L;
		vec4 R;
		vec4 V;
		vec4 S, C, S_, C_;
		S = ray.origin;
		C = ray.dir;
		S_ = sphere[index].inverse_transform*S;
		C_ = sphere[index].inverse_transform*C;
		hitpointnormal = S_ + tt* C_;
		hitpointnormal.w = 0.0f;
		hitpoint = S + tt * C;
		alteredPoint = transpose(sphere[index].inverse_transform)*hitpointnormal;
		alteredPoint.w = 0.0f;
		//alteredPoint = normalize(alteredPoint);
		vec4 N = alteredPoint;
		int size = light.size();
		Fcolor = sphere[index].color*Ambient*sphere[index].K_a;
		for (int i = 0; i < size; i++)
		{
			L = light[i].position - hitpoint;
			rayfromhitpoint.origin = hitpoint;
			rayfromhitpoint.dir = L;
			L = normalize(L);
			N = normalize(N);
			R = 2 * N*dot(N, L) - L;
			V = ray.origin - hitpoint;
			V = normalize(V);
			R = normalize(R);
			if (!lightintersection(rayfromhitpoint, index, depth) && dot(N, L) >0 ) //dif
				Fcolor = Fcolor + sphere[index].K_d*light[i].color*dot(N, L)*sphere[index].color;
			if (!lightintersection(rayfromhitpoint, index, depth) && dot(R, V) >0&& dot(N,V)>=0) //sp
				Fcolor = Fcolor + sphere[index].K_s*light[i].color*pow(dot(R, V), sphere[index].n);
		}
		if (depth == 3)
			return Fcolor;
		else
		{
			Ray new_ray;
			new_ray.origin = hitpoint;
			alteredPoint = normalize(alteredPoint);
			new_ray.dir = -2 * dot(alteredPoint, ray.dir) * alteredPoint + ray.dir;
			new_ray.dir.w = 0.0f;
			new_ray.dir *= 0.0001;
			vec4 refection_color = trace(new_ray, ++depth);
			//cout << refection_color.x << refection_color.y << refection_color.z << endl;
			Fcolor += sphere[index].K_r * refection_color;
			//cout << "reflection" << endl;
		}
		return Fcolor;
	}
}

vec4 getDir(int ix, int iy)
{
	// TODO: modify this. This should return the direction from the origin
	// to pixel (ix, iy), normalized.
	float x1 = g_left + ((float)ix / g_width)*(g_right - g_left);
	float y1 = g_bottom + ((float)iy / g_height)*(g_top - g_bottom);
	vec4 dir;
	dir.x = x1;
	dir.y = y1;
	dir.z = -1 * g_near;
	dir.w = 0.0f;
	//dir = normalize(dir);
	return dir;
}

void renderPixel(int ix, int iy)
{
	Ray ray;
	ray.origin = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	ray.dir = getDir(ix, iy);
	vec4 color = trace(ray, 0);
	setColor(ix, iy, color);
}

void render()
{
	for (int iy = 0; iy < g_height; iy++)
	for (int ix = 0; ix < g_width; ix++)
		renderPixel(ix, iy);
}


// -------------------------------------------------------------------
// PPM saving

void savePPM(int Width, int Height, char* fname, unsigned char* pixels)
{
	FILE *fp;
	const int maxVal = 255;

	printf("Saving image %s: %d x %d\n", fname, Width, Height);
	fp = fopen(fname, "wb");
	if (!fp) {
		printf("Unable to open file '%s'\n", fname);
		return;
	}
	fprintf(fp, "P6\n");
	fprintf(fp, "%d %d\n", Width, Height);
	fprintf(fp, "%d\n", maxVal);

	for (int j = 0; j < Height; j++) {
		fwrite(&pixels[j*Width * 3], 3, Width, fp);
	}

	fclose(fp);
}

void saveFile(string name)
{
	// Convert color components from floats to unsigned chars.
	// TODO: clamp values if out of range.
	unsigned char* buf = new unsigned char[g_width * g_height * 3];
	for (int y = 0; y < g_height; y++)
	for (int x = 0; x < g_width; x++)
	for (int i = 0; i < 3; i++)
	{
		if (((float*)g_colors[y*g_width + x])[i] > 1) ((float*)g_colors[y*g_width + x])[i] = 1;
		buf[y*g_width * 3 + x * 3 + i] = (unsigned char)(((float*)g_colors[y*g_width + x])[i] * 255.9f);
	}

	// TODO: change file name based on input file name.
	char *cstr = new char[name.length() + 1];
	strcpy(cstr, name.c_str());
	savePPM(g_width, g_height, cstr, buf);
	delete[] buf;
}


// -------------------------------------------------------------------
// Main

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		cout << "Usage: template-rt <input_file.txt>" << endl;
		exit(1);
	}
	loadFile(argv[1]);
	render();
	saveFile(name);
	return 0;
}