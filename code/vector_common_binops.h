#ifndef __QI_COMMON_BINOPS_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Common binary operators, inline friend etc. so proxies can also use these via arg dependent lookup
//
template<typename VectorType,
         typename ScalarType,
         typename VectorArgType = VectorType const &,
         typename ScalarArgType = ScalarType const &>
struct CommonBinaryOps
{
	friend VectorType operator+(VectorArgType a, ScalarArgType b)
	{
		VectorType result(a);
		result += b;
		return result;
	}
	friend VectorType operator+(ScalarArgType b, VectorArgType a) { return a + b; }
	friend VectorType operator+(VectorArgType a, VectorArgType b)
	{
		VectorType result(a);
		result += b;
		return result;
	}

	friend VectorType operator-(VectorArgType a, ScalarArgType b)
	{
		VectorType result(a);
		result -= b;
		return result;
	}
	friend VectorType operator-(ScalarArgType b, VectorArgType a) { return a - b; }
	friend VectorType operator-(VectorArgType a, VectorArgType b)
	{
		VectorType result(a);
		result -= b;
		return result;
	}

	friend VectorType operator*(VectorArgType a, ScalarArgType b)
	{
		VectorType result(a);
		result *= b;
		return result;
	}
	friend VectorType operator*(ScalarArgType b, VectorArgType a) { return a * b; }
	friend VectorType operator*(VectorArgType a, VectorArgType b)
	{
		VectorType result(a);
		result *= b;
		return result;
	}

	friend VectorType operator/(VectorArgType a, ScalarArgType b)
	{
		VectorType result(a);
		result /= b;
		return result;
	}
	friend VectorType operator/(ScalarArgType b, VectorArgType a) { return a / b; }
	friend VectorType operator/(VectorArgType a, VectorArgType b)
	{
		VectorType result(a);
		result /= b;
		return result;
	}
};

#define __QI_COMMON_BINOPS_H
#endif // #ifndef __QI_COMMON_BINOPS_H
