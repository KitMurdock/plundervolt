//
// Licensed by "The MIT License". See file LICENSE.
//

#include "dfa.hpp"

/* start of differential fault analysis */
vector<State> analyse(State &c, State &d, const size_t l, const size_t cores){

  printf("Applying standard filter. ");
  vector<VKeyTuple> cmb = combine( standard_filter( differentials(c,d,l) ) );
  printf("Done.\n");
  size_t n = cmb[0].size() * cmb[1].size() * cmb[2].size() * cmb[3].size();
  printf("Size of keyspace: %lu = 2^%f \n",n, log2(n));

  printf("Applying improved filter. ");
  fflush(stdout);

  /* pre-processing */
  vector<vector<VKeyTuple>> sliced_cmb = preproc(cmb,cores);

  /* set openmp parameters and feed sliced key space in parallel to the improved filter */
  vector<vector<State>> r;
  omp_set_num_threads(cores);

#pragma omp parallel for ordered
  for (size_t i = 0; i < sliced_cmb.size(); ++i )
  {
    vector<State> v = improved_filter(c,d,sliced_cmb[i],l);
#pragma omp ordered
    r.push_back(v);
  }
  printf("Done.\n");

  /* post-processing */
  vector<State> v = postproc(r);
  printf("Size of keyspace: %lu = 2^%f \n",v.size(), log2(v.size()));
  return v;
}


/* differentials */
DiffStat differentials(State &c, State &d, const size_t l) {

  /* choose inverse deltas depending on the fault location 'l' */
  const uint8_t** gm = ideltas1[ map_fault[l] ];

  /* init differential matrix */
  DiffStat x;
  for (size_t i = 0; i < 16; ++i)
  {
    multimap<uint8_t,uint8_t> v;
    x[i] = v;
  }

  for (size_t i = 0; i < 256; ++i)
  {
    uint8_t k = (uint8_t) i;

    x[ 0].insert( pair<uint8_t,uint8_t>( EQ( c[ 0], d[ 0], k, gm[ 0] ), k ) );
    x[ 1].insert( pair<uint8_t,uint8_t>( EQ( c[ 1], d[ 1], k, gm[ 1] ), k ) );
    x[ 2].insert( pair<uint8_t,uint8_t>( EQ( c[ 2], d[ 2], k, gm[ 2] ), k ) );
    x[ 3].insert( pair<uint8_t,uint8_t>( EQ( c[ 3], d[ 3], k, gm[ 3] ), k ) );

    x[ 4].insert( pair<uint8_t,uint8_t>( EQ( c[ 4], d[ 4], k, gm[ 4] ), k ) );
    x[ 5].insert( pair<uint8_t,uint8_t>( EQ( c[ 5], d[ 5], k, gm[ 5] ), k ) );
    x[ 6].insert( pair<uint8_t,uint8_t>( EQ( c[ 6], d[ 6], k, gm[ 6] ), k ) );
    x[ 7].insert( pair<uint8_t,uint8_t>( EQ( c[ 7], d[ 7], k, gm[ 7] ), k ) );

    x[ 8].insert( pair<uint8_t,uint8_t>( EQ( c[ 8], d[ 8], k, gm[ 8] ), k ) );
    x[ 9].insert( pair<uint8_t,uint8_t>( EQ( c[ 9], d[ 9], k, gm[ 9] ), k ) );
    x[10].insert( pair<uint8_t,uint8_t>( EQ( c[10], d[10], k, gm[10] ), k ) );
    x[11].insert( pair<uint8_t,uint8_t>( EQ( c[11], d[11], k, gm[11] ), k ) );

    x[12].insert( pair<uint8_t,uint8_t>( EQ( c[12], d[12], k, gm[12] ), k ) );
    x[13].insert( pair<uint8_t,uint8_t>( EQ( c[13], d[13], k, gm[13] ), k ) );
    x[14].insert( pair<uint8_t,uint8_t>( EQ( c[14], d[14], k, gm[14] ), k ) );
    x[15].insert( pair<uint8_t,uint8_t>( EQ( c[15], d[15], k, gm[15] ), k ) );
  }
  return x;
}


