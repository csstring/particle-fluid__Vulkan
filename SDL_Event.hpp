#pragma once

class Camera;
union SDL_Event;
namespace vkutil {
	void SDL_event_process(SDL_Event* e, Camera& camera, bool& bQuit);
}
