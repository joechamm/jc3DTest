#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>

typedef float real;

typedef glm::vec2 RealVec2;
typedef glm::vec3 RealVec3;
typedef glm::vec4 RealVec4;
typedef glm::mat2 RealMat2;
typedef glm::mat3 RealMat3;
typedef glm::mat4 RealMat4;

/**
	Abstract function class defined on 2 dimensional space
*/
class Field2d
{
public:
	Field2d () {}

	virtual ~Field2d () {}
};

/**
	Abstract function class defined on 3 dimensional space 
*/
class Field3d
{
public:
	Field3d () {}

	virtual ~Field3d () {}
};

/**
* Abstract function class f(x,y) : R^2 -> R
*/
class ScalarField2d : public Field2d
{
public:
	ScalarField2d () {}
	
	virtual ~ScalarField2d () {}

	virtual real sample ( real x, real y ) const = 0;
	virtual RealVec2 gradient ( real x, real y ) const = 0;
	virtual real laplacian ( real x, real y ) const = 0;
};

/*
* Abstract function class f(x, y) : R^2 -> R^2
*/
class VectorField2d : public Field2d
{
public:
	VectorField2d () {}

	virtual ~VectorField2d () {}

	virtual RealVec2 sample ( real x, real y ) const = 0;
	virtual RealMat2 gradient ( real x, real y ) const = 0;
	virtual real divergence ( real x, real y ) const = 0;
	virtual real curl ( real x, real y ) const = 0;
	virtual RealVec2 laplacian ( real x, real y ) const = 0;
};

/**
*	Abstract function class f(x, y, z) : R^3 -> R
*/
class ScalarField3d : public Field3d
{
public: 
	ScalarField3d () {}

	virtual ~ScalarField3d () {}

	virtual real sample ( real x, real y, real z ) const = 0;
	virtual RealVec3 gradient ( real x, real y, real z ) const = 0;
	virtual real laplacian ( real x, real y, real z ) const = 0;
};

/**
*	Abstract function class f(x, y, z) : R^3 -> R^3
*/
class VectorField3d : public Field3d
{
public:
	VectorField3d () {}

	virtual ~VectorField3d () {}

	virtual RealVec3 sample ( real x, real y, real z ) const = 0;
	virtual RealMat3 gradient ( real x, real y, real z ) const = 0;
	virtual real divergence ( real x, real y, real z ) const = 0;
	virtual RealVec3 curl ( real x, real y, real z ) const = 0;
	virtual RealVec3 laplacian ( real x, real y, real z ) const = 0;
};

// f(x,y) = sin(x)sin(y)
class MyCustomScalarField2d : public ScalarField2d
{
public:
	MyCustomScalarField2d () {}
	
	virtual ~MyCustomScalarField2d () {}

	virtual real sample ( real x, real y ) const override
	{
		return sin ( x ) * sin ( y );
	}

	virtual RealVec2 gradient ( real x, real y ) const override
	{
		return RealVec2 ( cos ( x ) * sin ( y ), cos ( y ) * sin ( x ) );
	}
	
	virtual real laplacian ( real x, real y ) const override
	{
		return -2.0 * sin ( x ) * sin ( y );
	}
};

// f(x,y) = (sin(y) / x, cos(x) / y)
class MyCustomVectorField2d : public VectorField2d
{
public:
	MyCustomVectorField2d () {}
	
	virtual ~MyCustomVectorField2d () {}

	virtual RealVec2 sample ( real x, real y ) const override
	{
		return RealVec2 ( sin ( y ) / x, cos ( x ) / y );
	}
	
	virtual RealMat2 gradient ( real x, real y ) const override
	{
		return RealMat2 ( -sin ( y ) / ( x * x ), -sin ( x ) / y,
			cos ( y ) / x, -cos ( x ) / ( y * y ) );
	}
	
	virtual real divergence ( real x, real y ) const override
	{
		return ( -cos ( x ) / ( y * y ) ) - ( sin ( y ) / ( x * x ) );
	}
	
	virtual real curl ( real x, real y ) const override
	{
		return ( -cos ( y ) / x ) - ( sin ( x ) / y );
	}
	
	virtual RealVec2 laplacian ( real x, real y ) const override
	{
		/*
		* Grad(Div(f)) - Curl(Curl(f)) = 
		* ( 2 sin(x) / y^2 + 2 sin(y) / x^3 + sin(y) / x  ,  2 cos(x) / x^3 + cos(x) / y - 2 cos(y) / x^2 )
		*/
		real xsq = x * x;
		real ysq = y * y;
		real oneoverx = 1.0 / x;
		real oneovery = 1.0 / y;
		real oneoverxsq = oneoverx * oneoverx;
		real oneoverysq = oneovery * oneovery;
		real sx = sin ( x );
		real sy = sin ( y );
		real cx = cos ( x );
		real cy = cos ( y );
		
		// x components 
		real sinyoverx = sy * oneoverx;
		real twosinyoverxcb = 2.0 * sinyoverx * oneoverxsq;
		real twosinxoverysq = 2.0 * sx * oneoverysq;
		// y components
		real cosxovery = cx * oneovery;
		real twocosxoverycb = 2.0 * cosxovery * oneoverysq;
		real twocosyoverxsq = 2.0 * cy * oneoverxsq;
		
		return RealVec2 ( twosinxoverysq + twosinyoverxcb + sinyoverx, twocosxoverycb + cosxovery - twocosyoverxsq );
	}
};