/* standard filter */
DiffStat standard_filter(DiffStat x)
{
  /* iterate over columns */
  for (size_t i = 0; i < 4; ++i)
  {
    /* init set of valid indices */
    set<uint8_t> s;
    for (size_t j = 0; j < 256; ++j) { s.insert( (uint8_t)j ); }

    /* compute valid indices for a given column i */
    for (size_t j = 0; j < 4; ++j)
    {
      vector<uint8_t> keys = getKeys( x[ rb[i][j] ] );
      set<uint8_t> t( begin(keys), end(keys) );
      set<uint8_t> r;
      set_intersection( s.begin(), s.end(), t.begin(), t.end(), inserter( r, r.begin() ) );
      s = r;
    }

    /* erase invalid entries */
    for (size_t j = 0; j < 4; ++j)
    {
      vector<uint8_t> keys = getKeys( x[ rb[i][j] ] );
      set<uint8_t> t( begin(keys), end(keys) );
      set<uint8_t> r;
      set_difference( t.begin(), t.end(), s.begin(), s.end(), inserter( r, r.begin() ) );
      vector<uint8_t> toDel( begin(r), end(r) );
      //cout << "from: " << getKeys(x[ rb[i][j] ]).size();
      for ( size_t k = 0; k < toDel.size(); ++k) { x[ rb[i][j] ].erase(toDel[k]); }
      //cout << " to: " << getKeys(x[ rb[i][j] ]).size() << endl;
    }
  }
  return x;
}


/* computes the cartesian product of all remaining key candidates of related elements */
vector<VKeyTuple> combine(DiffStat m)
{
  vector<VKeyTuple> result;

  /* iterate over columns */
  for (size_t i = 0; i < 4; ++i)
  {
    VKeyTuple v;

    auto w = m[ rb[i][0] ];
    auto x = m[ rb[i][1] ];
    auto y = m[ rb[i][2] ];
    auto z = m[ rb[i][3] ];

    vector<uint8_t> wk = getKeys( w );

    for (size_t j = 0; j < wk.size(); ++j)
    {
      /* extract elements with the same key and combine them */
      auto retw = w.equal_range(wk[j]);
      auto retx = x.equal_range(wk[j]);
      auto rety = y.equal_range(wk[j]);
      auto retz = z.equal_range(wk[j]);

      for (auto iw = retw.first; iw != retw.second; ++iw)
      {
        for (auto ix = retx.first; ix != retx.second; ++ix)
        {
          for (auto iy = rety.first; iy != rety.second; ++iy)
          {
            for (auto iz = retz.first; iz != retz.second; ++iz)
            {
              KeyTuple t;
              t[0] = iw->second; t[1] = ix->second; t[2] = iy->second; t[3] = iz->second;
              v.push_back(t);
            }
          }
        }
      }
    }
    result.push_back(v);
  }
  return result;
}


/* prepare data for application of improved filter on multiple cores */
vector<vector<VKeyTuple>> preproc(vector<VKeyTuple> cmb, size_t cores)
{
  vector<VKeyTuple> slices;
  for (size_t i = 0; i < cores; ++i) { VKeyTuple v; slices.push_back(v); }

  size_t n = cmb[0].size() / cores;
  size_t m = cmb[0].size() % cores;

  /* distribute elements of the first column */
  for (size_t i = 0; i < n; ++i)
    for (size_t j = 0; j < cores; ++j)
      slices[j].push_back( cmb[0][cores*i+j] );

  /* distribute the remaining elements, too */
  for (size_t j = 0; j < m; ++j)
    slices[j].push_back( cmb[0][cores*n+j] );

  vector<vector<VKeyTuple>> slices_cmb;
  for (size_t i = 0; i < slices.size(); ++i)
  {
    vector<VKeyTuple> v;
    v.push_back(slices[i]);
    v.push_back(cmb[1]);
    v.push_back(cmb[2]);
    v.push_back(cmb[3]);
    slices_cmb.push_back(v);
  }
  return slices_cmb;
}


