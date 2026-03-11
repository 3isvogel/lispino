#pragma once

/**
 * @brief Interface for underlying hash function
 *
 * @param string 
 * @param len 
 */
unsigned int hash(char* string, unsigned int len);

/**
 * @brief Returns the biggest prime number smaller than n
 *
 * NOTE: this is a stupid function
 *
 * @param n 
 */
unsigned int primeProbe(unsigned int n);
