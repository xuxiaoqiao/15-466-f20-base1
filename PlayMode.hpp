#include "PPU466.hpp"
#include "Mode.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"
#include "assets_res.h"

#include <iostream>
#include <fstream>

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	void update_target(std::vector<glm::vec2>& at, std::vector<glm::vec2>& velocity, std::vector<bool>& active, uint8_t num, float elapsed);

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space_key;
	// TODO: remove left right down up, if it's not needed -- xiaoqiao

	//some weird background animation:
	float background_fade = 0.0f;

	//boomerang:
	glm::vec2 boomerang_at = glm::vec2(0.0f, 128.0f);
	double boomerang_vec_x = 0.0;
	enum class BoomerangState { INACTIVE, HOLDING, FLYING };
	BoomerangState boomerang_state = BoomerangState::INACTIVE;
	double boomerang_holding_time = 0.0; // only used when boomerang == HOLDING
	static constexpr double BOOMERANG_INIT_SPEED_COEFFICIENT = 200;
	static constexpr double BOOMERANG_ACCELERATION = 100;
	const double BOOMERANG_MAX_SPEED = sqrt(256 * 2 * BOOMERANG_ACCELERATION);

	//fish
	std::vector<glm::vec2> fish_at;
	std::vector<glm::vec2> fish_velocity;
	std::vector<bool> fish_active;

	uint8_t num_fish = 10;

	//whale
	std::vector<glm::vec2> whale_at;
	std::vector<glm::vec2> whale_velocity;
	std::vector<bool> whale_active;

	uint8_t num_whale = 2;

	//bomb
	std::vector<glm::vec2> bomb_at;
	std::vector<glm::vec2> bomb_velocity;
	std::vector<bool> bomb_active;

	uint8_t num_bomb = 1;

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};
