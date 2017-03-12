#ifndef TOOLBOX_INCLUDED
#include "tool.h"
#endif

class SpritzEngine{

public:

  char* run(std::string input) {
    	
    sbox(key);

    int i = 0;									//first array pointer
    int j = 0;									//second array pointer
    int k = 0;									//third array pointer

    int *temp = 0;								//swap variable
    char *buffer = new char[input.size()];		//buffer output
    output_size = input.size()*8;

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
    	
    sbox(input);

    int i = 0;									//first array pointer
    int j = 0;									//second array pointer
    int k = 0;									//third array pointer

    int *temp = 0;								//swap variable
    char *buffer = new char[input.size()];		//buffer output
    output_size = input.size()*8;

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
	    char *output = new char[size];
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
	      output[i] = alpha[h % 70];
	      i++;
	    }
	    return output;
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