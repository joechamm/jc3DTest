#include "FluidSolver.h"

#include <stdexcept>

constexpr float kEnergyMaximum = 1000.0f;
constexpr float kEnergyEpsilon = 0.00001f;
constexpr float kDomainMargin = 0.0001f;

FluidSolver::FluidSolver()
{
	float viscCoarse = static_cast< float >( viscCoarse_ );
	float viscFine = static_cast< float >( viscFine_ );
	viscosity_ = ( viscCoarse + viscFine / 200.0f ) / 500.0f;

	initFields ();
	fillLookupTables ();
	precomputeBasisFields ();
	precomputeDynamics ();
	expandBasis ();
	updateTransforms ();
}

FluidSolver::~FluidSolver()
{
	if ( pBasisLookupTable_ != nullptr )
	{
		destroyLookupTables ();
	}

	if ( ppVelocityField_ != nullptr )
	{
		destroyVelocityField ();
	}

	if ( pppVelocityBasis_ != nullptr )
	{
		destroyBasisFields ();
	}

	if ( pEigenvalues_ != nullptr )
	{
		destroyEigenValues ();
	}

	if ( pZeroModes_ != nullptr )
	{
		destroyZeroModes ();
	}

	if ( pCkMatrices_ != nullptr )
	{
		destroyCkMatrices ();
	}
}

void FluidSolver::initFields()
{
	

	updateTransforms ();

	if ( pZeroModes_ != nullptr )
	{
		delete [] pZeroModes_;
		pZeroModes_ = nullptr;
	}

	pZeroModes_ = new int [ basisDimension_ ];

	for ( int k = 0; k < basisDimension_; k++ )
	{
		pZeroModes_ [ k ] = 1;
	}

	wCoefficients_.resize(basisDimension_);
	dwCoefficients_.resize(basisDimension_);
	dwForceCoefficients_.resize(basisDimension_);

	for (int i = 0; i < 4; i++)
	{
		rk4Qn_[i].resize(basisDimension_);
		rk4Dwt_[i].resize(basisDimension_);
	}

	wCoefficients_(0) = 1.0f;

	createCkMatrices();
}

void FluidSolver::createLookupTables()
{
	if (pBasisLookupTable_ != nullptr)
	{
		destroyLookupTables();
	}

	float sqrtBaseDim = floorf(sqrtf(basisDimension_));
	basisDimensionSqrt_ = static_cast<int>(sqrtBaseDim);

	pBasisLookupTable_ = new ivec2[basisDimension_];
	ppBasisReverseLookupTable_ = new int* [basisDimensionSqrt_ + 1];

	for (int i = 0; i < basisDimension_; i++)
	{
		pBasisLookupTable_[i] = ivec2(-1, -1);
	}

	for (int i = 0; i < basisDimensionSqrt_ + 1; i++)
	{
		ppBasisReverseLookupTable_[i] = new int[basisDimensionSqrt_ + 1];
	}

	for (int k1 = 0; k1 < basisDimensionSqrt_ + 1; k1++)
	{
		for (int k2 = 0; k2 < basisDimensionSqrt_ + 1; k2) 
		{
			ppBasisReverseLookupTable_ [ k1 ][ k2 ] = -1;
		}
	}	
}

void FluidSolver::createCkMatrices()
{
	if ( pCkMatrices_ != nullptr )
	{
		destroyCkMatrices();
	}

	pCkMatrices_ = new Eigen::SparseMatrix<float, Eigen::RowMajor> [ basisDimension_ ];

	for ( int k = 0; k < basisDimension_; k++ )
	{
		pCkMatrices_ [ k ].resize ( basisDimension_, basisDimension_ );
	}
}

void FluidSolver::createBasisField()
{
	if ( pppVelocityBasis_ != nullptr )
	{
		destroyBasisFields ();
	}

	int* tmpModes = new int [ basisDimension_ ];

	for ( int i = 0; i < basisDimension_; i++ )
	{
		if ( i < oldBasisDimension_ && pZeroModes_ != nullptr )
		{
			tmpModes [ i ] = pZeroModes_ [ i ];
		}
		else
		{
			tmpModes [ i ] = i;
		}
	}

	if ( pZeroModes_ != nullptr )
	{
		delete [] pZeroModes_;
		pZeroModes_ = nullptr;
	}

	pZeroModes_ = tmpModes;
	
	pppVelocityBasis_ = new vec2 * *[ basisDimension_ ];
	for ( int k = 0; k < basisDimension_; k++ )
	{
		pppVelocityBasis_ [ k ] = new vec2 * [ velocityGridResolutionY_ + 1 ];
		for(int row = 0; row < velocityGridResolutionY_ + 1; row++)
		{
			pppVelocityBasis_ [ k ][ row ] = new vec2 [ velocityGridResolutionX_ + 1 ];
			for ( int col = 0; col < velocityGridResolutionX_ + 1; col++ )
			{
				pppVelocityBasis_ [ k ][ row ][ col ] = vec2(0.0f, 0.0f);
			}
		}
	}
}

