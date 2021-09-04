#include "HotPotaPongMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

using namespace std;


HotPotaPongMode::HotPotaPongMode() {

	// initialize rand seed	
	srand(time(NULL));

	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of HotPotaPongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 + 4*1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1,1);
		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
}

HotPotaPongMode::~HotPotaPongMode() {

	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	glDeleteTextures(1, &white_tex);
	white_tex = 0;
}

bool HotPotaPongMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_MOUSEMOTION) {
		//convert mouse from window pixels (top-left origin, +y is down) to clip space ([-1,1]x[-1,1], +y is up):
		glm::vec2 clip_mouse = glm::vec2(
			(evt.motion.x + 0.5f) / window_size.x * 2.0f - 1.0f,
			(evt.motion.y + 0.5f) / window_size.y *-2.0f + 1.0f
		);
		player_pot.x = (clip_to_court * glm::vec3(clip_mouse, 1.0f)).x;
	}

	return false;
}

void HotPotaPongMode::update(float elapsed) {
	// Grab current vegetable reference
	veg.age += elapsed;

	static std::mt19937 mt; //mersenne twister pseudo-random number generator

	//----- pot update -----

	{ //right player ai:
		ai_offset_update -= elapsed;
		if (ai_offset_update < elapsed) {
			//update again in [0.5,1.0) seconds:
			ai_offset_update = (mt() / float(mt.max())) * 0.5f + 0.5f;
			ai_offset = (mt() / float(mt.max())) * 2.5f - 1.25f;
		}
		if (ai_pot.x < veg.pos.x + ai_offset) {
			ai_pot.x = std::min(veg.pos.x + ai_offset, ai_pot.x + 2.0f * elapsed);
		} else {
			ai_pot.x = std::max(veg.pos.x + ai_offset, ai_pot.x - 2.0f * elapsed);
		}
	}

	//clamp pots to court:
	ai_pot.x = std::max(ai_pot.x, -court_radius.x + pot_radius.x);
	ai_pot.x = std::min(ai_pot.x,  court_radius.x - pot_radius.x);

	player_pot.x = std::max(player_pot.x, -court_radius.x + pot_radius.x);
	player_pot.x = std::min(player_pot.x,  court_radius.x - pot_radius.x);

	//----- veg update -----

	// At random, change polarity of tomato/potato
	if ((rand() % 1000) < 10) veg.is_potato = !veg.is_potato;

	//speed of veg doubles every four points:
	float speed_multiplier = 4.0f * std::pow(2.0f, (player_score + ai_score) / 4.0f);

	//velocity cap, though (otherwise veg can pass through pots):
	speed_multiplier = std::min(speed_multiplier, 10.0f);

	// To simulate new veg, only let it start moving 2.5f seconds after reset
	// (reset set age to 0.0f after it was consumed)
	if (veg.age > 2.5f) {
		veg.pos += elapsed * speed_multiplier * veg.velocity;
	}

	//---- collision handling ----

	auto new_vel = []() {
		// Generate a new velocity for x, y in range [-3.0f, 3.0f]
		float vel_x = 4.0f * (float)(rand() / ((float) RAND_MAX)) - 2.0f;
		float vel_y = 4.0f * (float)(rand() / ((float) RAND_MAX)) - 2.0f;
		
		// To prevent "slow" absolute speed for y direction (vel_y < |0.2f|), clip
		if (vel_y < 0) {
			vel_y = std::min(vel_y, -0.2f); 
		}
		else {
			vel_y = std::max(vel_y, 0.2f);
		}
		return glm::vec2(vel_x, vel_y);
	};

	auto reset_veg = [&]() {
		veg.pos = glm::vec2(court_radius.x - 0.5f, 0.0f); // next veg emerges from entry
		veg.age = 0.0f;
		veg.velocity = new_vel(); // Generate new random initial velocity
	};

	//pots:
	auto pot_vs_veg = [&](glm::vec2 const &pot, Vegetable veg, bool is_player) {
		//compute area of overlap:
		glm::vec2 min = glm::max(pot - pot_radius, veg.pos - veg_radius);
		glm::vec2 max = glm::min(pot + pot_radius, veg.pos + veg_radius);

		//if no overlap, no collision:
		if (min.x > max.x || min.y > max.y) return;

		if (max.x - min.x > max.y - min.y) {		// *** FLIPPED, > ***
			//wider overlap in x => bounce in y direction:
			if (veg.pos.y > pot.y) {
				veg.pos.y = pot.y + pot_radius.y + veg_radius.y;
				veg.velocity.y = std::abs(veg.velocity.y);
			} else {
				veg.pos.y = pot.y - pot_radius.y - veg_radius.y;
				veg.velocity.y = -std::abs(veg.velocity.y);
			}
			// tomato vs. potato pot hit
			if (veg.is_potato) {
				if (is_player) {
					player_score = std::max(player_score - 1, 0);
					reset_veg();
				}
				else {
					ai_score = std::max(ai_score - 1, 0);
					reset_veg();
				}
			}
			else {
				if (is_player) {
					player_score += 1;
					reset_veg();
				}
				else {
					ai_score += 1;
					reset_veg();
				}
			}
		} else {
			//wider overlap in y => bounce in x direction:
			if (veg.pos.x > pot.x) {
				veg.pos.x = pot.x + pot_radius.x + veg_radius.x;
				veg.velocity.x = std::abs(veg.velocity.x);
			} else {
				veg.pos.x = pot.x - pot_radius.x - veg_radius.x;
				veg.velocity.x = -std::abs(veg.velocity.x);
			}
			// warp y velocity based on offset from pot center:
			float vel = (veg.pos.y - pot.y) / (pot_radius.y + veg_radius.y);
			veg.velocity.y = glm::mix(veg.velocity.y, vel, 0.75f);
		}
	};
	pot_vs_veg(player_pot, veg, true);
	pot_vs_veg(ai_pot, veg, false);

	//court walls:
	{
		// Hit ceiling
		if (veg.pos.y > court_radius.y - veg_radius.y) {
			veg.pos.y = court_radius.y - veg_radius.y;
			if (veg.velocity.y > 0.0f) {
				veg.velocity.y = -veg.velocity.y;
				if (!veg.is_potato) {
					ai_score = std::max(ai_score - 1, 0);
				}
			}
		}
		
		// Hit floor
		if (veg.pos.y < -court_radius.y + veg_radius.y) {
			veg.pos.y = -court_radius.y + veg_radius.y;
			if (veg.velocity.y < 0.0f) {
				veg.velocity.y = -veg.velocity.y;
				if (!veg.is_potato) {
					player_score = std::max(player_score - 1, 0);
				}
			}
		}

		// Hit right wall
		if (veg.pos.x > court_radius.x - veg_radius.x) {
			veg.pos.x = court_radius.x - veg_radius.x;
			if (veg.velocity.x > 0.0f) {
				veg.velocity.x = -veg.velocity.x;
			}
		}

		// Hit left wall
		if (veg.pos.x < -court_radius.x + veg_radius.x) {
			veg.pos.x = -court_radius.x + veg_radius.x;
			if (veg.velocity.x < 0.0f) {
				veg.velocity.x = -veg.velocity.x;
			}
		}
	}

}

