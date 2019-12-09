//
// Licensed by "The MIT License". See file LICENSE.
//

#ifndef DFA_H
#define DFA_H
#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <omp.h>
#include <set>
#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>

#include "tables.hpp"

using namespace std;

/* 4-byte array */
using KeyTuple = array<uint8_t, 4>;

/* 16-byte state */
using State = array<uint8_t, 16>;

/* data structure for differentials */
using DiffStat = array<multimap<uint8_t,uint8_t>, 16>;

/* data structure for vector of key candidate tuples */
using VKeyTuple = vector<KeyTuple>;

/* galois multiplication tables */
static map <uint8_t, const uint8_t*> gmt = { {0x01, gm_01}, {0x09, gm_09}, {0x0b, gm_0b}, {0x0d, gm_0d}, {0x0e, gm_0e}, {0x8d, gm_8d}, {0xf6, gm_f6} };

/* related bytes (column-wise) */
static const uint8_t rb[4][4] = { {0,7,10,13},{1,4,11,14},{2,5,8,15},{3,6,9,12} };

/* Maps a fault location 'l' to the correct set of fault deltas for the standard filter. Note: 'l' is enumerated column-wise */
static const size_t map_fault[16] = {0,1,2,3,3,0,1,2,2,3,0,1,1,2,3,0};

/* pointer to inverses of fault deltas in GF(256) for the standard filter (depend on the fault location) */
static const uint8_t* ideltas1[4][16] = {
  {gm_8d, gm_01, gm_8d, gm_01, gm_01, gm_f6, gm_01, gm_f6, gm_01, gm_8d, gm_01, gm_8d, gm_f6, gm_01, gm_f6, gm_01},
  {gm_01, gm_f6, gm_01, gm_f6, gm_01, gm_8d, gm_01, gm_8d, gm_f6, gm_01, gm_f6, gm_01, gm_8d, gm_01, gm_8d, gm_01},
  {gm_01, gm_8d, gm_01, gm_8d, gm_f6, gm_01, gm_f6, gm_01, gm_8d, gm_01, gm_8d, gm_01, gm_01, gm_f6, gm_01, gm_f6},
  {gm_f6, gm_01, gm_f6, gm_01, gm_8d, gm_01, gm_8d, gm_01, gm_01, gm_f6, gm_01, gm_f6, gm_01, gm_8d, gm_01, gm_8d}
};

/* pointer to inverses of fault deltas in GF(256) for the improved filter (depend on the fault location) */
static const uint8_t* ideltas2[4][4] = {
  {gm_8d,gm_01,gm_01,gm_f6}, {gm_f6,gm_8d,gm_01,gm_01}, {gm_01,gm_f6,gm_8d,gm_01}, {gm_01,gm_01,gm_f6,gm_8d}
};

/* indices for c,d and the 10-th round key k (improved fault equations) */
static const size_t indices_x[4][16] = {
  { 0,13,10, 7,  12, 9, 6, 3,   8, 5, 2,15,   4, 1,14,11},
  {12, 9, 6, 3,   8, 5, 2,15,   4, 1,14,11,   0,13,10, 7},
  { 8, 5, 2,15,   4, 1,14,11,   0,13,10, 7,  12, 9, 6, 3},
  { 4, 1,14,11,   0,13,10, 7,  12, 9, 6, 3,   8, 5, 2,15}
};

/* indices for the 9-th round key h (improved fault equations)*/
static const size_t indices_y[4][16] = {
  { 0, 1, 2, 3,  12,13,14,15,   8, 9,10,11,   4, 5, 6, 7},
  {12,13,14,15,   8, 9,10,11,   4, 5, 6, 7,   0, 1, 2, 3},
  { 8, 9,10,11,   4, 5, 6, 7,   0, 1, 2, 3,  12,13,14,15},
  { 4, 5, 6, 7,   0, 1, 2, 3,  12,13,14,15,   8, 9,10,11}
};

vector<State> analyse(State &c, State &d, const size_t l, const size_t cores);

DiffStat differentials(State &c, State &d, const size_t l);

DiffStat standard_filter(DiffStat x);

vector<VKeyTuple> combine(DiffStat x);

vector<vector<VKeyTuple>> preproc(vector<VKeyTuple> cmb, const size_t cores);

vector<State> improved_filter(State &c, State &d, vector<VKeyTuple> &v, const size_t l);

vector<State> postproc(vector<vector<State>> &v);

State reconstruct(State &k);

uint32_t ks_core(uint32_t t, size_t r);

vector<uint8_t> getKeys(multimap<uint8_t,uint8_t> m);

void printState(State x);

vector<pair<State,State>> readfile();

void writefile(vector<State> keys, const string file);

static uint8_t EQ(const uint8_t c, const uint8_t d, const uint8_t k, const uint8_t* gm) {
  return gm[ isbox[c^k] ^ isbox[d^k] ];
}

template<typename T>
static constexpr size_t bits(T = T{})
{
  return sizeof(T) * CHAR_BIT;
}

template<typename T>
static constexpr T ROTL(const T x, const unsigned c)
{
  return (x << c) | (x >> (bits(x) - c));
}

template<typename T>
static constexpr T ROTR(const T x, const unsigned c)
{
  return (x >> c) | (x << (bits(x) - c));
}

#endif
