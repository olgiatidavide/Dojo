#include <iostream>
#include "BMP.h"

int main(){
	


	std::string txt = "";		//	text to hide
  std::string file = "";

  BMP prova;

	std::cout << "Inserisci il nome dell'immagine bmp: ";
	std::cin >> file;

  if(!prova.openInput(file)){
    std::cout << "Errore: " << prova.getLastError() << std::endl;
    return 1;
  }
  std::cout << (prova.isFileOpen() ? "[*] File Aperto" :  "[!] File non Aperto") << std::endl;
  std::cout << (prova.exist() ? "[*] il File Esiste" :  "[!] il File non Esiste") << std::endl;
  std::cout << "[*] Image Size: " << prova.getWidth() << "x" << prova.getHeight() << std::endl;
  std::cout << "[*] Image Depth: " << prova.getDepth() << std::endl;
  std::cout << "[*] Image MB Size: " << prova.getImageSize() << std::endl;

  if(!prova.injectText((char *)"dadada", 6, (char *)"prova", 5)){
    std::cout << "Errore: " << prova.getLastError() << std::endl;
    return 1;
  }
  
  return 0;
}