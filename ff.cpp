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

static SDL_Joystick *master, *slave;
static SDL_Haptic *slave_haptic;

static bool init_joystick()
{
	int nb_joysticks;
	SDL_Joystick *joy = nullptr;

	nb_joysticks = SDL_NumJoysticks();
	std::cerr << nb_joysticks << " joysticks found" << std::endl;

	for (int i = 0; i < nb_joysticks && !(master && slave); i++) {
		joy = SDL_JoystickOpen(i);
		if (!joy)
			continue;

		if (!slave && SDL_JoystickIsHaptic(joy)) {
			slave = joy;
			std::cerr << "slave : " << SDL_JoystickName(slave)
				  << " " << SDL_JoystickInstanceID(slave)
				  << std::endl;

			slave_haptic = SDL_HapticOpenFromJoystick(slave);
			if (slave_haptic) {
				query_haptic(slave_haptic);
				SDL_HapticSetAutocenter(slave_haptic, 0);
			} else {
				SDL_JoystickClose(slave);
				slave = nullptr;
			}
		} else if (!master) {
			master = joy;
			std::cerr << "master: " << SDL_JoystickName(master)
				  << " " << SDL_JoystickInstanceID(master)
				  << std::endl;

			if (SDL_JoystickIsHaptic(master)) {
				SDL_Haptic *h;
				h = SDL_HapticOpenFromJoystick(master);
				if (h) {
					query_haptic(h);
					SDL_HapticSetAutocenter(h, 100);
				}
			}
		} else
			SDL_JoystickClose(joy);
	}

	return master && slave;
}

int main(int argc, char *argv[])
{
	if (!init_sdl())
		return -1;

	SDL_SetWindowGrab(window, SDL_TRUE);

	if (!init_joystick()) {
		std::cerr << "no joystick found" << std::endl;
		return -1;
	}

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

	int e1 = SDL_HapticNewEffect(slave_haptic, &effect);
	if (e1 < 0)
		std::cerr << "new effect fails" << std::endl;

	if (SDL_HapticRunEffect(slave_haptic, e1, SDL_HAPTIC_INFINITY))
		std::cerr << "run effect fails" << std::endl;

	bool run = true;
	while (run) {
		SDL_Event e;

		while (SDL_PollEvent(&e)) {
			switch (e.type) {
#if 0
			case SDL_MOUSEMOTION:
			{
				float center[2];
				center[0] = (float)e.motion.x / WINDOW_WIDTH - 0.5;
				center[1] = (float)e.motion.y / WINDOW_HEIGHT - 0.5;

				effect.condition.center[0] = 2 * center[0] * 0x7fff;
				effect.condition.center[1] = 2 * center[1] * 0x7fff;

				if (SDL_HapticUpdateEffect(slave_haptic, e1, &effect))
					std::cerr << "update effect fails" << std::endl;
				break;
			}
#endif
			case SDL_JOYAXISMOTION:
				if (e.jaxis.which == SDL_JoystickInstanceID(master)) {
					float center[2];
					center[0] = (float)SDL_JoystickGetAxis(master, 0) / (2 * 32767);
					center[1] = (float)SDL_JoystickGetAxis(master, 1) / (2 * 32767);

					effect.condition.center[0] = 2. * center[0] * 0x7fff;
					effect.condition.center[1] = 2. * center[1] * 0x7fff;

					if (SDL_HapticUpdateEffect(slave_haptic, e1, &effect))
						std::cerr << "update effect fails" << std::endl;
				}
				break;
			case SDL_KEYDOWN:
				if (e.key.keysym.sym != SDLK_ESCAPE)
					break;
			case SDL_QUIT:
				run = false;
				break;
			}
		}
	}

	SDL_HapticDestroyEffect(slave_haptic, e1);
	SDL_HapticClose(slave_haptic);
	SDL_JoystickClose(master);
	SDL_JoystickClose(slave);
	exit_sdl();
	return 0;
}
