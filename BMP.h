#include <cmath>
#include <exception>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <ctime>

#define DEBUG true

class BMP {

public:
  BMP() { reset(); }

  void reset() {
    this->isInFileSet = this->fileExist = this->isFileHeaderSet = this->isInfoHeaderSet = false;
    this->lastError = this->filename = "";
    this->EOF_in = true;
    this->pixelByteSize = this->pixelcount = -1;
  }

  bool openInput(std::string name) {
    try {

      /*==================================================================
        ||                  Controlli preliminari                       ||
        ||                                                              ||
        ||  - controllo la correttezza dell'estensione [checkName]      ||
        ||  - l'esistenza del file [fopen]                              ||
        ||  - verifico se posso cominciare la lettura, altrimenti abort ||
        ==================================================================*/

      this->filename = checkName(name);
      this->fileExist = (infile = std::fopen(filename.c_str(), "r"));

      if (this->fileExist) {

        /*========================================================================================
          ||                            Lettura Header del file                                 ||
          ||                                                                                    ||
          ||  - setto isFileSet a true in modo da tener traccia dell'avvenuta apertura del file ||
          ||  - verifico che la lettura dell'fileheader sia andata a buon fine [loadFileHeader] ||
          ||  - verifico che la lettura dell'infoheader sia andata a buon fine [loadInfoHeader] ||
          ||  - verifico che la lettura dell'RGBQuad sia andata a buon fine [loadRGBQuad]       ||
          ========================================================================================*/

        (this->loadFileHeader() && this->loadInfoHeader() && this->loadRGBQuad()) ? this->isInFileSet = true : throw(BMPException("Errore durante la lettura dell'Hearder"));
      } else {
        std::fclose(this->infile);
        throw(BMPException("Il file non esiste"));
      }

    } catch (BMPException &e) {
      this->lastError = e.what();
      if (DEBUG) {
            std::cout << this->lastError << std::endl;
      }
      return false;
    }
    return true;
  }

  char *nextPixel() {

    /*====================================================================================================
      ||                                          Lettura Pixel                                         ||
      ||                                                                                                ||
      ||  - alloco il buffer di lettura dei pixel  [char* buffer]                                       ||
      ||  - pulisco il buffer del file di input  [fflush]                                               ||
      ||  - contrllo che il file di input non abbia raggunto l'EOF  [fread]                             ||
      ||  - se non è stato generato nessun errore ritorno il pixel altrimenti inolto un errore [throw]  ||
      ||                                                                                                ||
      ||                ! NEL CASO NON RIESCA A LEGGERE IL PIXEL RITORNO nullptr !                      ||
      ||                                                                                                ||
      ====================================================================================================*/

    char *buffer = new char[this->pixelByteSize];

    try {

      std::fflush(this->infile);
      this->EOF_in = (std::fread(buffer, 1, this->pixelByteSize, infile) == this->pixelByteSize);

      if (this->EOF_in) {
          this->pixelcount++;
          return buffer;
      } else {
          std::string err = ((pixelcount > -1) ? ("Raggiunto EOF durante la lettura del pixel n. " + std::to_string(this->pixelcount)) : "Non è stato possibile leggere nessun pixel");
          delete[] buffer;
          throw(BMPException(err));
      }

    } catch (BMPException &e) {
      this->lastError = e.what();
      if (DEBUG) {
        std::cout << this->lastError << std::endl;
      }
      return nullptr;
    }
  }

