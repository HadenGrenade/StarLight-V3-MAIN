#include "../features.hpp"
#include "../../menu/menu.hpp"

#include <algorithm>

void animations::animation_fix(int stage) {
	menu::JunkCodeTutorial();

	static float proper_abs = 0.f;

	if (interfaces::engine->is_in_game() && interfaces::engine->is_connected() && stage == FRAME_RENDER_START && csgo::cmd && csgo::local_player != nullptr && csgo::local_player->is_alive()) {
		auto state = csgo::local_player->get_anim_state();
		auto layers = csgo::local_player->anim_overlays();
		if (state && layers) {
			auto fresh_tick = []() -> bool {
				static int old_tick = 0;
				if (old_tick != interfaces::globals->tick_count) {
					old_tick = interfaces::globals->tick_count;
					return true;
				}
				return false;
			};

			static animation_layer backup_layers[15];
			static auto sent_pose_params = csgo::local_player->pose_param();
			if (fresh_tick())
			{
				std::memcpy(backup_layers, layers, (sizeof(animation_layer) * 15));
				csgo::local_player->animations(true);
				csgo::local_player->update_state(state, csgo::cmd->viewangles);
				if (csgo::send_packet)
				{
					proper_abs = csgo::local_player->abs_angles().y;
					sent_pose_params = csgo::local_player->pose_param();
				}
			}
			csgo::local_player->animations(false);
			csgo::local_player->setup_bones(nullptr, 128, BONE_USED_BY_ANYTHING, interfaces::globals->cur_time);
			csgo::local_player->invalidate_bone_cache();
			csgo::local_player->update_client_side_animations();
			csgo::local_player->set_abs_angles(vec3_t(0, proper_abs, 0));
			std::memcpy(layers, backup_layers, (sizeof(animation_layer) * 15));
			csgo::local_player->pose_param() = sent_pose_params;
		}
		if (interfaces::clientstate->delta_tick != -1 && !interfaces::input->m_fCameraInThirdPerson)
			csgo::local_player->animations(true);
	}
}

fake_state_t fake_state;
void animations::manage_local_fake_animstate()
{
	menu::JunkCodeTutorial();

	if (!csgo::local_player || !csgo::local_player->is_alive() || !csgo::cmd)
		return;

	if (fake_state.m_fake_spawntime != csgo::local_player->spawn_time() || fake_state.m_should_update_fake)
	{
		fake_state.init_fake_anim = false;
		fake_state.m_fake_spawntime = csgo::local_player->spawn_time();
		fake_state.m_should_update_fake = false;
	}

	if (!fake_state.init_fake_anim)
	{
		fake_state.m_fake_state = static_cast<anim_state*> (interfaces::mem_alloc->alloc(sizeof(anim_state)));

		if (fake_state.m_fake_state != nullptr)
			csgo::local_player->create_state(fake_state.m_fake_state);

		fake_state.init_fake_anim = true;
	}
	menu::JunkCodeTutorial();

	if (csgo::send_packet)
	{
		std::array<animation_layer, 15> networked_layers;
		std::memcpy(&networked_layers, csgo::local_player->anim_overlays(), sizeof(animation_layer) * 15);
		auto backup_poses = csgo::local_player->pose_param();
		auto backup_abs_angles = csgo::local_player->abs_angles();

		csgo::local_player->update_state(fake_state.m_fake_state, csgo::cmd->viewangles);
		fake_state.m_fake_rotation = reinterpret_cast<anim_state*> (fake_state.m_fake_state)->m_flGoalFeety;
		csgo::local_player->set_abs_angles(vec3_t(0, fake_state.m_fake_rotation, 0));
		csgo::local_player->invalidate_bone_cache();
		fake_state.m_got_fake_matrix = csgo::local_player->setup_bones(fake_state.m_fake_matrix, 128, BONE_USED_BY_ANYTHING & ~BONE_USED_BY_ATTACHMENT, interfaces::globals->cur_time);
		const auto org_tmp = csgo::local_player->renderable()->get_render_origin();
		if (fake_state.m_got_fake_matrix)
		{
			for (auto& i : fake_state.m_fake_matrix)
			{
				i[0][3] -= org_tmp.x;
				i[1][3] -= org_tmp.y;
				i[2][3] -= org_tmp.z;
			}
		}
		csgo::local_player->set_abs_angles(backup_abs_angles);
		std::memcpy(csgo::local_player->anim_overlays(), &networked_layers, sizeof(animation_layer) * 15);
		csgo::local_player->pose_param() = backup_poses;
	}
}