/* improved filter */
vector<State> improved_filter(State &c, State &d, vector<VKeyTuple> &v, const size_t l) {

  /* configure fault equations depending on the fault location 'l' */

  const uint8_t** gm = ideltas2[ l%4 ];        // inverse deltas
  const size_t* x = indices_x[ map_fault[l] ]; // indices for c,d and k
  const size_t* y = indices_y[ map_fault[l] ]; // indices for h

  vector<State> candidates;

  for (size_t i0 = 0; i0 < v[0].size(); ++i0) {
    for (size_t i1 = 0; i1 < v[1].size(); ++i1) {
      for (size_t i2 = 0; i2 < v[2].size(); ++i2) {
        for (size_t i3 = 0; i3 < v[3].size(); ++i3) {

        /* index order of the tuples in 'key': (0,7,10,13),(1,4,11,14),(2,5,8,15),(3,6,9,12) */

        /* 10-th round key */
        State k = {
          v[0][i0][0], v[1][i1][0], v[2][i2][0], v[3][i3][0],   /*  0  1  2  3 */
          v[1][i1][1], v[2][i2][1], v[3][i3][1], v[0][i0][1],   /*  4  5  6  7 */
          v[2][i2][2], v[3][i3][2], v[0][i0][2], v[1][i1][2],   /*  8  9 10 11 */
          v[3][i3][3], v[0][i0][3], v[1][i1][3], v[2][i2][3] }; /* 12 13 14 15 */

        /* 9-th round key */
        State h;
        h[ 0] = k[ 0] ^ sbox[ k[ 9] ^ k[13] ] ^0x36;
        h[ 1] = k[ 1] ^ sbox[ k[10] ^ k[14] ];
        h[ 2] = k[ 2] ^ sbox[ k[11] ^ k[15] ];
        h[ 3] = k[ 3] ^ sbox[ k[ 8] ^ k[12] ] ;
        h[ 4] = k[ 0] ^ k[ 4];
        h[ 5] = k[ 1] ^ k[ 5];
        h[ 6] = k[ 2] ^ k[ 6];
        h[ 7] = k[ 3] ^ k[ 7];
        h[ 8] = k[ 4] ^ k[ 8];
        h[ 9] = k[ 5] ^ k[ 9];
        h[10] = k[ 6] ^ k[10];
        h[11] = k[ 7] ^ k[11];
        h[12] = k[ 8] ^ k[12];
        h[13] = k[ 9] ^ k[13];
        h[14] = k[10] ^ k[14];
        h[15] = k[11] ^ k[15];

        uint8_t f0 = gm[0][
          isbox[
          gm_0e[ isbox[ c[ x[ 0] ] ^ k[ x[ 0] ] ] ^ h[ y[ 0] ] ] ^
          gm_0b[ isbox[ c[ x[ 1] ] ^ k[ x[ 1] ] ] ^ h[ y[ 1] ] ] ^
          gm_0d[ isbox[ c[ x[ 2] ] ^ k[ x[ 2] ] ] ^ h[ y[ 2] ] ] ^
          gm_09[ isbox[ c[ x[ 3] ] ^ k[ x[ 3] ] ] ^ h[ y[ 3] ] ] ] ^
          isbox[
          gm_0e[ isbox[ d[ x[ 0] ] ^ k[ x[ 0] ] ] ^ h[ y[ 0] ] ] ^
          gm_0b[ isbox[ d[ x[ 1] ] ^ k[ x[ 1] ] ] ^ h[ y[ 1] ] ] ^
          gm_0d[ isbox[ d[ x[ 2] ] ^ k[ x[ 2] ] ] ^ h[ y[ 2] ] ] ^
          gm_09[ isbox[ d[ x[ 3] ] ^ k[ x[ 3] ] ] ^ h[ y[ 3] ] ] ]
          ];

        uint8_t f1 = gm[1][
          isbox[
          gm_09[ isbox[ c[ x[ 4] ] ^ k[ x[ 4] ] ] ^ h[ y[ 4] ] ] ^
          gm_0e[ isbox[ c[ x[ 5] ] ^ k[ x[ 5] ] ] ^ h[ y[ 5] ] ] ^
          gm_0b[ isbox[ c[ x[ 6] ] ^ k[ x[ 6] ] ] ^ h[ y[ 6] ] ] ^
          gm_0d[ isbox[ c[ x[ 7] ] ^ k[ x[ 7] ] ] ^ h[ y[ 7] ] ] ] ^
          isbox[
          gm_09[ isbox[ d[ x[ 4] ] ^ k[ x[ 4] ] ] ^ h[ y[ 4] ] ] ^
          gm_0e[ isbox[ d[ x[ 5] ] ^ k[ x[ 5] ] ] ^ h[ y[ 5] ] ] ^
          gm_0b[ isbox[ d[ x[ 6] ] ^ k[ x[ 6] ] ] ^ h[ y[ 6] ] ] ^
          gm_0d[ isbox[ d[ x[ 7] ] ^ k[ x[ 7] ] ] ^ h[ y[ 7] ] ] ]
          ];

        uint8_t f2 = gm[2][
          isbox[
          gm_0d[ isbox[ c[ x[ 8] ] ^ k[ x[ 8] ] ] ^ h[ y[ 8] ] ] ^
          gm_09[ isbox[ c[ x[ 9] ] ^ k[ x[ 9] ] ] ^ h[ y[ 9] ] ] ^
          gm_0e[ isbox[ c[ x[10] ] ^ k[ x[10] ] ] ^ h[ y[10] ] ] ^
          gm_0b[ isbox[ c[ x[11] ] ^ k[ x[11] ] ] ^ h[ y[11] ] ] ] ^
          isbox[
          gm_0d[ isbox[ d[ x[ 8] ] ^ k[ x[ 8] ] ] ^ h[ y[ 8] ] ] ^
          gm_09[ isbox[ d[ x[ 9] ] ^ k[ x[ 9] ] ] ^ h[ y[ 9] ] ] ^
          gm_0e[ isbox[ d[ x[10] ] ^ k[ x[10] ] ] ^ h[ y[10] ] ] ^
          gm_0b[ isbox[ d[ x[11] ] ^ k[ x[11] ] ] ^ h[ y[11] ] ] ]
          ];

        uint8_t f3 = gm[3][
          isbox[
          gm_0b[ isbox[ c[ x[12] ] ^ k[ x[12] ] ] ^ h[ y[12] ] ] ^
          gm_0d[ isbox[ c[ x[13] ] ^ k[ x[13] ] ] ^ h[ y[13] ] ] ^
          gm_09[ isbox[ c[ x[14] ] ^ k[ x[14] ] ] ^ h[ y[14] ] ] ^
          gm_0e[ isbox[ c[ x[15] ] ^ k[ x[15] ] ] ^ h[ y[15] ] ] ] ^
          isbox[
          gm_0b[ isbox[ d[ x[12] ] ^ k[ x[12] ] ] ^ h[ y[12] ] ] ^
          gm_0d[ isbox[ d[ x[13] ] ^ k[ x[13] ] ] ^ h[ y[13] ] ] ^
          gm_09[ isbox[ d[ x[14] ] ^ k[ x[14] ] ] ^ h[ y[14] ] ] ^
          gm_0e[ isbox[ d[ x[15] ] ^ k[ x[15] ] ] ^ h[ y[15] ] ] ]
          ];

        if ((f0 == f1) && (f1 == f2) && (f2 == f3))
          candidates.push_back(k);
        }
      }
    }
  }
  return candidates;
}