void FluidSolver::createVelocityField ()
{
	if ( ppVelocityField_ != nullptr )
	{
		destroyVelocityField ();
	}

	ppVelocityField_ = new vec2 * [ velocityGridResolutionY_ + 1];
	for ( int row = 0; row < velocityGridResolutionY_ + 1; row++ )
	{
		ppVelocityField_ [ row ] = new vec2 [ velocityGridResolutionX_ + 1 ];
	}
}

void FluidSolver::createEigenValues()
{	
	if ( pEigenvalues_ != nullptr )
	{
		destroyEigenValues ();
	}
	
	pEigenvalues_ = new float[ basisDimension_ ];
	pEigenvaluesInverse_ = new float [ basisDimension_ ];
	pEigenvaluesInverseRoot_ = new float [ basisDimension_ ];

	for ( int k = 0; k < basisDimension_; k++ )
	{
		pEigenvalues_ [ k ] = 0.0f;
		pEigenvaluesInverse_ [ k ] = 0.0f;
		pEigenvaluesInverseRoot_ [ k ] = 0.0f;
	}
}

void FluidSolver::createZeroModes ()
{
	if ( pZeroModes_ != nullptr )
	{
		destroyZeroModes ();
	}

	pZeroModes_ = new int[ basisDimension_ ];
}

void FluidSolver::fillLookupTables()
{
	createLookupTables();

	int idx = 0;
	for ( int k1 = 0; k1 < basisDimensionSqrt_ + 1; k1++ )
	{
		for ( int k2 = 0; k2 < basisDimensionSqrt_ + 1; k2++ )
		{
			if ( k1 > basisDimensionSqrt_ || k1 < 1 || k2 > basisDimensionSqrt_ || k2 < 1 )
			{
				continue;
			}

			pBasisLookupTable_ [ idx ] = ivec2 ( k1, k2 );
			ppBasisReverseLookupTable_ [ k1 ][ k2 ] = idx++;
		}
	}
}

void FluidSolver::precomputeBasisFields()
{
	createBasisField();

	for(int k = 0; k < basisDimension_; k++)
	{
		ivec2 idxK = basisLookup ( k );
		basisFieldRect2d ( idxK, initialAmpScale_, pppVelocityBasis_ [ k ] );
	}
}

void FluidSolver::precomputeDynamics()
{
	createEigenValues ();

	for ( int k = 0; k < basisDimension_; k++ )
	{
		ivec2 idxK = basisLookup ( k );

//		float eigenvalue = static_cast< float >( idxK.x * idxK.x + idxK.y * idxK.y ); // eigenvalue for this domain is the sum of the squares of the basis indices
		float eigenvalue = static_cast< float >( glm::dot ( idxK, idxK ) );
		pEigenvalues_ [ k ] = eigenvalue;
		pEigenvaluesInverse_ [ k ] = 1.0f / eigenvalue;
		pEigenvaluesInverseRoot_[k] = 1.0f / sqrtf(eigenvalue);
	}

	for ( int d1 = 0; d1 < basisDimension_; d1++ )
	{
		ivec2 idxA = basisLookup ( d1 );

//		float eigenvalueA = static_cast< float >( idxA.x * idxA.x + idxA.y * idxA.y );
		float eigenvalueA = static_cast< float >( glm::dot ( idxA, idxA ) );

		for ( int d2 = 0; d2 < basisDimension_; d2++ )
		{
			ivec2 idxB = basisLookup ( d2 );

//			float eigenvalueB = static_cast< float >( idxB.x * idxB.x + idxB.y * idxB.y );
			float eigenvalueB = static_cast< float >( glm::dot ( idxB, idxB ) );
			float inverseEigenvalB = 1.0f / eigenvalueB;

			int k1 = basisReverseLookup ( idxA );
			int k2 = basisReverseLookup ( idxB );

			ivec2 antipairs [ 4 ];

			antipairs [ 0 ].x = idxA.x - idxB.x;
			antipairs [ 1 ].x = idxA.x - idxB.x;
			antipairs [ 2 ].x = idxA.x + idxB.x;
			antipairs [ 3 ].x = idxA.x + idxB.x;

			antipairs [ 0 ].y = idxA.y - idxB.y;
			antipairs [ 1 ].y = idxA.y + idxB.y;
			antipairs [ 2 ].y = idxA.y - idxB.y;
			antipairs [ 3 ].y = idxA.y + idxB.y;

			for ( int c = 0; c < 4; c++ )
			{
				int idx = basisReverseLookup ( antipairs [ c ] );
				
				if ( idx != -1 )
				{
					ivec2 idxC = ivec2 ( c, 0 );
					float coefDens = coefDensity ( idxA, idxB, idxC );
					float coef = - coefDens * inverseEigenvalB;
					float coef2 = -coef * ( eigenvalueB / eigenvalueA );

					if ( coef != 0.0f )
					{
						pCkMatrices_ [ idx ].coeffRef ( k1, k2 ) = coef;
					}

					if( coef2 != 0.0f )
					{
						pCkMatrices_ [ idx ].coeffRef ( k2, k1 ) = coef2;
					}
				}
			}
		}
	}
}