void HotPotaPongMode::draw(glm::uvec2 const &drawable_size) {

	#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0x5f8253ff);
	const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0xf2d2b6ff);

	// ********* game feature colors **************
	const glm::u8vec4 tomato_color = HEX_TO_U8VEC4(0x941010ff);
	const glm::u8vec4 tomato_leaf_color = HEX_TO_U8VEC4(0x26540fff);
	const glm::u8vec4 potato_color = HEX_TO_U8VEC4(0x4a3807ff);
	const glm::u8vec4 pot_color    = HEX_TO_U8VEC4(0x4f4f59ff);
	const glm::u8vec4 pot_handle_color = HEX_TO_U8VEC4(0x2d2d2eff);
	
	#undef HEX_TO_U8VEC4

	//other useful drawing constants:
	const float wall_radius = 0.05f;
	const float padding = 0.14f; //padding between outside of walls and edge of window

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;

	//inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
		//draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	//walls:
	draw_rectangle(glm::vec2(-court_radius.x-wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2( court_radius.x+wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2( 0.0f,-court_radius.y-wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);
	draw_rectangle(glm::vec2( 0.0f, court_radius.y+wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);
	// draw a doorway to allow new vegetables to come through
	draw_rectangle(glm::vec2( court_radius.x+wall_radius, 0.0f), glm::vec2(wall_radius, veg_radius.y + 0.1f), bg_color);

	//pots:
	draw_rectangle(glm::vec2(player_pot.x, player_pot.y + 0.2f), pot_radius, pot_color);

	auto lp_lhandle = player_pot - pot_radius;
	lp_lhandle.y += 2 * pot_radius.y + 0.2f;
	lp_lhandle -= 0.2f;
	draw_rectangle(lp_lhandle, glm::vec2(0.2f, 0.1f), pot_handle_color);

	auto lp_rhandle = player_pot + pot_radius;
	// lp_rhandle.y -= 0.2f +0.2f; functionally no change
	lp_rhandle.x += 0.2f;
	draw_rectangle(lp_rhandle, glm::vec2(0.2f, 0.1f), pot_handle_color);

	draw_rectangle(glm::vec2(ai_pot.x, ai_pot.y - 0.2f), pot_radius, pot_color);

	auto rp_lhandle = ai_pot + pot_radius;
	rp_lhandle.y -= 2 * pot_radius.y + 0.2f;
	rp_lhandle += 0.2f;
	draw_rectangle(rp_lhandle, glm::vec2(0.2f, 0.1f), pot_handle_color);

	auto rp_rhandle = ai_pot - pot_radius;
	rp_rhandle.x -= 0.2f;
	draw_rectangle(rp_rhandle, glm::vec2(0.2f, 0.1f), pot_handle_color);

	// inline helper function for filled circle drawing:
	// adapted from https://gist.github.com/linusthe3rd/803118
	// (reference found from + implementation inspired by lassyla's implementation: https://github.com/lassyla/game0/blob/master/BobMode.cpp)
	auto draw_filled_circle = [&](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
		const size_t num_tri = 12;
		for (size_t i = 0; i < num_tri; i++) {		// low poly circle with 12 sides
			auto pi = 2.0f * std::acos(0.f);
			auto a_theta = i 	 * 2.0f * pi / num_tri;
			auto b_theta = (i+1) * 2.0f * pi / num_tri;
			auto a = glm::vec3(center.x + (radius.x * std::cos(a_theta)), center.y + (radius.y * std::sin(a_theta)), 0.0f);
			auto b = glm::vec3(center.x + (radius.x * std::cos(b_theta)), center.y + (radius.y * std::sin(b_theta)), 0.0f);
			auto c = glm::vec3(center.x, center.y, 0.0f);
		
			vertices.emplace_back(a, color, glm::vec2(0.5f, 0.5f));
			vertices.emplace_back(b, color, glm::vec2(0.5f, 0.5f));
			vertices.emplace_back(c, color, glm::vec2(0.5f, 0.5f));
		}
	};

	// inline helper function for drawing leaves, which is a star
	auto draw_leaves = [&](glm::vec2 const &center,glm::vec2 const &radius, glm::u8vec4 const &color) {
		const size_t num_tri = 10;
		for (int i = 0; i < num_tri; i += 2) {
			auto pi = 2.0f * std::acos(0.f);
			auto a_theta = i 	 * 2.0f * pi / num_tri;
			auto b_theta = (i+2) * 2.0f * pi / num_tri;
			auto p_theta = (i+1) * 2.0f * pi / num_tri;

			auto a = glm::vec3(center.x + (radius.x * std::cos(a_theta)), center.y + (radius.y * std::sin(a_theta)), 0.0f);
			auto b = glm::vec3(center.x + (radius.x * std::cos(b_theta)), center.y + (radius.y * std::sin(b_theta)), 0.0f);
			auto c = glm::vec3(center.x, center.y, 0.0f);

			auto p = glm::vec3(center.x + ((radius.x + 0.2f) * std::cos(p_theta)), center.y + ((radius.y + 0.2f) * std::sin(p_theta)), 0.0f);

			vertices.emplace_back(a, color, glm::vec2(0.5f, 0.5f));
			vertices.emplace_back(b, color, glm::vec2(0.5f, 0.5f));
			vertices.emplace_back(c, color, glm::vec2(0.5f, 0.5f));

			vertices.emplace_back(a, color, glm::vec2(0.5f, 0.5f));
			vertices.emplace_back(p, color, glm::vec2(0.5f, 0.5f));
			vertices.emplace_back(b, color, glm::vec2(0.5f, 0.5f));
		}
	};

	//vegetable:
	if (veg.age > 2.5f) {
		if (veg.is_potato) {
			draw_filled_circle(veg.pos, veg_radius, potato_color);
		}
		else {
			draw_filled_circle(veg.pos, veg_radius, tomato_color);
			draw_leaves(veg.pos, glm::vec2(0.3f, 0.3f), tomato_leaf_color);
		}
	}
	
	//scores:
	glm::vec2 score_radius = glm::vec2(0.1f, 0.1f);
	for (uint32_t i = 0; i < player_score; ++i) {
		draw_rectangle(glm::vec2( -court_radius.x + (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, fg_color);
	}
	for (uint32_t i = 0; i < ai_score; ++i) {
		draw_rectangle(glm::vec2( court_radius.x - (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, fg_color);
	}


	//------ compute court-to-window transform ------

	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-court_radius.x - 2.0f * wall_radius - padding,
		-court_radius.y - 2.0f * wall_radius - padding
	);
	glm::vec2 scene_max = glm::vec2(
		court_radius.x + 2.0f * wall_radius + padding,
		court_radius.y + 2.0f * wall_radius + 3.0f * score_radius.y + padding
	);

	//compute window aspect ratio:
	float aspect = drawable_size.x / float(drawable_size.y);
	//we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
	);

	glm::vec2 center = 0.5f * (scene_max + scene_min);

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
	);
	//NOTE: glm matrices are specified in *Column-Major* order,
	// so each line above is specifying a *column* of the matrix(!)

	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
	clip_to_court = glm::mat3x2(
		glm::vec2(aspect / scale, 0.0f),
		glm::vec2(0.0f, 1.0f / scale),
		glm::vec2(center.x, center.y)
	);

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero so things will be drawn just with their colors:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);
	

	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.

}