/* post processing of subkey candidates */
vector<State> postproc(vector<vector<State>> &v)
{
  vector<State> r;
  for (size_t i = 0; i < v.size(); ++i)
    for (size_t j = 0; j < v[i].size(); ++j)
      r.push_back(reconstruct(v[i][j]));
  return r;
}


/* reconstructs the master key from the 10-th round subkey */
State reconstruct(State &k)
{
  array<uint32_t,44> sk;
  sk.fill(0);

  for (size_t i = 0; i < 4; ++i)
    for (size_t j = 0; j < 4; ++j)
      sk[i] ^= k[4*i+j] << (24-8*j);

  size_t n = 10;
  for (size_t i = 0; i < n; ++i)
  {
    sk[ 4*(i+1)   ] = sk[ 4*i   ] ^ ks_core( sk[ 4*i+2 ] ^ sk[ 4*i+3 ], n-i );
    sk[ 4*(i+1)+1 ] = sk[ 4*i   ] ^ sk[ 4*i+1 ];
    sk[ 4*(i+1)+2 ] = sk[ 4*i+1 ] ^ sk[ 4*i+2 ];
    sk[ 4*(i+1)+3 ] = sk[ 4*i+2 ] ^ sk[ 4*i+3 ];
  }

  n = sk.size();
  State mk;
  for (size_t i = 0; i < 4; ++i)
    for (size_t j = 0; j < 4; ++j)
      mk[4*i+j] =  ( sk[n - 4 + i] >> (24 - 8*j) ) & 0xff;

  return mk;
}


