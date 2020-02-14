#include <iostream>

#include <SDL2/SDL.h>

static SDL_Window *window;
static SDL_Renderer *renderer;

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

static bool init_sdl()
{
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_HAPTIC|SDL_INIT_JOYSTICK))
		return false;

	if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer))
		return false;

	std::cerr << "sdl initialized" << std::endl;
	return true;
}

static void exit_sdl()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

static SDL_Joystick *get_joystick()
{
	int nb_joysticks;
	SDL_Joystick *joy = nullptr;

	nb_joysticks = SDL_NumJoysticks();
	std::cerr << nb_joysticks << " joysticks found" << std::endl;

	for (int i = 0; i < nb_joysticks; i++) {
		joy = SDL_JoystickOpen(i);
		if (joy) {
			if (SDL_JoystickIsHaptic(joy)) {
				std::cerr << "name: " << SDL_JoystickName(joy)
					  << std::endl;
				break;
			}
		}
	}

	return joy;
}

static int query_haptic(SDL_Haptic *haptic)
{
	static std::string properties[] = {
		"constant", "sine", "leftright", "triangle",
		"sawtoothup", "sawtoothdown", "ramp", "spring",
		"damper", "inertia", "friction", "custom",
		"gain", "autocenter", "status", "pause" };
	unsigned int caps = SDL_HapticQuery(haptic);
	unsigned int i = 0;

	std::cerr << "haptic capabilities:" << std::endl;
	for (const std::string &p: properties)
		if (caps & (1u << i++))
			std::cerr << " " << p << std::endl;

	return caps;
}

int main(int argc, char *argv[])
{
	if (!init_sdl())
		return -1;

	SDL_SetWindowGrab(window, SDL_TRUE);

	SDL_Joystick *joy;
	joy = get_joystick();
	if (!joy)
		std::cerr << "no joystick found" << std::endl;

	SDL_Haptic *haptic;
	haptic = SDL_HapticOpenFromJoystick(joy);
	if (haptic == nullptr) {
		std::cerr << "HapticOpen fails" << std::endl;
		return -1;
	}

	query_haptic(haptic);
	SDL_HapticSetAutocenter(haptic, 0);

	// if (SDL_HapticRumbleInit(haptic))
	// 	std::cerr << "RumbleInit error" << std::endl;
	// if (SDL_HapticRumblePlay(haptic, 0.5, 2000))
	// 	std::cerr << "RumblePlay error" << std::endl;

	SDL_HapticEffect effect = {};
//	SDL_memset(&effect, 0, sizeof(SDL_HapticEffect));

	effect.type = SDL_HAPTIC_SPRING;
	effect.condition.length = SDL_HAPTIC_INFINITY;

	effect.condition.right_sat[0] = 0xffff;
	effect.condition.left_sat[0] = 0xffff;
	effect.condition.right_coeff[0] = 0x7fff;
	effect.condition.left_coeff[0] = 0x7fff;

	effect.condition.right_sat[1] = 0xffff;
	effect.condition.left_sat[1] = 0xffff;
	effect.condition.right_coeff[1] = 0x7fff;
	effect.condition.left_coeff[1] = 0x7fff;

 	int e1 = SDL_HapticNewEffect(haptic, &effect);
	if (e1 < 0)
		std::cerr << "new effect fails" << std::endl;

	if (SDL_HapticRunEffect(haptic, e1, SDL_HAPTIC_INFINITY))
		std::cerr << "run effect fails" << std::endl;

	bool run = true;
	while (run) {
		SDL_Event e;

		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_MOUSEMOTION:
			{
				float center[2];
				center[0] = (float)e.motion.x / WINDOW_WIDTH - 0.5;
				center[1] = (float)e.motion.y / WINDOW_HEIGHT - 0.5;

				effect.condition.center[0] = 2 * center[0] * 0x7fff;
				effect.condition.center[1] = 2 * center[1] * 0x7fff;

				if (SDL_HapticUpdateEffect(haptic, e1, &effect))
					std::cerr << "update effect fails" << std::endl;
				break;
			}
			case SDL_KEYDOWN:
				if (e.key.keysym.sym != SDLK_ESCAPE)
					break;
			case SDL_QUIT:
				run = false;
				break;
			}
		}
		SDL_Delay(1);
	}

	SDL_HapticDestroyEffect(haptic, e1);
	SDL_HapticClose(haptic);
	SDL_JoystickClose(joy);
	exit_sdl();
	return 0;
}
