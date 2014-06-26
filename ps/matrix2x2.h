#ifndef MATRIX_2X2_H
#define MATRIX_2X2_H

#include <complex>

class Matrix2x2
{
public:
	template<typename LHS_T, typename RHS_T>
	static void Assign(std::complex<LHS_T>* dest, const std::complex<RHS_T>* source)
	{
		for(size_t p=0; p!=4; ++p)
			dest[p] = source[p];
	}
	
	template<typename T, typename RHS_T>
	static void Add(std::complex<T>* dest, RHS_T * rhs)
	{
		for(size_t p=0; p!=4; ++p)
			dest[p] += rhs[p];
	}
	
	template<typename T>
	static void Subtract(std::complex<T>* dest, std::complex<T>* rhs)
	{
		for(size_t p=0; p!=4; ++p)
			dest[p] -= rhs[p];
	}
	
	template<typename T>
	static bool IsFinite(std::complex<T>* matrix)
	{
		return
			std::isfinite(matrix[0].real()) && std::isfinite(matrix[0].imag()) &&
			std::isfinite(matrix[1].real()) && std::isfinite(matrix[1].imag()) &&
			std::isfinite(matrix[2].real()) && std::isfinite(matrix[2].imag()) &&
			std::isfinite(matrix[3].real()) && std::isfinite(matrix[3].imag());
	}
	
	template<typename T>
	static void ScalarMultiply(std::complex<T>* dest, T factor)
	{
		for(size_t p=0; p!=4; ++p)
			dest[p] *= factor;
	}
	
	template<typename T>
	static void ScalarMultiply(T* dest, T factor)
	{
		for(size_t p=0; p!=4; ++p)
			dest[p] *= factor;
	}
	
	template<typename T, typename RHS>
	static void MultiplyAdd(std::complex<T>* dest, const RHS* rhs, T factor)
	{
		for(size_t p=0; p!=4; ++p)
			dest[p] += rhs[p] * factor;
	}
	
	template<typename ComplType, typename LHS_T, typename RHS_T>
	static void ATimesB(std::complex<ComplType>* dest, const LHS_T* lhs, const RHS_T* rhs)
	{
		dest[0] = lhs[0] * rhs[0] + lhs[1] * rhs[2];
		dest[1] = lhs[0] * rhs[1] + lhs[1] * rhs[3];
		dest[2] = lhs[2] * rhs[0] + lhs[3] * rhs[2];
		dest[3] = lhs[2] * rhs[1] + lhs[3] * rhs[3];
	}
	
	static void PlusATimesB(std::complex<double> *dest, const std::complex<double> *lhs, const std::complex<double> *rhs)
	{
		dest[0] += lhs[0] * rhs[0] + lhs[1] * rhs[2];
		dest[1] += lhs[0] * rhs[1] + lhs[1] * rhs[3];
		dest[2] += lhs[2] * rhs[0] + lhs[3] * rhs[2];
		dest[3] += lhs[2] * rhs[1] + lhs[3] * rhs[3];
	}
	
	template<typename ComplType, typename LHS_T, typename RHS_T>
	static void ATimesHermB(std::complex<ComplType> *dest, const LHS_T* lhs, const RHS_T* rhs)
	{
		dest[0] = lhs[0] * std::conj(rhs[0]) + lhs[1] * std::conj(rhs[1]);
		dest[1] = lhs[0] * std::conj(rhs[2]) + lhs[1] * std::conj(rhs[3]);
		dest[2] = lhs[2] * std::conj(rhs[0]) + lhs[3] * std::conj(rhs[1]);
		dest[3] = lhs[2] * std::conj(rhs[2]) + lhs[3] * std::conj(rhs[3]);
	}

	template<typename ComplType, typename LHS_T, typename RHS_T>
	static void PlusATimesHermB(std::complex<ComplType> *dest, const LHS_T* lhs, const RHS_T* rhs)
	{
		dest[0] += lhs[0] * std::conj(rhs[0]) + lhs[1] * std::conj(rhs[1]);
		dest[1] += lhs[0] * std::conj(rhs[2]) + lhs[1] * std::conj(rhs[3]);
		dest[2] += lhs[2] * std::conj(rhs[0]) + lhs[3] * std::conj(rhs[1]);
		dest[3] += lhs[2] * std::conj(rhs[2]) + lhs[3] * std::conj(rhs[3]);
	}

	template<typename ComplType, typename LHS_T, typename RHS_T>
	static void HermATimesB(std::complex<ComplType> *dest, const LHS_T* lhs, const RHS_T* rhs)
	{
		dest[0] = std::conj(lhs[0]) * rhs[0] + std::conj(lhs[2]) * rhs[2];
		dest[1] = std::conj(lhs[0]) * rhs[1] + std::conj(lhs[2]) * rhs[3];
		dest[2] = std::conj(lhs[1]) * rhs[0] + std::conj(lhs[3]) * rhs[2];
		dest[3] = std::conj(lhs[1]) * rhs[1] + std::conj(lhs[3]) * rhs[3];
	}