/* core of AES key schedule */
uint32_t ks_core(uint32_t t, size_t r)
{
  uint32_t b = ROTL<uint32_t>(t,8);
  uint32_t c = rcon[r] << 24;
  for (size_t i = 0; i < 4; ++i)
    c ^= sbox[ ( b >> (24-8*i) ) & 0xff ] << (24 - 8*i);
  return c;
}


/* get keys from a multimap */
vector<uint8_t> getKeys(multimap<uint8_t,uint8_t> m)
{
  vector<uint8_t> keys;
  for ( auto it = m.begin(), end = m.end(); it != end; it = m.upper_bound(it->first)  ) { keys.push_back(it->first); }
  return keys;
}


void printState(State x)
{
  for (size_t i = 0; i < x.size(); ++i) { printf("%02x",x[i] ); }
}


vector<pair<State,State>> readfile(const string file)
{
  vector<pair<State,State>> pairs;

  ifstream infile (file);

  if (infile.is_open())
  {
    string in;
    while(getline(infile,in))
    {
      pair<State,State> cts;
      istringstream iin ( in );
      string x,y;
      getline (iin, x, ' ');
      getline (iin, y, ' ');
      State c,d;
      c.fill(0);
      d.fill(0);
      for (size_t i = 0; i < x.length(); i += 2 )
      {
        c[ i/2 ] = strtol(x.substr( i , 2 ).c_str(),0,16);
        d[ i/2 ] = strtol(y.substr( i , 2 ).c_str(),0,16);
      }
      cts.first = c;
      cts.second = d;
      pairs.push_back(cts);
    }
    infile.close();
  }
  return pairs;
}


void writefile(vector<State> keys, const string file)
{
  FILE * outfile;
  outfile = fopen (file.c_str(), "a");

  for (size_t i = 0; i < keys.size(); ++i)
    {
      for (size_t j = 0; j < keys[i].size(); ++j)
        {
          fprintf(outfile, "%02x",keys[i][j]);
        }
      fprintf(outfile, "\n");
    }
  fclose(outfile);
}


void help(){
  printf("Usage: ./dfa c l f\n\n" );
  printf("Parameters\n");
  printf("%2sc: Number of cores >= 1.\n","");
  printf("%2sl: Byte number of the AES state affected by the fault.\n%5sMust be in {-1,0,...,15}, where -1 means unknown.\n","","");
  printf("%2sf: Input file with one or more pairs of correct and faulty ciphertexts.\n\n","");
}


int main (int argc, char **argv)
{
  if (argc != 4) { help(); return -1; }

  const size_t c = atoi(argv[1]); // number of cores
  const int l = atoi(argv[2]); // fault location
  const string f = argv[3]; // input file

  if (c < 0 || l < -1 || l > 15 || f.empty() ) { help(); return -1; }

  vector<pair<State,State>> pairs = readfile(f);

  /* set fault location range */
  size_t j = 0;
  size_t n = 0;
  if (l == -1) { n = 16; } else { j = l; n = l+1;}

  for (size_t i = 0; i < pairs.size(); ++i)
  {
    printf("(%lu) Analysing ciphertext pair:\n\n",i);
    printState(pairs[i].first);
    printf(" ");
    printState(pairs[i].second);
    printf("\n\nNumber of core(s): %lu \n", c);

    /* create new output file */
    stringstream ss;
    ss << "keys-" << i << ".csv";
    FILE * outfile;
    const string name = ss.str();
    outfile = fopen (name.c_str(), "w");
    fclose(outfile);

    size_t count = 0;
    while(j < n){
      printf("----------------------------------------------------\n");
      printf("Fault location: %lu\n",j);
      vector<State> keys = analyse(pairs[i].first, pairs[i].second, j, c);
      count += keys.size();
      writefile(keys,name);
      j++;
    }
    if ( l == -1 ) { j = 0; } else { j = l; } // reset j
    printf("\n%lu masterkeys written to %s\n\n",count,name.c_str());
  }
  return 0;
}
