#include <iostream>
#include <chrono>
#include <thread>
#include "aqua2/gamepad.h"

using namespace ssr::aqua2;

int main(void) {
  std::cout << "libaqua2 / GamePad test" << std::endl;
  GamePad* gamePad = new GamePad("");
  while(true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      gamePad->update();
      
      
      for(size_t i = 0;i < gamePad->axis.size();i++) {
          std::cout << "A["<<i<<"]:" << gamePad->axis[i] << std::endl;
      }/*
      for(size_t i = 0;i < gamePad.buttons.size();i++) {
          std::cout << "B["<<i<<"]:" << gamePad.buttons[i] << std::endl;
      }
      */
  }
  return(0);
}
