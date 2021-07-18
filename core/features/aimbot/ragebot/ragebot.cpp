#include "../../features.hpp"
#include<windows.h>

player_t* ragebot::GetBestTarget(c_usercmd* cmd) {
	int oFov = 180;
	int nFov = 0;
	player_t* ply = nullptr;

	for (int iPly = 0; iPly < interfaces::globals->max_clients; iPly++) {
		auto pCSPlayer = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(iPly));

		if (!pCSPlayer || pCSPlayer == csgo::local_player || pCSPlayer->team() == csgo::local_player->team() && variables::teamcheck || pCSPlayer->has_gun_game_immunity() ||
			!pCSPlayer->is_alive() || pCSPlayer->dormant())
			continue;

		vec3_t eyepos = csgo::local_player->get_eye_pos();
		vec3_t enemyheadpos = pCSPlayer->get_bone_position(8);
		vec3_t angleTo = math::calculate_angle(eyepos, enemyheadpos);
		angleTo.clamp();

		nFov = cmd->viewangles.distance_to(angleTo);

		if (nFov < oFov) {
			oFov = nFov;
			ply = pCSPlayer;
		}
	}

	return ply;
}

void ragebot::aimbot(c_usercmd* cmd) {
		
	if (!variables::aimbot || GetAsyncKeyState(variables::onkey) || !interfaces::engine->is_connected() || !interfaces::engine->is_in_game() || !csgo::local_player
			|| !csgo::local_player->is_alive())
			return;

		const auto weapon_type = csgo::local_player->active_weapon()->get_weapon_data()->weapon_type;

		if (weapon_type == WEAPONTYPE_GRENADE || weapon_type == WEAPONTYPE_C4 || weapon_type == WEAPONTYPE_KNIFE)
			return;

		player_t* target = GetBestTarget(cmd);
		auto weapon = csgo::local_player->active_weapon();
		if (target) {
			if (!csgo::local_player->is_scoped() && weapon_type == WEAPONTYPE_SNIPER_RIFLE && variables::autoscope)
				cmd->buttons |= in_attack2;

			vec3_t eyepos = csgo::local_player->get_eye_pos();
			vec3_t targetbonepos = target->get_bone_position(8);

			vec3_t viewangles = math::calculate_angle(eyepos, targetbonepos);
			viewangles.clamp();

			vec3_t delta = cmd->viewangles - viewangles;
			delta.clamp();

			vec3_t finalAng = cmd->viewangles - delta;
			finalAng.clamp();

			if (variables::autoshoot && weapon_type == WEAPONTYPE_SNIPER_RIFLE && csgo::local_player->is_scoped())
				cmd->buttons |= in_attack;

			if (variables::aimbot_isvisiblecheck && csgo::local_player->can_see_player_pos(target, target->get_eye_pos())) {
				cmd->viewangles = finalAng;
				if (!variables::silent)
					interfaces::engine->set_view_angles(cmd->viewangles);
			}
			else if (!variables::aimbot_isvisiblecheck) {
				cmd->viewangles = finalAng;
				if (!variables::silent)
					interfaces::engine->set_view_angles(cmd->viewangles);
			}
		}
}