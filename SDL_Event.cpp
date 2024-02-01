#include "SDL_Event.hpp"
#include "SDL.h"
#include "SDL_vulkan.h"
#include "Camera.hpp"
#include "imgui_impl_sdl2.h"
namespace vkutil {
  void SDL_event_process(SDL_Event* e, Camera& camera, bool& bQuit)
  {
    while (SDL_PollEvent(e) != 0)
		{
      ImGui_ImplSDL2_ProcessEvent(e);
			if (e->type == SDL_QUIT || (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_ESCAPE)) 
        bQuit = true;
			else if (e->type == SDL_MOUSEMOTION)
			{
        camera.mouse_callback(e->motion.x, e->motion.y);
        // std::cout << "SDL_MOUSEMOTION" << std::endl;
			}
      else if (e->type == SDL_MOUSEBUTTONDOWN){
        camera._clickOn = true;
        // std::cout << "SDL_MOUSEBUTTONDOWN" << std::endl;
      }
      else if (e->type == SDL_MOUSEBUTTONUP){
        camera._clickOn = false;
        // std::cout << "SDL_MOUSEBUTTONUP" << std::endl;
      }
      else if (e->type == SDL_KEYDOWN)
      {
        if (e->key.keysym.sym == SDLK_c){
          camera._isOn = camera._isOn == false ? true : false;
        }
				if (e->key.keysym.sym == SDLK_w){
					camera._moveDir = CAMERA_MOVE_DIR::MOVE_W;
				}
				else if (e->key.keysym.sym == SDLK_s){
					camera._moveDir = CAMERA_MOVE_DIR::MOVE_S;
				}
				else if (e->key.keysym.sym == SDLK_a){
					camera._moveDir = CAMERA_MOVE_DIR::MOVE_A;
				}
				else if (e->key.keysym.sym == SDLK_d){
					camera._moveDir = CAMERA_MOVE_DIR::MOVE_D;
				}
      }
      else if (e->type == SDL_KEYUP){
        if (e->key.keysym.sym == SDLK_w || e->key.keysym.sym == SDLK_s
              || e->key.keysym.sym == SDLK_a||e->key.keysym.sym == SDLK_d){
          camera._moveDir = CAMERA_MOVE_DIR::MOVE_STOP;
        }
      }
      else {
        if (e->type == SDL_WINDOWEVENT) {
        switch (e->window.event) {
        case SDL_WINDOWEVENT_SHOWN:
            SDL_Log("Window %d shown", e->window.windowID);
            break;
        case SDL_WINDOWEVENT_HIDDEN:
            SDL_Log("Window %d hidden", e->window.windowID);
            break;
        case SDL_WINDOWEVENT_EXPOSED:
            SDL_Log("Window %d exposed", e->window.windowID);
            break;
        case SDL_WINDOWEVENT_MOVED:
            SDL_Log("Window %d moved to %d,%d",
                    e->window.windowID, e->window.data1,
                    e->window.data2);
            break;
        case SDL_WINDOWEVENT_RESIZED:
            SDL_Log("Window %d resized to %dx%d",
                    e->window.windowID, e->window.data1,
                    e->window.data2);
            break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            SDL_Log("Window %d size changed to %dx%d",
                    e->window.windowID, e->window.data1,
                    e->window.data2);
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
            SDL_Log("Window %d minimized", e->window.windowID);
            break;
        case SDL_WINDOWEVENT_MAXIMIZED:
            SDL_Log("Window %d maximized", e->window.windowID);
            break;
        case SDL_WINDOWEVENT_RESTORED:
            SDL_Log("Window %d restored", e->window.windowID);
            break;
        case SDL_WINDOWEVENT_ENTER:
            SDL_Log("Mouse entered window %d",
                    e->window.windowID);
            break;
        case SDL_WINDOWEVENT_LEAVE:
            SDL_Log("Mouse left window %d", e->window.windowID);
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            SDL_Log("Window %d gained keyboard focus",
                    e->window.windowID);
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            SDL_Log("Window %d lost keyboard focus",
                    e->window.windowID);
            break;
        case SDL_WINDOWEVENT_CLOSE:
            SDL_Log("Window %d closed", e->window.windowID);
            break;
#if SDL_VERSION_ATLEAST(2, 0, 5)
        case SDL_WINDOWEVENT_TAKE_FOCUS:
            SDL_Log("Window %d is offered a focus", e->window.windowID);
            break;
        case SDL_WINDOWEVENT_HIT_TEST:
            SDL_Log("Window %d has a special hit test", e->window.windowID);
            break;
#endif
        default:
            SDL_Log("Window %d got unknown event %d",
                    e->window.windowID, e->window.event);
            break;
        }
    }
      }
    }
  }
}