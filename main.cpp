#include "vk_engine.hpp"
#include <iostream>
#include "Camera.hpp"

void check(){
  system("leaks a.out");
}

int main()
{
  VulkanEngine engine;
  Camera& _camera = Camera::getInstance();
  _camera.initialize();
  engine.init();
  engine.run();
  engine.cleanup();
  return 0;
}