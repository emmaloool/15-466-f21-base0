#include "ColorTextureProgram.hpp"

#include "Mode.hpp"
#include "GL.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

#include <math.h>
#include <stdlib.h>
#include <time.h>

/*
 * HotPotaPongMode is a game mode that implements a single-player game of Pong,
 * with a twist - you want to catch as many tomatoes as possible, but not potatoes
 */

struct HotPotaPongMode : Mode {
	HotPotaPongMode();
	virtual ~HotPotaPongMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	glm::vec2 court_radius = glm::vec2(7.0f, 5.0f);
	glm::vec2 pot_radius = glm::vec2(1.2f, 0.7f);
	glm::vec2 veg_radius = glm::vec2(0.75f, 0.75f);

	glm::vec2 player_pot = glm::vec2(0.0f, -court_radius.y + 0.5f);
	glm::vec2 ai_pot = glm::vec2(0.0f, court_radius.y - 0.5f);

	// Vegetable definition
	struct Vegetable {
		explicit Vegetable(glm::vec2 court_radius) {
			pos = glm::vec2(court_radius.x - 0.5f, 0.0f); // vegetable emerges from hole in right wall
			velocity = glm::vec2(-1.5f, -0.5f);			  // initial velocity
			age = 0.0f;				// start off not visible
			is_potato = true;		// start off as tomato
		};
		~Vegetable() {};

		glm::vec2 pos;
		glm::vec2 velocity;
		float age;			// lifetime of current veg
		bool is_potato;		// display veg as potato or tomato
	};
	Vegetable veg = Vegetable(court_radius); // One instance is maintained in the game.
	// As a tomato gets "consumed" (i.e. caught by the pot), its values are reset

	int player_score = 0;
	int ai_score = 0;

	float ai_offset = 0.0f;
	float ai_offset_update = 0.0f;

	//----- opengl assets / helpers ------

	struct Vertex {
		Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 4*3 + 1*4 + 4*2, "HotPotaPongMode::Vertex should be packed");

	//Shader program that draws transformed, vertices tinted with vertex colors:
	ColorTextureProgram color_texture_program;

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;

	//Solid white texture:
	GLuint white_tex = 0;

	//matrix that maps from clip coordinates to court-space coordinates:
	glm::mat3x2 clip_to_court = glm::mat3x2(1.0f);
	// computed in draw() as the inverse of OBJECT_TO_CLIP
	// (stored here so that the mouse handling code can use it to position the paddle)

};
