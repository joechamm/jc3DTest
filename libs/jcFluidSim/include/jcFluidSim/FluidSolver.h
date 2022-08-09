#ifndef __FLUID_SOLVER_H__
#define __FLUID_SOLVER_H__

#include <eigen/Eigen/Sparse>

#include <vector>
#include <array>
#include <numbers>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using std::vector;

using glm::mat4;
using glm::mat3;
using glm::mat2;
using glm::vec4;
using glm::vec3;
using glm::vec2;

using glm::ivec2;

class FluidSolver
{
	ivec2* pBasisLookupTable_ = nullptr;
	int** ppBasisReverseLookupTable_ = nullptr;
	vec2*** pppVelocityBasis_ = nullptr;
	vec2** ppVelocityField_ = nullptr;
	float* pEigenvalues_ = nullptr;
	float* pEigenvaluesInverse_ = nullptr;
	float* pEigenvaluesInverseRoot_ = nullptr;
	
	Eigen::SparseMatrix<float, Eigen::RowMajor>* pCkMatrices_ = nullptr;
	Eigen::VectorXf wCoefficients_;
	Eigen::VectorXf dwCoefficients_;
	Eigen::VectorXf dwForceCoefficients_;

	std::array<Eigen::VectorXf, 4> rk4Qn_;
	std::array<Eigen::VectorXf, 4> rk4Dwt_;

	float viscosity_ = 0.01f;
	float dt_ = 0.01f;
	float initialAmpScale_ = 1.0f;
	
	int viscCoarse_ = 0;
	int viscFine_ = 100;
	
	int numSquareSamples_ = 5;

	float domainMinX_ = 0.0f;
	float domainMinY_ = 0.0f;
	float domainMaxX_ = std::numbers::pi_v<float>;
	float domainMaxY_ = std::numbers::pi_v<float>;
	float domainWidth_ = std::numbers::pi_v<float>;
	float domainHeight_ = std::numbers::pi_v<float>;

	int	basisDimension_ = 36;
	int	basisDimensionSqrt_ = 6;
	int	oldBasisDimension_ = 36;
	int	velocityGridResolutionX_ = 36;
	int	velocityGridResolutionY_ = 36;

	int* pZeroModes_ = nullptr;

	mat3 domainToUnitTransform_ = mat3 ( 1.0f );
	mat3 unitToDomainTransform_ = mat3 ( 1.0f );
	mat3 unitToGridTransform_ = mat3 ( 1.0f );
	mat3 gridToUnitTransform_ = mat3 ( 1.0f );

public:
	FluidSolver();
	~FluidSolver();

	// initialization
	void initFields();
	
	void createLookupTables();
	void createCkMatrices();
	void createBasisField();
	void createVelocityField ();
	void createEigenValues();
	void createZeroModes ();
	
	void fillLookupTables();
	void precomputeBasisFields();
	void precomputeDynamics();
	
	void basisFieldRect2d(const ivec2& k, float amplitude, vec2** vBasisElem);
	
	// teardown
	void destroyLookupTables();
	void destroyCkMatrices();
	void destroyBasisFields();
	void destroyVelocityField ();
	void destroyEigenValues();
	void destroyZeroModes ();

	// energy helper functions
	float currentEnergy();
	void setEnergy(float energy);

	// coefficients
	float coefDensity(const ivec2& a, const ivec2& b, const ivec2& c);

	// index lookup functions
	ivec2 basisLookup(int index);
	int basisReverseLookup(const ivec2& k);

	// force functions
	Eigen::VectorXf projectForces(const vector<vec4>& forcePath);
	void stir(const vector<vec4>& forcePath);

	Eigen::VectorXf projectDirectForce(const vec2& unitPos, const vec2& forceVec);

	// updates
	void updateFields();
	void expandBasis ();
	void updateTransforms ();
	

	void zeroVelocity(void);
	void zeroModes(void);
	void zeroMultiplesOf(int m);
	void zeroRowsOf(int col);
	void insertMultiplesOf(int m);
	void insertRowsOf(int col);
	void zeroColumnsOf(int row);
	void insertColumnsOf(int row);

	vec2 sampleVelocityAt(const vec2& unitPos);
	vec2 computeNormalVelocityComponent(const vec2& unitPos, const vec2& normComp);

	void setBasisDimension ( int dim );
	void setVelocityResolution ( int res );

	const void* getVelocityField(int* size) const;
};

#endif // !__FLUID_SOLVER_H__
