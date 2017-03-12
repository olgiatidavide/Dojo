#define TOOLBOX_INCLUDED TRUE

#ifndef _GLIBCXX_NO_ASSERT
#include <cassert>
#endif
#include <cctype>
#include <cerrno>
#include <cfloat>
#include <ciso646>
#include <climits>
#include <clocale>
#include <cmath>
#include <csetjmp>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#if __cplusplus >= 201103L
#include <ccomplex>
#include <cfenv>
#include <cinttypes>
#include <cstdalign>
#include <cstdbool>
#include <cstdint>
#include <ctgmath>
#include <cwchar>
#include <cwctype>
#endif

// C++
#include <algorithm>
#include <bitset>
#include <complex>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <new>
#include <numeric>
#include <ostream>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <typeinfo>
#include <utility>
#include <valarray>
#include <vector>

#if __cplusplus >= 201103L
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <forward_list>
#include <future>
#include <initializer_list>
#include <mutex>
#include <random>
#include <ratio>
#include <regex>
#include <scoped_allocator>
#include <system_error>
#include <thread>
#include <tuple>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#endif

#define SHOW(var) { std::cout << #var << ": " << (var) << std::endl; }

#define SHOWHEX(var) { std::cout << #var << ": " << std::hex << (var) << std::endl; }

namespace blackmagic {

	void swap(void* a, void* b, size_t size){
	  char *temp = new char[size];
	  std::memcpy(temp, &b, size);
	  std::memcpy(&b, &a, size);
	  std::memcpy(&a, temp, size);
	  delete[] temp;
	}

	template<typename T>
	T abs(T x){ return x < 0 ? x*-1 : x; }							//modulo

	template<typename T>
	bool lsb(T x){ return (x & -x); }								//less significant bit

	class SpritzEngine{												//stream chipher
	public:

	  char* run(std::string input) {

	    int i = 0;									//first array pointer
	    int j = 0;									//second array pointer
	    int k = 0;									//third array pointer
	    char *buffer = new char[input.size()];		//buffer output

	    sbox(key);

	    for (int l = 0; l < input.size(); l++) {
	      i = (i + 5503) % 256;
	      j = (k + S[(j + S[i]) % 256]) % 256;
	      k = (i + k + S[j]) % 256;
	      blackmagic::swap(&S[i], &S[j], sizeof(int));
	      buffer[l] = S[j + S[i + S[(input[l] + k) % 256] % 256] % 256];
	    }
	    return buffer;
	  }

	  char* hash(std::string input, int size) {	

	    int i = 0;									//first array pointer
	    int j = 0;									//second array pointer
	    int k = 0;									//third array pointer
	    char *buffer = new char[input.size()];		//buffer output

	   	sbox(input);

	    for (int l = 0; l < input.size(); l++) {
	      i = (i + 5503) % 256;
	      j = (k + S[(j + S[i]) % 256]) % 256;
	      k = (i + k + S[j]) % 256;
	      blackmagic::swap(&S[i], &S[j], sizeof(int));
	      buffer[l] = S[j + S[i + S[(input[l] + k) % 256] % 256] % 256];
	    }
	    return expand(buffer, size);
	  }

	  bool setkey(std::string to_set){
	  	std::memcpy(key, this->expand(to_set, 256), 256);
	  	return true;
	  }
	  
	  int getOutputSize(){ return output_size; }
	private:

		#define A 92623
		#define B 115163
		#define C 192497
		#define FIRSTH 491

	  	char *expand(const std::string s, int size = 256) {
	    	char *alpha = (char*)"0123456789abcdefghilmnopqrstuvzxyABCDEFGHILMNOPQRSTUVZXY!=-><$+*_[]./\\";
		    std::string output = "";
		    unsigned int h = FIRSTH;
		    int i = 0;
		    int j = 0;

		    int temp = 0;

		    for(int x = 0; x < 70; x++){
		    	j = (j + alpha[i] + (FIRSTH % (s[blackmagic::abs(x^j) % (s.size() - 1)]))) % 70;
		      	blackmagic::swap(&alpha[i], &alpha[j], sizeof(char));
		    }

		    while (i < size) {
		      for (int x = 1; x < 51; ++x) {
		        h = (h * A);
		        if (blackmagic::lsb(h)) {
		          h ^= ((h ^ FIRSTH) * ((C * x) % 2147483647)) % 2147483647;
		          h = h >> (s[i % s.size()] ^ (int)(B / x));
		        } else {
		          h = h << (s[i % s.size()] ^ ((B * x) % 2147483647));
		        }

		        h = alpha[blackmagic::abs(h << alpha[s[blackmagic::abs(i ^ h) % s.size()] % 70])%70];
		      }
		      output += alpha[h % 70];
		      i++;
		    }
		    return (char*)output.c_str();
		 }

		void sbox(std::string key) {
		    int i;
		    for (i = 255; i--;) {
		      S[i] = i;
		    }
		    int j = 0;
		    int temp = 0;
		    for (i = 255; i--;) {
		      j = (j + S[i] + key[i]) % 256;
		      blackmagic::swap(&S[i], &S[j], sizeof(int));
		    }
		}

		/* Variabili & define */
		
		int S[255];
		char key[256];
		int output_size = -1;
	};
}

#ifdef __DBG_MEM
	
	#undef new
	template<typename T>
	struct track_alloc : std::allocator<T> {
	    typedef typename std::allocator<T>::pointer pointer;
	    typedef typename std::allocator<T>::size_type size_type;

	    template<typename U>
	    struct rebind {
	        typedef track_alloc<U> other;
	    };

	    track_alloc() {}

	    template<typename U>
	    track_alloc(track_alloc<U> const& u)
	        :std::allocator<T>(u) {}

	    pointer allocate(size_type size, 
	                     std::allocator<void>::const_pointer = 0) {
	        void * p = std::malloc(size * sizeof(T));
	        if(p == 0) {
	            throw std::bad_alloc();
	        }
	        return static_cast<pointer>(p);
	    }

	    void deallocate(pointer p, size_type) {
	        std::free(p);
	    }
	};

	typedef std::map< void*, std::size_t, std::less<void*>, track_alloc< std::pair<void* const, std::size_t> > > track_type;

	struct track_printer {
	    track_type * track;
	    track_printer(track_type * track):track(track) {}
	    ~track_printer() {
	        track_type::const_iterator it = track->begin();
	        while(it != track->end()) {
	            std::cerr << "TRACK: leaked at " << it->first << ", "
	                      << it->second << " bytes\n";
	            ++it;
	        }
	    }
	};

	track_type * get_map() {
	    // don't use normal new to avoid infinite recursion.
	    static track_type * track = new (std::malloc(sizeof *track)) track_type;
	    static track_printer printer(track);
	    return track;
	}

	void * operator new(std::size_t size) throw(std::bad_alloc) {
	    // we are required to return non-null
	    void * mem = std::malloc(size == 0 ? 1 : size);
	    if(mem == 0) {
	        throw std::bad_alloc();
	    }
	    (*get_map())[mem] = size;
	    return mem;
	}

	void operator delete(void * mem) throw() {
	    if(get_map()->erase(mem) == 0) {
	        // this indicates a serious bug
	        std::cerr << "bug: memory at " 
	                  << mem << " wasn't allocated by us\n";
	    }
	    std::free(mem);
	}
#endif