void FluidSolver::basisFieldRect2d(const ivec2& k, float amplitude, vec2** vBasisElem)
{
	float scaleX = 1.0f;
	float scaleY = 1.0f;

	if ( k.x != 0 )
	{
//		scaleX = - 1.0f / static_cast< float >( k.x * k.x + k.y * k.y );
		scaleX = -1.0f / static_cast< float >( glm::dot ( k, k ) );
	}
	if ( k.y != 0 )
	{
//		scaleY = - 1.0f / static_cast< float >( k.x * k.x + k.y * k.y );
		scaleY = -1.0f / static_cast< float >( glm::dot ( k, k ) );
	}
	
	float dx = domainWidth_ / static_cast< float >( velocityGridResolutionX_ );
	float dy = domainHeight_ / static_cast< float >( velocityGridResolutionY_ );
	float k1 = static_cast< float >( k.x );
	float k2 = static_cast< float >( k.y );

	for ( int row = 0; row < velocityGridResolutionY_ + 1; row++ )
	{
		float y0 = dy * static_cast< float >( row );
		float y1 = y0 + 0.5f * dy;
		float sinY0K2 = sinf ( y0 * k2 );
		float cosY1K2 = cosf ( y1 * k2 );

		for ( int col = 0; col < velocityGridResolutionX_ + 1; col++ )
		{
			float x0 = dx * static_cast< float >( col );
			float x1 = x0 + 0.5f * dx;
			float sinX0K1 = sinf ( x0 * k1 );
			float cosX1K1 = cosf ( x1 * k1 );

			vBasisElem [ row ][ col ].x = -amplitude * scaleX * k2 * sinX0K1 * cosY1K2;
			vBasisElem [ row ][ col ].y = amplitude * scaleY * k1 * cosX1K1 * sinY0K2;
		}
	}
}

void FluidSolver::destroyLookupTables()
{
	if ( pBasisLookupTable_ != nullptr )
	{
		delete [] pBasisLookupTable_;
		pBasisLookupTable_ = nullptr;
	}

	if ( ppBasisReverseLookupTable_ != nullptr )
	{
		for ( int i = 0; i < basisDimensionSqrt_ + 1; i++ )
		{
			if ( ppBasisReverseLookupTable_ [ i ] != nullptr )
			{
				delete [] ppBasisReverseLookupTable_ [ i ];
				ppBasisReverseLookupTable_ [ i ] = nullptr;
			}
		}
		
		delete [] ppBasisReverseLookupTable_;
		ppBasisReverseLookupTable_ = nullptr;
	}	
}

void FluidSolver::destroyCkMatrices()
{
	if ( pCkMatrices_ != nullptr )
	{
		delete [] pCkMatrices_;
		pCkMatrices_ = nullptr;
	}
}