  bool injectText(char *secret, int length, char *key, int key_length) {
    try {

      /*=================================*/
      /*||  Preparo il file di output  ||*/
      /*=================================*/

      std::string outname = "sec_" + this->filename;                                                   //aggiungo il prefisso sec_ al nome del file
      outfile = std::fopen(outname.c_str(), "w");                                                      //creo un puntatore al nuovo file

      char final_key[40];
      secret = this->RC4(secret, length, this->expand(key, key_length, 40), 40);                       //cripto il messaggio in input

      /*=====================*/
      /*||  STEGANOHEADER  ||*/
      /*=====================*/

      STEGINFOHEADER StegHeader;

      StegHeader.stgType = (short int) 'ST';
      StegHeader.stgDatalen = length * 8;
      getlogin_r(StegHeader.stgAutor, 10);  
      std::memcpy(&StegHeader.stgAutor, this->expand(StegHeader.stgAutor, 10, 10), 10);

      /*====================================*/
      /*||  Scrivo gli Headers in output  ||*/
      /*====================================*/ 

      if (this->pushFileHeader(this->fileHeader) &&                                                   //controllo se la scrittura del fileHeader è andata a buon fine
          this->pushInfoHeader(this->infoHeader) &&                                                   //controllo se la scrittura del infoHeader è andata a buon fine
          this->pushRGBQuad(this->RGBQuad)){                                                          //controllo se la scrittura del RGBQuad Array è andata a buon fine

            pushStegInfoHeader(StegHeader);

            int index = 0;
            char *edited_pixel = new char[this->pixelByteSize];
            edited_pixel = nextPixel();
            while (EOF_in || edited_pixel != NULL) {
              
              if(index < length*8){
                  
                if((secret[(int)index/8] >> index) & 1){ edited_pixel[this->pixelByteSize - 1] &= ~1; }
                else { edited_pixel[this->pixelByteSize - 1] |= 1; }

                pushPixel(edited_pixel);
                index++;
              }
              else { pushPixel(edited_pixel); }
              edited_pixel = nextPixel();
            }
            delete[] edited_pixel;
            std::fclose(outfile);
      }
    } catch (std::exception &e) {
      lastError = e.what();
      if (DEBUG) {
        std::cout << e.what() << std::endl;
      }
      return false;
    }
    return true;
  }

  /* Info Functions */

  bool isFileOpen() { return isInFileSet; }

  bool exist() { return fileExist; }

  std::string getLastError() { return lastError; }

  unsigned int getHeight() { return infoHeader.biHeight; }

  unsigned int getWidth() { return infoHeader.biWidth; }

  short int getDepth() { return infoHeader.biBitCount; }

  short int getImageSize() {
    return (short int)((this->infoHeader.biWidth * this->infoHeader.biHeight) / 10000000);
  }

  ~BMP() {
    std::fclose(infile);
    reset();
  }

private:
  /* Structures */

class BMPException : public std::runtime_error {
    
    public: 
      BMPException(std::string x/*, int priority*/) : runtime_error(x.c_str()){
        time_t rawtime;
        struct tm * timeinfo;
        char buffer[80];

        time (&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer,80,"%d/%m/%Y %H:%M:%S",timeinfo);
        errtime = buffer;
        error = "[" + getTime() + "] : " + runtime_error::what();
      }

      virtual const char* what() const throw(){
        return error.c_str();
      }

      std::string getTime() const { return errtime; }

      virtual ~BMPException() throw() {}

    private:
      std::string errtime;
      std::string error;
      //int priority;
};

  struct BMPFILEHEADER {
    short int bfType;    // 2 Bitmap identifier. Must be 'BM'.
    unsigned int bfSize; // 4 Can be set to 0 for for uncompressed bitmaps, which is the kind we have.
    unsigned int bfReserved; // 4 Set to 0.
    unsigned int bfOffbits;  // 4 Specifies the location (in bytes) in the file of the image data.
  };