	static void HermATimesHermB(std::complex<double> *dest, const std::complex<double> *lhs, const std::complex<double> *rhs)
	{
		dest[0] = std::conj(lhs[0]) * std::conj(rhs[0]) + std::conj(lhs[2]) * std::conj(rhs[1]);
		dest[1] = std::conj(lhs[0]) * std::conj(rhs[2]) + std::conj(lhs[2]) * std::conj(rhs[3]);
		dest[2] = std::conj(lhs[1]) * std::conj(rhs[0]) + std::conj(lhs[3]) * std::conj(rhs[1]);
		dest[3] = std::conj(lhs[1]) * std::conj(rhs[2]) + std::conj(lhs[3]) * std::conj(rhs[3]);
	}
	
	template<typename ComplType, typename LHS_T, typename RHS_T>
	static void PlusHermATimesB(std::complex<ComplType> *dest, const LHS_T* lhs, const RHS_T* rhs)
	{
		dest[0] += std::conj(lhs[0]) * rhs[0] + std::conj(lhs[2]) * rhs[2];
		dest[1] += std::conj(lhs[0]) * rhs[1] + std::conj(lhs[2]) * rhs[3];
		dest[2] += std::conj(lhs[1]) * rhs[0] + std::conj(lhs[3]) * rhs[2];
		dest[3] += std::conj(lhs[1]) * rhs[1] + std::conj(lhs[3]) * rhs[3];
	}

	template<typename T>
	static bool Invert(T* matrix)
	{
		T d = ((matrix[0]*matrix[3]) - (matrix[1]*matrix[2]));
		if(d == 0.0)
			return false;
		T oneOverDeterminant = 1.0 / d;
		T temp;
		temp      = matrix[3] * oneOverDeterminant;
		matrix[1] = -matrix[1] * oneOverDeterminant;
		matrix[2] = -matrix[2] * oneOverDeterminant;
		matrix[3] = matrix[0] * oneOverDeterminant;
		matrix[0] = temp;
		return true;
	}

	static bool MultiplyWithInverse(std::complex<double>* lhs, const std::complex<double>* rhs)
	{
		std::complex<double> d = ((rhs[0]*rhs[3]) - (rhs[1]*rhs[2]));
		if(d == 0.0) return false;
		std::complex<double> oneOverDeterminant = 1.0 / d;
		std::complex<double> temp[4];
		temp[0] = rhs[3] * oneOverDeterminant;
		temp[1] = -rhs[1] * oneOverDeterminant;
		temp[2] = -rhs[2] * oneOverDeterminant;
		temp[3] = rhs[0] * oneOverDeterminant;
		
		std::complex<double> temp2 = lhs[0];
		lhs[0] = lhs[0] * temp[0] + lhs[1] * temp[2];
		lhs[1] =  temp2 * temp[1] + lhs[1] * temp[3];
		
		temp2 = lhs[2];
		lhs[2] = lhs[2] * temp[0] + lhs[3] * temp[2];
		lhs[3] = temp2 * temp[1] + lhs[3] * temp[3];
		return true;
	}

	static void SingularValues(const std::complex<double>* matrix, double &e1, double &e2)
	{
		// This is not the ultimate fastest method, since we
		// don't need to calculate the imaginary values of b,c at all.
		// Calculate M M^H
		std::complex<double> temp[4] = {
			matrix[0] * std::conj(matrix[0]) + matrix[1] * std::conj(matrix[1]),
			matrix[0] * std::conj(matrix[2]) + matrix[1] * std::conj(matrix[3]),
			matrix[2] * std::conj(matrix[0]) + matrix[3] * std::conj(matrix[1]),
			matrix[2] * std::conj(matrix[2]) + matrix[3] * std::conj(matrix[3])
		};
		// Use quadratic formula, with a=1.
		double
			b = -temp[0].real() - temp[3].real(),
			c = temp[0].real()*temp[3].real() - (temp[1]*temp[2]).real(),
			d = b*b - (4.0*1.0)*c,
			sqrtd = sqrt(d);

		e1 = sqrt((-b + sqrtd) * 0.5);
		e2 = sqrt((-b - sqrtd) * 0.5);
	}
	
	template<typename T>
	static T RotationAngle(const std::complex<T>* matrix)
	{
		return std::atan2((matrix[2].real()-matrix[1].real())*0.5, (matrix[0].real()+matrix[3].real())*0.5);
	}
	
	template<typename T>
	static void RotationMatrix(std::complex<T>* matrix, double alpha)
	{
		T cosAlpha, sinAlpha;
		sincos(alpha, &sinAlpha, &cosAlpha);
		matrix[0] = cosAlpha; matrix[1] = -sinAlpha;
		matrix[2] = sinAlpha; matrix[3] = cosAlpha;
	}
};

#endif