void FluidSolver::destroyBasisFields()
{
	if ( pppVelocityBasis_ != nullptr )
	{
		for ( int k = 0; k < basisDimension_; k++ )
		{
			if ( pppVelocityBasis_ [ k ] != nullptr )
			{
				for ( int row = 0; row < velocityGridResolutionY_ + 1; row++ )
				{
					if ( pppVelocityBasis_ [ k ][ row ] != nullptr )
					{
						delete [] pppVelocityBasis_ [ k ][ row ];
						pppVelocityBasis_ [ k ][ row ] = nullptr;
					}
				}
				
				delete [] pppVelocityBasis_ [ k ];
				pppVelocityBasis_ [ k ] = nullptr;
			}
		}
		
		delete [] pppVelocityBasis_;
		pppVelocityBasis_ = nullptr;
	}	
}

void FluidSolver::destroyVelocityField ()
{
	if ( ppVelocityField_ != nullptr )
	{
		for ( int row = 0; row < velocityGridResolutionY_ + 1; row++ )
		{
			if ( ppVelocityField_ [ row ] != nullptr )
			{
				delete [] ppVelocityField_ [ row ];
				ppVelocityField_ [ row ] = nullptr;
			}
		}

		delete [] ppVelocityField_;
		ppVelocityField_ = nullptr;
	}
}

void FluidSolver::destroyEigenValues()
{
	if ( pEigenvalues_ != nullptr )
	{
		delete [] pEigenvalues_;
		pEigenvalues_ = nullptr;
	}

	if ( pEigenvaluesInverse_ != nullptr )
	{
		delete [] pEigenvaluesInverse_;
		pEigenvaluesInverse_ = nullptr;
	}

	if ( pEigenvaluesInverseRoot_ != nullptr )
	{
		delete [] pEigenvaluesInverseRoot_;
		pEigenvaluesInverseRoot_ = nullptr;
	}
}

void FluidSolver::destroyZeroModes ()
{
	if ( pZeroModes_ != nullptr )
	{
		delete [] pZeroModes_;
		pZeroModes_ = nullptr;
	}
}

float FluidSolver::currentEnergy()
{
	float energy = 0.0f;
	
	for ( int k = 0; k < basisDimension_; k++ )
	{
		float c2 = wCoefficients_ ( k ) * wCoefficients_ ( k );
		energy += c2 * pEigenvaluesInverse_ [ k ];
	}

	return energy;
}

void FluidSolver::setEnergy(float energy)
{
	float currEnergy = currentEnergy ();
	float energyRatio = sqrtf ( energy / currEnergy );
	
	wCoefficients_ = energyRatio * wCoefficients_;
}

float FluidSolver::coefDensity(const ivec2& a, const ivec2& b, const ivec2& c)
{
	switch ( c.y )
	{
	case 0:
		switch ( c.x )
		{
		case 0:
			return -0.25f * static_cast< float >( a.x * b.y - b.x * a.y );
		case 1:
			return 0.25f * static_cast< float >( a.x * b.y + b.x * a.y );
		case 2:
			return -0.25f * static_cast< float >( a.x * b.y + b.x * a.y );
		case 3:
			return 0.25f * static_cast< float >( a.x * b.y - b.x * a.y );
		default:
			break;
		}
	case 1:
		switch ( c.x )
		{
		case 0:
			return -0.25f * static_cast< float >( a.x * b.y - b.x * a.y );
		case 1:
			return - 0.25f * static_cast< float >( a.x * b.y + b.x * a.y );
		case 2:
			return 0.25f * static_cast< float >( a.x * b.y + b.x * a.y );
		case 3:
			return 0.25f * static_cast< float >( a.x * b.y - b.x * a.y );
		default:
			break;
		}
	case 2:
		switch ( c.x )
		{
		case 0:
			return -0.25f * static_cast< float >( a.x * b.y - b.x * a.y );
		case 1:
			return -0.25f * static_cast< float >( a.x * b.y + b.x * a.y );
		case 2:
			return 0.25f * static_cast< float >( a.x * b.y + b.x * a.y );
		case 3:
			return 0.25f * static_cast< float >( a.x * b.y - b.x * a.y );
		default:
			break;
		}
	case 3:
		switch ( c.x )
		{
		case 0:
			return -0.25f * static_cast< float >( a.x * b.y - b.x * a.y );
		case 1:
			return -0.25f * static_cast< float >( a.x * b.y + b.x * a.y );
		case 2:
			return 0.25f * static_cast< float >( a.x * b.y + b.x * a.y );
		case 3:
			return 0.25f * static_cast< float >( a.x * b.y - b.x * a.y );
		default:
			break;
		}
	default:
		break;
	}

	return 0.0f;
}

ivec2 FluidSolver::basisLookup(int index)
{
	return pBasisLookupTable_ [ index ];
}