  struct BMPINFOHEADER {
    unsigned int biSize; // 4 This is the size of the BMPINFOHEADER structure.
    unsigned int biWidth;  // 4 The width of the bitmap, in pixels.
    unsigned int biHeight; // 4 The height of the bitmap, in pixels.
    short int biPlanes;    // 2 Set to 1.
    short int biBitCount;  // 2 The bit depth of the bitmap. For 8-bit bitmaps, this is 8.
    unsigned int biCompression; // 4 Our bitmaps are uncompressed, so this field is set to 0.
    unsigned int biSizeImage;     // 4 The size of the padded image, in bytes.
    unsigned int biXPelsPerMeter; // 4 Horizontal resolution, in pixels per meter, of device displaying bitmap. This number is not significant for us, and will be set to 0.
    unsigned int biYPelsPerMeter; // 4 Vertical resolution, in pixels per meter, of device displaying bitmap. This number is not significant for us, and will be set to 0.
    unsigned int biClrUsed; // 4 This indicates how many colors are in the palette.
    unsigned int biClrImportant; // 4 This indicates how many colors are needed to display the bitmap. We will set it to 0, indicating all colors should be used.
  };

  struct STEGINFOHEADER {
    short int stgType;
    short int stgDatalen;
    char stgAutor[10];
  };

  /* Functions */

  inline bool exists(const std::string &name) {     //	http://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
  }

  bool loadFileHeader() {
    try {
      std::fseek(infile, 0, SEEK_SET);
      std::fflush(infile);
      if (std::fread(&fileHeader, sizeof(char), 14, infile) == 14) {
        isFileHeaderSet = true;
      } else {
        throw(BMPException("Errore durante la lettura del fileHeader"));
      }
    } catch (BMPException &e) {
      lastError = e.what();
      if (DEBUG) {
        std::cout << e.what() << std::endl;
      }
      return false;
    }
    return true;
  }

  bool loadInfoHeader() {
    try {
      std::fseek(infile, 14, SEEK_SET);
      std::fflush(this->infile);
      if (std::fread(&this->infoHeader, sizeof(char), 40, infile) == 40) {
        this->RGBQuad = new uint32_t[this->infoHeader.biClrUsed + 1];
        this->pixelByteSize = (int)this->infoHeader.biBitCount / 8;
        this->isInfoHeaderSet = true;
      } else {
        throw(BMPException("Errore durante la lettura del infoHeader"));
      }
    } catch (BMPException &e) {
      lastError = e.what();
      if (DEBUG) {
        std::cout << e.what() << std::endl;
      }
      return false;
    }
    return true;
  }

  bool loadRGBQuad() {
    try {
      std::fseek(infile, 54, SEEK_SET);
      std::fflush(infile);
      if (std::fread(RGBQuad, sizeof(uint32_t), infoHeader.biClrUsed + 1, infile) == infoHeader.biClrUsed + 1) {
        return true;
      } else {
        lastError = "error while reading RGBQuad";
        return false;
      }
    } catch (std::exception &e) {
      lastError = e.what();
      if (DEBUG) {
        std::cout << e.what() << std::endl;
      }
      return false;
    }
  }

  bool pushFileHeader(BMPFILEHEADER to_be_pushed) {
    try {
      std::fflush(outfile);
      std::fwrite(&to_be_pushed, sizeof(char), 14, outfile);
      return true;
    } catch (std::exception &e) {
      lastError = e.what();
      if (DEBUG) {
        std::cout << e.what() << std::endl;
      }
      return false;
    }
  }

  bool pushInfoHeader(BMPINFOHEADER to_be_pushed) {
    try {
      std::fflush(outfile);
      std::fwrite(&to_be_pushed, sizeof(char), 40, outfile);
      return true;
    } catch (std::exception &e) {
      lastError = e.what();
      if (DEBUG) {
        std::cout << e.what() << std::endl;
      }
      return false;
    }
  }

  bool pushRGBQuad(uint32_t* to_be_pushed) {
    try {
      std::fflush(outfile);
      std::fwrite(to_be_pushed, sizeof(uint32_t), infoHeader.biClrUsed + 1, outfile);
      return true;
    } catch (std::exception &e) {
      lastError = e.what();
      if (DEBUG) {
        std::cout << e.what() << std::endl;
      }
      return false;
    }
  }

