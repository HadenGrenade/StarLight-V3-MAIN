#include "../features.hpp"

/* 
*   Github: @amizu03
*   Discord: ses#1997
*   Site: Sesame.one
*   
*   -- PLEASE DO NOT REMOVE --      // I wouldn't do that to you ses (:
*/

/* NOTE: SORRY FOR THE C-STYLE CODE (FROM MY PROJECT) */

/* goes outside of resolver */
enum resolver_flag_e {
    resolver_flag_none             = 0,
    resolver_flag_has_aa           = ( 1 << 0 ),
    resolver_flag_has_lby          = ( 1 << 1 ),
    resolver_flag_has_micromovents = ( 1 << 2 )
};

enum resolver_side_e {
    resolver_side_middle = 0,
    resolver_side_left,
    resolver_side_right
};

static float resolver_last_simtime [ 65 ]         = { 0.0f };
static float resolver_balance_adjust_lby [ 65 ]   = { 0.0f };
static bool resolver_balance_adjusted_once [ 65 ] = { false };
static bool resolver_small_movement_once [ 65 ]   = { false };
static int resolver_flags [ 65 ]                  = { resolver_flag_none };
static int resolver_sides [ 65 ]                  = { resolver_side_middle };

/* call this when map switches */
void resolver_reset ( ) {
    memset ( resolver_last_simtime, 0, sizeof ( resolver_last_simtime ) );
    memset ( resolver_balance_adjusted_once, 0, sizeof ( resolver_balance_adjusted_once ) );
    memset ( resolver_balance_adjust_lby, 0, sizeof ( resolver_balance_adjust_lby ) );
    memset ( resolver_small_movement_once, 0, sizeof ( resolver_small_movement_once ) );
    memset ( resolver_flags, 0, sizeof ( resolver_flags ) );
    memset ( resolver_sides, 0, sizeof ( resolver_sides ) );
}

/* you can use this for aa indicator for enemies! */
bool resolver_has_antiaim ( player_t* player ) {
    if ( !csgo::local_player || !csgo::local_player->is_alive ( ) || !player || player->dormant ( ) || !player->is_alive ( ) )
        return false;

    int idx = player->index ( );

    return resolver_flags [ idx ] & resolver_flag_has_aa && ( resolver_flags [ idx ] & resolver_flag_has_lby || resolver_flags [ idx ] & resolver_flag_has_micromovents );
}

