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
#define __DBG_MEM true
#ifndef TOOLBOX_INCLUDED
#include "tool.h"
#endif

#define DEBUG true

class BMP {
public:
  BMP() { reset(); }

                                                   /*==========
                                                    || Reset ||
                                                    ===========
     
                    1. File
     
                        1.1 fileExist - controllo esistenza del file di input - bool - default: false
                        1.2 isInFileSet - controllo stato del file di input - bool - default: false
                        1.3 filename - nome del file di input - string - default: ""
                        1.4 EOF_in - controlla EOF del file di input - bool - default: false
                        1.5 infile - puntatore al file di input - FILE - default:
                        1.6 outfile - puntatore al file di output - FIle - default:
                    
                    2. Struttura
                                     
                        2.1 isFileHeaderSet - controllo stato del fileheader - bool - default: false
                        2.2 isInfoHeaderSet - controllo stato dell'infoheader - bool - default: false
                        2.3 fileHeader - struttura che conterrà i primi 14 byte del file - BMPFILEHEADER - default:
                        2.4 infoHeader -  struttura che conterrà i 40 byte dal 14 al 45 del file - BMPINFOHEADER - default:
                        2.5 RGBQuad - array che conterrà le aventuali tabelle di colore - uint32_t - default:
                        
                    3. Pixel
                        
                        3.1 pixelByteSize - grandezza in byte dei pixel - int - default: -1
                        3.2 pixelcount - contatore dei pixel letti in totale - deafult: -1
                    
                    4. Errori
                        
                        4.1 lastError - log dell'ultimo errore occorso - string - default: ""*/
    
  void reset() {
      this->isInFileSet = this->fileExist = this->isFileHeaderSet = this->isInfoHeaderSet = this->EOF_in = false;
      this->lastError = this->filename = "";
      this->pixelByteSize = this->pixelcount = -1;
  }

                                                   /*===============
                                                    || File Setup ||
                                                    ================
                                                    
                    nome metodo: openInput
                    parametri : 1
                                name - nome del file da caricare - string
                    n. variabili locali: 0
                    variabili globali usate: 7 - [ filename, fileExist, infile, fileHeader, infoHeader, RGBQuad, isInFileSet ]
                    possibili return: true (tutto ok) oppure false (errore)
    
                    1. Controlli preliminari
                    
                        1.1 controllo la correttezza dell'estensione [checkName]
                        1.2 l'esistenza del file [fopen]
                        1.3 verifico se posso cominciare la lettura, altrimenti abort
                                     
                    2. Lettura Header del file
                        
                        2.1 setto isFileSet a true in modo da tener traccia dell'avvenuta apertura del file
                        2.2 verifico che la lettura dell'fileheader sia andata a buon fine [loadFileHeader]
                        2.3 verifico che la lettura dell'infoheader sia andata a buon fine [loadInfoHeader]
                        2.4 verifico che la lettura dell'RGBQuad sia andata a buon fine [loadRGBQuad]
                        2.5 se è andato tuttoa buon fine setto isInFileSet a true, altrimenti abort*/
    
    
  bool openInput(std::string name) {
      try {
          filename = name;
          fileExist = (infile = std::fopen(filename.c_str(), "r"));
        
          if (fileExist) {
              (loadFileHeader() && loadInfoHeader() && loadRGBQuad()) ? isInFileSet = true : throw(BMPException("Errore durante la lettura dell'Hearder"));
          } else {
              std::fclose(infile);
              throw(BMPException("Il file non esiste"));
          }
      } catch (BMPException &e) {
          lastError = e.what();
          if (DEBUG) {
              std::cout << lastError << std::endl;
          }
          return false;
      }
      return true;
  }

