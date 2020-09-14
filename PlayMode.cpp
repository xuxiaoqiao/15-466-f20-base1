#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <array>

PlayMode::PlayMode() {
	std::vector<PPU466::Tile> tile_input;
	std::vector<PPU466::Palette> palette_input;
	//TODO: decide which kind of path to use
	std::ifstream tile_stream(data_path("assets/tiles.chunk"), std::ios::binary);
	std::ifstream palette_stream(data_path("assets/palettes.chunk"), std::ios::binary);
	if (tile_stream.is_open() && palette_stream.is_open()){
		std::cout<<"Input stream open success"<<std::endl;
	}
	read_chunk(tile_stream, "til0", &tile_input);
	read_chunk(palette_stream, "plt0", &palette_input);
	std::copy(tile_input.begin(),tile_input.end(), ppu.tile_table.begin());
	std::copy(palette_input.begin(), palette_input.end(), ppu.palette_table.begin());

	for (uint32_t i = 0; i < ppu.background.size(); ++i) {
		ppu.background[i] = int16_t(
			WHITE_PALETTE_IDX << 8
			| WHITE_TILE_IDX
		);
	}

	for (uint32_t i=0; i<num_fish; i++){
		fish_at.push_back(glm::vec2(0.0f, 240.0f));
		fish_velocity.push_back(glm::vec2(0.0f, 0.0f));
		fish_active.push_back(false);
	}

	for (uint32_t i=0; i<num_whale; i++){
		whale_at.push_back(glm::vec2(0.0f, 240.0f));
		whale_velocity.push_back(glm::vec2(0.0f, 0.0f));
		whale_active.push_back(false);
	}

	for (uint32_t i=0; i<num_bomb; i++){
		bomb_at.push_back(glm::vec2(0.0f, 240.0f));
		bomb_velocity.push_back(glm::vec2(0.0f, 0.0f));
		bomb_active.push_back(false);
	}
	

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			// TODO: implement me
			space_key.downs += 1;
			space_key.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space_key.pressed = false;
			return true;
		}
	}

	return false;
}


void PlayMode::update_target(std::vector<glm::vec2>& at, std::vector<glm::vec2>& velocity, std::vector<bool>& active, uint8_t num, float elapsed){
	constexpr float Gravity = 20.0f;
	for (int i=0; i<num; i++){
		if (active[i]){
			velocity[i].y -= Gravity * elapsed;
			at[i].x += velocity[i].x * elapsed;
			at[i].y += velocity[i].y * elapsed;
			if (at[i].y < 0.0f || at[i].x < 0.0f || at[i].x > 256.0f){
				active[i] = false;
				at[i].y = 240;
			}
		} else {
			static std::mt19937 mt;
			if ((mt()/float(mt.max())) < 0.01f){
				active[i] = true;
			 	at[i].y = 0.0f;
				at[i].x = (mt()/float(mt.max())) * 256.0f;
				velocity[i].x = (mt()/float(mt.max()))*40.0f - 20.0f;
				velocity[i].y = 80.0f;
			}
		}
	}
}
bool pt_in(float pt_x,float pt_y, glm::vec2 target){
	if (pt_x>=target.x && pt_x<=target.x+8 && pt_y >= target.y && pt_y<=target.y+8){
		return true;
	}
	return false;
}

void check_hit(std::vector<glm::vec2>& at, glm::vec2 p, std::vector<bool>& active, int pts){
	for(int i = 0; i<at.size();i++){
		if(at[i].y == 240){
			continue;
		}
		bool hit = pt_in(p.x,p.y,at[i]) || pt_in(p.x+8,p.y,at[i]) 
					|| pt_in(p.x+8,p.y+8,at[i]) || pt_in(p.x,p.y+8,at[i]);
		if(hit){
			active[i] = false;
			at[i].y = 240;
			score += pts;
		}
	}
}

void PlayMode::update(float elapsed) {
	time_remain -= elapsed;
	//slowly rotates through [0,1):
	// (will be used to set background color)
	background_fade += elapsed / 10.0f;
	background_fade -= std::floor(background_fade);

	// constexpr float PlayerSpeed = 30.0f;
	// if (left.pressed) boomerang_at.x -= PlayerSpeed * elapsed;
	// if (right.pressed) boomerang_at.x += PlayerSpeed * elapsed;
	// if (down.pressed) boomerang_at.y -= PlayerSpeed * elapsed;
	// if (up.pressed) boomerang_at.y += PlayerSpeed * elapsed;

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	space_key.downs = 0;

	if (space_key.pressed) {
		if (boomerang_state == BoomerangState::INACTIVE) {
			boomerang_state = BoomerangState::HOLDING;
			boomerang_holding_time = elapsed;
		} else if (boomerang_state == BoomerangState::HOLDING) {
			boomerang_holding_time += elapsed;
		}
	} else {
		if (boomerang_state == BoomerangState::HOLDING) {
			boomerang_state = BoomerangState::FLYING;
			boomerang_vec_x =
				std::min<double>(BOOMERANG_INIT_SPEED_COEFFICIENT * boomerang_holding_time, BOOMERANG_MAX_SPEED);
		}
	}

	if (boomerang_state == BoomerangState::FLYING) {
		boomerang_at.x += boomerang_vec_x * elapsed;
		boomerang_vec_x -= BOOMERANG_ACCELERATION * elapsed;
		if (boomerang_at.x <= 0) {
			boomerang_vec_x = 0;
			boomerang_state = BoomerangState::INACTIVE;
			boomerang_at.x = 0;
		}
	}



	update_target(fish_at, fish_velocity, fish_active, num_fish, elapsed);
	update_target(whale_at, whale_velocity, whale_active, num_whale, elapsed);
	update_target(bomb_at, bomb_velocity, bomb_active, num_bomb, elapsed);

	//check if boomrang hit target
	check_hit(fish_at, boomerang_at, fish_active,1);
	check_hit(whale_at,boomerang_at, whale_active,10);
	check_hit(bomb_at, boomerang_at,bomb_active,-10);


}


