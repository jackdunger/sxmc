/**
 * \file utils.h
 * \brief Collected utility structures and functions
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#include <vector>
#include <string>

class TNtuple;

/**
 * Get a value from a TNtuple by event ID and field name.
 *
 * \param nt The source TNtuple
 * \param i The event ID
 * \param field The name of the variable to extract
 * \returns The requested value as a float
 */
float get_ntuple_entry(TNtuple* nt, int i, std::string field);


/**
 * Build a correlation matrix for a TNtuple.
 *
 * Creates a matrix with Pearson product-moment correlation coefficients
 * computed between pairs of variables in a TNtuple. The matrix expressed
 * as a vector of length (entries x entries). Only the upper half is set.
 *
 * \param nt The source TNtuple
 * \returns A correlation matrix as a 1D vector
 */
std::vector<float> get_correlation_matrix(TNtuple* nt);


/**
 * Get the index of an object in a vector.
 *
 * If the object isn't found, add it to the end and then return the index.
 * Useful for creating unique ordered lists.
 *
 * \param v The vector
 * \param o The object to locate
 # \tparam T The types contained in the vector v
 * \return The index of the object
 */
template <typename T>
size_t get_index_with_append(std::vector<T>& v, T o) {
  size_t index = std::find(v.begin(), v.end(), o) - v.begin();
  if (index == v.size()) {
    v.push_back(o);
    return v.size() - 1;
  }
  return index;
}

#endif  // __UTILS_H__