  bool pushStegInfoHeader(STEGINFOHEADER &to_be_pushed) {
    try {
      int out_index = 0;
      int index = 0;
      char *edited_pixel = new char[infoHeader.biBitCount / 8];
      char buffer[14];
      std::memcpy(&to_be_pushed, &buffer, 14);

      while (out_index < 14) {
        edited_pixel = nextPixel();
        edited_pixel[(infoHeader.biBitCount / 8) - 1] &=
            ((buffer[out_index] >> index) & 1);
        pushPixel(edited_pixel);
        index++;
        if (index > 7) {
          index = 0;
          out_index++;
        }
      }

      return true;
    } catch (std::exception &e) {
      lastError = e.what();
      if (DEBUG) {
        std::cout << e.what() << std::endl;
      }
      return false;
    }
  }

  bool pushPixel(char *to_be_pushed) {
    try {
      int size = infoHeader.biBitCount / 8;
      std::fflush(outfile);
      std::fwrite(to_be_pushed, 1, size, outfile);
      return true;
    } catch (std::exception &e) {
      lastError = e.what();
      if (DEBUG) {
        std::cout << e.what() << std::endl;
      }
      return false;
    }
    return true;
  }

  std::string checkName(std::string name) {
    if (name.find(".bmp") == std::string::npos &&
        name.find(".BMP") == std::string::npos) {
      std::string candidate = name + ".BMP";
      if (exists(candidate)) {
        return candidate;
      } else {
        return name + ".bmp";
      }
    } else {
      return name;
    }
  }

#define A 92623
#define B 115163
#define C 192497
#define FIRSTH 491

  char *expand(const char *s, int i_size, int size) {
    char *alphabet = (char*)"0123456789abcdefghilmnopqrstuvzxyABCDEFGHILMNOPQRSTUVZXY!=-><$";
    char *output = new char[size];
    unsigned int h = FIRSTH;
    int i = 0;

    while (i < size) {
      for (int x = 1; x < 51; ++x) {
        h = (h * A);
        if (h % x == 0) {
          h ^= ((h ^ FIRSTH) * ((C * x) % 2147483647)) % 2147483647;
          h = h >> (s[i % i_size] ^ (int)(B / x));
        } else {
          h = h << (s[i % i_size] ^ ((B * x) % 2147483647));
        }

        h = h << (int)alphabet[s[(i ^ h) % i_size] % 61];
      }
      output[i] = alphabet[h % 61];
      i++;
    }
    return output;
  }

  void sbox(char *key, int size) {
    int i;
    for (i = 255; i--;) {
      S[i] = i;
    }
    int j = 0;
    int temp = 0;
    for (i = 255; i--;) {
      j = (j + S[i] + key[i % size]) % 256;
      temp = S[i];
      S[i] = S[j];
      S[j] = temp;
    }
  }

  char *RC4(char *input, int isize, char *key, int ksize) {
    int i = 0;
    int j = 0;
    int temp = 0;
    char *temp_ch = new char[isize];
    sbox(key, ksize);
    for (int l = 0; l < isize; l++) {
      i = (i + 1) % 256;
      j = (j + S[i]) % 256;
      temp = S[i];
      S[i] = S[j];
      S[j] = temp;
      temp_ch[l] = S[(S[i] + S[j]) % 256] ^ input[l];
    }
    return temp_ch;
  }

  /* Variables */

  bool isInFileSet;
  bool fileExist;
  bool isFileHeaderSet;
  bool isInfoHeaderSet;
  int pixelByteSize;
  int pixelcount;

  std::string filename;
  std::string lastError;
  std::FILE *infile;
  std::FILE *outfile;

  BMPFILEHEADER fileHeader;
  BMPINFOHEADER infoHeader;
  uint32_t *RGBQuad;
  bool EOF_in;
  int S[255];
};
