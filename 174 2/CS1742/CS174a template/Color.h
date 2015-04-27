#ifndef COLOR_H       
#define COLOR_H
//#include <iostream>	
//using namespace std;

class Color
{
public:
	Color(float a, float b, float c)
	{
		h[0] = a;
		h[1] = b;
		h[2] = c;
	}
	Color(){};
	float first()
	{
		return h[0];
	}
	float second()
	{
		return h[1];
	}
	float third()
	{
		return h[2];
	}
private:
	float h[3];
};

class ColorTwo
{
public:
	ColorTwo(Color a, Color b)
	{
		n = a;
		m = b;
	}
	ColorTwo(){};
	Color upbotandleft()
	{
		return n;
	}
	Color noupbotandright()
	{
		return m;
	}
	void setupbotandleft(Color left)
	{
		n = left;
	}
	void setnoupbotandright(Color right)
	{
		m = right;
	}
private:
	Color n; //upbot and left;
	Color m; //noupandbot and right;
};

class ColorThree
{
public:
	ColorThree(Color up, Color left, Color right)
	{
		n = up;
		m = left;
		l = right;
	}
	ColorThree(){};
	Color upbot()
	{
		return n;
	}
	Color left()
	{
		return m;
	}
	Color right()
	{
		return l;
	}
	void setupbot(Color up)
	{
		n = up;
	}
	void setleft(Color left)
	{
		m = left;
	}
	void setright(Color right)
	{
		l = right;
	}
private:
	Color n;// upbot
	Color m; // left
	Color l; //right
};
#endif