int FluidSolver::basisReverseLookup(const ivec2& k)
{
	if(k.x > basisDimensionSqrt_ || k.x < 1 || k.y > basisDimensionSqrt_ || k.y < 1)
		return -1;

	return ppBasisReverseLookupTable_ [ k.x ][ k.y ];
}

Eigen::VectorXf FluidSolver::projectForces(const vector<vec4>& forcePath)
{
	vec2 pos, baseFunc, force;
	Eigen::VectorXf delW ( basisDimension_ );
	for ( int k = 0; k < basisDimension_; k++ )
	{
		ivec2 idxK = basisLookup ( k );
		
		float xfact = 1.0f;
		float yfact = 1.0f;

		if ( idxK.x != 0 )
		{
//			xfact = 1.0f / static_cast< float >( idxK.x * idxK.x + idxK.y * idxK.y );
			xfact = 1.0f / static_cast< float >( glm::dot ( idxK, idxK ) );
		}
		if ( idxK.y != 0 )
		{
//			yfact = 1.0f / static_cast< float >( idxK.x * idxK.x + idxK.y + idxK.y );
			yfact = 1.0f / static_cast< float >( glm::dot ( idxK, idxK ) );
		}

		for(const auto& forcePathPoint : forcePath)
		{
			pos = vec2 ( forcePathPoint.x, forcePathPoint.y );
			
			if ( pos.x < 0.0f || pos.x > 1.0f || pos.y < 0.0f || pos.y > 1.0f )
				continue;

			pos = vec2 ( unitToDomainTransform_ * vec3 ( pos, 1.0f ) );
			force = vec2 ( forcePathPoint.z, forcePathPoint.w );

			// TODO: check the math here

			float kx = static_cast< float >( idxK.x );
			float ky = static_cast< float >( idxK.y );

			float sinKx = sinf ( kx * pos.x );
			float cosKx = cosf ( kx * pos.x );
			float sinKy = sinf ( ky * pos.y );
			float cosKy = cosf ( ky * pos.y );

			baseFunc.x = xfact * kx * sinKx * cosKy * dt_;
//			baseFunc.y = -yfact * ky * cosKx * sinKy * dt_;
			baseFunc.y = -yfact * ky * sinKx * sinKy * dt_;

			delW ( k ) += glm::dot ( force, baseFunc );			
		}		
	}

	return delW;
}

void FluidSolver::stir(const vector<vec4>& forcePath)
{
	Eigen::VectorXf delW = projectForces ( forcePath );
	dwForceCoefficients_ += delW;
}

Eigen::VectorXf FluidSolver::projectDirectForce(const vec2& unitPos, const vec2& forceVec)
{
	Eigen::VectorXf delW ( basisDimension_ );
	vec2 forcePos = vec2 ( unitToDomainTransform_ * vec3 ( unitPos, 1.0f ) );
	for ( int k = 0; k < basisDimension_; k++ )
	{
		ivec2 idxK = basisLookup ( k );

		float xfact = 1.0f;
		float yfact = 1.0f;

		if ( idxK.x != 0 )
		{
			xfact = -1.0f / static_cast< float >( glm::dot ( idxK, idxK ) );
		}
		if ( idxK.y != 0 )
		{
			yfact = -1.0f / static_cast< float >( glm::dot ( idxK, idxK ) );
		}

		float x = forcePos.x;
		float y = forcePos.y;
		float fx = forceVec.x;
		float fy = forceVec.y;

		if ( x > domainMaxX_ ) x = domainMaxX_;
		if ( x < domainMinX_ ) x = domainMinX_;
		if ( y > domainMaxY_ ) y = domainMaxY_;
		if ( y < domainMinY_ ) y = domainMinY_;

		float kx = static_cast< float >( idxK.x );
		float ky = static_cast< float >( idxK.y );

		float sinKx = sinf ( kx * x );
		float cosKx = cosf ( kx * x );
		float sinKy = sinf ( ky * y );
		float cosKy = cosf ( ky * y );

		float vx = -kx * xfact * sinKx * cosKy * dt_;
//		float vy = ky * yfact * cosKx * sinKy * dt_;
		float vy = kx * yfact * cosKx * sinKy * dt_;

		delW ( k ) += ( vx * fx + vy * fy );
	}

	return delW;
}