/* actual resolver ( call this in some player loop EVERY TICK -> CreateMove ) */
bool resolver_resolve_player ( player_t* player ) {
    if ( !csgo::local_player || !csgo::local_player->is_alive ( ) || !player || player->dormant ( ) || !player->is_alive ( ) ) {
        if (player)
            resolver_last_simtime [ player->index ( ) ] = player->simulation_time ( );

        return false;
    }

    int idx = player->index ( );

    animation_layer* animlayers = player->anim_overlays ( );
    anim_state* animstate = player->get_anim_state ( );
   
    if ( !animlayers || !animstate )
        return false;

    if ( time_to_ticks ( player->simulation_time ( ) ) == time_to_ticks ( resolver_last_simtime [ idx ] ) )
        return true;

    resolver_last_simtime [ idx ] = player->simulation_time ( );

    //resolver_last_simtime [ idx ] = player->simulation_time ( );
    // int choke = time_to_ticks ( player->simulation_time ( ) - resolver_last_simtime [ idx ] ) - 1;

    //if ( choke >= 4 )
    //    resolver_flags [ idx ] |= resolver_flag_has_aa;

    bool moving = animlayers [ 6 ].m_flWeight > 0.0f;

    /* aa detection */
    if ( !moving ) {
        /* detect lby desync */
        if ( !animlayers [ 3 ].m_flCycle && !animlayers [ 3 ].m_flWeight && !animlayers [ 6 ].m_flPlaybackRate ) {
            if ( abs ( math::NormalizeAngleFloat ( animstate->m_flEyey - player->lower_body_y ( ) ) ) > 35.0f
                && abs( resolver_balance_adjust_lby [ idx ] = player->lower_body_y ( ) ) < 5.0f ) {
                if ( resolver_balance_adjusted_once [ idx ] )
                    resolver_flags [ idx ] |= resolver_flag_has_aa | resolver_flag_has_lby;

                resolver_balance_adjusted_once [ idx ] = true;
            }
            else {
                resolver_balance_adjusted_once [ idx ] = false;
            }

            resolver_balance_adjust_lby [ idx ] = player->lower_body_y ( );
        }

        /* detect micromovements */
        if ( animlayers [ 6 ].m_flPlaybackRate > 0.0f && animlayers [ 6 ].m_flPlaybackRate < 0.05f ) {
            if ( resolver_small_movement_once [ idx ] )
                resolver_flags [ idx ] |= resolver_flag_has_aa | resolver_flag_has_micromovents;

            resolver_small_movement_once [ idx ] = true;
        }
        else {
            resolver_small_movement_once [ idx ] = false;
        }
    }
    else {
        resolver_balance_adjusted_once [ idx ] = resolver_small_movement_once [ idx ] = false;
        resolver_balance_adjust_lby [ idx ] = FLT_MAX;
    }

    /* resolve */
    if ( resolver_flags [ idx ] & resolver_flag_has_aa ) {
        float delta_yaw = math::NormalizeAngleFloat ( player->lower_body_y() - animstate->m_flEyey );

        if ( resolver_flags [ idx ] & resolver_flag_has_lby ) {
            /* push feet yaw towards opposite side */
            if ( fabsf ( delta_yaw ) > 50.0f )
                resolver_sides [ idx ] = ( delta_yaw <= 0.0f ) ? resolver_side_right : resolver_side_left;
        }
        else if ( resolver_flags [ idx ] & resolver_flag_has_micromovents ) {
            /* push feet yaw towards edge side */
            vec3_t my_eyes = csgo::local_player->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f );
            vec3_t their_eyes = player->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f );

            vec3_t fwd_vec = ( their_eyes - my_eyes ).normalized ( );
            vec3_t right_vec = fwd_vec.cross ( vec3_t ( 0.0f, 0.0f, 1.0f ) );
            vec3_t left_vec = -1.0f * right_vec /* for some reason -right_vec doesnt work on this garbage sdk */;

            bool left_visible = false, right_visible = false;

            /* trace rays towards their right and left sides */
            trace_t      tr;
            ray_t        ray;
            trace_filter filter;

            filter.skip = csgo::local_player;

            memset ( &tr, 0, sizeof ( tr ) );
            memset ( &ray, 0, sizeof ( ray ) );

            ray.initialize ( my_eyes + left_vec * 35.0f, their_eyes + left_vec * 35.0f );
            interfaces::trace_ray->trace_ray ( ray, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &tr );
            left_visible = tr.entity == player || tr.flFraction > 0.97f;

            memset ( &tr, 0, sizeof ( tr ) );
            memset ( &ray, 0, sizeof ( ray ) );

            ray.initialize ( my_eyes + right_vec * 35.0f, their_eyes + right_vec * 35.0f );
            interfaces::trace_ray->trace_ray ( ray, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &tr );
            right_visible = tr.entity == player || tr.flFraction > 0.97f;

            /* check which sides are visible */
            if ( left_visible && !right_visible )
                resolver_sides [ idx ] = resolver_side_right;
            else if ( !left_visible && right_visible )
                resolver_sides [ idx ] = resolver_side_left;
        }
    }

    /* done */
    return true;
}

/* apply resolver angles ( call this in some player loop EVERY FRAME --> FrameStageNotify FRAME_RENDER_START ) */
bool resolver_apply_angles ( player_t* player ) {
    if ( !csgo::local_player || !csgo::local_player->is_alive ( ) || !player || player->dormant ( ) || !player->is_alive ( ) )
        return false;

    int idx = player->index ( );

    animation_layer* animlayers = player->anim_overlays ( );
    anim_state* animstate = player->get_anim_state ( );

    if ( !animlayers || !animstate )
        return false;

    if ( resolver_flags [ idx ] & resolver_flag_has_aa && resolver_sides [ idx ] != resolver_side_middle ) {
        animstate->m_flGoalFeety = animstate->m_flCurrentFeety = math::NormalizeAngleFloat( animstate->m_flEyey + ( ( resolver_sides [ idx ] == resolver_side_right ) ? 60.0f : -60.0f ) );
        return true;
    }

    return false;
}