                                                /*==================
                                                 || Lettura Pixel ||
                                                 ===================
                    nome metodo: nextPixel
                    parametri: 0
                    n. variabili locali: 1
                                         buffer - continitore del pixel da leggere - char*
                    variabili globali usate: 4 - [ pixelByteSize, infile, EOF_in, pixelcount ]
                    possibili return: puntatore a buffer (tutto ok) oppure nullptr (errore)
                    
                    1. Passi Preliminari
                    
                       1.1 alloco il buffer di lettura dei pixel  [char* buffer]
                       1.2 pulisco il buffer del file di input  [fflush]
                    
                    2. Lettura pixel
                        
                       2.1 contrllo che il file di input non abbia raggunto l'EOF  [fread]
                       2.2 se non è stato generato nessun errore aumento pixelcount di uno e ritorno il pixel altrimenti inolto un errore*/
    
    
  char *nextPixel() {
      char *buffer = new char[this->pixelByteSize];
      try {
          std::fflush(this->infile);
          this->EOF_in = !(std::fread(buffer, 1, this->pixelByteSize, infile) == this->pixelByteSize);

          if (!this->EOF_in) {
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

                                                /*=================
                                                 || Steganogrfia ||
                                                 ==================
                                             
                    nome metodo: injectText
                    parametri: 3
                               secret - informazioni da processare - char*
                               length - lunghezza in byte dell'informazione da salvare nel file - int
                               key - chiave di crittografia da usare per proteggere le informazioni - char*
                    variabili locali : 5
                                       cipher - macchina crittografica usata per proteggere i dati - SpritzEngine
                                       outname - nome del file di output generato a partire dal file di input - string
                                       StegHeader - Header che conterrà le informazioni dei dati nascosti - STEGINFOHEADER
                                       edited_pixel - puntatore al ritorno del metodo nextPixel - char*
                                       index - contatore al numero di pixel processati
                    variabili globali usate: 7  - [ filename, outfile, EOF_in, pixelByteSize, fileHeader, infoHeader, RGBQuad ]
                    possibili return: true (tutto ok) oppure false (errore)
                                             
                    1. File
                       
                       1.1 aggiungo il prefisso sec_ al nome del file di input
                       1.2 apro il file di output in sola scrittura [ fopen ]
                    
                    2. Crittografia
                        
                       2.1 alloco la macchina crittografica
                       2.2 setto la chiave di crittografia [ setkey ]
                       2.3 critpo i dati in input [ run ]
                    
                    3. SteganoHeader
                                             
                       3.1 Alloco una variabile chiamata StegHeader di tipo STEGINFOHEADER
                       3.2 Setto il valore di stgType a 0x5354 (ST)
                       3.3 Setto stgDatalen a length * 8
                       3.4 Leggo 10 caratteri del nome dell'utilizzatore del pc e li salvo in stgAutor
                       3.5 uso la funzione hash di cipher per nascondere l'identià dell'autore e renderlo anonimo ma identificabile
                       3.6 Riporto il valore generato in precedeanza nella variabile stgAutor
                                             
                    4. Scrittura Header sul file di output
                        
                       4.1 Scrivo il fileHeader sul file di output [ pushFileHeader ]
                       4.2 Scrivo l'infoHeader sul file di output [ pushInfoHeader ]
                       4.3 Scrivo l'RGBQuad sul file di output  [ pushRGBQuad ]
                       4.3 Se tutto è andro a buon fine continuo a lavorare sul file altrimenti abort
                       4.4 Scrivo lo StegHeader sul file  [ pushStegInfoHeader ]
                                             
                    5. Scrittura Pixel
                                             
                       5.1 Leggo un Pixel e mi salvo il suo puntatore
                       5.2 ciclo finchè non ho raggiunto l'EOF o è avvenuto qualche altro errore (nullptr)
                       5.3 Controllo se ho ancore delle informazioni da nascondere
                       5.3a Se si, Modifico il lsb del pixel e scrivo
                       5.3b Se no, Scrivo il pixel letto senza modificarlo
                       5.4 Una volta finito il ciclo chiudo il file di output
                    */
    
    
  bool injectText(char *secret, int length, char *key) {
      try {

          std::string outname = "sec_" + filename;
          outfile = std::fopen(outname.c_str(), "w");

          blackmagic::SpritzEngine cipher;
          cipher.setkey(key);
          secret = cipher.run(secret);

          STEGINFOHEADER StegHeader;
          StegHeader.stgType = (short int) 0x5354;
          StegHeader.stgDatalen = length * 8;
          getlogin_r(StegHeader.stgAutor, 10);
          std::memcpy(&StegHeader.stgAutor, cipher.hash(StegHeader.stgAutor, 10), 10);

          if (pushFileHeader(fileHeader) &&
              pushInfoHeader(infoHeader) &&
              pushRGBQuad(this->RGBQuad) &&
              pushStegInfoHeader(StegHeader)){

              int index = 0;
              char *edited_pixel = nextPixel();
              while (!EOF_in || edited_pixel != NULL) {
                  if(index < length*8){
                      if((secret[(int)index/8] >> (index%8)) & 1){ edited_pixel[this->pixelByteSize - 1] &= ~1; }
                      else { edited_pixel[this->pixelByteSize - 1] |= 1; }
                      pushPixel(edited_pixel);
                      index++;
                  }
                  else { pushPixel(edited_pixel); }
                  delete[] edited_pixel;
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
    return (int)((this->infoHeader.biWidth * this->infoHeader.biHeight) / 10000000);
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

                                        /*==================
                                         || BMPFILEHEADER ||
                                         ===================
                                         
                    grandezza: 14 byte
                    descrizione: struttura che conterrà le informazioni salvate nel fileHeader del file di input
                    visibilità: privata
                    n. elementi: 4
                                         
                    Descrizione dei componenti:
                      
                       1. bfType - uint16_t - 2 byte - identificatore Bitmap - vincoli: BM (0x4D42)
                       2. bfSize - uint32_t - 4 byte - indicatore della compressione dell'immagine - vincoli: 0x00000000 (uncompressed)
                       3. bfReserved - uint32_t - 4 byte - spazio riservato - vincoli: 0x00000000
                       4. bfOffbits - uint32_t - 4 byte - posizione in bytes dalla quale cominciano il pixel - vincoli: NO*/
    
    
  struct BMPFILEHEADER {
      uint16_t bfType;
      uint32_t bfSize;
      uint32_t bfReserved;
      uint32_t bfOffbits;
  };
                                        /*==================
                                         || BMPINFOHEADER ||
                                         ===================
     
                    grandezza: 40 byte
                    descrizione: struttura che conterrà le informazioni salvate nel infoHeader del file di input
                    visibilità: privata
                    n. elementi: 11
     
                    Descrizione dei componenti:
     
                      1. biSize - uint32_t  - 4 byte - indica la grandezza totale del infoHeader - vincoli: NO
                      2. biWidth - uint32_t  - 4 byte - indica la larghezza dell'immagine in pixel - vincoli: NO
                      3. biHeight - uint32_t - 4 byte - indica l'altezza dell'immagine in pixel - vincoli: NO
                      4. biPlanes - uint16_t - 4 byte - Planes - vincoli: 0
                      5. biBitCount - uint16_t - 2 byte - profondità in bit dell'immagine - vincoli: 8/16/24/32
                      6. biCmpression - uint32_t- 4 byte - comprssione usata per l'immagine - vincoli: 0 */
    
  struct BMPINFOHEADER {
      uint32_t biSize; // 4 This is the size of the BMPINFOHEADER structure.
      uint32_t biWidth;  // 4 The width of the bitmap, in pixels.
      uint32_t biHeight; // 4 The height of the bitmap, in pixels.
      uint16_t biPlanes;    // 2 Set to 1.
      uint16_t biBitCount;  // 2 The bit depth of the bitmap. For 8-bit bitmaps, this is 8.
      uint32_t biCompression; // 4 Our bitmaps are uncompressed, so this field is set to 0.
      uint32_t biSizeImage;     // 4 The size of the padded image, in bytes.
      uint32_t biXPelsPerMeter; // 4 Horizontal resolution, in pixels per meter, of device displaying bitmap. This number is not significant for us, and will be set to 0.
      uint32_t biYPelsPerMeter; // 4 Vertical resolution, in pixels per meter, of device displaying bitmap. This number is not significant for us, and will be set to 0.
      uint32_t biClrUsed; // 4 This indicates how many colors are in the palette.
      uint32_t biClrImportant; // 4 This indicates how many colors are needed to display the bitmap. We will set it to 0, indicating all colors should be used.
  };

  struct STEGINFOHEADER {
    short int stgType;
    short int stgDatalen;
    char stgAutor[10];
  };

  /* Functions */

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
      std::fwrite(&to_be_pushed, sizeof(uint32_t), infoHeader.biClrUsed + 1, outfile);
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
      char *edited_pixel;
      char buffer[14];
      std::memcpy(&to_be_pushed, &buffer, 14);

      while (out_index < 14) {
        edited_pixel = nextPixel();
        edited_pixel[(infoHeader.biBitCount / 8) - 1] &= ((buffer[out_index] >> index) & 1);
        pushPixel(edited_pixel);
        index++;
        if (index > 7) {
          index = 0;
          out_index++;
        }
        delete[] edited_pixel;
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
      std::fflush(outfile);
      std::fwrite(to_be_pushed, 1, pixelByteSize, outfile);
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
};