void FluidSolver::updateFields()
{
	float previousEnergy = currentEnergy ();
	float oneOverSix = 1.0f / 6.0f;
	float frcEnergy = 0.0f;
	float dwEnergy = 0.0f;

	// 4th order Runge-Kutta to update velocity
	rk4Qn_ [ 0 ] = wCoefficients_;

	for ( int k = 0; k < basisDimension_; k++ )
	{
		rk4Dwt_ [ 0 ] ( k ) = rk4Qn_ [ 0 ].dot ( pCkMatrices_ [ k ] * rk4Qn_ [ 0 ] );
		rk4Qn_ [ 1 ] ( k ) = rk4Qn_ [ 0 ] ( k ) + rk4Dwt_ [ 0 ] ( k ) * dt_ * 0.5f;
	}

	for ( int k = 0; k < basisDimension_; k++ )
	{
		rk4Dwt_ [ 1 ] ( k ) = rk4Qn_ [ 1 ].dot ( pCkMatrices_ [ k ] * rk4Qn_ [ 1 ] );
		rk4Qn_ [ 2 ] ( k ) = rk4Qn_ [ 0 ] ( k ) + rk4Dwt_ [ 1 ] ( k ) * dt_ * 0.5f;
	}

	for ( int k = 0; k < basisDimension_; k++ )
	{
		rk4Dwt_ [ 2 ] ( k ) = rk4Qn_ [ 2 ].dot ( pCkMatrices_ [ k ] * rk4Qn_ [ 2 ] );
		rk4Qn_ [ 3 ] ( k ) = rk4Qn_ [ 0 ] ( k ) + rk4Dwt_ [ 2 ] ( k ) * dt_ * 0.5f;
	}

	for ( int k = 0; k < basisDimension_; k++ )
	{
		rk4Dwt_ [ 3 ] ( k ) = rk4Qn_ [ 3 ].dot ( pCkMatrices_ [ k ] * rk4Qn_ [ 3 ] );
		dwCoefficients_ ( k ) = oneOverSix * ( rk4Dwt_ [ 0 ] ( k ) + rk4Dwt_ [ 1 ] ( k ) * 2.0f + rk4Dwt_ [ 2 ] ( k ) * 2.0f + rk4Dwt_ [ 3 ] ( k ) );
	}

	for ( int k = 0; k < basisDimension_; k++ )
	{
		dwEnergy += dwCoefficients_ ( k ) * dwCoefficients_ ( k );
	}

	if ( dwEnergy > kEnergyMaximum )
	{
		float factor = sqrtf ( kEnergyMaximum / dwEnergy );
		dwCoefficients_ *= factor;
	}

	wCoefficients_ += dwCoefficients_ * dt_;

	if ( previousEnergy > 1e-5 )
	{
		setEnergy ( previousEnergy );
	}

	for ( int k = 0; k < basisDimension_; k++ )
	{
		frcEnergy += dwForceCoefficients_ ( k ) * dwForceCoefficients_ ( k );
	}

	if ( frcEnergy > kEnergyMaximum )
	{
		float factor = sqrtf ( kEnergyMaximum / frcEnergy );
		dwForceCoefficients_ *= factor;
	}

	for ( int k = 0; k < basisDimension_; k++ )
	{
		float eigenvalue = -pEigenvalues_ [ k ];
		wCoefficients_ ( k ) = wCoefficients_ ( k ) * expf ( eigenvalue * dt_ * viscosity_ ) + dwForceCoefficients_ ( k );
		dwForceCoefficients_ ( k ) = 0.0f;
	}

	expandBasis ();
}

void FluidSolver::expandBasis ()
{
	for ( int row = 0; row < velocityGridResolutionY_ + 1; row++ )
	{
		for ( int col = 0; col < velocityGridResolutionX_ + 1; col++ )
		{
			vec2 vsum = vec2 ( 0.0f, 0.0f );
			for ( int k = 0; k < basisDimension_; k++ )
			{
				vsum += pppVelocityBasis_ [ k ][ row ][ col ] * wCoefficients_ ( k );
			}

			ppVelocityField_ [ row ][ col ] = vsum;
		}
	}
}

