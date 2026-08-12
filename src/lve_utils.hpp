#pragma once 

namespace lve {
	/*
		typename... Rest: This represents a parameter pack. This tells C++ that the function can take zero or more additional arguments of any type.
		^=: Bitwise XOR. 
		XOR truth table:
		A | B | X
		0 | 0 | 0
		0 | 1 | 1
		1 | 0 | 1
		1 | 1 | 0
		0x9e3779b9: Golden ratio constant value. 
		<< x: shift x amount to left
		>> x: shift x amount to right.

		(hashCombine(seed, rest), ...); This is C++17 fold expression. 
		If you call the function with three items (A,B,C), C++ expands this line into: hashCombine(seed, B); hashCombine(seed, C);
		It calls itself recursively, peeling off one item at a time until no items are left.


	*/
	/*
		From lve_model.cpp - 
		template<> struct hash<lve::LveModel::Vertex> {
		size_t operator()(lve::LveModel::Vertex const& vertex) const {
			size_t seed = 0;
			lve::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
	*/
	template <typename T, typename... Rest> void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
		seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2); // This line should call std::hash<glm::vec3> from glm/gtx/hash.hpp after the main call from hash<lve::LveModel::Vertex>
		(hashCombine(seed, rest), ...);
	}
} // namespace lve