void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	//background color will be some hsv-like fade:
	ppu.background_color = glm::u8vec4(
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 0.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 1.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 2.0f / 3.0f) ) ) ))),
		0xff
	);

	// background scroll feature removed

	//player sprite:
	ppu.sprites[0].x = uint8_t(std::min<double>(boomerang_at.x, 255.0));
	ppu.sprites[0].y = uint8_t(boomerang_at.y);
	if (boomerang_state != BoomerangState::FLYING || boomerang_vec_x >= 0.0) {
		ppu.sprites[0].index = BOOMERANG_RIGHT_TILE_IDX;
		ppu.sprites[0].attributes = BOOMERANG_RIGHT_PALETTE_IDX;
	} else {
		ppu.sprites[0].index = BOOMERANG_LEFT_TILE_IDX;
		ppu.sprites[0].attributes = BOOMERANG_LEFT_PALETTE_IDX;
	}


	//target sprite:
	for (int i=0; i<num_fish; i++){
		ppu.sprites[i+1].x = int32_t(fish_at[i].x);
		ppu.sprites[i+1].y = int32_t(fish_at[i].y);
		ppu.sprites[i+1].index = FISH_TILE_IDX;
		ppu.sprites[i+1].attributes = FISH_PALETTE_IDX;
	}

	for (int i=0; i<num_whale; i++){
		ppu.sprites[i+num_fish+1].x = int32_t(whale_at[i].x);
		ppu.sprites[i+num_fish+1].y = int32_t(whale_at[i].y);
		ppu.sprites[i+num_fish+1].index = WHALE_TILE_IDX;
		ppu.sprites[i+num_fish+1].attributes = WHALE_PALETTE_IDX;
	}

	for (int i=0; i<num_bomb; i++){
		ppu.sprites[i+num_fish+num_whale+1].x = int32_t(bomb_at[i].x);
		ppu.sprites[i+num_fish+num_whale+1].y = int32_t(bomb_at[i].y);
		ppu.sprites[i+num_fish+num_whale+1].index = BOMB_TILE_IDX;
		ppu.sprites[i+num_fish+num_whale+1].attributes = BOMB_PALETTE_IDX;
	}

	constexpr int SCORE_DISPLAY_WIDTH = 3;
	int score_sprites_begin = num_fish + num_whale + num_bomb + 1;
	constexpr std::array<uint8_t, 10> NUMBERS_TILE_IDX = {
		ZERO_TILE_IDX,
		ONE_TILE_IDX,
		TWO_TILE_IDX,
		THREE_TILE_IDX,
		FOUR_TILE_IDX,
		FIVE_TILE_IDX,
		SIX_TILE_IDX,
		SEVEN_TILE_IDX,
		EIGHT_TILE_IDX,
		NINE_TILE_IDX
	};
	constexpr std::array<uint8_t, 10> NUMBERS_PALETTE_IDX = {
		ZERO_PALETTE_IDX,
		ONE_PALETTE_IDX,
		TWO_PALETTE_IDX,
		THREE_PALETTE_IDX,
		FOUR_PALETTE_IDX,
		FIVE_PALETTE_IDX,
		SIX_PALETTE_IDX,
		SEVEN_PALETTE_IDX,
		EIGHT_PALETTE_IDX,
		NINE_PALETTE_IDX
	};
	{
		std::array<int, 3> score_separate_digits;
		if (score > 1000) {
			score_separate_digits = {9, 9, 9};
		} else {
			score_separate_digits = {(score / 100) % 10, (score / 10) % 10, score % 10};
		}

		for (int i = 0; i < SCORE_DISPLAY_WIDTH; i++) {
			ppu.sprites[score_sprites_begin + i].x = 255 - 8 * 3 + i * 8;
			ppu.sprites[score_sprites_begin + i].y = 239 - 8;
			ppu.sprites[score_sprites_begin + i].index = NUMBERS_TILE_IDX.at(score_separate_digits.at(i));
			ppu.sprites[score_sprites_begin + i].attributes = NUMBERS_PALETTE_IDX.at(score_separate_digits.at(i));
		}
	}

	int time_sprites_begin = score_sprites_begin + 3;
	std::array<int,2> time_digits = {(int)time_remain/10, (int)time_remain%10};

	for(int i = 0; i<2;i++){
		ppu.sprites[i+time_sprites_begin].x = 8*(i+1);
		ppu.sprites[i+time_sprites_begin].y =239-8;
		ppu.sprites[i+time_sprites_begin].index = NUMBER_TILE_IDX[time_digits[i]];
		ppu.sprites[i+time_sprites_begin].attributes = NUMBER_PALETTE_IDX[time_digits[i]];
	}

	
	//--- actually draw ---
	ppu.draw(drawable_size);
}