void FluidSolver::updateTransforms ()
{
	float left = static_cast< float >( domainMinX_ );
	float right = static_cast< float >( domainMaxX_ );
	float bottom = static_cast< float >( domainMinY_ );
	float top = static_cast< float >( domainMaxY_ );
	float scaleWidth = right - left;
	float scaleHeight = top - bottom;

	vec3 col0 = vec3 ( 1.0f / scaleWidth, 0.0f, 0.0f );
	vec3 col1 = vec3 ( 0.0f, 1.0f / scaleHeight, 0.0f );
	vec3 col2 = vec3 ( -left / scaleWidth, -bottom / scaleHeight, 1.0f );

	domainToUnitTransform_ = mat3 ( col0, col1, col2 );

	col0 = vec3 ( scaleWidth, 0.0f, 0.0f );
	col1 = vec3 ( 0.0f, scaleHeight, 0.0f );
	col2 = vec3 ( left, bottom, 1.0f );

	unitToDomainTransform_ = mat3 ( col0, col1, col2 );

	left = 0.0f;
	right = static_cast< float >( velocityGridResolutionX_ );
	bottom = 0.0f;
	top = static_cast< float >( velocityGridResolutionY_ );
	scaleWidth = right - left;
	scaleHeight = top - bottom;

	col0 = vec3 ( 1.0f / scaleWidth, 0.0f, 0.0f );
	col1 = vec3 ( 0.0f, 1.0f / scaleHeight, 0.0f );
	col2 = vec3 ( -left / scaleWidth, -bottom / scaleHeight, 1.0f );

	gridToUnitTransform_ = mat3 ( col0, col1, col2 );

	col0 = vec3 ( scaleWidth, 0.0f, 0.0f );
	col1 = vec3 ( 0.0f, scaleHeight, 0.0f );
	col2 = vec3 ( left, bottom, 1.0f );

	unitToGridTransform_ = mat3 ( col0, col1, col2 );
}

void FluidSolver::zeroVelocity(void)
{
	for ( int k = 0; k < basisDimension_; k++ )
	{
		wCoefficients_ ( k ) = 0.0f;
	}
}

void FluidSolver::zeroModes(void)
{
	for ( int k = 0; k < basisDimension_; k++ )
	{
		if ( pZeroModes_ [ k ] == 0 )
		{
			wCoefficients_ ( k ) = 0.0f;
		}
	}
}

void FluidSolver::zeroMultiplesOf(int m)
{
	if ( m == 0 )
	{
		pZeroModes_ [ 0 ] = 0;
		return;
	}

	for ( int i = 1; i < basisDimension_; i++ )
	{
		if ( i % m == 0 )
		{
			pZeroModes_ [ i ] = 0;
		}
	}
}

void FluidSolver::zeroRowsOf(int col)
{
	if ( col < 0 || col > basisDimensionSqrt_ )
	{
		throw std::runtime_error ( "Column out of range" );
		return;
	}

	for ( int row = 0; row < basisDimensionSqrt_ + 1; row++ )
	{
		int k = basisReverseLookup ( ivec2 ( row, col ) );
		pZeroModes_ [ k ] = 0;
	}
}

void FluidSolver::insertMultiplesOf(int m)
{
	if ( m == 0 )
	{
		pZeroModes_ [ 0 ] = 1;
		return;
	}

	for ( int i = 1; i < basisDimension_; i++ )
	{
		if ( i % m == 0 )
		{
			pZeroModes_ [ i ] = 1;
		}
	}
}

void FluidSolver::insertRowsOf(int col)
{
	if ( col < 0 || col > basisDimensionSqrt_ )
	{
		throw std::runtime_error ( "Column out of range" );
		return;
	}

	for ( int row = 0; row < basisDimensionSqrt_ + 1; row++ )
	{
		int k = basisReverseLookup ( ivec2 ( row, col ) );
		pZeroModes_ [ k ] = 1;
	}
}

void FluidSolver::zeroColumnsOf(int row)
{
	if ( row < 0 || row > basisDimensionSqrt_ )
	{
		throw std::runtime_error ( "Row out of range" );
		return;
	}

	for ( int col = 0; col < basisDimensionSqrt_ + 1; col++ )
	{
		int k = basisReverseLookup ( ivec2 ( row, col ) );
		pZeroModes_ [ k ] = 0;
	}
}

void FluidSolver::insertColumnsOf(int row)
{
	if ( row < 0 || row > basisDimensionSqrt_ )
	{
		throw std::runtime_error ( "Row out of range" );
		return;
	}

	for ( int col = 0; col < basisDimensionSqrt_ + 1; col++ )
	{
		int k = basisReverseLookup ( ivec2 ( row, col ) );
		pZeroModes_ [ k ] = 1;
	}
}

vec2 FluidSolver::sampleVelocityAt(const vec2& unitPos)
{
//	vec2 pos, vel00, vel01, vel10, vel11, vel;
//	float fractX, fractY, w00, w01, w10, w11;
//	int col0, col1, row0, row1;
	
	vec2 pos = vec2 ( unitToGridTransform_ * vec3 ( unitPos, 1.0f ) );
	
	int col0 = static_cast< int >( floorf ( pos.x ) );
	col0 = glm::clamp ( col0, 0, velocityGridResolutionX_ );
	
	int col1 = glm::clamp ( col0 + 1, 0, velocityGridResolutionX_ );
	
	int row0 = static_cast< int >( floorf ( pos.y ) );
	row0 = glm::clamp ( row0, 0, velocityGridResolutionY_ );
	
	int row1 = glm::clamp ( row0 + 1, 0, velocityGridResolutionY_ );
	
	float fractX = pos.x - static_cast< float >( col0 );
	float fractY = pos.y - static_cast< float >( row0 );
	if ( col0 == col1 )
	{
		fractX = 0.0f;
	}

	if ( row0 == row1 )
	{
		fractY = 0.0f;
	}

	float w00 = ( 1.0f - fractX ) * ( 1.0f - fractY );
	float w01 = ( 1.0f - fractX ) * fractY;
	float w10 = fractX * ( 1.0f - fractY );
	float w11 = fractX * fractY;

	vec2 vel00 = vec2 ( 0.0f );
	vec2 vel01 = vec2 ( 0.0f );
	vec2 vel10 = vec2 ( 0.0f );
	vec2 vel11 = vec2 ( 0.0f );
	
	for ( int k = 0; k < basisDimension_; k++ )
	{
		vel00 += ( pppVelocityBasis_ [ k ][ row0 ][ col0 ] * wCoefficients_ ( k ) );
		vel01 += ( pppVelocityBasis_ [ k ][ row0 ][ col1 ] * wCoefficients_ ( k ) );
		vel10 += ( pppVelocityBasis_ [ k ][ row1 ][ col0 ] * wCoefficients_ ( k ) );
		vel11 += ( pppVelocityBasis_ [ k ][ row1 ][ col1 ] * wCoefficients_ ( k ) );
	}

	return vel00 * w00 + vel01 * w01 + vel10 * w10 + vel11 * w11;
}

vec2 FluidSolver::computeNormalVelocityComponent(const vec2& unitPos, const vec2& normComp)
{
	vec2 velAtPos = sampleVelocityAt ( unitPos );
	float velDotNorm = glm::dot ( velAtPos, normComp );
	float normSq = glm::dot ( normComp, normComp );
	return ( velDotNorm / normSq ) * normComp;
}

void FluidSolver::setBasisDimension ( int dim )
{
	if ( dim <= 0 )
	{
		throw std::runtime_error ( "Basis dimension out of range" );
		return;
	}

	destroyLookupTables ();
	destroyCkMatrices ();
	destroyBasisFields ();
	destroyVelocityField ();
	destroyEigenValues ();

	oldBasisDimension_ = basisDimension_;

	basisDimension_ = dim;
	basisDimensionSqrt_ = static_cast< int >( glm::floor ( sqrtf ( static_cast< float >( basisDimension_ ) ) ) );

	initFields ();
	fillLookupTables ();
	precomputeBasisFields ();
	precomputeDynamics ();
	expandBasis ();
	updateTransforms ();

	/*
	fillGridPositionBuffer();
	fillGridVelocityBuffer();
	fillParticleBuffers();
	fillDensityTexture();
	
	*/
}

void FluidSolver::setVelocityResolution ( int res )
{
	if ( res <= 0 )
	{
		throw std::runtime_error ( "Invalid Velocity Resolution" );
		return;
	}

	destroyLookupTables ();
	destroyCkMatrices ();
	destroyBasisFields ();
	destroyEigenValues ();

	velocityGridResolutionX_ = res;
	velocityGridResolutionY_ = res;

	initFields ();
	fillLookupTables ();
	precomputeBasisFields ();
	precomputeDynamics ();
	expandBasis ();
	updateTransforms ();
}

const void* FluidSolver::getVelocityField ( int* size ) const
{
	if ( ppVelocityField_ == nullptr )
	{
		if ( size )
		{
			*size = -1;
		}
		return nullptr;
	}

	if ( size )
	{
		*size = (velocityGridResolutionX_ + 1) * (velocityGridResolutionY_ + 1) * sizeof ( vec2 );
	}

	return reinterpret_cast< void* >( ppVelocityField_ );	
}
