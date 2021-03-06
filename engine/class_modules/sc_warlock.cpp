// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
//
// TODO:
// Proper Stats calc for childs of cataclysm
// ==========================================================================
namespace { // unnamed namespace

// ==========================================================================
// Warlock
// ==========================================================================

static const int META_FURY_MINIMUM = 40;

struct warlock_t;

namespace pets {
struct wild_imp_pet_t;
struct t18_illidari_satyr_t;
struct t18_prince_malchezaar_t;
struct t18_vicious_hellhound_t;
}

struct warlock_td_t: public actor_target_data_t
{
  dot_t*  dots_agony;
  dot_t*  dots_corruption;
  dot_t*  dots_doom;
  dot_t*  dots_drain_soul;
  dot_t*  dots_haunt;
  dot_t*  dots_immolate;
  dot_t*  dots_seed_of_corruption;
  dot_t*  dots_shadowflame;
  dot_t*  dots_soulburn_seed_of_corruption;
  dot_t*  dots_unstable_affliction;

  buff_t* debuffs_haunt;
  buff_t* debuffs_shadowflame;
  buff_t* debuffs_agony;
  buff_t* debuffs_flamelicked;

  bool ds_started_below_20;
  int agony_stack;
  double soc_trigger, soulburn_soc_trigger;

  warlock_t& warlock;
  warlock_td_t( player_t* target, warlock_t& p );

  void reset()
  {
    ds_started_below_20 = false;
    agony_stack = 1;
    soc_trigger = 0;
    soulburn_soc_trigger = 0;
  }

  void target_demise();
};

struct warlock_t: public player_t
{
public:
  player_t* havoc_target;
  player_t* latest_corruption_target;
  int double_nightfall;


  // Active Pet
  struct pets_t
  {
    pet_t* active;
    pet_t* last;
    static const int WILD_IMP_LIMIT = 25;
    static const int T18_PET_LIMIT = 6 ;
    std::array<pets::wild_imp_pet_t*, WILD_IMP_LIMIT> wild_imps;
    pet_t* inner_demon;
    std::array<pets::t18_illidari_satyr_t*, T18_PET_LIMIT> t18_illidari_satyr;
    std::array<pets::t18_prince_malchezaar_t*, T18_PET_LIMIT> t18_prince_malchezaar;
    std::array<pets::t18_vicious_hellhound_t*, T18_PET_LIMIT> t18_vicious_hellhound;
  } pets;

  std::vector<std::string> pet_name_list;

  // Talents
  struct talents_t
  {
    const spell_data_t* dark_regeneration;
    const spell_data_t* soul_leech;
    const spell_data_t* harvest_life;

    const spell_data_t* howl_of_terror;
    const spell_data_t* mortal_coil;
    const spell_data_t* shadowfury;

    const spell_data_t* soul_link;
    const spell_data_t* sacrificial_pact;
    const spell_data_t* dark_bargain;

    const spell_data_t* blood_horror;
    const spell_data_t* burning_rush;
    const spell_data_t* unbound_will;

    const spell_data_t* grimoire_of_supremacy;
    const spell_data_t* grimoire_of_service;
    const spell_data_t* grimoire_of_sacrifice;
    const spell_data_t* grimoire_of_synergy;

    const spell_data_t* archimondes_darkness;
    const spell_data_t* kiljaedens_cunning;
    const spell_data_t* mannoroths_fury;

    const spell_data_t* soulburn_haunt; //Affliction only
    const spell_data_t* demonbolt; // Demonology only
    const spell_data_t* charred_remains; // Destruction only
    const spell_data_t* cataclysm;
    const spell_data_t* demonic_servitude;
  } talents;

  // Glyphs
  struct glyphs_t
  {
    // Major Glyphs
    // All Specs
    const spell_data_t* curses;
    const spell_data_t* dark_soul;
    const spell_data_t* demon_training;
    const spell_data_t* demonic_circle;
    const spell_data_t* drain_life;
    const spell_data_t* eternal_resolve;
    const spell_data_t* fear;
    const spell_data_t* healthstone;
    const spell_data_t* life_pact;
    const spell_data_t* life_tap;
    const spell_data_t* siphon_life;
    const spell_data_t* soul_consumption;
    const spell_data_t* soul_swap;
    const spell_data_t* soulstone;
    const spell_data_t* strengthened_resolve;
    const spell_data_t* twilight_ward;
    const spell_data_t* unending_resolve;
    // Affliction only
    const spell_data_t* curse_of_exhaustion;
    const spell_data_t* unstable_affliction;
    // Demonology only
    const spell_data_t* demon_hunting;
    const spell_data_t* imp_swarm;
    const spell_data_t* shadowflame;
    // Destruction only
    const spell_data_t* conflagrate;
    const spell_data_t* ember_tap;
    const spell_data_t* flames_of_xoroth;
    const spell_data_t* havoc;
    const spell_data_t* supernova;

    // Minor Glyphs
    // All Specs
    const spell_data_t* crimson_banish;
    const spell_data_t* enslave_demon;
    const spell_data_t* eye_of_kilrogg;
    const spell_data_t* gateway_attunement;
    const spell_data_t* health_funnel;
    const spell_data_t* nightmares;
    const spell_data_t* soulwell;
    const spell_data_t* unending_breath;
    //Affliction and Destruction
    const spell_data_t* verdant_spheres;
    // Affliction only
    const spell_data_t* subtlety;
    // Demonology only
    const spell_data_t* carrion_swarm;
    const spell_data_t* falling_meteor;
    const spell_data_t* felguard;
    const spell_data_t* hand_of_guldan;
    const spell_data_t* metamorphosis;
    const spell_data_t* shadow_bolt;
  } glyphs;

  // Mastery Spells
  struct mastery_spells_t
  {
    const spell_data_t* potent_afflictions;
    const spell_data_t* master_demonologist;
    const spell_data_t* emberstorm;
  } mastery_spells;

  //Procs and RNG
  real_ppm_t grimoire_of_synergy; //caster ppm, i.e., if it procs, the wl will create a buff for the pet.
  real_ppm_t grimoire_of_synergy_pet; //pet ppm, i.e., if it procs, the pet will create a buff for the wl.
  real_ppm_t rppm_chaotic_infusion;

  // Perks
  struct
  {
    // Affliction and Demonology
    const spell_data_t* empowered_drain_life;
    // Affliction only
    const spell_data_t* enhanced_haunt;
    const spell_data_t* empowered_corruption;
    const spell_data_t* improved_drain_soul;
    // Demonology only
    const spell_data_t* empowered_demons;
    const spell_data_t* empowered_doom;
    const spell_data_t* enhanced_corruption;
    // Destruction only
    const spell_data_t* enhanced_havoc; // code this
    const spell_data_t* empowered_immolate;
    const spell_data_t* enhanced_chaos_bolt;
    const spell_data_t* improved_ember_tap; // ember tap doesn't even exist in the core.
  } perk;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* demonic_calling;
    cooldown_t* infernal;
    cooldown_t* doomguard;
    cooldown_t* imp_swarm;
    cooldown_t* hand_of_guldan;
    cooldown_t* dark_soul;
    cooldown_t* t17_2pc_demonology;
  } cooldowns;

  // Passives
  struct specs_t
  {
    // All Specs
    const spell_data_t* dark_soul;
    const spell_data_t* fel_armor;
    const spell_data_t* nethermancy;

    // Affliction only
    const spell_data_t* eradication;
    const spell_data_t* improved_fear;
    const spell_data_t* nightfall;
    const spell_data_t* readyness_affliction;

    // Demonology only
    const spell_data_t* improved_demons;
    const spell_data_t* chaos_wave;
    const spell_data_t* decimation;
    const spell_data_t* demonic_fury;
    const spell_data_t* demonic_rebirth;
    const spell_data_t* demonic_tactics;
    const spell_data_t* doom;
    const spell_data_t* immolation_aura;
    const spell_data_t* imp_swarm;
    const spell_data_t* metamorphosis;
    const spell_data_t* touch_of_chaos;
    const spell_data_t* molten_core;
    const spell_data_t* readyness_demonology;
    const spell_data_t* wild_imps;

    // Destruction only
    const spell_data_t* aftermath;
    const spell_data_t* backdraft;
    const spell_data_t* backlash;
    const spell_data_t* devastation;
    const spell_data_t* immolate;
    const spell_data_t* burning_embers;
    const spell_data_t* chaotic_energy;
    const spell_data_t* fire_and_brimstone;
    const spell_data_t* readyness_destruction;

  } spec;

  // Buffs
  struct buffs_t
  {
    buff_t* backdraft;
    buff_t* dark_soul;
    buff_t* demonbolt;
    buff_t* demonic_calling;
    buff_t* demonic_rebirth;
    buff_t* demonic_synergy;
    buff_t* fire_and_brimstone;
    buff_t* immolation_aura;
    buff_t* grimoire_of_sacrifice;
    buff_t* haunting_spirits;
    buff_t* havoc;
    buff_t* mannoroths_fury;
    buff_t* metamorphosis;
    buff_t* molten_core;
    buff_t* soul_swap;
    buff_t* soulburn;
    buff_t* kiljaedens_cunning;

    buff_t* chaotic_infusion;

    buff_t* tier16_4pc_ember_fillup;
    buff_t* tier16_2pc_destructive_influence;
    buff_t* tier16_2pc_empowered_grasp;
    buff_t* tier16_2pc_fiery_wrath;
    buff_t* tier18_2pc_demonology;
  } buffs;

  // Gains
  struct gains_t
  {
    gain_t* life_tap;
    gain_t* soul_leech;
    gain_t* nightfall;
    gain_t* incinerate;
    gain_t* incinerate_fnb;
    gain_t* incinerate_t15_4pc;
    gain_t* conflagrate;
    gain_t* conflagrate_fnb;
    gain_t* rain_of_fire;
    gain_t* immolate;
    gain_t* immolate_fnb;
    gain_t* immolate_t17_2pc;
    gain_t* shadowburn_ember;
    gain_t* miss_refund;
    gain_t* siphon_life;
    gain_t* seed_of_corruption;
    gain_t* haunt_tier16_4pc;
    gain_t* shard_target_death;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* wild_imp;
    proc_t* t17_2pc_demo;
    proc_t* havoc_waste;
    proc_t* fragment_wild_imp;
    proc_t* t18_4pc_destruction;
    proc_t* t18_illidari_satyr;
    proc_t* t18_vicious_hellhound;
    proc_t* t18_prince_malchezaar;
  } procs;

  struct spells_t
  {
    spell_t* seed_of_corruption_aoe;
    spell_t* soulburn_seed_of_corruption_aoe;
    spell_t* metamorphosis;
    spell_t* melee;
    spell_t* immolation_aura;

    const spell_data_t* tier15_2pc;
  } spells;

  struct soul_swap_buffer_t
  {
    player_t* source;//where inhaled from
    //the 6.0 dot mechanics make it close to irrelevant how the dots were buffed when inhaling, as they are dynamically recalculated every tick.
    //thus we only store the whether the dot is up and its duration( and agony stacks).

    bool agony_was_inhaled;
    timespan_t agony_remains;
    int agony_stack;

    bool corruption_was_inhaled;
    timespan_t corruption_remains;

    bool unstable_affliction_was_inhaled;
    timespan_t unstable_affliction_remains;

    bool seed_of_corruption_was_inhaled;
    timespan_t seed_of_corruption_remains;

  } soul_swap_buffer;

  struct demonic_calling_event_t: player_event_t
  {
    bool initiator;

    demonic_calling_event_t( player_t* p, timespan_t delay, bool init = false ):
      player_event_t( *p ), initiator( init )
    {
      add_event( delay );
    }
    virtual const char* name() const override
    { return  "demonic_calling"; }
    virtual void execute() override
    {
      warlock_t* p = static_cast<warlock_t*>( player() );
      p -> demonic_calling_event = new ( sim() ) demonic_calling_event_t( p,
                                                                          timespan_t::from_seconds( ( p -> spec.wild_imps -> effectN( 1 ).period().total_seconds() + p -> spec.imp_swarm -> effectN( 3 ).base_value() ) * p -> cache.spell_speed() ) );
      if ( ! initiator ) p -> buffs.demonic_calling -> trigger();
    }
  };

  event_t* demonic_calling_event;

  int initial_burning_embers, initial_demonic_fury;
  std::string default_pet;

  timespan_t ember_react, shard_react;

    // Tier 18 (WoD 6.2) trinket effects
  const special_effect_t* affliction_trinket;
  const special_effect_t* demonology_trinket;
  const special_effect_t* destruction_trinket;

  warlock_t( sim_t* sim, const std::string& name, race_e r = RACE_UNDEAD );

  // Character Definition
  virtual void      init_spells() override;
  virtual void      init_base_stats() override;
  virtual void      init_scaling() override;
  virtual void      create_buffs() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init_rng() override;
  virtual void      init_action_list() override;
  virtual void      init_resources( bool force ) override;
  virtual void      reset() override;
  virtual void      create_options() override;
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() ) override;
  virtual void      create_pets() override;
  virtual std::string      create_profile( save_e = SAVE_ALL ) override;
  virtual void      copy_from( player_t* source ) override;
  virtual resource_e primary_resource() const override { return RESOURCE_MANA; }
  virtual role_e    primary_role() const override     { return ROLE_SPELL; }
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    composite_rating_multiplier( rating_e rating ) const override;
  virtual void      invalidate_cache( cache_e ) override;
  virtual double    composite_spell_crit() const override;
  virtual double    composite_spell_haste() const override;
  virtual double    composite_melee_crit() const override;
  virtual double    composite_mastery() const override;
  virtual double    resource_gain( resource_e, double, gain_t* = nullptr, action_t* = nullptr ) override;
  virtual double    mana_regen_per_second() const override;
  virtual double    composite_armor() const override;

  virtual void      halt() override;
  virtual void      combat_begin() override;
  virtual expr_t*   create_expression( action_t* a, const std::string& name_str ) override;

  double emberstorm_e3_from_e1() const
  {
    return mastery_spells.emberstorm -> effectN( 3 ).sp_coeff() / mastery_spells.emberstorm -> effectN( 1 ).sp_coeff();
  }

  target_specific_t<warlock_td_t> target_data;

  virtual warlock_td_t* get_target_data( player_t* target ) const override
  {
    warlock_td_t*& td = target_data[target];
    if ( ! td )
    {
      td = new warlock_td_t( target, const_cast<warlock_t&>( *this ) );
    }
    return td;
  }

  void trigger_demonology_t17_2pc( const action_state_t* state ) const;
  void trigger_demonology_t17_2pc_cast() const;
private:
  void apl_precombat();
  void apl_default();
  void apl_affliction();
  void apl_demonology();
  void apl_destruction();
  void apl_global_filler();
};

static void do_trinket_init(  warlock_t*               player,
                              specialization_e         spec,
                              const special_effect_t*& ptr,
                              const special_effect_t&  effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from working on live
  // Simulationcraft. Also ensure correct specialization.
  if ( !player -> find_spell( effect.spell_id ) -> ok() ||
    player -> specialization() != spec )
  {
    return;
  }
  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}

static void affliction_trinket( special_effect_t& effect )
{
  warlock_t* warlock = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( warlock, WARLOCK_AFFLICTION, warlock -> affliction_trinket, effect );
}

static void demonology_trinket( special_effect_t& effect )
{
  warlock_t* warlock = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( warlock, WARLOCK_DEMONOLOGY, warlock -> demonology_trinket, effect );
}

static void destruction_trinket( special_effect_t& effect )
{
  warlock_t* warlock = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( warlock, WARLOCK_DESTRUCTION, warlock -> destruction_trinket, effect);
}

void parse_spell_coefficient( action_t& a )
{
  for ( size_t i = 1; i <= a.data()._effects -> size(); i++ )
  {
    if ( a.data().effectN( i ).type() == E_SCHOOL_DAMAGE )
      a.spell_power_mod.direct = a.data().effectN( i ).sp_coeff();
    else if ( a.data().effectN( i ).type() == E_APPLY_AURA && a.data().effectN( i ).subtype() == A_PERIODIC_DAMAGE )
      a.spell_power_mod.tick = a.data().effectN( i ).sp_coeff();
  }
}

// Pets
namespace pets {

  struct warlock_pet_t: public pet_t
  {
    gain_t* owner_fury_gain;
    action_t* special_action;
    action_t* special_action_two;
    melee_attack_t* melee_attack;
    stats_t* summon_stats;
    const spell_data_t* supremacy;
    const spell_data_t* command;

    warlock_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_e pt, bool guardian = false );
    virtual void init_base_stats() override;
    virtual void init_action_list() override;
    virtual void create_buffs() override;
    virtual void schedule_ready( timespan_t delta_time = timespan_t::zero(),
      bool   waiting = false ) override;
    virtual double composite_player_multiplier( school_e school ) const override;
    virtual double composite_melee_crit() const override;
    virtual double composite_spell_crit() const override;
    virtual double composite_melee_haste() const override;
    virtual double composite_spell_haste() const override;
    virtual double composite_melee_speed() const override;
    virtual double composite_spell_speed() const override;
    virtual resource_e primary_resource() const override { return RESOURCE_ENERGY; }
    warlock_t* o()
    {
      return static_cast<warlock_t*>( owner );
    }
    const warlock_t* o() const
    {
      return static_cast<warlock_t*>( owner );
    }

    struct buffs_t
    {
      buff_t* demonic_synergy;
    } buffs;

    struct travel_t: public action_t
    {
      travel_t( player_t* player ): action_t( ACTION_OTHER, "travel", player ) {}
      void execute() override { player -> current.distance = 1; }
      timespan_t execute_time() const override { return timespan_t::from_seconds( player -> current.distance / 10.0 ); }
      bool ready() override { return ( player -> current.distance > 1 ); }
      bool usable_moving() const override { return true; }
    };

    action_t* create_action( const std::string& name,
      const std::string& options_str ) override
    {
      if ( name == "travel" ) return new travel_t( this );

      return pet_t::create_action( name, options_str );
    }
  };

namespace actions {

// Template for common warlock pet action code. See priest_action_t.
template <class ACTION_BASE>
struct warlock_pet_action_t: public ACTION_BASE
{
public:
private:
  typedef ACTION_BASE ab; // action base, eg. spell_t
public:
  typedef warlock_pet_action_t base_t;

  double generate_fury;

  warlock_pet_action_t( const std::string& n, warlock_pet_t* p,
                        const spell_data_t* s = spell_data_t::nil() ):
                        ab( n, p, s ),
                        generate_fury( get_fury_gain( ab::data() ) )
  {
    ab::may_crit = true;
  }
  virtual ~warlock_pet_action_t() {}

  warlock_pet_t* p()
  {
    return static_cast<warlock_pet_t*>( ab::player );
  }
  const warlock_pet_t* p() const
  {
    return static_cast<warlock_pet_t*>( ab::player );
  }

  virtual void execute()
  {
    ab::execute();

    if ( ab::result_is_hit( ab::execute_state -> result ) && p() -> o() -> specialization() == WARLOCK_DEMONOLOGY && generate_fury > 0 )
      p() -> o() -> resource_gain( RESOURCE_DEMONIC_FURY, generate_fury, p() -> owner_fury_gain );

    if ( ab::result_is_hit( ab::execute_state -> result ) && p() -> o() -> talents.grimoire_of_synergy -> ok() )
    {
      bool procced = p() -> o() -> grimoire_of_synergy_pet.trigger(); //check for RPPM
      if ( procced ) p() -> o() -> buffs.demonic_synergy -> trigger(); //trigger the buff
    }
  }

  double get_fury_gain( const spell_data_t& data )
  {
    if ( data._effects -> size() >= 3 && data.effectN( 3 ).trigger_spell_id() == 104330 )
    {
      if ( p() -> o() -> talents.grimoire_of_supremacy -> ok() && p() -> pet_type != PET_WILD_IMP )
      {
        return  std::floor( data.effectN( 3 ).base_value() * ( 1 + p() -> o() -> talents.grimoire_of_supremacy -> effectN( 1 ).percent()) );
      }
      else 
      {
        return  data.effectN( 3 ).base_value();
      }
    }
    else
      return 0.0;
  }
};

struct warlock_pet_melee_t: public melee_attack_t
{
  struct off_hand_swing: public melee_attack_t
  {
    off_hand_swing( warlock_pet_t* p, const char* name = "melee_oh" ):
      melee_attack_t( name, p, spell_data_t::nil() )
    {
      school = SCHOOL_PHYSICAL;
      weapon = &( p -> off_hand_weapon );
      base_execute_time = weapon -> swing_time;
      may_crit = true;
      background = true;
      base_multiplier = 0.5;
    }
  };

  off_hand_swing* oh;

  warlock_pet_melee_t( warlock_pet_t* p, const char* name = "melee" ):
    melee_attack_t( name, p, spell_data_t::nil() ), oh( nullptr )
  {
    school = SCHOOL_PHYSICAL;
    weapon = &( p -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    may_crit = background = repeating = true;

    if ( p -> dual_wield() )
      oh = new off_hand_swing( p );
  }

  virtual void execute() override
  {
    if ( ! player -> executing && ! player -> channeling )
    {
      melee_attack_t::execute();
      if ( oh )
      {
        oh -> time_to_execute = time_to_execute;
        oh -> execute();
      }
    }
    else
    {
      schedule_execute();
    }
  }
};

struct warlock_pet_melee_attack_t: public warlock_pet_action_t < melee_attack_t >
{
private:
  void _init_warlock_pet_melee_attack_t()
  {
    weapon = &( player -> main_hand_weapon );
    special = true;
  }

public:
  warlock_pet_melee_attack_t( warlock_pet_t* p, const std::string& n ):
    base_t( n, p, p -> find_pet_spell( n ) )
  {
    _init_warlock_pet_melee_attack_t();
  }

  warlock_pet_melee_attack_t( const std::string& token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil() ):
    base_t( token, p, s )
  {
    _init_warlock_pet_melee_attack_t();
  }
};

struct warlock_pet_spell_t: public warlock_pet_action_t < spell_t >
{
private:
  void _init_warlock_pet_spell_t()
  {
    parse_spell_coefficient( *this );
  }

public:
  warlock_pet_spell_t( warlock_pet_t* p, const std::string& n ):
    base_t( n, p, p -> find_pet_spell( n ) )
  {
    _init_warlock_pet_spell_t();
  }

  warlock_pet_spell_t( const std::string& token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil() ):
    base_t( token, p, s )
  {
    _init_warlock_pet_spell_t();
  }
};

struct firebolt_t: public warlock_pet_spell_t
{
  firebolt_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Firebolt" )
  {
    if ( p -> owner -> bugs )
      min_gcd = timespan_t::from_seconds( 1.5 );
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = warlock_pet_spell_t::execute_time();

    if ( p() -> o() -> glyphs.demon_training -> ok() )
    {
      t *= 0.5;
    }

    return t;
  }
};

struct legion_strike_t: public warlock_pet_melee_attack_t
{
  legion_strike_t( warlock_pet_t* p ):
    warlock_pet_melee_attack_t( p, "Legion Strike" )
  {
    aoe = -1;
    split_aoe_damage = true;
    weapon           = &( p -> main_hand_weapon );
  }

  virtual bool ready() override
  {
    if ( p() -> special_action -> get_dot() -> is_ticking() ) return false;

    return warlock_pet_melee_attack_t::ready();
  }
};

struct felstorm_tick_t: public warlock_pet_melee_attack_t
{
  felstorm_tick_t( warlock_pet_t* p, const spell_data_t& s ):
    warlock_pet_melee_attack_t( "felstorm_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe = -1;
    background = true;
    weapon = &( p -> main_hand_weapon );
  }
};

struct felstorm_t: public warlock_pet_melee_attack_t
{
  felstorm_t( warlock_pet_t* p ):
    warlock_pet_melee_attack_t( "felstorm", p, p -> find_spell( 89751 ) )
  {
    tick_zero = true;
    hasted_ticks = false;
    may_miss = false;
    may_crit = false;
    weapon_multiplier = 0;

    dynamic_tick_action = true;
    tick_action = new felstorm_tick_t( p, data() );
  }

  virtual void cancel() override
  {
    warlock_pet_melee_attack_t::cancel();

    get_dot() -> cancel();
  }

  virtual void execute() override
  {
    warlock_pet_melee_attack_t::execute();

    p() -> melee_attack -> cancel();
  }

  virtual void last_tick( dot_t* d ) override
  {
    warlock_pet_melee_attack_t::last_tick( d );

    if ( ! p() -> is_sleeping() && ! p() -> melee_attack -> target -> is_sleeping() )
      p() -> melee_attack -> execute();
  }
};

struct shadow_bite_t: public warlock_pet_spell_t
{
  shadow_bite_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Shadow Bite" )
  { }
};

struct lash_of_pain_t: public warlock_pet_spell_t
{
  lash_of_pain_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Lash of Pain" )
  {
    if ( p -> owner -> bugs ) min_gcd = timespan_t::from_seconds( 1.5 );
  }
};

struct whiplash_t: public warlock_pet_spell_t
{
  whiplash_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Whiplash" )
  {
    aoe = -1;
  }
};

struct torment_t: public warlock_pet_spell_t
{
  torment_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Torment" )
  { }
};

struct felbolt_t: public warlock_pet_spell_t
{
  felbolt_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Felbolt" )
  {
    if ( p -> owner -> bugs )
      min_gcd = timespan_t::from_seconds( 1.5 );
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = warlock_pet_spell_t::execute_time();

    if ( p() -> o() -> glyphs.demon_training -> ok() ) t *= 0.5;

    return t;
  }
};

struct mortal_cleave_t: public warlock_pet_melee_attack_t
{
  mortal_cleave_t( warlock_pet_t* p ):
    warlock_pet_melee_attack_t( "mortal_cleave", p, p -> find_spell( 115625 ))
  {
    aoe = -1;
    split_aoe_damage = true;
    weapon = &( p -> main_hand_weapon );
  }

  virtual bool ready() override
  {
    if ( p() -> special_action -> get_dot() -> is_ticking() ) return false;

    return warlock_pet_melee_attack_t::ready();
  }
};

struct wrathstorm_tick_t: public warlock_pet_melee_attack_t
{
  wrathstorm_tick_t( warlock_pet_t* p, const spell_data_t& s ):
    warlock_pet_melee_attack_t( "wrathstorm_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe         = -1;
    background  = true;
    weapon = &( p -> main_hand_weapon );
  }
};

struct wrathstorm_t: public warlock_pet_melee_attack_t
{
  wrathstorm_t( warlock_pet_t* p ):
    warlock_pet_melee_attack_t( "wrathstorm", p, p -> find_spell( 115831 ) )
  {
    tick_zero = true;
    hasted_ticks = false;
    may_miss = false;
    may_crit = false;
    weapon_multiplier = 0;

    dynamic_tick_action = true;
    tick_action = new wrathstorm_tick_t( p, data() );
  }

  virtual void cancel() override
  {
    warlock_pet_melee_attack_t::cancel();

    get_dot() -> cancel();
  }

  virtual void execute() override
  {
    warlock_pet_melee_attack_t::execute();

    p() -> melee_attack -> cancel();
  }

  virtual void last_tick( dot_t* d ) override
  {
    warlock_pet_melee_attack_t::last_tick( d );

    if ( ! p() -> is_sleeping() ) p() -> melee_attack -> execute();
  }
};

struct tongue_lash_t: public warlock_pet_spell_t
{
  tongue_lash_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Tongue Lash" )
  { }
};


struct bladedance_t: public warlock_pet_spell_t
{
  bladedance_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Bladedance" )
  {
    if ( p -> owner -> bugs ) min_gcd = timespan_t::from_seconds( 1.5 );
  }
};

struct fellash_t: public warlock_pet_spell_t
{
  fellash_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Fellash" )
  {
    aoe = -1;
  }
};

struct immolation_tick_t: public warlock_pet_spell_t
{
  immolation_tick_t( warlock_pet_t* p, const spell_data_t& s ):
    warlock_pet_spell_t( "immolation_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe = -1;
    background = true;
    may_crit = true;
    may_multistrike = true;
  }
};

struct immolation_t: public warlock_pet_spell_t
{
  immolation_t( warlock_pet_t* p, const std::string& options_str ):
    warlock_pet_spell_t( "immolation", p, p -> find_spell( 19483 ) )
  {
    parse_options( options_str );

    dynamic_tick_action = hasted_ticks = true;
    tick_action = new immolation_tick_t( p, data() );
  }

  void init() override
  {
    warlock_pet_spell_t::init();

    // Explicitly snapshot haste, as the spell actually has no duration in spell data
    snapshot_flags |= STATE_HASTE;
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    return player -> sim -> expected_iteration_time * 2;
  }

  virtual void cancel() override
  {
    dot_t* dot = find_dot( target );
    if ( dot && dot -> is_ticking() )
    {
      dot -> cancel();
    }
    action_t::cancel();
  }
};

struct doom_bolt_t: public warlock_pet_spell_t
{
  doom_bolt_t( warlock_pet_t* p ):
    warlock_pet_spell_t( "Doom Bolt", p, p -> find_spell( 85692 ) )
  {
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_pet_spell_t::composite_target_multiplier( target );

    if ( target -> health_percentage() < 20 )
    {
      m *= 1.0 + data().effectN( 2 ).percent();
    }
    return m;
  }
};

struct meteor_strike_t: public warlock_pet_spell_t
{
  meteor_strike_t( warlock_pet_t* p, const std::string& options_str ):
    warlock_pet_spell_t( "Meteor Strike", p, p -> find_spell( 171018 ) )
  {
    parse_options( options_str );
    aoe = -1;
  }
};

struct wild_firebolt_t: public warlock_pet_spell_t
{
  wild_firebolt_t( warlock_pet_t* p ):
    warlock_pet_spell_t( "fel_firebolt", p, p -> find_spell( 104318 ) )
  {
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_pet_spell_t::impact( s );

    if ( result_is_hit( s -> result )
         && p() -> o() -> spec.molten_core -> ok()
         && rng().roll( 0.08 ) )
         p() -> o() -> buffs.molten_core -> trigger();
  }

  virtual void execute() override
  {
    warlock_pet_spell_t::execute();

    if ( player -> resources.current[RESOURCE_ENERGY] <= 0 )
    {
      p() -> dismiss();
      return;
    }
  }

  virtual bool ready() override
  {
    return spell_t::ready();
  }
};

} // pets::actions

warlock_pet_t::warlock_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_e pt, bool guardian ):
pet_t( sim, owner, pet_name, pt, guardian ), special_action( nullptr ), special_action_two( nullptr ), melee_attack( nullptr ), summon_stats( nullptr )
{
  owner_fury_gain = owner -> get_gain( pet_name );
  owner_coeff.ap_from_sp = 1.0;
  owner_coeff.sp_from_sp = 1.0;
  supremacy = find_spell( 115578 );
  command = find_spell( 21563 );
}

void warlock_pet_t::init_base_stats()
{
  pet_t::init_base_stats();

  resources.base[RESOURCE_ENERGY] = 200;
  base_energy_regen_per_second = 10;

  base.spell_power_per_intellect = 1;

  intellect_per_owner = 0;
  stamina_per_owner = 0;

  main_hand_weapon.type = WEAPON_BEAST;

  //double dmg = dbc.spell_scaling( owner -> type, owner -> level );

  main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
}

void warlock_pet_t::init_action_list()
{
  if ( special_action )
  {
    if ( type == PLAYER_PET )
      special_action -> background = true;
    else
      special_action -> action_list = get_action_priority_list( "default" );
  }

  if ( special_action_two )
  {
    if ( type == PLAYER_PET )
      special_action_two -> background = true;
    else
      special_action_two -> action_list = get_action_priority_list( "default" );
  }

  pet_t::init_action_list();

  if ( summon_stats )
    for ( size_t i = 0; i < action_list.size(); ++i )
      summon_stats -> add_child( action_list[i] -> stats );
}

void warlock_pet_t::create_buffs()
{
  pet_t::create_buffs();

  buffs.demonic_synergy = buff_creator_t( this, "demonic_synergy", find_spell( 171982 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .chance( 1 );
}

void warlock_pet_t::schedule_ready( timespan_t delta_time, bool waiting )
{
  dot_t* d;
  if ( melee_attack && ! melee_attack -> execute_event && ! ( special_action && ( d = special_action -> get_dot() ) && d -> is_ticking() ) )
  {
    melee_attack -> schedule_execute();
  }

  pet_t::schedule_ready( delta_time, waiting );
}

double warlock_pet_t::composite_player_multiplier( school_e school ) const
{
  double m = pet_t::composite_player_multiplier( school );

  if ( o() -> race == RACE_ORC )
    m *= 1.0 + command -> effectN( 1 ).percent();

  if ( o() -> mastery_spells.master_demonologist -> ok() )
  {
    double mastery = o() -> cache.mastery();
    m *= 1.0 + mastery * o() -> mastery_spells.master_demonologist -> effectN( 1 ).mastery_value();
  }

  m *= 1.0 + o() -> buffs.tier18_2pc_demonology -> stack_value();

  if ( o() -> talents.grimoire_of_supremacy -> ok() && pet_type != PET_WILD_IMP )
    m *= 1.0 + supremacy -> effectN( 1 ).percent(); // The relevant effect is not attatched to the talent spell, weirdly enough

  if ( o() -> buffs.tier16_2pc_fiery_wrath -> up() )
    m *= 1.0 + o() -> buffs.tier16_2pc_fiery_wrath -> value();

  if ( buffs.demonic_synergy -> up() )
    m *= 1.0 + buffs.demonic_synergy -> data().effectN( 1 ).percent();

  m *= 1.0 + o() -> spec.improved_demons -> effectN( 1 ).percent();
  return m;
}

double warlock_pet_t::composite_melee_crit() const
{
  double mc = pet_t::composite_melee_crit();

  if ( pet_type != PET_WILD_IMP )
    mc += o() -> perk.empowered_demons -> effectN( 1 ).percent();

  return mc;
}

double warlock_pet_t::composite_spell_crit() const
{
  double sc = pet_t::composite_spell_crit();

  if ( pet_type != PET_WILD_IMP )
    sc += o() -> perk.empowered_demons -> effectN( 1 ).percent();

  return sc;
}

double warlock_pet_t::composite_melee_haste() const
{
  double mh = pet_t::composite_melee_haste();

  if ( pet_type != PET_WILD_IMP )
    mh /= 1.0 + o() -> perk.empowered_demons -> effectN( 2 ).percent();

  return mh;
}

double warlock_pet_t::composite_spell_haste() const
{
  double sh = pet_t::composite_spell_haste();

  if ( pet_type != PET_WILD_IMP )
    sh /= 1.0 + o() -> perk.empowered_demons -> effectN( 2 ).percent();

  return sh;
}

double warlock_pet_t::composite_melee_speed() const
{
  // Make sure we get our overridden haste values applied to melee_speed
  return player_t::composite_melee_speed();
}

double warlock_pet_t::composite_spell_speed() const
{
  // Make sure we get our overridden haste values applied to spell_speed
  return player_t::composite_spell_speed();
}

struct imp_pet_t: public warlock_pet_t
{
  imp_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "imp" ):
    warlock_pet_t( sim, owner, name, PET_IMP, name != "imp" )
  {
    action_list_str = "firebolt";
    owner_coeff.ap_from_sp = 1.0;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "firebolt" ) return new actions::firebolt_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct felguard_pet_t: public warlock_pet_t
{
  felguard_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "felguard" ):
    warlock_pet_t( sim, owner, name, PET_FELGUARD, name != "felguard" )
  {
    action_list_str = "legion_strike";
    owner_coeff.ap_from_sp = 1.0;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    melee_attack = new actions::warlock_pet_melee_t( this );
    special_action = new actions::felstorm_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "legion_strike" ) return new actions::legion_strike_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct t18_illidari_satyr_t: public warlock_pet_t
{
  t18_illidari_satyr_t(sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "illidari_satyr", PET_FELGUARD, true )
  {
    owner_coeff.ap_from_sp = 1;
    regen_type = REGEN_DISABLED;
    action_list_str = "travel";
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    base_energy_regen_per_second = 0;
    melee_attack = new actions::warlock_pet_melee_t( this );
    if ( o() -> pets.t18_illidari_satyr[0] )
      melee_attack -> stats = o() -> pets.t18_illidari_satyr[0] -> get_stats( "melee" );
  }
};

struct t18_prince_malchezaar_t: public warlock_pet_t
{
  t18_prince_malchezaar_t(  sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "prince_malchezaar", PET_GHOUL, true )
  {
    owner_coeff.ap_from_sp = 1;
    regen_type = REGEN_DISABLED;
    action_list_str = "travel";
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    base_energy_regen_per_second = 0;
    melee_attack = new actions::warlock_pet_melee_t( this );
    if ( o() -> pets.t18_prince_malchezaar[0] )
      melee_attack -> stats = o() -> pets.t18_prince_malchezaar[0] -> get_stats( "melee" );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = warlock_pet_t::composite_player_multiplier( school );
    m *= 9.45; // Prince deals 9.45 times normal damage.. you know.. for reasons.
    return m;
  }
};

struct t18_vicious_hellhound_t: public warlock_pet_t
{
  t18_vicious_hellhound_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "vicious_hellhound", PET_DOG, true )
  {
    owner_coeff.ap_from_sp = 1;
    regen_type = REGEN_DISABLED;
    action_list_str = "travel";
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    base_energy_regen_per_second = 0;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );
    melee_attack = new actions::warlock_pet_melee_t( this );
    melee_attack -> base_execute_time = timespan_t::from_seconds( 1.0 );
    if ( o() -> pets.t18_vicious_hellhound[0] )
      melee_attack -> stats = o() -> pets.t18_vicious_hellhound[0] -> get_stats( "melee" );
  }
};

struct felhunter_pet_t: public warlock_pet_t
{
  felhunter_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "felhunter" ):
    warlock_pet_t( sim, owner, name, PET_FELHUNTER, name != "felhunter" )
  {
    action_list_str = "shadow_bite";
    owner_coeff.ap_from_sp = 1.0;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    melee_attack = new actions::warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "shadow_bite" ) return new actions::shadow_bite_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct succubus_pet_t: public warlock_pet_t
{
  succubus_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "succubus" ):
    warlock_pet_t( sim, owner, name, PET_SUCCUBUS, name != "succubus" )
  {
    action_list_str = "lash_of_pain";
    owner_coeff.ap_from_sp = 0.5;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    main_hand_weapon.swing_time = timespan_t::from_seconds( 3.0 );
    melee_attack = new actions::warlock_pet_melee_t( this );
    if ( ! util::str_compare_ci( name_str, "service_succubus" ) )
      special_action = new actions::whiplash_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "lash_of_pain" ) return new actions::lash_of_pain_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct voidwalker_pet_t: public warlock_pet_t
{
  voidwalker_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "voidwalker" ):
    warlock_pet_t( sim, owner, name, PET_VOIDWALKER, name != "voidwalker" )
  {
    action_list_str = "torment";
    owner_coeff.ap_from_sp = 1.0;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    melee_attack = new actions::warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "torment" ) return new actions::torment_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct infernal_pet_t: public warlock_pet_t
{
  infernal_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "infernal"   ):
    warlock_pet_t( sim, owner, name, PET_INFERNAL, name != "infernal" )
  {
    owner_coeff.ap_from_sp = 0.065934;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    action_list_str = "immolation,if=!ticking";
    if ( o() -> talents.demonic_servitude -> ok() )
      action_list_str += "/meteor_strike";
    resources.base[RESOURCE_ENERGY] = 100;
    melee_attack = new actions::warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "immolation" ) return new actions::immolation_t( this, options_str );
    if ( name == "meteor_strike" ) return new actions::meteor_strike_t( this, options_str );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct doomguard_pet_t: public warlock_pet_t
{
    doomguard_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "doomguard"  ):
    warlock_pet_t( sim, owner, name, PET_DOOMGUARD, name != "doomguard" )
  {
    owner_coeff.ap_from_sp = 0.065934;
    action_list_str = "doom_bolt";
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    resources.base[RESOURCE_ENERGY] = 100;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "doom_bolt" ) return new actions::doom_bolt_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct wild_imp_pet_t: public warlock_pet_t
{
  stats_t** firebolt_stats;
  stats_t* regular_stats;
  stats_t* swarm_stats;

  wild_imp_pet_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "wild_imp", PET_WILD_IMP, true ), firebolt_stats( nullptr )
  {
    owner_coeff.sp_from_sp = 0.75;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    action_list_str = "firebolt";

    resources.base[RESOURCE_ENERGY] = 10;
    base_energy_regen_per_second = 0;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override
  {
    if ( name == "firebolt" )
    {
      action_t* a = new actions::wild_firebolt_t( this );
      firebolt_stats = &( a -> stats );
      if ( this == o() -> pets.wild_imps[ 0 ] || sim -> report_pets_separately )
      {
        regular_stats = a -> stats;
        swarm_stats = get_stats( "fel_firebolt_swarm", a );
        swarm_stats -> school = a -> school;
      }
      else
      {
        regular_stats = o() -> pets.wild_imps[ 0 ] -> get_stats( "fel_firebolt" );
        swarm_stats = o() -> pets.wild_imps[ 0 ] -> get_stats( "fel_firebolt_swarm" );
      }
      return a;
    }

    return warlock_pet_t::create_action( name, options_str );
  }

  void trigger( bool swarm = false )
  {
    if ( swarm )
      *firebolt_stats = swarm_stats;
    else
      *firebolt_stats = regular_stats;

    summon();
  }
};

struct fel_imp_pet_t: public warlock_pet_t
{
  fel_imp_pet_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "fel_imp", PET_IMP )
  {
    action_list_str = "felbolt";
    owner_coeff.ap_from_sp = 1;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "felbolt" ) return new actions::felbolt_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct wrathguard_pet_t: public warlock_pet_t
{
  wrathguard_pet_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "wrathguard", PET_FELGUARD )
  {
    owner_coeff.ap_from_sp = 0.66599;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    main_hand_weapon.min_dmg = main_hand_weapon.max_dmg = main_hand_weapon.damage = main_hand_weapon.damage * 0.667; // FIXME: Retest this later
    off_hand_weapon = main_hand_weapon;

    melee_attack = new actions::warlock_pet_melee_t( this );
    special_action = new actions::wrathstorm_t( this );
    special_action_two = new actions::mortal_cleave_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "mortal_cleave" ) return new actions::mortal_cleave_t( this );
    if ( name == "wrathstorm" ) return new actions::wrathstorm_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct observer_pet_t: public warlock_pet_t
{
  observer_pet_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "observer", PET_FELHUNTER )
  {
    action_list_str = "tongue_lash";
    owner_coeff.ap_from_sp = 1.0;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    melee_attack = new actions::warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "tongue_lash" ) return new actions::tongue_lash_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct shivarra_pet_t: public warlock_pet_t
{
  shivarra_pet_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "shivarra", PET_SUCCUBUS )
  {
    action_list_str = "bladedance";
    owner_coeff.ap_from_sp = 0.33333333333333333333;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    main_hand_weapon.swing_time = timespan_t::from_seconds( 3.0 );
    main_hand_weapon.min_dmg = main_hand_weapon.max_dmg = main_hand_weapon.damage = main_hand_weapon.damage * 0.667;
    off_hand_weapon = main_hand_weapon;

    melee_attack = new actions::warlock_pet_melee_t( this );
    special_action = new actions::fellash_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "bladedance" ) return new actions::bladedance_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct voidlord_pet_t: public warlock_pet_t
{
  voidlord_pet_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "voidlord", PET_VOIDWALKER )
  {
    action_list_str = "torment";
    owner_coeff.ap_from_sp = 1;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    melee_attack = new actions::warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "torment" ) return new actions::torment_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct abyssal_pet_t: public warlock_pet_t
{
  abyssal_pet_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "abyssal", PET_INFERNAL, true )
  {
    owner_coeff.ap_from_sp = 0.065934;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    action_list_str = "immolation,if=!ticking";
    if ( o() -> talents.demonic_servitude -> ok() )
      action_list_str += "/meteor_strike";

    resources.base[RESOURCE_ENERGY] = 100;

    melee_attack = new actions::warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "immolation" ) return new actions::immolation_t( this, options_str );
    if ( name == "meteor_strike" ) return new actions::meteor_strike_t( this, options_str );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct terrorguard_pet_t: public warlock_pet_t
{
  terrorguard_pet_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "terrorguard", PET_DOOMGUARD, true )
  {
    action_list_str = "doom_bolt";
    owner_coeff.ap_from_sp = 0.065934;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    resources.base[RESOURCE_ENERGY] = 100;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "doom_bolt" ) return new actions::doom_bolt_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct inner_demon_t : public pet_t
{
  struct soul_fire_t : public spell_t
  {
    soul_fire_t( inner_demon_t* p ) :
      spell_t( "soul_fire", p, p -> find_spell( 166864 ) )
    {
      min_gcd = data().gcd();
      may_crit = true;
    }

    bool usable_moving() const override
    { return true; }

    // Add ability lag through different means, because guardians in WoD do not
    // suffer from it, except this one does.
    timespan_t execute_time() const override
    {
      timespan_t cast_time = spell_t::execute_time();
      cast_time += rng().gauss( timespan_t::from_seconds( 0.2 ), timespan_t::from_seconds( 0.025 ) );

      return cast_time;
    }
  };

  soul_fire_t* soul_fire;

  inner_demon_t( warlock_t* owner ) :
    pet_t( owner -> sim, owner, "inner_demon", true ),
    soul_fire( nullptr )
  {
    owner_coeff.sp_from_sp = 0.75;
  }

  void init_spells() override
  {
    pet_t::init_spells();

    soul_fire = new soul_fire_t( this );
  }

  warlock_t* o()
  { return static_cast<warlock_t*>( owner ); }
  const warlock_t* o() const
  { return static_cast<warlock_t*>( owner ); }

  void init_action_list() override
  {
    pet_t::init_action_list();

    action_list_str = "soul_fire";
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "soul_fire" ) return soul_fire;

    return pet_t::create_action( name, options_str );
  }

  // TODO: Orc racial?
  double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( o() -> mastery_spells.master_demonologist -> ok() )
    {
      double mastery = o() -> cache.mastery();
      m *= 1.0 + mastery * o() -> mastery_spells.master_demonologist -> effectN( 1 ).mastery_value();
    }

    return m;
  }

  void summon( timespan_t duration ) override
  {
    // Enable Soul Fire cast on the next spell execute of the Warlock
    soul_fire -> background = true;

    pet_t::summon( duration );
  }
};

} // end namespace pets

// Spells
namespace actions {

struct warlock_heal_t: public heal_t
{
  warlock_heal_t( const std::string& n, warlock_t* p, const uint32_t id ):
    heal_t( n, p, p -> find_spell( id ) )
  {
    target = p;
  }

  warlock_t* p()
  {
    return static_cast<warlock_t*>( player );
  }
};

// Custom state to carry "ds tickness" across event boundaries in ability
// execution. Works for multistrikes too.
struct warlock_state_t: public action_state_t
{
  bool ds_tick;
  bool periodic_hit;

  warlock_state_t( action_t* a, player_t* target ):
    action_state_t( a, target ), ds_tick( false ),
    periodic_hit( false )
  { }

  void initialize() override
  {
    action_state_t::initialize();
    ds_tick = false;
    periodic_hit = false;
  }

  std::ostringstream& debug_str( std::ostringstream& ds ) override
  {
    action_state_t::debug_str( ds );
    ds << " ds_tick=" << ds_tick;
    ds << " periodic_hit=" << periodic_hit;
    return ds;
  }

  void copy_state( const action_state_t* other ) override
  {
    action_state_t::copy_state( other );
    auto other_c = debug_cast<const warlock_state_t*>( other );
    ds_tick = other_c -> ds_tick;
    periodic_hit = other_c -> periodic_hit;
  }
};

struct warlock_spell_t: public spell_t
{
  bool demo_mastery;
private:
  void _init_warlock_spell_t()
  {
    may_crit = true;
    tick_may_crit = true;
    weapon_multiplier = 0.0;
    gain = player -> get_gain( name_str );
    generate_fury = 0;
    cost_event = nullptr;
    havoc_consume = backdraft_consume = 0;
    fury_in_meta = data().powerN( POWER_DEMONIC_FURY ).aura_id() == 54879;
    ds_tick_stats = player -> get_stats( name_str + "_ds", this );
    ds_tick_stats -> school = school;
    mg_tick_stats = player -> get_stats( name_str + "_mg", this );
    mg_tick_stats -> school = school;

    havoc_proc = nullptr;

    if ( p() -> destruction_trinket )
    {
      affected_by_flamelicked = data().affected_by( p() -> destruction_trinket -> driver() -> effectN( 1 ).trigger() -> effectN( 1 ) );
    }
    else
    {
      affected_by_flamelicked = false;
    }

    parse_spell_coefficient( *this );

  }

public:
  double generate_fury;
  gain_t* gain;
  bool fury_in_meta;
  stats_t* ds_tick_stats;
  stats_t* mg_tick_stats;
  mutable std::vector< player_t* > havoc_targets;

  int havoc_consume, backdraft_consume;

  proc_t* havoc_proc;

  bool affected_by_flamelicked;

  struct cost_event_t: player_event_t
  {
    warlock_spell_t* spell;
    resource_e resource;

    cost_event_t( player_t* p, warlock_spell_t* s, resource_e r = RESOURCE_NONE ):
      player_event_t( *p ), spell( s ), resource( r )
    {
      if ( resource == RESOURCE_NONE ) resource = spell -> current_resource();
      add_event( timespan_t::from_seconds( 1 ) );
    }
    virtual const char* name() const override
    { return  "cost_event"; }
    virtual void execute() override
    {
      spell -> cost_event = new ( sim() ) cost_event_t( p(), spell, resource );
      p() -> resource_loss( resource, spell -> base_costs_per_second[resource], spell -> gain );
    }
  };

  event_t* cost_event;

  warlock_spell_t( warlock_t* p, const std::string& n ):
    spell_t( n, p, p -> find_class_spell( n ) ),
    demo_mastery( spell_t::data().affected_by( p -> mastery_spells.master_demonologist -> effectN( 3 ) ) )
  {
    _init_warlock_spell_t();
  }

  warlock_spell_t( const std::string& token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() ):
    spell_t( token, p, s ),
    demo_mastery( spell_t::data().affected_by( p -> mastery_spells.master_demonologist -> effectN( 3 ) ) )
  {
    _init_warlock_spell_t();
  }

  warlock_t* p()
  {
    return static_cast<warlock_t*>( player );
  }
  const warlock_t* p() const
  {
    return static_cast<warlock_t*>( player );
  }

  warlock_td_t* td( player_t* t ) const
  {
    return p() -> get_target_data( t );
  }

  action_state_t* new_state() override
  {
    return new warlock_state_t( this, target );
  }

  bool use_havoc() const
  {
    if ( ! p() -> havoc_target || target == p() -> havoc_target || ! havoc_consume )
      return false;

    if ( p() -> buffs.havoc -> check() < havoc_consume )
      return false;

    return true;
  }

  bool use_backdraft() const
  {
    if ( ! backdraft_consume )
      return false;

    if ( p() -> buffs.backdraft -> check() < backdraft_consume )
      return false;

    return true;
  }

  virtual bool usable_moving() const override
  {
    bool um = spell_t::usable_moving();

    if ( p() -> buffs.kiljaedens_cunning -> up() )
      return true;

    return um;
  }

  virtual void init() override
  {
    spell_t::init();

    if ( harmful && ! tick_action ) trigger_gcd += p() -> spec.chaotic_energy -> effectN( 3 ).time_value();

    if ( havoc_consume > 0 )
    {
      havoc_proc = player -> get_proc( "Havoc: " + ( data().id() ? std::string( data().name_cstr() ) : name_str ) );
    }
  }

  virtual void reset() override
  {
    spell_t::reset();

    event_t::cancel( cost_event );
  }

  virtual int n_targets() const override
  {
    if ( aoe == 0 && use_havoc() )
      return 2;

    return spell_t::n_targets();
  }

  virtual std::vector< player_t* >& target_list() const override
  {
    if ( use_havoc() )
    {
      if ( ! target_cache.is_valid )
        available_targets( target_cache.list );

      havoc_targets.clear();
      if ( std::find( target_cache.list.begin(), target_cache.list.end(), target ) != target_cache.list.end() )
        havoc_targets.push_back( target );

      if ( ! p() -> havoc_target -> is_sleeping() &&
           std::find( target_cache.list.begin(), target_cache.list.end(), p() -> havoc_target ) != target_cache.list.end() )
           havoc_targets.push_back( p() -> havoc_target );
      return havoc_targets;
    }
    else
      return spell_t::target_list();
  }

  virtual double cost() const override
  {
    double c = spell_t::cost();

    if ( current_resource() == RESOURCE_MANA && p() -> glyphs.life_pact -> ok() )
      c *= ( 1 + p() -> glyphs.life_pact -> effectN( 2 ).percent() );

    if ( current_resource() == RESOURCE_DEMONIC_FURY && p() -> buffs.dark_soul -> check() )
      c *= 1.0 + p() -> sets.set( SET_CASTER, T15, B2 ) -> effectN( 3 ).percent();

    if ( use_backdraft() && current_resource() == RESOURCE_MANA )
      c *= 1.0 + p() -> buffs.backdraft -> data().effectN( 1 ).percent();

    return c;
  }

  virtual void execute() override
  {
    spell_t::execute();

    if ( result_is_hit( execute_state -> result ) && p() -> specialization() == WARLOCK_DEMONOLOGY
         && generate_fury > 0 && ! p() -> buffs.metamorphosis -> check() )
         p() -> resource_gain( RESOURCE_DEMONIC_FURY, generate_fury, gain );
    if ( result_is_hit( execute_state -> result ) && p() -> talents.grimoire_of_synergy -> ok() )
    {
      pets::warlock_pet_t* my_pet = static_cast<pets::warlock_pet_t*>( p() -> pets.active ); //get active pet
      if ( my_pet != nullptr )
      {
        bool procced = p() -> grimoire_of_synergy.trigger();
        if ( procced ) my_pet -> buffs.demonic_synergy -> trigger();
      }
    }

    p() -> trigger_demonology_t17_2pc_cast();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t h = spell_t::execute_time();

    if ( use_backdraft() )
      h *= 1.0 + p() -> buffs.backdraft -> data().effectN( 1 ).percent();

    return h;
  }

  void consume_resource() override
  {
    spell_t::consume_resource();

    if ( p() -> spells.immolation_aura &&
         p() -> resources.current[RESOURCE_DEMONIC_FURY] < p() -> spells.immolation_aura -> tick_action -> base_costs[RESOURCE_DEMONIC_FURY] )
    {
      p() -> spells.immolation_aura -> get_dot() -> cancel();
      p() -> buffs.immolation_aura -> expire();
    }

    if ( use_havoc() )
    {
      havoc_proc -> occur();

      p() -> buffs.havoc -> decrement( havoc_consume );
      if ( p() -> buffs.havoc -> check() == 0 )
        p() -> havoc_target = nullptr;
    }

    if ( p() -> resources.current[RESOURCE_BURNING_EMBER] < 1.0 )
      p() -> buffs.fire_and_brimstone -> expire();

    if ( use_backdraft() )
      p() -> buffs.backdraft -> decrement( backdraft_consume );
  }

  virtual bool ready() override
  {
    if ( p() -> buffs.metamorphosis -> check() && p() -> resources.current[RESOURCE_DEMONIC_FURY] < META_FURY_MINIMUM )
      p() -> spells.metamorphosis -> cancel();

    return spell_t::ready();
  }

  virtual void tick( dot_t* d ) override
  {
    spell_t::tick( d );

    if ( p() -> specialization() == WARLOCK_DEMONOLOGY && generate_fury > 0 )
      p() -> resource_gain( RESOURCE_DEMONIC_FURY, generate_fury, gain );

    trigger_seed_of_corruption( td( d -> state -> target ), p(), d -> state -> result_amount );
  }

  virtual void impact( action_state_t* s ) override
  {
    spell_t::impact( s );

    trigger_seed_of_corruption( td( s -> target ), p(), s -> result_amount );
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double m = 1.0;

    warlock_td_t* td = this -> td( t );

    //haunt debuff
    if ( td -> debuffs_haunt -> check() && ( channeled || spell_power_mod.tick ) ) // Only applies to channeled or dots
    {
      m *= 1.0 + td -> debuffs_haunt -> data().effectN( 4 ).percent();
    }

    //sb:haunt buff
    if ( p() -> buffs.haunting_spirits -> check() && ( channeled || spell_power_mod.tick ) ) // Only applies to channeled or dots
    {
      m *= 1.0 + p() -> buffs.haunting_spirits -> data().effectN( 1 ).percent();
    }

    return spell_t::composite_target_multiplier( t ) * m;
  }

  virtual double action_multiplier() const override
  {
    double pm = spell_t::action_multiplier();

    if ( p() -> buffs.metamorphosis -> up() && demo_mastery )
    {
      double mastery = p() -> cache.mastery();
      pm *= 1.0 + mastery * p() -> mastery_spells.master_demonologist -> effectN( 3 ).mastery_value();
    }

    return pm;
  }

  virtual resource_e current_resource() const override
  {
    if ( fury_in_meta && p() -> buffs.metamorphosis -> data().ok() )
    {
      if ( p() -> buffs.metamorphosis -> check() )
        return RESOURCE_DEMONIC_FURY;
      else
        return RESOURCE_MANA;
    }
    else
      return spell_t::current_resource();
  }

  virtual double composite_target_crit( player_t* target ) const override
  {
    double c = spell_t::composite_target_crit( target );
    if ( affected_by_flamelicked && p() -> destruction_trinket )
      c += td( target ) -> debuffs_flamelicked -> stack_value();

    return c;
  }

  void trigger_seed_of_corruption( warlock_td_t* td, warlock_t* p, double amount )
  {
    if ( ( ( td -> dots_seed_of_corruption -> current_action && id == td -> dots_seed_of_corruption -> current_action -> id )
      || td -> dots_seed_of_corruption -> is_ticking() ) && td -> soc_trigger > 0 )
    {
      td -> soc_trigger -= amount;
      if ( td -> soc_trigger <= 0 )
      {
        p -> spells.seed_of_corruption_aoe -> execute();
        td -> dots_seed_of_corruption -> cancel();
      }
    }
    if ( ( ( td -> dots_soulburn_seed_of_corruption -> current_action && id == td -> dots_soulburn_seed_of_corruption -> current_action -> id )
      || td -> dots_soulburn_seed_of_corruption -> is_ticking() ) && td -> soulburn_soc_trigger > 0 )
    {
      td -> soulburn_soc_trigger -= amount;
      if ( td -> soulburn_soc_trigger <= 0 )
      {
        p -> spells.soulburn_seed_of_corruption_aoe -> execute();
        td -> dots_soulburn_seed_of_corruption -> cancel();
      }
    }
  }

  bool consume_cost_per_second( timespan_t tick_time ) override
  {
    bool consume = spell_t::consume_cost_per_second( tick_time );

    resource_e r = current_resource();

    if ( p() -> resources.current[r] < resource_consumed && r == RESOURCE_DEMONIC_FURY && p() -> buffs.metamorphosis -> check() )
      p() -> spells.metamorphosis -> cancel();

    return consume;
  }

  // ds_tick is set, and we will record the damage as "direct", even if it is
  // from extra ticks
  dmg_e report_amount_type( const action_state_t* state ) const override
  {
    if ( debug_cast<const warlock_state_t*>( state ) -> ds_tick )
      return DMG_DIRECT;

    return spell_t::report_amount_type( state );
  }

  // DS extra ticks can multistrike, so we need to do the stats juggling in
  // it's correct place, since multistrikes do not strike immediately, but
  // rather on their own event
  void assess_damage( dmg_e type, action_state_t* s ) override
  {
    warlock_state_t* state = debug_cast<warlock_state_t*>( s );
    stats_t* tmp = nullptr;
    // ds tick -> adjust the spell's "stats" object so we collect information
    // to a separate SPELL_ds entry in the report
    if ( state -> ds_tick )
    {
      tmp = stats;
      stats = ds_tick_stats;
    }

    spell_t::assess_damage( type, s );

    if ( tmp )
    {
      stats -> add_execute( timespan_t::zero(), s -> target );
      stats = tmp;
    }
  }

  void trigger_extra_tick( dot_t* dot, double multiplier )
  {
    if ( ! dot -> is_ticking() ) return;
    double tempmulti;
    assert( multiplier != 0.0 );

    action_state_t* tmp_state = dot -> state;
    dot -> state = get_state( tmp_state );
    dot -> state -> ta_multiplier *= multiplier;

    // Carry "ds tickness" in the state, so it works across events etc
    auto w_state = debug_cast<warlock_state_t*>( dot -> state );
    w_state -> ds_tick = true;

    snapshot_internal( dot -> state, update_flags | STATE_CRIT, tmp_state -> result_type );

    tempmulti = dot -> current_action -> base_multiplier;
    w_state -> periodic_hit = true;
    dot -> current_action -> base_multiplier = multiplier;
    dot -> current_action -> tick( dot );
    w_state -> periodic_hit = false;

    action_state_t::release( dot -> state );
    dot -> state = tmp_state;
    dot -> current_action -> base_multiplier = tempmulti;
  }

  void extend_dot( dot_t* dot, timespan_t extend_duration )
  {
    if ( dot -> is_ticking() )
    {
      //FIXME: This is roughly how it works, but we need more testing
      dot -> extend_duration( extend_duration, dot -> current_action -> dot_duration * 1.5 );
    }
  }

  static void trigger_ember_gain( warlock_t* p, double amount, gain_t* gain, double chance = 1.0 )
  {
    if ( ! p -> rng().roll( chance ) ) return;

    if ( ( p -> sets.has_set_bonus( SET_CASTER, T16, B4 ) || p -> sets.has_set_bonus( WARLOCK_DESTRUCTION, T17, B4 )) && //check whether we fill one up.
         ( ( p -> resources.current[RESOURCE_BURNING_EMBER] < 1.0 && p -> resources.current[RESOURCE_BURNING_EMBER] + amount >= 1.0 ) ||
         ( p -> resources.current[RESOURCE_BURNING_EMBER] < 2.0 && p -> resources.current[RESOURCE_BURNING_EMBER] + amount >= 2.0 ) ||
         ( p -> resources.current[RESOURCE_BURNING_EMBER] < 3.0 && p -> resources.current[RESOURCE_BURNING_EMBER] + amount >= 3.0 ) ||
         ( p -> resources.current[RESOURCE_BURNING_EMBER] < 4.0 && p -> resources.current[RESOURCE_BURNING_EMBER] + amount >= 4.0 ) ) )
    {
      if(p -> sets.has_set_bonus( SET_CASTER, T16, B4 ))
        p -> buffs.tier16_4pc_ember_fillup -> trigger();

      if( p -> sets.has_set_bonus( WARLOCK_DESTRUCTION, T17, B4 ) && p -> rppm_chaotic_infusion.trigger())
        p -> buffs.chaotic_infusion ->trigger();
    }

    p -> resource_gain( RESOURCE_BURNING_EMBER, amount, gain );

    // If getting to 1 full ember was a surprise, the player would have to react to it
    if ( p -> resources.current[RESOURCE_BURNING_EMBER] == 1.0 && ( amount > 0.1 || chance < 1.0 ) )
      p -> ember_react = p -> sim -> current_time() + p -> total_reaction_time();
    else if ( p -> resources.current[RESOURCE_BURNING_EMBER] >= 1.0 )
      p -> ember_react = p -> sim -> current_time();
    else
      p -> ember_react = timespan_t::max();
  }

  static void refund_embers( warlock_t* p )
  {
    double refund = ceil( ( p -> resources.current[RESOURCE_BURNING_EMBER] + 1.0 ) / 4.0 );

    if ( refund > 0 ) p -> resource_gain( RESOURCE_BURNING_EMBER, refund, p -> gains.miss_refund );
  }

  static void trigger_soul_leech( warlock_t* p, double amount )
  {
    if ( p -> talents.soul_leech -> ok() )
    {
      p -> resource_gain( RESOURCE_HEALTH, amount, p -> gains.soul_leech );
    }
  }

  static void trigger_wild_imp( warlock_t* p )
  {
    for ( size_t i = 0; i < p -> pets.wild_imps.size() ; i++ )
    {
      if ( p -> pets.wild_imps[i] -> is_sleeping() )
      {
        p -> pets.wild_imps[i] -> trigger();
        p -> procs.wild_imp -> occur();
        return;
      }
    }
    p -> sim -> errorf( "Player %s ran out of wild imps.\n", p -> name() );
    assert( false ); // Will only get here if there are no available imps
  }
};

struct agony_t: public warlock_spell_t
{
  agony_t( warlock_t* p ):
    warlock_spell_t( p, "Agony" )
  {
    may_crit = false;

    if ( p -> affliction_trinket )
    {
      const spell_data_t* data = p -> affliction_trinket -> driver();
      double period_value = data -> effectN( 1 ).average( p -> affliction_trinket -> item ) / 100.0;
      double duration_value = data -> effectN( 2 ).average( p -> affliction_trinket -> item ) / 100.0;

      base_tick_time *= 1.0 + period_value;
      dot_duration *= 1.0 + duration_value;
    }
  }

  virtual void last_tick( dot_t* d ) override
  {
    td( d -> state -> target ) -> agony_stack = 1;
    warlock_spell_t::last_tick( d );
  }

  virtual void tick( dot_t* d ) override
  {
    if ( td( d -> state -> target ) -> agony_stack < ( 10 ) ) 
      td( d -> state -> target ) -> agony_stack++;

    td( d -> target ) -> debuffs_agony -> trigger();
    warlock_spell_t::tick( d );
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_spell_t::composite_target_multiplier( target );

    m *= td( target ) -> agony_stack;

    return m;
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> mastery_spells.potent_afflictions -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }
};

struct doom_t: public warlock_spell_t
{
  doom_t( warlock_t* p ):
    warlock_spell_t( "doom", p, p -> spec.doom )
  {
    may_crit = false;
    base_crit += p -> perk.empowered_doom -> effectN( 1 ).percent();
  }

  double action_multiplier() const override
  {
    double am = spell_t::action_multiplier();

    double mastery = p() -> cache.mastery();
    am *= 1.0 + mastery * p() -> mastery_spells.master_demonologist -> effectN( 3 ).mastery_value();

    return am;
  }

  virtual void tick( dot_t* d ) override
  {
    warlock_spell_t::tick( d );

    if ( d -> state -> result == RESULT_CRIT ) trigger_wild_imp( p() );

    if ( p() -> glyphs.siphon_life -> ok() )
      p() -> resource_gain( RESOURCE_HEALTH, d -> state -> result_amount * p() -> glyphs.siphon_life -> effectN( 1 ).percent(), p() -> gains.siphon_life );
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};

struct kiljaedens_cunning_t: public warlock_spell_t
{
  kiljaedens_cunning_t( warlock_t* p ):
    warlock_spell_t( "kiljaedens_cunning", p, p -> talents.kiljaedens_cunning )
  {
  }

  void execute() override
  {
    warlock_spell_t::execute();
    p() -> buffs.kiljaedens_cunning -> trigger();
  }
};

struct demonbolt_t: public warlock_spell_t
{
  demonbolt_t( warlock_t* p ):
    warlock_spell_t( "demonbolt", p, p -> talents.demonbolt )
  {
  }

  virtual double cost() const override
  {
    double c = warlock_spell_t::cost();
    c *= 1.0 + p() -> buffs.demonbolt -> stack() * p() -> talents.demonbolt -> effectN( 3 ).percent();
    return c;
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    //Reduce "de"buff duration by spellhaste
    p() -> buffs.demonbolt -> buff_duration = p() -> buffs.demonbolt -> data().duration() * composite_haste();
    p() -> buffs.demonbolt -> trigger();

  }

  virtual double action_multiplier() const override
  {
    double am = warlock_spell_t::action_multiplier();

    am *= 1.0 + p() -> buffs.demonbolt -> stack() * p() -> talents.demonbolt -> effectN( 2 ).percent();

    return am;
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> talents.demonbolt -> ok() ) r = false;

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};

struct havoc_t: public warlock_spell_t
{
  havoc_t( warlock_t* p ): warlock_spell_t( p, "Havoc" )
  {
    may_crit = false;
    cooldown -> duration = data().cooldown() + p -> glyphs.havoc -> effectN( 2 ).time_value();
    cooldown -> duration += p -> perk.enhanced_havoc -> effectN( 1 ).time_value();
    cooldown -> charges = data().charges() + p -> glyphs.havoc -> effectN( 1 ).base_value();
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    p() -> buffs.havoc -> trigger( p() -> buffs.havoc -> max_stack() );
    p() -> havoc_target = execute_state -> target;
  }
};

struct shadowflame_t: public warlock_spell_t
{
  shadowflame_t( warlock_t* p ):
    warlock_spell_t( "shadowflame", p, p -> find_spell( 47960 ) )
  {
    background = true;
    may_miss = false;
    generate_fury = 2;
    spell_power_mod.tick *= 0.8; // Check
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.5 );
  }

  virtual void tick( dot_t* d ) override
  {
    warlock_spell_t::tick( d );

    if ( p() -> spec.molten_core -> ok() && rng().roll( 0.08 ) )
      p() -> buffs.molten_core -> trigger();
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_spell_t::composite_target_multiplier( target );

    m *= td( target ) -> debuffs_shadowflame -> stack();

    return m;
  }

  virtual void last_tick( dot_t* d ) override
  {
    warlock_spell_t::last_tick( d );

    td ( d -> state -> target ) -> debuffs_shadowflame -> expire();
  }
};

struct hand_of_guldan_t: public warlock_spell_t
{
  shadowflame_t* shadowflame;

  double demonology_trinket_chance;

  hand_of_guldan_t( warlock_t* p ):
    warlock_spell_t( p, "Hand of Gul'dan" ),
    demonology_trinket_chance( 0.0 )
  {
    aoe = -1;

    cooldown -> duration = timespan_t::from_seconds( 15 );
    cooldown -> charges = 2 + p -> sets.set( WARLOCK_DEMONOLOGY, T17, B4 ) -> effectN( 1 ).base_value();

    shadowflame = new shadowflame_t( p );
    add_child( shadowflame );

    parse_effect_data( p -> find_spell( 86040 ) -> effectN( 1 ) );

    if ( p -> demonology_trinket && p -> specialization() == WARLOCK_DEMONOLOGY )
    {
      const spell_data_t* data = p -> find_spell( p -> demonology_trinket -> spell_id );
      demonology_trinket_chance = data -> effectN( 1 ).average( p -> demonology_trinket -> item );
      demonology_trinket_chance /= 100.0;
    }
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.5 );
  }

  void schedule_travel( action_state_t* s ) override
  {
    /* Executed at the same time as HoG and given a travel time,
    so that it can snapshot meta at the appropriate time. */
    shadowflame -> target = s -> target;
    shadowflame -> execute();
    warlock_spell_t::schedule_travel( s );
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    p() -> trigger_demonology_t17_2pc( execute_state );

    if ( p() -> demonology_trinket && p() -> rng().roll( demonology_trinket_chance ) )
    {
      trigger_wild_imp( p() );
      trigger_wild_imp( p() );
      trigger_wild_imp( p() );
      p() -> procs.fragment_wild_imp -> occur();
      p() -> procs.fragment_wild_imp -> occur();
      p() -> procs.fragment_wild_imp -> occur();
    }
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs_shadowflame -> trigger();
    }
  }
};

struct shadow_bolt_t: public warlock_spell_t
{
  hand_of_guldan_t* hand_of_guldan;
  shadow_bolt_t( warlock_t* p ):
    warlock_spell_t( p, "Shadow Bolt" ), hand_of_guldan( new hand_of_guldan_t( p ) )
  {
    base_multiplier *= 1.0 + p -> sets.set( SET_CASTER, T14, B2 ) -> effectN( 3 ).percent();
    hand_of_guldan               -> background = true;
    hand_of_guldan               -> base_costs[RESOURCE_MANA] = 0;
    hand_of_guldan               -> cooldown = p -> get_cooldown( "t16_4pc_demo" );
    generate_fury = data().effectN( 2 ).base_value();
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() );
    }
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> sets.has_set_bonus( SET_CASTER, T16, B4 ) && rng().roll( 0.08 ) )
    {
      hand_of_guldan -> target = target;
      hand_of_guldan -> execute();
    }

    if ( p() -> buffs.demonic_calling -> up() )
    {
      trigger_wild_imp( p() );
      p() -> buffs.demonic_calling -> expire();
    }

    if ( result_is_hit( execute_state -> result ) )
    {
      // Only applies molten core if molten core is not already up
      if ( target -> health_percentage() < p() -> spec.decimation -> effectN( 1 ).base_value() && ! p() -> buffs.molten_core -> check() )
        p() -> buffs.molten_core -> trigger();
    }
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};

struct shadowburn_t: public warlock_spell_t
{
  struct resource_event_t: public player_event_t
  {
    shadowburn_t* spell;
    gain_t* ember_gain;
    player_t* target;

    resource_event_t( warlock_t* p, shadowburn_t* s, player_t* t ):
      player_event_t( *p ), spell( s ), ember_gain( p -> gains.shadowburn_ember), target(t)
    {
      add_event( spell -> delay );
    }
    virtual const char* name() const override
    { return "shadowburn_execute_gain"; }
    virtual void execute() override
    {
      if (target -> is_sleeping()) //if it is dead return ember, else return mana
      {
          p() -> resource_gain(RESOURCE_BURNING_EMBER, 2, ember_gain); //TODO look up ember amount in shadowburn spell
      }
    }
  };

  resource_event_t* resource_event;
  timespan_t delay;
  shadowburn_t( warlock_t* p ):
    warlock_spell_t( p, "Shadowburn" ), resource_event( nullptr )
  {
    min_gcd = timespan_t::from_millis( 500 );
    havoc_consume = 1;
    delay = data().effectN( 1 ).trigger() -> duration();
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    resource_event = new ( *sim ) resource_event_t( p(), this, s -> target );
  }

  virtual double cost() const override
  {
    double c = warlock_spell_t::cost();

    if ( p() -> buffs.dark_soul -> check() )
      c *= 1.0 + p() -> sets.set( SET_CASTER, T15, B2 ) -> effectN( 2 ).percent();

    return c;
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> mastery_spells.emberstorm -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

      m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    if ( ! result_is_hit( execute_state -> result ) ) refund_embers( p() );
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( target -> health_percentage() >= 20 ) r = false;

    return r;
  }
};

struct corruption_t: public warlock_spell_t
{
  corruption_t( warlock_t* p ):
    warlock_spell_t( "Corruption", p, p -> find_spell( 172 ) ) //Use original corruption until DBC acts more friendly.
  {
    may_crit = false;
    generate_fury = 4;
    generate_fury += p -> perk.enhanced_corruption -> effectN( 1 ).base_value();
    base_multiplier *= 1.0 + p -> sets.set( SET_CASTER, T14, B2 ) -> effectN( 1 ).percent();
    dot_duration = data().effectN( 1 ).trigger() -> duration();
    spell_power_mod.tick = data().effectN( 1 ).trigger() -> effectN( 1 ).sp_coeff();
    base_tick_time = data().effectN( 1 ).trigger() -> effectN( 1 ).period();

    if ( p -> affliction_trinket )
    {
      const spell_data_t* data = p -> affliction_trinket ->  driver();
      double period_value = data -> effectN( 1 ).average( p -> affliction_trinket -> item ) / 100.0;
      double duration_value = data -> effectN( 2 ).average( p -> affliction_trinket -> item ) / 100.0;

      base_tick_time *= 1.0 + period_value;
      dot_duration *= 1.0 + duration_value;
    }
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_millis( 100 );
  }

  virtual void execute() override
  {
    p() -> latest_corruption_target = target;

    warlock_spell_t::execute();
  }

  virtual void tick( dot_t* d ) override
  {
    warlock_spell_t::tick( d );

    auto w_state = debug_cast<warlock_state_t*>( d -> state );

    if ( p() -> spec.nightfall -> ok() && d -> state -> target == p() -> latest_corruption_target && ! w_state -> periodic_hit ) //5.4 only the latest corruption procs it
    {

      double nightfall_chance = p() -> spec.nightfall -> effectN( 1 ).percent() / 10;

      if ( p() -> sets.has_set_bonus( WARLOCK_AFFLICTION, T17, B2 ) && td( d -> state -> target ) -> dots_drain_soul -> is_ticking() && td( d -> state -> target ) -> dots_agony -> is_ticking() && td( d -> state -> target ) -> dots_unstable_affliction -> is_ticking() ) //Caster Has T17 2pc and UA/Agony are ticking as well on the target
      {
        nightfall_chance += p() -> sets.set( WARLOCK_AFFLICTION, T17, B2 ) -> effectN( 1 ).percent();
      }

      if ( rng().roll( nightfall_chance ) )
      {
        if ( p() -> double_nightfall == 3 )
        {
          p() -> resource_gain( RESOURCE_SOUL_SHARD, 2, p() -> gains.nightfall );
          p() -> double_nightfall = 0;
        }
        else
        {
          p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.nightfall );
          p() -> double_nightfall++;
        }
        // If going from 0 to 1 shard was a surprise, the player would have to react to it
        if ( p() -> resources.current[RESOURCE_SOUL_SHARD] == 1 )
          p() -> shard_react = p() -> sim -> current_time() + p() -> total_reaction_time();
        else if ( p() -> resources.current[RESOURCE_SOUL_SHARD] >= 1 )
          p() -> shard_react = p() -> sim -> current_time();
        else
          p() -> shard_react = timespan_t::max();


        if ( p() -> sets.has_set_bonus( WARLOCK_AFFLICTION, T17, B4 ) ) //if we ticked, then reduce time till next DS goes off CD by 10 seconds
        {
          timespan_t cd_reduction = p() -> sets.set( WARLOCK_AFFLICTION, T17, B4 ) -> effectN( 1 ).time_value();

          p() -> cooldowns.dark_soul -> adjust(- cd_reduction);
        }
      }
    }

    if ( p() -> sets.has_set_bonus( WARLOCK_DEMONOLOGY, T17, B4 )
        && rng().roll( p() -> sets.set( WARLOCK_DEMONOLOGY, T17, B4 ) -> effectN( 2 ).percent() )
        && ( p() -> cooldowns.hand_of_guldan -> current_charge < p() -> cooldowns.hand_of_guldan -> charges ))
    {
        p() -> cooldowns.hand_of_guldan -> adjust( -p() -> cooldowns.hand_of_guldan -> duration); //decrease remaining time by the duration of one charge, i.e., add one charge
    }

    if ( p() -> glyphs.siphon_life -> ok() )
    {
      if ( d -> state -> result_amount > 0 )
        p() -> resource_gain( RESOURCE_HEALTH,
        player -> resources.max[RESOURCE_HEALTH] * p() -> glyphs.siphon_life -> effectN( 1 ).percent() / 100.0,
        p() -> gains.siphon_life );
    }
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> mastery_spells.potent_afflictions -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};

struct drain_life_heal_t: public warlock_heal_t
{
  const spell_data_t* real_data;
  const spell_data_t* soulburned_data;

  drain_life_heal_t( warlock_t* p ):
    warlock_heal_t( "drain_life_heal", p, 89653 )
  {
    background = true;
    may_miss = false;
    base_dd_min = base_dd_max = 0; // Is parsed as 2
    real_data = p -> find_spell( 689 );
    soulburned_data = p -> find_spell( 89420 );
  }

  virtual void execute() override
  {
    double heal_pct = real_data -> effectN( 2 ).percent();

    if ( p() -> buffs.soulburn -> up() )
      heal_pct = soulburned_data -> effectN( 2 ).percent();

    base_dd_min = base_dd_max = player -> resources.max[RESOURCE_HEALTH] * heal_pct;

    warlock_heal_t::execute();
  }
};

struct drain_life_t: public warlock_spell_t
{
  drain_life_heal_t* heal;

  drain_life_t( warlock_t* p ):
    warlock_spell_t( p, "Drain Life" ), heal( nullptr )
  {
    channeled = true;
    hasted_ticks = false;
    may_crit = false;
    generate_fury = 10;

    heal = new drain_life_heal_t( p );
  }

  void tick( dot_t* d ) override
  {
    spell_t::tick( d );

    heal -> execute();

    if ( p() -> specialization() == WARLOCK_DEMONOLOGY && !p() -> buffs.metamorphosis -> check() )
      p() -> resource_gain( RESOURCE_DEMONIC_FURY, generate_fury, gain );

    trigger_seed_of_corruption( td( d -> state -> target ), p(), d -> state -> result_amount );
  }

  virtual double action_multiplier() const override
  {
    double am = warlock_spell_t::action_multiplier();

    if ( p() -> talents.harvest_life -> ok() )
      am *= 1.0 + p() -> talents.harvest_life -> effectN( 1 ).percent();

    return am;
  }
};

struct unstable_affliction_t: public warlock_spell_t
{
  unstable_affliction_t( warlock_t* p ):
    warlock_spell_t( p, "Unstable Affliction" )
  {
    may_crit = false;
    if ( p -> glyphs.unstable_affliction -> ok() )
      base_execute_time *= 1.0 + p -> glyphs.unstable_affliction -> effectN( 1 ).percent();

    if ( p -> affliction_trinket )
    {
      const spell_data_t* data = p -> affliction_trinket -> driver();
      double period_value = data -> effectN( 1 ).average( p -> affliction_trinket -> item ) / 100.0;
      double duration_value = data -> effectN( 2 ).average( p -> affliction_trinket -> item ) / 100.0;

      base_tick_time *= 1.0 + period_value;
      dot_duration *= 1.0 + duration_value;
    }
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> mastery_spells.potent_afflictions -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual void tick( dot_t* d ) override
  {
    warlock_spell_t::tick( d );

    if ( p() -> sets.has_set_bonus( SET_CASTER, T16, B2 ) && d -> state -> result == RESULT_CRIT )
    {
      p() ->  buffs.tier16_2pc_empowered_grasp -> trigger();
    }
  }
};

struct haunt_t: public warlock_spell_t
{
  haunt_t( warlock_t* p ):
    warlock_spell_t( p, "Haunt" )
  {
    hasted_ticks = false;
    tick_may_crit = false;
    dot_duration += p -> perk.enhanced_haunt -> effectN( 1 ).time_value();
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

      m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  void try_to_trigger_soul_shard_refund()//t16_4pc_bonus
  {
    if ( rng().roll( p() -> sets.set( SET_CASTER, T16, B4 ) -> effectN( 1 ).percent() ) )//refund shard when haunt expires
    {
      p() -> resource_gain( RESOURCE_SOUL_SHARD, p() -> find_spell( 145159 ) -> effectN( 1 ).resource( RESOURCE_SOUL_SHARD ), p() -> gains.haunt_tier16_4pc );
    }
  }

  virtual void impact( action_state_t* s ) override
  {

    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( td( s-> target ) -> debuffs_haunt -> check() )//also can refund on refresh
      {
        try_to_trigger_soul_shard_refund();
      }

      td( s -> target ) -> debuffs_haunt -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, td( s -> target ) -> dots_haunt -> remains() );

      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() * 2 );
    }
  }

  virtual void last_tick( dot_t* d ) override
  {
    warlock_spell_t::last_tick( d );

    try_to_trigger_soul_shard_refund();
  }

  virtual void execute() override
  {
    if ( p() -> talents.soulburn_haunt -> ok() && p() -> buffs.soulburn -> up() )
    {
      p() -> buffs.soulburn -> expire();
      p() -> buffs.haunting_spirits -> trigger();
    }
    warlock_spell_t::execute();
  }
};

struct immolate_t: public warlock_spell_t
{
  immolate_t* fnb;

  immolate_t( warlock_t* p ):
    warlock_spell_t( p, "Immolate" ),
    fnb( new immolate_t( "immolate", p, p -> find_spell( 108686 ) ) )
  {
    havoc_consume = 1;
    base_costs[RESOURCE_MANA] *= 1.0 + p -> spec.chaotic_energy -> effectN( 2 ).percent();
    base_tick_time = p -> find_spell( 157736 ) -> effectN( 1 ).period();
    dot_duration = p -> find_spell( 157736 ) -> duration();
    spell_power_mod.tick = p -> spec.immolate -> effectN( 1 ).sp_coeff();
    hasted_ticks = true;
    tick_may_crit = true;
  }

  immolate_t( const std::string& n, warlock_t* p, const spell_data_t* spell ):
    warlock_spell_t( n, p, spell ),
    fnb( nullptr )
  {
    base_tick_time = p -> find_spell( 157736 ) -> effectN( 1 ).period();
    dot_duration = p -> find_spell( 157736 ) -> duration();
    hasted_ticks = true;
    tick_may_crit = true;
    spell_power_mod.tick = data().effectN( 1 ).sp_coeff();
    aoe = -1;
    stats = p -> get_stats( "immolate_fnb", this );
    gain = p -> get_gain( "immolate_fnb" );
  }

  void schedule_travel( action_state_t* s ) override
  {
    if ( result_is_hit( s -> result ) )
    { // Embers are granted on execute, but are granted depending on the amount of targets hit. 
      if ( s -> result == RESULT_CRIT ) trigger_ember_gain( p(), 0.1, gain );
      if ( p() -> sets.has_set_bonus( WARLOCK_DESTRUCTION, T17, B2 ) )
      {
        trigger_ember_gain( p(), 1, p() -> gains.immolate_t17_2pc, p() -> sets.set( WARLOCK_DESTRUCTION, T17, B2 ) -> effectN( 1 ).percent() );
      }
    }
    warlock_spell_t::schedule_travel( s );
  }

  void init() override
  {
    warlock_spell_t::init();
    spell_power_mod.direct *= 1.0 + p() -> perk.empowered_immolate -> effectN( 1 ).percent();
  }

  void schedule_execute( action_state_t* state ) override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> up() )
      fnb -> schedule_execute( state );
    else
      warlock_spell_t::schedule_execute( state );
  }

  double cost() const override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> check() )
      return fnb -> cost();

    return warlock_spell_t::cost();
  }

  virtual double composite_crit() const override
  {
    double cc = warlock_spell_t::composite_crit();

    if ( p() -> sets.has_set_bonus( SET_CASTER, T16, B2 ) && p() -> buffs.tier16_2pc_destructive_influence -> check() )
      cc += p() -> buffs.tier16_2pc_destructive_influence -> value();

    return cc;
  }

  virtual double composite_persistent_multiplier( const action_state_t* state ) const override
  {
    double m = warlock_spell_t::composite_persistent_multiplier( state );

    // Snapshot the FnB penalty
    if ( p() -> buffs.fire_and_brimstone -> check() )
    {
      m *= p() -> buffs.fire_and_brimstone -> data().effectN( 5 ).percent();
    }

    return m;
  }

  virtual double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = warlock_spell_t::composite_ta_multiplier( state );

    // A rather impressive hack to determine if the immolate dot was cast with FnB or not
    if ( state -> persistent_multiplier < 1 )
    {
      if ( p() -> mastery_spells.emberstorm -> ok() )
      {
        m *= 1.0 + p() -> cache.mastery_value();
      }
    }

    return m;
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> mastery_spells.emberstorm -> ok() )
      m *= 1.0 + p() -> mastery_spells.emberstorm -> effectN( 3 ).percent() + p() -> cache.mastery_value() * p() -> emberstorm_e3_from_e1();

    return m;
  }

  // TODO: FIXME
  /*
  virtual void impact( action_state_t* s )
  {
    warlock_spell_t::impact( s );

    gain_t* gain;
    if ( ! fnb && p() -> spec.fire_and_brimstone -> ok() )
      gain = p() -> gains.immolate_fnb;
    else
      gain = p() -> gains.immolate;
  }
  */

  virtual void tick( dot_t* d ) override
  {
    warlock_spell_t::tick( d );

    gain_t* gain;
    if ( ! fnb && p() -> spec.fire_and_brimstone -> ok() )
      gain = p() -> gains.immolate_fnb;
    else
      gain = p() -> gains.immolate;

    if ( d -> state -> result == RESULT_CRIT ) trigger_ember_gain( p(), 0.1, gain );

    if ( p() -> sets.has_set_bonus( WARLOCK_DESTRUCTION, T17, B2 ) )
    {
      trigger_ember_gain( p(), 1, p() -> gains.immolate_t17_2pc, p() -> sets.set( WARLOCK_DESTRUCTION, T17, B2 ) -> effectN( 1 ).percent() );
    }
    if ( p() -> glyphs.siphon_life -> ok() )
    {
      if ( d -> state -> result_amount > 0 )
        p() -> resource_gain( RESOURCE_HEALTH,
        player -> resources.max[RESOURCE_HEALTH] * p() -> glyphs.siphon_life -> effectN( 1 ).percent() / 100.0,
        p() -> gains.siphon_life );
    }
  }
};

struct conflagrate_t: public warlock_spell_t
{
  conflagrate_t* fnb;

  conflagrate_t( warlock_t* p ):
    warlock_spell_t( p, "Conflagrate" ),
    fnb( new conflagrate_t( "conflagrate", p, p -> find_spell( 108685 ) ) )
  {
    if ( p -> talents.charred_remains -> ok() ){
      base_multiplier *= 1.0 + p -> talents.charred_remains -> effectN( 1 ).percent();
    }
    havoc_consume = 1;
    base_costs[RESOURCE_MANA] *= 1.0 + p -> spec.chaotic_energy -> effectN( 2 ).percent();
  }

  conflagrate_t( const std::string& n, warlock_t* p, const spell_data_t* spell ):
    warlock_spell_t( n, p, spell ),
    fnb( nullptr )
  {
    aoe = -1;
    stats = p -> get_stats( "conflagrate_fnb", this );
    gain = p -> get_gain( "conflagrate_fnb" );
    if ( p -> talents.charred_remains -> ok() )
      base_multiplier *= 1.0 + p -> talents.charred_remains -> effectN( 1 ).percent();
  }

  void schedule_execute( action_state_t* state ) override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> up() )
      fnb -> schedule_execute( state );
    else
      warlock_spell_t::schedule_execute( state );
  }

  double cost() const override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> check() )
      return fnb -> cost();

    return warlock_spell_t::cost();
  }

  void init() override
  {
    warlock_spell_t::init();

    cooldown -> duration = timespan_t::from_seconds( 12.0 );
    cooldown -> charges = 2;
  }

  void schedule_travel( action_state_t* s ) override
  {
    if ( p() -> talents.charred_remains -> ok() )
    {
      trigger_ember_gain( p(), s -> result == RESULT_CRIT ? 0.2 * ( 1.0 + p() -> talents.charred_remains -> effectN( 2 ).percent() ) : 0.1 * ( 1.0 + p() -> talents.charred_remains -> effectN( 2 ).percent() ), gain );
    }
    else
      trigger_ember_gain( p(), s -> result == RESULT_CRIT ? 0.2 : 0.1, gain );
    warlock_spell_t::schedule_travel( s );
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) && p() -> spec.backdraft -> ok() )
      p() -> buffs.backdraft -> trigger( 3 );
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> buffs.fire_and_brimstone -> check() )
    {
      if ( p() -> mastery_spells.emberstorm -> ok() )
        m *= 1.0 + p() -> cache.mastery_value();
      m *= p() -> buffs.fire_and_brimstone -> data().effectN( 5 ).percent();
    }

    if ( p() -> mastery_spells.emberstorm -> ok() )
      m *= 1.0 + p() -> mastery_spells.emberstorm -> effectN( 3 ).percent() + p() -> cache.mastery_value() * p() -> emberstorm_e3_from_e1();

    return m;
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );
    // TODO: FIXME
    //gain_t* gain;

    if ( result_is_hit( s -> result ) )
    {
      /*
      if ( ! fnb && p() -> spec.fire_and_brimstone -> ok() )
        gain = p() -> gains.conflagrate_fnb;
      else
        gain = p() -> gains.conflagrate;
        */

      if ( s -> result == RESULT_CRIT &&  p() -> sets.has_set_bonus( SET_CASTER, T16, B2 ) )
        p() -> buffs.tier16_2pc_destructive_influence -> trigger();
    }
  }

  virtual bool ready() override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> check() )
      return fnb -> ready();
    return warlock_spell_t::ready();
  }
};

struct incinerate_t: public warlock_spell_t
{
  incinerate_t* fnb;
  // Normal incinerate
  incinerate_t( warlock_t* p ):
    warlock_spell_t( p, "Incinerate" ),
    fnb( new incinerate_t( "incinerate", p, p -> find_spell( 114654 ) ) )
  {
    if ( p -> talents.charred_remains -> ok() )
    {
      if ( p -> talents.charred_remains -> ok() )
        base_multiplier *= 1.0 + p -> talents.charred_remains -> effectN( 1 ).percent();
    }
    havoc_consume = 1;
    base_costs[RESOURCE_MANA] *= 1.0 + p -> spec.chaotic_energy -> effectN( 2 ).percent();
  }

  // Fire and Brimstone incinerate
  incinerate_t( const std::string& n, warlock_t* p, const spell_data_t* spell ):
    warlock_spell_t( n, p, spell ),
    fnb( nullptr )
  {
    aoe = -1;
    stats = p -> get_stats( "incinerate_fnb", this );
    gain = p -> get_gain( "incinerate_fnb" );
    if ( p -> talents.charred_remains -> ok() )
    {
      if ( p -> talents.charred_remains -> ok() )
        base_multiplier *= 1.0 + p -> talents.charred_remains -> effectN( 1 ).percent();
    }
  }

  void init() override
  {
    warlock_spell_t::init();

    backdraft_consume = 1;
    base_multiplier *= 1.0 + p() -> sets.set( SET_CASTER, T14, B2 ) -> effectN( 2 ).percent();
  }

  void schedule_execute( action_state_t* state ) override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> up() )
      fnb -> schedule_execute( state );
    else
      warlock_spell_t::schedule_execute( state );
  }

  double cost() const override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> check() )
      return fnb -> cost();

    return warlock_spell_t::cost();
  }

  virtual double composite_crit() const override
  {
    double cc = warlock_spell_t::composite_crit();

    if ( p() -> sets.has_set_bonus( SET_CASTER, T16, B2 ) && p() -> buffs.tier16_2pc_destructive_influence -> check() )
      cc += p() -> buffs.tier16_2pc_destructive_influence -> value();

    return cc;
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> buffs.fire_and_brimstone -> check() )
    {
      if ( p() -> mastery_spells.emberstorm -> ok() )
        m *= 1.0 + p() -> cache.mastery_value();
      m *= p() -> buffs.fire_and_brimstone -> data().effectN( 5 ).percent();
    }

    if ( !p() -> buffs.fire_and_brimstone -> check() )
      m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    m *= 1.0 + p() -> mastery_spells.emberstorm -> effectN( 3 ).percent() + p() -> composite_mastery() * p() -> mastery_spells.emberstorm -> effectN( 3 ).mastery_value();

    return m;
  }

  void schedule_travel( action_state_t * s ) override
  {
    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talents.charred_remains -> ok() )
      {
        trigger_ember_gain( p(), s -> result == RESULT_CRIT ? 0.2 * ( 1.0 + p() -> talents.charred_remains -> effectN( 2 ).percent() ) : 0.1 * ( 1.0 + p() -> talents.charred_remains -> effectN( 2 ).percent() ), gain );
      }
      else
        trigger_ember_gain( p(), s -> result == RESULT_CRIT ? 0.2 : 0.1, gain );

      if ( rng().roll( p() -> sets.set( SET_CASTER, T15, B4 ) -> effectN( 2 ).percent() ) )
        trigger_ember_gain( p(), s -> result == RESULT_CRIT ? 0.2 : 0.1, p() -> gains.incinerate_t15_4pc );
    }
    warlock_spell_t::schedule_travel( s );
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    // TODO: FIXME
    /*
    gain_t* gain;
    if ( ! fnb && p() -> spec.fire_and_brimstone -> ok() )
      gain = p() -> gains.incinerate_fnb;
    else
      gain = p() -> gains.incinerate;
      */

    if ( result_is_hit( s -> result ) )
      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() );

    if ( p() -> destruction_trinket )
    {
      td( s -> target ) -> debuffs_flamelicked -> trigger( 1 );
    }
  }

  virtual bool ready() override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> check() )
      return fnb -> ready();
    return warlock_spell_t::ready();
  }
};

struct soul_fire_t: public warlock_spell_t
{
  warlock_spell_t* meta_spell;

  soul_fire_t( warlock_t* p, bool meta = false ):
    warlock_spell_t( meta ? "soul_fire_meta" : "soul_fire", p, meta ? p -> find_spell( 104027 ) : p -> find_spell( 6353 ) ), meta_spell( nullptr )
  {
    if ( ! meta )
    {
      generate_fury = data().effectN( 2 ).base_value();
      meta_spell = new soul_fire_t( p, true );
    }
    else
    {
      background = true;
    }
  }

  virtual void parse_options( const std::string& options_str ) override
  {
    warlock_spell_t::parse_options( options_str );
    if ( meta_spell ) meta_spell -> parse_options( options_str );
  }

  virtual void execute() override
  {
    if ( meta_spell && p() -> buffs.metamorphosis -> check() )
    {
      meta_spell -> time_to_execute = time_to_execute;
      meta_spell -> execute();
    }
    else
    {
      warlock_spell_t::execute();

      if ( p() -> buffs.demonic_calling -> up() )
      {
        trigger_wild_imp( p() );
        p() -> buffs.demonic_calling -> expire();
      }

      if ( p() -> buffs.molten_core -> check() )
        p() -> buffs.molten_core -> decrement();

      if ( result_is_hit( execute_state -> result ) && target -> health_percentage() < p() -> spec.decimation -> effectN( 1 ).base_value() )
        p() -> buffs.molten_core -> trigger();
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() );
      p() -> buffs.tier18_2pc_demonology -> trigger();
    }
    if ( p() -> sets.has_set_bonus( SET_CASTER, T16, B2 ) )
    {
      p() -> buffs.tier16_2pc_fiery_wrath -> trigger();
    }
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = warlock_spell_t::execute_time();

    if ( p() -> buffs.molten_core -> up() )
      t *= 1.0 + p() -> buffs.molten_core -> data().effectN( 1 ).percent();

    return t;
  }

  // Soul fire always crits
  virtual double composite_crit() const override
  {
    return 1.0;
  }

  virtual double composite_target_crit( player_t* ) const override
  {
    return 0.0;
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> cache.spell_crit();

    return m;
  }

  virtual double cost() const override
  {
    double c = warlock_spell_t::cost();

    if ( p() -> buffs.molten_core -> check() )
      c *= 1.0 + p() -> buffs.molten_core -> data().effectN( 1 ).percent();

    return c;
  }

  virtual bool ready() override
  {
    return ( meta_spell && p() -> buffs.metamorphosis -> check() ) ? meta_spell -> ready() : warlock_spell_t::ready();
  }
};

struct chaos_bolt_t: public warlock_spell_t
{
  chaos_bolt_t* fnb;
  chaos_bolt_t( warlock_t* p ):
    warlock_spell_t( p, "Chaos Bolt" ),
    fnb( new chaos_bolt_t( "chaos_bolt", p, p -> find_spell( 157701 ) ) )
  {
    if ( !p -> talents.charred_remains -> ok() )
      fnb = nullptr;

    havoc_consume = 3;
    backdraft_consume = 3;
    base_execute_time += p -> perk.enhanced_chaos_bolt -> effectN( 1 ).time_value();

    base_multiplier *= 1.0 + ( p -> sets.set( WARLOCK_DESTRUCTION, T18, B2 ) -> effectN( 2 ).percent() );
    base_execute_time += p -> sets.set( WARLOCK_DESTRUCTION, T18, B2 ) -> effectN( 1 ).time_value();

  }

  chaos_bolt_t( const std::string& n, warlock_t* p, const spell_data_t* spell ):
    warlock_spell_t( n, p, spell ),
    fnb( nullptr )
  {
    aoe = -1;
    backdraft_consume = 3;
    base_execute_time += p -> perk.enhanced_chaos_bolt -> effectN( 1 ).time_value();
    radius = 10;
    range = 40;

    base_multiplier *= 1.0 + ( p -> sets.set( WARLOCK_DESTRUCTION, T18, B2 ) -> effectN( 2 ).percent() );
    base_execute_time += ( p -> sets.set( WARLOCK_DESTRUCTION, T18, B2 ) -> effectN( 1 ).time_value() );

    stats = p -> get_stats( "chaos_bolt_fnb", this );
    gain = p -> get_gain( "chaos_bolt_fnb" );
  }

  void schedule_execute( action_state_t* state ) override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> up() )
      fnb -> schedule_execute( state );
    else
      warlock_spell_t::schedule_execute( state );
  }

  void consume_resource() override
  {
    bool t18_procced = rng().roll( p() -> sets.set( WARLOCK_DESTRUCTION, T18, B4 ) -> effectN( 1 ).percent() );
    double base_cost = 0;

    if ( t18_procced )
    {
      base_cost = base_costs[ RESOURCE_BURNING_EMBER ];
      base_costs[ RESOURCE_BURNING_EMBER ] = p() -> buffs.fire_and_brimstone -> check() ? 1 : 0;
      p() -> procs.t18_4pc_destruction -> occur();
    }

    warlock_spell_t::consume_resource();

    if ( t18_procced )
    {
      base_costs[ RESOURCE_BURNING_EMBER ] = base_cost;
    }
  }

  // Force spell to always crit
  double composite_crit() const override
  {
    return 1.0;
  }

  // Record non-crit suppressed target-based crit% to state object
  double composite_target_crit( player_t* target ) const override
  {
    double c = warlock_spell_t::composite_target_crit( target );

    int level_delta = player -> level() - target -> level();
    if ( level_delta < 0 )
    {
      c += abs( level_delta ) / 100.0;
    }

    return c;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    warlock_spell_t::calculate_direct_amount( state );

    // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
    // player spell crit instead. The state target crit chance of the state object is correct.
    // Targeted Crit debuffs function as a separate multiplier.
    state -> result_total *= 1.0 + player -> cache.spell_crit() + state -> target_crit;

    return state -> result_total;
  }

  void multistrike_direct( const action_state_t* source_state, action_state_t* ms_state ) override
  {
    warlock_spell_t::multistrike_direct( source_state, ms_state );

    // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
    // player spell crit instead. The state target crit chance of the state object is correct.
    // Targeted Crit debuffs function as a separate multiplier.
    ms_state -> result_total *= 1.0 + player-> cache.spell_crit() + source_state -> target_crit;
    ms_state -> result_amount = ms_state -> result_total;
  }

  double cost() const override
  {
    double c = warlock_spell_t::cost();

    if ( fnb && p() -> buffs.fire_and_brimstone -> check() )
      return fnb -> cost();

    if ( p() -> buffs.dark_soul -> check() )
      c *= 1.0 + p() -> sets.set( SET_CASTER, T15, B2 ) -> effectN( 2 ).percent();

    return c;
  }

  double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if (p()->buffs.fire_and_brimstone->check())
    {
      m *= p() -> buffs.fire_and_brimstone -> data().effectN( 5 ).percent();
      m *= 1.0 + p() -> cache.mastery_value();
    }

    if ( p() -> mastery_spells.emberstorm -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    if ( !p() -> buffs.fire_and_brimstone -> check() )
      m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    return m;
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( !result_is_hit( execute_state -> result ) ) refund_embers( p() );

    p() -> buffs.chaotic_infusion -> expire();
  }

  //overwrite MS behavior for the T17 4pc buff
  int schedule_multistrike( action_state_t* state, dmg_e type, double tick_multiplier ) override
  {
    if ( !may_multistrike )
      return 0;

    if ( state -> result_amount <= 0 )
      return 0;

    if ( !result_is_hit( state -> result ) )
      return 0;

    int n_strikes = 0;

    // Only first two targets get the T17 4PC bonus
    if ( p() -> buffs.chaotic_infusion -> up() && state -> chain_target <= 1 )
    {
      int extra_ms = static_cast<int>( p() -> buffs.chaotic_infusion -> value() );
      for ( int i = 0; i < extra_ms; i++ )
      {
        result_e r = RESULT_MULTISTRIKE_CRIT;
        action_state_t* ms_state = get_state( state );
        ms_state -> target = state -> target;
        ms_state -> n_targets = 1;
        ms_state -> chain_target = 0;
        ms_state -> result = r;
        // Multistrikes can be blocked
        ms_state -> block_result = calculate_block_result( state );

        multistrike_direct( state, ms_state );

        // Schedule multistrike "execute"; in reality it calls either impact, or
        // assess_damage (for ticks).
        new ( *sim ) multistrike_execute_event_t( ms_state );

        n_strikes++;
      }
    }

    return n_strikes + action_t::schedule_multistrike( state, type, tick_multiplier );
  }
};

struct life_tap_t: public warlock_spell_t
{
  life_tap_t( warlock_t* p ):
    warlock_spell_t( p, "Life Tap" )
  {
    if ( p -> glyphs.life_pact -> ok() )
      background = true;
    harmful = false;
    ignore_false_positive = true;
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    double health = player -> resources.max[RESOURCE_HEALTH];

    // FIXME: Implement reduced healing debuff
    if ( ! p() -> glyphs.life_tap -> ok() ) player -> resource_loss( RESOURCE_HEALTH, health * data().effectN( 3 ).percent() );
    player -> resource_gain( RESOURCE_MANA, health * data().effectN( 1 ).percent(), p() -> gains.life_tap );

  }
};

struct t: public warlock_spell_t
{
  t( warlock_t* p ):
    warlock_spell_t( p, "Metamorphosis" )
  {
    trigger_gcd = timespan_t::zero();
    harmful = false;
    background = true;
    ignore_false_positive = true;
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    assert( cost_event == nullptr );
    p() -> buffs.metamorphosis -> trigger();
    cost_event = new ( *sim ) cost_event_t( p(), this );
  }

  virtual void cancel() override
  {
    warlock_spell_t::cancel();

    if ( p() -> spells.melee ) p() -> spells.melee -> cancel();
    if ( p() -> spells.immolation_aura )
    {
      p() -> spells.immolation_aura -> get_dot() -> cancel();
      p() -> buffs.immolation_aura -> expire();
    }
    p() -> buffs.metamorphosis -> expire();
    event_t::cancel( cost_event );
  }
};

struct activate_t: public warlock_spell_t
{
  activate_t( warlock_t* p ):
    warlock_spell_t( "activate_metamorphosis", p )
  {
    trigger_gcd = timespan_t::zero();
    harmful = false;
    ignore_false_positive = true;

    if ( ! p -> spells.metamorphosis ) p -> spells.metamorphosis = new t( p );
  }

  virtual void execute() override
  {
    if ( ! p() -> buffs.metamorphosis -> check() ) p() -> spells.metamorphosis -> execute();
  }

  virtual bool ready() override
  {
    if ( p() -> specialization() != WARLOCK_DEMONOLOGY )
    {
      return false;
    }

    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.metamorphosis -> check() ) r = false;
    else if ( p() -> resources.current[RESOURCE_DEMONIC_FURY] <= META_FURY_MINIMUM ) r = false;
    else if ( ! p() -> spells.metamorphosis -> ready() ) r = false;

    return r;
  }
};

struct cancel_t: public warlock_spell_t
{
  cancel_t( warlock_t* p ):
    warlock_spell_t( "cancel_metamorphosis", p )
  {
    trigger_gcd = timespan_t::zero();
    harmful = false;
    ignore_false_positive = true;
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    p() -> spells.metamorphosis -> cancel();
  }

  virtual bool ready() override
  {
    if ( p() -> specialization() != WARLOCK_DEMONOLOGY )
    {
      return false;
    }

    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};

struct chaos_wave_dmg_t: public warlock_spell_t
{
  chaos_wave_dmg_t( warlock_t* p ):
    warlock_spell_t( "chaos_wave_dmg", p, p -> find_spell( 124915 ) )
  {
    aoe = -1;
    background = true;
    dual = true;
  }
};

struct chaos_wave_t: public warlock_spell_t
{
  chaos_wave_dmg_t* cw_damage;

  double demonology_trinket_chance;

  chaos_wave_t( warlock_t* p ):
    warlock_spell_t( "chaos_wave", p, p -> find_spell( 124916 ) ),
    cw_damage( nullptr ),
    demonology_trinket_chance( 0.0 )
  {
    cooldown = p -> cooldowns.hand_of_guldan;

    impact_action = new chaos_wave_dmg_t( p );
    impact_action -> stats = stats;

    if ( p -> demonology_trinket && p -> specialization() == WARLOCK_DEMONOLOGY )
    {
      const spell_data_t* data = p -> find_spell( p -> demonology_trinket -> spell_id );
      demonology_trinket_chance = data -> effectN( 1 ).average( p -> demonology_trinket -> item );
      demonology_trinket_chance /= 100.0;
    }
  }

  void execute() override
  {
    warlock_spell_t::execute();

    p() -> buffs.molten_core -> trigger();
    p() -> trigger_demonology_t17_2pc( execute_state );

    if ( p() -> demonology_trinket && p() -> rng().roll( demonology_trinket_chance ) )
    {
      trigger_wild_imp( p() );
      trigger_wild_imp( p() );
      trigger_wild_imp( p() );
      p() -> procs.fragment_wild_imp -> occur();
      p() -> procs.fragment_wild_imp -> occur();
      p() -> procs.fragment_wild_imp -> occur();
    }
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.5 );
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};

struct touch_of_chaos_t: public warlock_spell_t
{
  chaos_wave_t* chaos_wave;
  touch_of_chaos_t( warlock_t* p ):
    warlock_spell_t( "touch_of_chaos", p, p -> find_spell( 103964 ) ), chaos_wave( new chaos_wave_t( p ) )
  {
    base_multiplier *= 1.0 + p -> sets.set( SET_CASTER, T14, B2 ) -> effectN( 3 ).percent();

    chaos_wave               -> background = true;
    chaos_wave               -> base_costs[RESOURCE_DEMONIC_FURY] = 0;
    chaos_wave               -> cooldown = p -> get_cooldown( "t16_4pc_demo" );
    base_tick_time = timespan_t::from_seconds( 2.0 ); //FIX: It got lost in some dbc update. Somebody should try to find it correctly in the dbc.
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() );
      extend_dot( td( s -> target ) -> dots_corruption, 4 * base_tick_time );
    }
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> buffs.demonic_calling -> up() )
    {
      trigger_wild_imp( p() );
      p() -> buffs.demonic_calling -> expire();
    }

    //cast CW
    if ( p() -> sets.has_set_bonus( SET_CASTER, T16, B4 ) && rng().roll( 0.08 ) )
    {
      chaos_wave -> target = target;
      /*
      int current_charge = chaos_wave -> cooldown -> current_charge;
      bool pre_execute_add = true;
      if ( current_charge == chaos_wave -> cooldown -> charges - 1 )
      {
        pre_execute_add = false;
      }
      if ( pre_execute_add ) chaos_wave -> cooldown -> current_charge++;
      */
      chaos_wave -> execute();
      //if ( !pre_execute_add ) chaos_wave -> cooldown -> current_charge++;
    }

  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};

struct drain_soul_t: public warlock_spell_t
{
  bool haunt_t18_4p_bonus;
  drain_soul_t( warlock_t* p ):
    warlock_spell_t( "Drain Soul", p, p -> find_spell( 103103 ) ),
    haunt_t18_4p_bonus( false )
  {
    channeled = true;
    hasted_ticks = false;
    may_crit = false;

    stats -> add_child( p -> get_stats( "agony_ds" ) );
    stats -> add_child( p -> get_stats( "corruption_ds" ) );
    stats -> add_child( p -> get_stats( "unstable_affliction_ds" ) );

    if ( p -> sets.has_set_bonus( WARLOCK_AFFLICTION, T18, B4 ) )
      haunt_t18_4p_bonus = true;
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double m = warlock_spell_t::composite_target_multiplier( t );

    if ( t -> health_percentage() <= 20 )
      m *= 1.0 + p() -> perk.improved_drain_soul -> effectN( 1 ).percent();

    return m;
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p() -> sets.set( SET_CASTER, T15, B4 ) -> effectN( 1 ).percent();

    m *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    if ( p() ->  buffs.tier16_2pc_empowered_grasp -> up() )
    {
      m *= 1.0 + p() ->  buffs.tier16_2pc_empowered_grasp -> value();
    }
    return m;
  }

  virtual void tick( dot_t* d ) override
  {
    warlock_spell_t::tick( d );

    if ( haunt_t18_4p_bonus && p() -> buffs.dark_soul -> check() && td( d -> target ) -> debuffs_haunt -> check() )
    {
      td( d -> target ) -> dots_haunt -> refresh_duration();
      td( d -> target ) -> debuffs_haunt -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, td( d -> target ) -> dots_haunt -> remains() );
    }

    if ( p() -> sets.has_set_bonus( WARLOCK_AFFLICTION, T18, B2 ) && p() -> buffs.dark_soul -> check() )
    {
      if ( rng().roll( p() -> sets.set( WARLOCK_AFFLICTION, T18, B2 ) -> proc_chance() ) )
        p() -> buffs.dark_soul -> extend_duration( p(), p() -> sets.set( WARLOCK_AFFLICTION, T18, B2 ) -> effectN( 1 ).time_value() );
    }

    trigger_soul_leech( p(), d -> state -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() * 2 );

    double multiplier = data().effectN( 3 ).percent();

    multiplier *= 1.0 + p() -> talents.grimoire_of_sacrifice -> effectN( 4 ).percent() * p() -> buffs.grimoire_of_sacrifice -> stack();

    trigger_extra_tick( td( d -> state -> target ) -> dots_agony, multiplier );
    trigger_extra_tick( td( d -> state -> target ) -> dots_corruption, multiplier );
    trigger_extra_tick( td( d -> state -> target ) -> dots_unstable_affliction, multiplier );
  }
};

struct dark_intent_t: public warlock_spell_t
{
  dark_intent_t( warlock_t* p ):
    warlock_spell_t( p, "Dark Intent" )
  {
    harmful = false;
    background = ( sim -> overrides.spell_power_multiplier != 0 );
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    if ( ! sim -> overrides.spell_power_multiplier )
      sim -> auras.spell_power_multiplier -> trigger();
  }
};


struct soulburn_t: public warlock_spell_t
{
  soulburn_t( warlock_t* p ):
    warlock_spell_t( p, "Soulburn" )
  {
    harmful = false;
  }

  virtual void execute() override
  {
    p() -> buffs.soulburn -> trigger();

    warlock_spell_t::execute();
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.soulburn -> check() ) r = false;

    return r;
  }
};


struct dark_soul_t: public warlock_spell_t
{
  dark_soul_t( warlock_t* p ):
    warlock_spell_t( "dark_soul", p, p -> spec.dark_soul )
  {
    harmful = false;

    if ( p -> talents.archimondes_darkness -> ok() && ! p -> glyphs.dark_soul -> ok() ) //AD + noGlyph
    {
      cooldown -> charges = p -> talents.archimondes_darkness -> effectN( 1 ).base_value();
      p -> buffs.dark_soul -> cooldown -> duration = timespan_t::zero();
    }

    else if ( p -> talents.archimondes_darkness -> ok() && p -> glyphs.dark_soul -> ok() ) // AD + glyph
    {
      cooldown -> charges = p -> talents.archimondes_darkness -> effectN( 1 ).base_value(); //give two charges and make sure the buff CD doesn't give us problems
      p -> buffs.dark_soul -> cooldown -> duration = timespan_t::zero();

      cooldown -> duration = data().cooldown() * ( 1.0 + p -> glyphs.dark_soul -> effectN( 1 ).percent() ); //reduce CD of DS
      p -> buffs.dark_soul -> buff_duration = p -> buffs.dark_soul -> data().duration() * ( 1.0 + p -> glyphs.dark_soul -> effectN( 2 ).percent() ); //reduce buff duration
    }

    else if ( !p -> talents.archimondes_darkness -> ok() && p -> glyphs.dark_soul -> ok() ) // noAD + glyph
    {
      cooldown -> duration = data().cooldown() * ( 1.0 + p -> glyphs.dark_soul -> effectN( 1 ).percent() ); //reduce CD of DS
      p -> buffs.dark_soul -> buff_duration = p -> buffs.dark_soul -> data().duration() * ( 1.0 + p -> glyphs.dark_soul -> effectN( 2 ).percent() ); //reduce buff duration
    }

  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    p() -> buffs.dark_soul -> trigger();
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.dark_soul -> check() ) r = false;

    return r;
  }
};


struct imp_swarm_t: public warlock_spell_t
{
  timespan_t base_cooldown;

  imp_swarm_t( warlock_t* p ):
    warlock_spell_t( "imp_swarm", p, ( p -> specialization() == WARLOCK_DEMONOLOGY && p -> glyphs.imp_swarm -> ok() ) ? p -> find_spell( 104316 ) : spell_data_t::not_found() ),
    base_cooldown( cooldown -> duration )
  {
    harmful = false;

    stats -> add_child( p -> get_stats( "firebolt" ) );
  }

  virtual void execute() override
  {
    cooldown -> duration = base_cooldown * composite_haste();

    warlock_spell_t::execute();

    event_t::cancel( p() -> demonic_calling_event );
    p() -> demonic_calling_event = new ( *sim ) warlock_t::demonic_calling_event_t( player, cooldown -> duration, true );

    int imp_count = data().effectN( 1 ).base_value();
    int j = 0;

    for ( size_t i = 0; i < p() -> pets.wild_imps.size(); i++ )
    {
      if ( p() -> pets.wild_imps[i] -> is_sleeping() )
      {
        p() -> pets.wild_imps[i] -> trigger( true );
        if ( ++j == imp_count ) break;
      }
    }
    if ( j != imp_count ) sim -> errorf( "Player %s ran out of wild imps during imp_swarm.\n", p() -> name() );
    assert( j == imp_count );  // Assert fails if we didn't have enough available wild imps
  }
};

struct fire_and_brimstone_t: public warlock_spell_t
{
  fire_and_brimstone_t( warlock_t* p ):
    warlock_spell_t( p, "Fire and Brimstone" )
  {
    harmful = false;
  }

  virtual void execute() override
  {
    assert( player -> resources.current[RESOURCE_BURNING_EMBER] >= 1.0 );

    warlock_spell_t::execute();

    if ( p() -> buffs.fire_and_brimstone -> check() )
      p() -> buffs.fire_and_brimstone -> expire();
    else
      p() -> buffs.fire_and_brimstone -> trigger();
  }

  virtual bool ready() override
  {
    if ( player -> resources.current[RESOURCE_BURNING_EMBER] < 1 )
      return false;

    return warlock_spell_t::ready();
  }
};

// AOE SPELLS

struct seed_of_corruption_aoe_t: public warlock_spell_t
{
  seed_of_corruption_aoe_t( warlock_t* p ):
    warlock_spell_t( "seed_of_corruption_aoe", p, p -> find_spell( 27285 ) )
  {
    aoe = -1;
    dual = true;
    background = true;
    callbacks = false;
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> buffs.mannoroths_fury -> up() )
    {
      m *= 1.0 + p() -> talents.mannoroths_fury -> effectN( 3 ).percent();
    }
    return m;
  }

  virtual void init() override
  {
    warlock_spell_t::init();

    p() -> spells.soulburn_seed_of_corruption_aoe -> stats = stats;
  }
};

struct soulburn_seed_of_corruption_aoe_t: public warlock_spell_t
{
  corruption_t* corruption;

  soulburn_seed_of_corruption_aoe_t( warlock_t* p ):
    warlock_spell_t( "soulburn_seed_of_corruption_aoe", p, p -> find_spell( 27285 ) ), corruption( new corruption_t( p ) )
  {
    aoe = -1;
    dual = true;
    background = true;
    callbacks = false;
    corruption -> background = true;
    corruption -> dual = true;
    corruption -> may_miss = false;
    corruption -> base_costs[RESOURCE_MANA] = 0;
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.seed_of_corruption );
    p() -> shard_react = p() -> sim -> current_time();
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    corruption -> target = s -> target;
    corruption -> execute();
  }
};

struct soulburn_seed_of_corruption_t: public warlock_spell_t
{
  double coefficient;

  soulburn_seed_of_corruption_t( warlock_t* p, double coeff ):
    warlock_spell_t( "soulburn_seed_of_corruption", p, p -> find_spell( 114790 ) ), coefficient( coeff )
  {
    may_crit = false;
    background = true;
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> soulburn_soc_trigger = s -> composite_spell_power() * data().effectN( 1 ).sp_coeff() * 3;
      if (td( s -> target ) -> dots_seed_of_corruption -> is_ticking()) //cancel SoC
      {
          td( s -> target ) -> dots_seed_of_corruption -> cancel();
      }
    }
  }
};

struct seed_of_corruption_t: public warlock_spell_t
{
  soulburn_seed_of_corruption_t* soulburn_spell;

  seed_of_corruption_t( warlock_t* p ):
    warlock_spell_t( "seed_of_corruption", p, p -> find_spell( 27243 ) ), soulburn_spell( new soulburn_seed_of_corruption_t( p, data().effectN( 3 ).sp_coeff() ) )
  {
    may_crit = false;

    if ( ! p -> spells.seed_of_corruption_aoe ) p -> spells.seed_of_corruption_aoe = new seed_of_corruption_aoe_t( p );
    if ( ! p -> spells.soulburn_seed_of_corruption_aoe ) p -> spells.soulburn_seed_of_corruption_aoe = new soulburn_seed_of_corruption_aoe_t( p );

    add_child( p -> spells.seed_of_corruption_aoe );
    soulburn_spell -> stats = stats;
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> soc_trigger = s -> composite_spell_power() * data().effectN( 1 ).sp_coeff() * 3;
        if (td( s -> target ) -> dots_soulburn_seed_of_corruption -> is_ticking()) //cancel SB:SoC
        {
            td( s -> target ) -> dots_soulburn_seed_of_corruption -> cancel();
        }
    }
  }

  virtual void execute() override
  {
    if ( p() -> buffs.soulburn -> up() )
    {
      p() -> buffs.soulburn -> expire();
      soulburn_spell -> target = target;
      soulburn_spell -> time_to_execute = time_to_execute;
      soulburn_spell -> execute();
    }
    else
    {
      warlock_spell_t::execute();
    }
  }
};

struct rain_of_fire_tick_t: public warlock_spell_t
{
  const spell_data_t& parent_data;

  rain_of_fire_tick_t( warlock_t* p, const spell_data_t& pd ):
    warlock_spell_t( "rain_of_fire_tick", p, pd.effectN( 2 ).trigger() ), parent_data( pd )
  {
    aoe = -1;
    background = true;
  }

  virtual proc_types proc_type() const override
  {
    return PROC1_PERIODIC;
  }
};

struct rain_of_fire_t: public warlock_spell_t
{
  rain_of_fire_t( warlock_t* p ):
    warlock_spell_t( "rain_of_fire", p, ( p -> specialization() == WARLOCK_DESTRUCTION ) ? p -> find_spell( 104232 ) : ( p -> specialization() == WARLOCK_AFFLICTION ) ? p -> find_spell( 5740 ) : spell_data_t::not_found() )
  {
    dot_behavior = DOT_CLIP;
    may_miss = false;
    may_crit = false;
    channeled = ( p -> spec.aftermath -> ok() ) ? false : true;
    tick_zero = ( p -> spec.aftermath -> ok() ) ? false : true;
    ignore_false_positive = true;
    trigger_gcd += p -> spec.chaotic_energy -> effectN( 3 ).time_value();

    tick_action = new rain_of_fire_tick_t( p, data() );
  }

  bool consume_cost_per_second( timespan_t tick_time ) override
  {
    if ( channeled )
      return false;
    return warlock_spell_t::consume_cost_per_second( tick_time );
  }

  timespan_t composite_dot_duration( const action_state_t* state ) const override
  { return tick_time( state -> haste ) * ( data().duration() / base_tick_time ); }

  // TODO: Bring Back dot duration haste scaling ?

  virtual double composite_target_ta_multiplier( player_t* t ) const override
  {
    double m = warlock_spell_t::composite_target_ta_multiplier( t );

    if ( td( t ) -> dots_immolate -> is_ticking() )
      m *= 1.0 + data().effectN( 1 ).percent();

    if ( p() -> buffs.mannoroths_fury -> up() )
    {
      m *= 1.0 + p() -> talents.mannoroths_fury -> effectN( 3 ).percent();
    }

    return m;

  }
};

struct hellfire_tick_t: public warlock_spell_t
{
  hellfire_tick_t( warlock_t* p, const spell_data_t& s ):
    warlock_spell_t( "hellfire_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe = -1;
    background = true;
  }


  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      p() -> resource_gain( RESOURCE_DEMONIC_FURY, 3, gain );
  }
};

struct hellfire_t: public warlock_spell_t
{
  hellfire_t( warlock_t* p ):
    warlock_spell_t( p, "Hellfire" )
  {
    tick_zero = false;
    may_miss = false;
    channeled = true;
    may_crit = false;

    spell_power_mod.tick = base_td = 0;

    dynamic_tick_action = true;
    tick_action = new hellfire_tick_t( p, data() );
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> buffs.mannoroths_fury -> up() )
    {
      m *= 1.0 + p() -> talents.mannoroths_fury -> effectN( 3 ).percent();
    }
    return m;
  }

  virtual bool usable_moving() const override
  {
    return true;
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.metamorphosis -> check() ) r = false;
    return r;
  }
};

// Immolation Aura ===================================================================

struct immolation_aura_tick_t: public warlock_spell_t
{
  action_t* parent;
  immolation_aura_tick_t( warlock_t* p, action_t* parent ):
    warlock_spell_t( "immolation_aura_tick", p, p -> find_spell( 129476 ) ),
    parent( parent )
  {
    aoe = -1;
    background = true;
    resource_current = RESOURCE_DEMONIC_FURY;
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> buffs.mannoroths_fury -> up() )
    {
      m *= 1.0 + p() -> talents.mannoroths_fury -> effectN( 3 ).percent();
    }
    return m;
  }
};

struct immolation_aura_t: public warlock_spell_t
{
  immolation_aura_t( warlock_t* p ):
    warlock_spell_t( "immolation_aura", p, p -> find_spell( 104025 ) )
  {
    may_miss = may_crit = callbacks = tick_zero = false;
    hasted_ticks = ignore_false_positive = true;
    dot_duration = data().duration();
    base_tick_time = dot_duration / 10.0;
    tick_action = new immolation_aura_tick_t( p, this );
    tick_action -> base_costs[RESOURCE_DEMONIC_FURY] = base_costs_per_second[RESOURCE_DEMONIC_FURY];
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t tt = tick_time( s -> haste );
    return tt * 10.0;
  }

  void execute() override
  {
    dot_t* d = get_dot( target );

    if ( d -> is_ticking() )
      d -> cancel();
    else
    {
      p() -> spells.immolation_aura = this;
      warlock_spell_t::execute();
      p() -> buffs.immolation_aura -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, composite_dot_duration( execute_state ) );
    }
  }

  void last_tick( dot_t* dot ) override
  {
    warlock_spell_t::last_tick( dot );
    p() -> spells.immolation_aura = nullptr;
    p() -> buffs.immolation_aura -> expire();
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( ! p() -> buffs.metamorphosis -> check() ) r = false;

    return r;
  }
};

struct cataclysm_t: public warlock_spell_t
{
  agony_t* agony;
  unstable_affliction_t* unstable_affliction;
  corruption_t* corruption;
  doom_t* doom;
  immolate_t* immolate;

  cataclysm_t( warlock_t* p ):
    warlock_spell_t( "cataclysm", p, p -> find_spell( 152108 ) ),
    agony( new agony_t( p ) ),
    unstable_affliction( new unstable_affliction_t( p ) ),
    corruption( new corruption_t( p ) ),
    doom( new doom_t( p ) ),
    immolate( new immolate_t( p ) )
  {
    aoe = -1;

    agony               -> background = true;
    agony               -> dual = true;
    agony               -> base_costs[RESOURCE_MANA] = 0;
    unstable_affliction -> background = true;
    unstable_affliction -> dual = true;
    unstable_affliction -> base_costs[RESOURCE_MANA] = 0;
    corruption          -> background = true;
    corruption          -> dual = true;
    corruption          -> base_costs[RESOURCE_MANA] = 0;
    doom          -> background = true;
    doom          -> dual = true;
    doom          -> base_costs[RESOURCE_MANA] = 0;
    doom -> base_costs[RESOURCE_DEMONIC_FURY] = 0; //Celestalons tweet from 30/31.07.2014 says it should be 0
    ignore_false_positive = true;

    immolate -> background = true;
    immolate -> dual = true;
    immolate -> base_costs[RESOURCE_MANA] = 0;

    //TODO Add proper reporting of spawned dots via add_child, etc.
    //stats = p -> get_stats( "immolate_fnb", this );
    //stats -> add_child(p -> get_stats("agony_cata", this));
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      switch ( p() -> specialization() ) {
      case WARLOCK_AFFLICTION:
        agony -> target = s -> target;
        agony -> execute();
        unstable_affliction -> target = s -> target;
        unstable_affliction -> execute();
        break;
      case WARLOCK_DEMONOLOGY:
        if ( p() -> buffs.metamorphosis -> check() )
        {
          doom -> target = s -> target;
          doom -> execute();
        }
        else
        {
          corruption -> target = s -> target;
          corruption -> execute();
        }
        break;
      case WARLOCK_DESTRUCTION:
        immolate -> target = s -> target;
        immolate -> execute();
        break;
      default:
        break;
      }

    }
  }
  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( !p() -> talents.cataclysm -> ok() ) r = false;

    return r;
  }
};

// SOUL SWAP

struct soul_swap_t: public warlock_spell_t
{
  agony_t* agony;
  corruption_t* corruption;
  unstable_affliction_t* unstable_affliction;
  seed_of_corruption_t* seed_of_corruption;

  cooldown_t* glyph_cooldown;

  soul_swap_t( warlock_t* p ):
    warlock_spell_t( "Soul Swap", p, p -> find_spell( 86121 ) ),
    agony( new agony_t( p ) ),
    corruption( new corruption_t( p ) ),
    unstable_affliction( new unstable_affliction_t( p ) ),
    seed_of_corruption( new seed_of_corruption_t( p ) ),
    glyph_cooldown( p -> get_cooldown( "glyphed_soul_swap" ) )
  {
    agony               -> background = true;
    agony               -> dual = true;
    agony               -> base_costs[RESOURCE_MANA] = 0;
    corruption          -> background = true;
    corruption          -> dual = true;
    corruption          -> base_costs[RESOURCE_MANA] = 0;
    unstable_affliction -> background = true;
    unstable_affliction -> dual = true;
    unstable_affliction -> base_costs[RESOURCE_MANA] = 0;
    seed_of_corruption  -> background = true;
    seed_of_corruption  -> dual = true;
    seed_of_corruption  -> base_costs[RESOURCE_MANA] = 0;
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> buffs.soul_swap -> up() )  // EXHALE: Copy(!) the dots from  p() -> soul_swap_buffer.target to target.
    {
      if ( target == p() -> soul_swap_buffer.source ) return;

      p() -> buffs.soul_swap -> expire();

      if ( p() -> soul_swap_buffer.agony_was_inhaled )
      {
          agony -> target = target;
          agony -> execute();
          td( target ) -> agony_stack = p() -> soul_swap_buffer.agony_stack;
          td( target ) -> dots_agony -> trigger(p() -> soul_swap_buffer.agony_remains);

        p() -> soul_swap_buffer.agony_was_inhaled = false;
      }

      if ( p() -> soul_swap_buffer.corruption_was_inhaled )
      {
          corruption -> target = target;
          corruption -> execute();
          td( target ) -> dots_corruption -> trigger(p() -> soul_swap_buffer.corruption_remains);
          p() -> soul_swap_buffer.corruption_was_inhaled = false;
      }
      if ( p() -> soul_swap_buffer.unstable_affliction_was_inhaled )
      {
          corruption -> target = target;
          corruption -> execute();
          td( target ) -> dots_corruption -> trigger(p() -> soul_swap_buffer.corruption_remains);
          p() -> soul_swap_buffer.corruption_was_inhaled = false;

      }
      if ( p() -> soul_swap_buffer.seed_of_corruption_was_inhaled )
      {
          seed_of_corruption -> target = target;
          seed_of_corruption -> execute();
          td( target ) -> dots_seed_of_corruption -> trigger(p() -> soul_swap_buffer.seed_of_corruption_remains);
          p() -> soul_swap_buffer.seed_of_corruption_was_inhaled = false;

      }
    }
    else if ( p() -> buffs.soulburn -> up() ) /// SB:SS, just cast Agony/Corruption/UA no matter what the SS buff is.
    {
      p() -> buffs.soulburn -> expire();

      agony -> target = target;
      agony -> execute();

      corruption -> target = target;
      corruption -> execute();

      unstable_affliction -> target = target;
      unstable_affliction -> execute();

    }
    else //INHALE : Store SS_target and dot ticking state
    {
      p() -> buffs.soul_swap -> trigger();

      p() -> soul_swap_buffer.source = target;

      if ( td( target ) -> dots_agony -> is_ticking() )
      {
          p() -> soul_swap_buffer.agony_was_inhaled = true; //dot is ticking, so copy the state into our buffer dot
          p() -> soul_swap_buffer.agony_remains = td( target ) -> dots_agony -> remains();
          p() -> soul_swap_buffer.agony_stack = td( target ) -> agony_stack;

      }
      if ( td( target ) -> dots_corruption -> is_ticking() )
      {
        p() -> soul_swap_buffer.corruption_was_inhaled = true; //dot is ticking, so copy the state into our buffer dot
        p() -> soul_swap_buffer.corruption_remains = td( target ) -> dots_corruption -> remains();
      }

      if ( td( target ) -> dots_unstable_affliction -> is_ticking() )
      {
        p() -> soul_swap_buffer.unstable_affliction_was_inhaled = true; //dot is ticking, so copy the state into our buffer dot
        p() -> soul_swap_buffer.unstable_affliction_remains = td( target ) -> dots_unstable_affliction -> remains();
      }



      if ( td( target ) -> dots_seed_of_corruption -> is_ticking() )
      {
        p() -> soul_swap_buffer.seed_of_corruption_was_inhaled = true; //dot is ticking, so copy the state into our buffer dot
        p() -> soul_swap_buffer.seed_of_corruption_remains = td( target ) -> dots_seed_of_corruption -> remains();
      }

    }
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> buffs.soul_swap -> check() )
    {
      if ( target == p() -> soul_swap_buffer.source ) r = false;
    }
    else if ( ! p() -> buffs.soulburn -> check() )
    {
      if ( ! td( target ) -> dots_agony               -> is_ticking()
           && ! td( target ) -> dots_corruption          -> is_ticking()
           && ! td( target ) -> dots_unstable_affliction -> is_ticking()
           && ! td( target ) -> dots_seed_of_corruption  -> is_ticking() )
           r = false;
    }

    return r;
  }
};

// SUMMONING SPELLS

struct summon_pet_t: public warlock_spell_t
{
  timespan_t summoning_duration;
  std::string pet_name;
  pets::warlock_pet_t* pet;

private:
  void _init_summon_pet_t()
  {
    util::tokenize( pet_name );
    harmful = false;

    if ( data().ok() &&
         std::find( p() -> pet_name_list.begin(), p() -> pet_name_list.end(), pet_name ) ==
         p() -> pet_name_list.end() )
    {
      p() -> pet_name_list.push_back( pet_name );
    }
  }

public:
  summon_pet_t( const std::string& n, warlock_t* p, const std::string& sname = "" ):
    warlock_spell_t( p, sname.empty() ? "Summon " + n : sname ),
    summoning_duration( timespan_t::zero() ),
    pet_name( sname.empty() ? n : sname ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( const std::string& n, warlock_t* p, int id ):
    warlock_spell_t( n, p, p -> find_spell( id ) ),
    summoning_duration( timespan_t::zero() ),
    pet_name( n ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( const std::string& n, warlock_t* p, const spell_data_t* sd ):
    warlock_spell_t( n, p, sd ),
    summoning_duration( timespan_t::zero() ),
    pet_name( n ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  bool init_finished() override
  {
    pet = debug_cast<pets::warlock_pet_t*>( player -> find_pet( pet_name ) );
    return warlock_spell_t::init_finished();
  }

  virtual void execute() override
  {
    pet -> summon( summoning_duration );

    warlock_spell_t::execute();
  }

  bool ready() override
  {
    if ( ! pet )
    {
      return false;
    }

    return warlock_spell_t::ready();
  }
};

struct summon_main_pet_t: public summon_pet_t
{
  cooldown_t* instant_cooldown;

  summon_main_pet_t( const std::string& n, warlock_t* p ):
    summon_pet_t( n, p ), instant_cooldown( p -> get_cooldown( "instant_summon_pet" ) )
  {
    instant_cooldown -> duration = timespan_t::from_seconds( 60 );
    ignore_false_positive = true;
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    warlock_spell_t::schedule_execute( state );

    if ( p() -> pets.active )
    {
      p() -> pets.active -> dismiss();
      p() -> pets.active = nullptr;
    }
  }

  virtual bool ready() override
  {
    if ( p() -> pets.active == pet )
      return false;

    // FIXME: Currently on beta (2012/05/06) summoning a pet during metamorphosis makes you unable
    //        to summon another one for a full minute, regardless of meta. This may not be intended.
    if ( ( p() -> buffs.soulburn -> check() || p() -> specialization() == WARLOCK_DEMONOLOGY ) && instant_cooldown -> down() )
      return false;

    if ( p() -> talents.demonic_servitude -> ok() ) //if we have the uberpets, we can't summon our standard pets
      return false;
    return summon_pet_t::ready();
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buffs.soulburn -> check() || p() -> buffs.demonic_rebirth -> check() || p() -> buffs.metamorphosis -> check() )
      return timespan_t::zero();

    return warlock_spell_t::execute_time();
  }

  virtual void execute() override
  {
    summon_pet_t::execute();

    p() -> pets.active = p() -> pets.last = pet;

    if ( p() -> buffs.soulburn -> up() )
    {
      instant_cooldown -> start();
      p() -> buffs.soulburn -> expire();
    }

    if ( p() -> buffs.metamorphosis -> check() )
      instant_cooldown -> start();

    if ( p() -> buffs.demonic_rebirth -> up() )
      p() -> buffs.demonic_rebirth -> expire();

    if ( p() -> buffs.grimoire_of_sacrifice -> check() )
      p() -> buffs.grimoire_of_sacrifice -> expire();
  }
};

struct flames_of_xoroth_t: public warlock_spell_t
{
  gain_t* ember_gain;

  flames_of_xoroth_t( warlock_t* p ):
    warlock_spell_t( p, "Flames of Xoroth" ), ember_gain( p -> get_gain( "flames_of_xoroth" ) )
  {
    harmful = false;
  }

  virtual double cost() const override
  {
    if ( p() -> pets.active || p() -> buffs.grimoire_of_sacrifice -> check() )
      return 0;
    else
      return warlock_spell_t::cost();
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    bool gain_ember = false;

    if ( p() -> buffs.grimoire_of_sacrifice -> check() )
    {
      p() -> buffs.grimoire_of_sacrifice -> expire();
      gain_ember = true;
    }
    else if ( p() -> pets.active )
    {
      p() -> pets.active -> dismiss();
      p() -> pets.active = nullptr;
      gain_ember = true;
    }
    else if ( p() -> pets.last )
    {
      p() -> pets.last -> summon();
      p() -> pets.active = p() -> pets.last;
    }

    if ( gain_ember ) p() -> resource_gain( RESOURCE_BURNING_EMBER, 1, ember_gain );
  }
};

struct infernal_awakening_t: public warlock_spell_t
{
  infernal_awakening_t( warlock_t* p, spell_data_t* spell ):
    warlock_spell_t( "infernal_awakening", p, spell )
  {
    aoe = -1;
    background = true;
    dual = true;
    trigger_gcd = timespan_t::zero();
  }
};

struct summon_infernal_t: public summon_pet_t
{
  infernal_awakening_t* infernal_awakening;

  summon_infernal_t( warlock_t* p ):
    summon_pet_t( p -> talents.grimoire_of_supremacy -> ok() ? "abyssal" : "infernal", p ),
    infernal_awakening( nullptr )
  {
    harmful = false;

    cooldown = p -> cooldowns.infernal;
    cooldown -> duration = data().cooldown();

    if ( p -> talents.demonic_servitude -> ok() )
      summoning_duration = timespan_t::from_seconds( -1 );
    else
    {
      summoning_duration = p -> find_spell( 111685 ) -> duration();
      infernal_awakening = new infernal_awakening_t( p, data().effectN( 1 ).trigger() );
      infernal_awakening -> stats = stats;
    }
  }

  virtual void execute() override
  {
    summon_pet_t::execute();

    p() -> cooldowns.doomguard -> start();
    if ( infernal_awakening )
      infernal_awakening -> execute();
  }
};

struct summon_doomguard2_t: public summon_pet_t
{
  summon_doomguard2_t( warlock_t* p, spell_data_t* spell ):
    summon_pet_t( p -> talents.grimoire_of_supremacy -> ok() ? "terrorguard" : "doomguard", p, spell )
  {
    harmful = false;
    background = true;
    dual = true;
    callbacks = false;
    if ( p -> talents.demonic_servitude -> ok() ){
      summoning_duration = timespan_t::from_seconds( -1 );
    }
    else 
      summoning_duration = p -> find_spell( 60478 ) -> duration();
  }
};

struct summon_doomguard_t: public warlock_spell_t
{
  summon_doomguard2_t* summon_doomguard2;

  summon_doomguard_t( warlock_t* p ):
    warlock_spell_t( p, p -> talents.grimoire_of_supremacy -> ok() ? "Summon Terrorguard" : "Summon Doomguard" ),
    summon_doomguard2( nullptr )
  {
    cooldown = p -> cooldowns.doomguard;
    cooldown -> duration = data().cooldown();

    harmful = false;
    summon_doomguard2 = new summon_doomguard2_t( p, data().effectN( 2 ).trigger() );
    summon_doomguard2 -> stats = stats;
  }

  bool init_finished() override
  {
    if ( summon_doomguard2 -> pet )
    {
      summon_doomguard2 -> pet -> summon_stats = stats;
    }

    return warlock_spell_t::init_finished();
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    p() -> cooldowns.infernal -> start();
    summon_doomguard2 -> execute();
  }
};

// TALENT SPELLS

struct mortal_coil_heal_t: public warlock_heal_t
{
  mortal_coil_heal_t( warlock_t* p, const spell_data_t& s ):
    warlock_heal_t( "mortal_coil_heal", p, s.effectN( 3 ).trigger_spell_id() )
  {
    background = true;
    may_miss = false;
  }

  virtual void execute() override
  {
    double heal_pct = data().effectN( 1 ).percent();
    base_dd_min = base_dd_max = player -> resources.max[RESOURCE_HEALTH] * heal_pct;

    warlock_heal_t::execute();
  }
};

struct mortal_coil_t: public warlock_spell_t
{
  mortal_coil_heal_t* heal;

  mortal_coil_t( warlock_t* p ):
    warlock_spell_t( "mortal_coil", p, p -> talents.mortal_coil ), heal( nullptr )
  {
    havoc_consume = 1;
    base_dd_min = base_dd_max = 0;
    heal = new mortal_coil_heal_t( p, data() );
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      heal -> execute();
  }
};

struct grimoire_of_sacrifice_t: public warlock_spell_t
{
  grimoire_of_sacrifice_t( warlock_t* p ):
    warlock_spell_t( "grimoire_of_sacrifice", p, p -> talents.grimoire_of_sacrifice )
  {
    harmful = false;
    ignore_false_positive = true;
  }

  virtual bool ready() override
  {
    if ( ! p() -> pets.active ) return false;

    return warlock_spell_t::ready();
  }

  virtual void execute() override
  {
    if ( p() -> pets.active )
    {
      warlock_spell_t::execute();

      p() -> pets.active -> dismiss();
      p() -> pets.active = nullptr;
      p() -> buffs.grimoire_of_sacrifice -> trigger();

      // FIXME: Demonic rebirth should really trigger on any pet death, but this is the only pet death we care about for now
      if ( p() -> spec.demonic_rebirth -> ok() )
        p() -> buffs.demonic_rebirth -> trigger();

      // FIXME: Add heal event.
    }
  }
};

struct grimoire_of_service_t: public summon_pet_t
{
  grimoire_of_service_t( warlock_t* p, const std::string& pet_name ):
    summon_pet_t( "service_" + pet_name, p, p -> talents.grimoire_of_service -> ok() ? p -> find_class_spell( "Grimoire: " + pet_name ) : spell_data_t::not_found() )
  {
    cooldown = p -> get_cooldown( "grimoire_of_service" );
    cooldown -> duration = data().cooldown();
    summoning_duration = data().duration();
  }

  bool init_finished() override
  {
    if ( pet )
      pet -> summon_stats = stats;

    return summon_pet_t::init_finished();
  }
};

struct mannoroths_fury_t: public warlock_spell_t
{
  mannoroths_fury_t( warlock_t* p ):
    warlock_spell_t( "mannoroths_fury", p, p -> talents.mannoroths_fury )
  {
    harmful = false;
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    p() -> buffs.mannoroths_fury -> trigger();
  }
};

} // end actions namespace

warlock_td_t::warlock_td_t( player_t* target, warlock_t& p ):
actor_target_data_t( target, &p ),
ds_started_below_20( false ),
agony_stack( 1 ),
soc_trigger( 0 ),
soulburn_soc_trigger( 0 ),
warlock( p )
{
  dots_corruption = target -> get_dot( "corruption", &p );
  dots_unstable_affliction = target -> get_dot( "unstable_affliction", &p );
  dots_agony = target -> get_dot( "agony", &p );
  dots_doom = target -> get_dot( "doom", &p );
  dots_drain_soul = target -> get_dot( "drain_soul", &p );
  dots_immolate = target -> get_dot( "immolate", &p );
  dots_shadowflame = target -> get_dot( "shadowflame", &p );
  dots_seed_of_corruption = target -> get_dot( "seed_of_corruption", &p );
  dots_soulburn_seed_of_corruption = target -> get_dot( "soulburn_seed_of_corruption", &p );
  dots_haunt = target -> get_dot( "haunt", &p );

  debuffs_haunt = buff_creator_t( *this, "haunt", source -> find_class_spell( "Haunt" ) )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  debuffs_shadowflame = buff_creator_t( *this, "shadowflame", source -> find_spell( 47960 ) )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  debuffs_agony = buff_creator_t( *this, "agony", source -> find_spell( 980 ) )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  if ( warlock.destruction_trinket )
  {
    debuffs_flamelicked = buff_creator_t( *this, "flamelicked", warlock.destruction_trinket -> driver() -> effectN( 1 ).trigger() )
      .default_value( warlock.destruction_trinket -> driver() -> effectN( 1 ).trigger() -> effectN( 1 ).average( warlock.destruction_trinket -> item ) / 100.0 );
  }
  else
  {
    debuffs_flamelicked = buff_creator_t( *this, "flamelicked" )
      .chance( 0 );
  }

  target -> callbacks_on_demise.push_back( std::bind( &warlock_td_t::target_demise, this ) );
}

void warlock_td_t::target_demise()
{
  if ( warlock.specialization() == WARLOCK_AFFLICTION && dots_drain_soul -> is_ticking() )
  {
    if ( warlock.sim -> log )
    {
      warlock.sim -> out_debug.printf( "Player %s demised. Warlock %s gains a shard by channeling drain soul during this.", target -> name(), warlock.name() );
    }
    warlock.resource_gain( RESOURCE_SOUL_SHARD, 1, warlock.gains.shard_target_death );
  }
}

warlock_t::warlock_t( sim_t* sim, const std::string& name, race_e r ):
  player_t( sim, WARLOCK, name, r ),
    havoc_target( nullptr ),
    latest_corruption_target( nullptr ),
    double_nightfall( 0 ),
    pets( pets_t() ),
    talents( talents_t() ),
    glyphs( glyphs_t() ),
    mastery_spells( mastery_spells_t() ),
    grimoire_of_synergy( *this ),
    grimoire_of_synergy_pet( *this ),
    rppm_chaotic_infusion( *this, std::numeric_limits<double>::min(), 1.0, RPPM_HASTE ),
    perk(),
    cooldowns( cooldowns_t() ),
    spec( specs_t() ),
    buffs( buffs_t() ),
    gains( gains_t() ),
    procs( procs_t() ),
    spells( spells_t() ),
    soul_swap_buffer( soul_swap_buffer_t() ),
    demonic_calling_event( nullptr ),
    initial_burning_embers( 1 ),
    initial_demonic_fury( 200 ),
    default_pet( "" ),
    ember_react( ( initial_burning_embers >= 1.0 ) ? timespan_t::zero() : timespan_t::max() ),
    shard_react( timespan_t::zero() ),
    affliction_trinket( nullptr ),
    demonology_trinket( nullptr ),
    destruction_trinket( nullptr )
{
  base.distance = 40;

  cooldowns.infernal = get_cooldown( "summon_infernal" );
  cooldowns.doomguard = get_cooldown( "summon_doomguard" );
  cooldowns.imp_swarm = get_cooldown( "imp_swarm" );
  cooldowns.hand_of_guldan = get_cooldown( "hand_of_guldan" );
  cooldowns.dark_soul = get_cooldown( "dark_soul" );
  cooldowns.t17_2pc_demonology = get_cooldown( "t17_2pc_demonology" );

  regen_type = REGEN_DYNAMIC;
  regen_caches[CACHE_HASTE] = true;
  regen_caches[CACHE_SPELL_HASTE] = true;
}


double warlock_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.tier16_2pc_fiery_wrath -> up() )
    m *= 1.0 + buffs.tier16_2pc_fiery_wrath -> value();

  if ( buffs.demonic_synergy -> up() )
    m *= 1.0 + buffs.demonic_synergy -> data().effectN( 1 ).percent();

  if ( mastery_spells.master_demonologist -> ok() )
  {
    double mastery = cache.mastery();
    m *= 1.0 + mastery * mastery_spells.master_demonologist -> effectN( 1 ).mastery_value();
  }

  return m;
}

void warlock_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
  case CACHE_MASTERY:
    if ( mastery_spells.master_demonologist -> ok() )
      player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    break;
  default: break;
  }
}

double warlock_t::composite_spell_crit() const
{
  double sc = player_t::composite_spell_crit();

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    if ( buffs.dark_soul -> up() )
      sc += spec.dark_soul -> effectN( 1 ).percent() ;
    if ( buffs.tier16_4pc_ember_fillup -> up() )
      sc += find_spell( 145164 ) -> effectN( 1 ).percent();
  }

  return sc;
}

double warlock_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    if ( buffs.dark_soul -> up() )
    {
      h *= 1.0 / ( 1.0 + spec.dark_soul -> effectN( 1 ).percent() );
    }

  }

  return h;
}

double warlock_t::composite_melee_crit() const
{
  double mc = player_t::composite_melee_crit();

  return mc;
}

double warlock_t::composite_mastery() const
{
  double m = player_t::composite_mastery();

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    if ( buffs.dark_soul -> up() )
    {
      m += spec.dark_soul -> effectN( 1 ).base_value();
    }
  }
  return m;
}

double warlock_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  switch ( rating )
  {
  case RATING_SPELL_HASTE:
    m *= 1.0 + spec.eradication -> effectN( 1 ).percent();
    break;
  case RATING_MELEE_HASTE:
    m *= 1.0 + spec.eradication -> effectN( 1 ).percent();
    break;
  case RATING_RANGED_HASTE:
    m *= 1.0 + spec.eradication -> effectN( 1 ).percent();
    break;
  case RATING_SPELL_CRIT:
    m *= 1.0 + spec.devastation -> effectN( 1 ).percent();
    break;
  case RATING_MELEE_CRIT:
    m *= 1.0 + spec.devastation -> effectN( 1 ).percent();
    break;
  case RATING_RANGED_CRIT:
    m *= 1.0 + spec.devastation -> effectN( 1 ).percent();
    break;
  case RATING_MASTERY:
    return m *= 1.0 + spec.demonic_tactics -> effectN( 1 ).percent();
    break;
  default: break;
  }

  return m;
}

double warlock_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action )
{
  if ( resource_type == RESOURCE_DEMONIC_FURY )
    amount *= 1.0 + sets.set( SET_CASTER, T15, B4 ) -> effectN( 3 ).percent();

  return player_t::resource_gain( resource_type, amount, source, action );
}

double warlock_t::mana_regen_per_second() const
{
  double mp5 = player_t::mana_regen_per_second();

  mp5 /= cache.spell_haste();

  return mp5;
}

double warlock_t::composite_armor() const
{
  return player_t::composite_armor() + spec.fel_armor -> effectN( 2 ).base_value();
}

void warlock_t::halt()
{
  player_t::halt();

  if ( spells.melee ) spells.melee -> cancel();
}

double warlock_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return spec.nethermancy -> effectN( 1 ).percent();

  return 0.0;
}

static const std::string supremacy_pet( const std::string& pet_name, bool translate = true )
{
  if ( ! translate ) return pet_name;
  if ( pet_name == "felhunter" )  return "observer";
  if ( pet_name == "felguard" )   return "wrathguard";
  if ( pet_name == "succubus" )   return "shivarra";
  if ( pet_name == "voidwalker" ) return "voidlord";
  if ( pet_name == "imp" )        return "fel imp";
  return "";
}

action_t* warlock_t::create_action( const std::string& action_name,
                                    const std::string& options_str )
{
  action_t* a;

  if ( ( action_name == "summon_pet" || action_name == "service_pet" ) && default_pet.empty() )
  {
    sim -> errorf( "Player %s used a generic pet summoning action without specifying a default_pet.\n", name() );
    return nullptr;
  }

  using namespace actions;

  if      ( action_name == "conflagrate"           ) a = new           conflagrate_t( this );
  else if ( action_name == "corruption"            ) a = new            corruption_t( this );
  else if ( action_name == "agony"                 ) a = new                 agony_t( this );
  else if ( action_name == "demonbolt"             ) a = new             demonbolt_t( this );
  else if ( action_name == "doom"                  ) a = new                  doom_t( this );
  else if ( action_name == "chaos_bolt"            ) a = new            chaos_bolt_t( this );
  else if ( action_name == "chaos_wave"            ) a = new            chaos_wave_t( this );
  else if ( action_name == "touch_of_chaos"        ) a = new        touch_of_chaos_t( this );
  else if ( action_name == "drain_life"            ) a = new            drain_life_t( this );
  else if ( action_name == "drain_soul"            ) a = new            drain_soul_t( this );
  else if ( action_name == "grimoire_of_sacrifice" ) a = new grimoire_of_sacrifice_t( this );
  else if ( action_name == "haunt"                 ) a = new                 haunt_t( this );
  else if ( action_name == "immolate"              ) a = new              immolate_t( this );
  else if ( action_name == "incinerate"            ) a = new            incinerate_t( this );
  else if ( action_name == "life_tap"              ) a = new              life_tap_t( this );
  else if ( action_name == "metamorphosis"         ) a = new              activate_t( this );
  else if ( action_name == "cancel_metamorphosis"  ) a = new                cancel_t( this );
  else if ( action_name == "mortal_coil"           ) a = new           mortal_coil_t( this );
  else if ( action_name == "shadow_bolt"           ) a = new           shadow_bolt_t( this );
  else if ( action_name == "shadowburn"            ) a = new            shadowburn_t( this );
  else if ( action_name == "soul_fire"             ) a = new             soul_fire_t( this );
  else if ( action_name == "unstable_affliction"   ) a = new   unstable_affliction_t( this );
  else if ( action_name == "hand_of_guldan"        ) a = new        hand_of_guldan_t( this );
  else if ( action_name == "dark_intent"           ) a = new           dark_intent_t( this );
  else if ( action_name == "dark_soul"             ) a = new             dark_soul_t( this );
  else if ( action_name == "soulburn"              ) a = new              soulburn_t( this );
  else if ( action_name == "havoc"                 ) a = new                 havoc_t( this );
  else if ( action_name == "kiljaedens_cunning"    ) a = new    kiljaedens_cunning_t( this );
  else if ( action_name == "seed_of_corruption"    ) a = new    seed_of_corruption_t( this );
  else if ( action_name == "cataclysm"             ) a = new             cataclysm_t( this );
  else if ( action_name == "rain_of_fire"          ) a = new          rain_of_fire_t( this );
  else if ( action_name == "hellfire"              ) a = new              hellfire_t( this );
  else if ( action_name == "immolation_aura"       ) a = new       immolation_aura_t( this );
  else if ( action_name == "imp_swarm"             ) a = new             imp_swarm_t( this );
  else if ( action_name == "fire_and_brimstone"    ) a = new    fire_and_brimstone_t( this );
  else if ( action_name == "soul_swap"             ) a = new             soul_swap_t( this );
  else if ( action_name == "flames_of_xoroth"      ) a = new      flames_of_xoroth_t( this );
  else if ( action_name == "mannoroths_fury"       ) a = new       mannoroths_fury_t( this );
  else if ( action_name == "summon_infernal"       ) a = new       summon_infernal_t( this );
  else if ( action_name == "summon_doomguard"      ) a = new      summon_doomguard_t( this );
  else if ( action_name == "summon_felhunter"      ) a = new summon_main_pet_t( supremacy_pet( "felhunter",  talents.grimoire_of_supremacy -> ok() ), this );
  else if ( action_name == "summon_felguard"       ) a = new summon_main_pet_t( supremacy_pet( "felguard",   talents.grimoire_of_supremacy -> ok() ), this );
  else if ( action_name == "summon_succubus"       ) a = new summon_main_pet_t( supremacy_pet( "succubus",   talents.grimoire_of_supremacy -> ok() ), this );
  else if ( action_name == "summon_voidwalker"     ) a = new summon_main_pet_t( supremacy_pet( "voidwalker", talents.grimoire_of_supremacy -> ok() ), this );
  else if ( action_name == "summon_imp"            ) a = new summon_main_pet_t( supremacy_pet( "imp",        talents.grimoire_of_supremacy -> ok() ), this );
  else if ( action_name == "summon_pet"            ) a = new summon_main_pet_t( supremacy_pet( default_pet,  talents.grimoire_of_supremacy -> ok() ), this );
  else if ( action_name == "service_felguard"      ) a = new grimoire_of_service_t( this, "felguard" );
  else if ( action_name == "service_felhunter"     ) a = new grimoire_of_service_t( this, "felhunter" );
  else if ( action_name == "service_imp"           ) a = new grimoire_of_service_t( this, "imp" );
  else if ( action_name == "service_succubus"      ) a = new grimoire_of_service_t( this, "succubus" );
  else if ( action_name == "service_voidwalker"    ) a = new grimoire_of_service_t( this, "voidwalker" );
  else if ( action_name == "service_infernal"      ) a = new grimoire_of_service_t( this, "infernal" );
  else if ( action_name == "service_doomguard"     ) a = new grimoire_of_service_t( this, "doomguard" );
  else if ( action_name == "service_pet"           ) a = new grimoire_of_service_t( this,  talents.demonic_servitude -> ok() ? "doomguard" : default_pet );
  else return player_t::create_action( action_name, options_str );

  a -> parse_options( options_str );

  return a;
}

pet_t* warlock_t::create_pet( const std::string& pet_name,
                              const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  using namespace pets;

  if ( pet_name == "felguard"     ) return new    felguard_pet_t( sim, this );
  if ( pet_name == "felhunter"    ) return new   felhunter_pet_t( sim, this );
  if ( pet_name == "imp"          ) return new         imp_pet_t( sim, this );
  if ( pet_name == "succubus"     ) return new    succubus_pet_t( sim, this );
  if ( pet_name == "voidwalker"   ) return new  voidwalker_pet_t( sim, this );
  if ( pet_name == "infernal"     ) return new    infernal_pet_t( sim, this );
  if ( pet_name == "doomguard"    ) return new   doomguard_pet_t( sim, this );

  if ( pet_name == "wrathguard"   ) return new  wrathguard_pet_t( sim, this );
  if ( pet_name == "observer"     ) return new    observer_pet_t( sim, this );
  if ( pet_name == "fel_imp"      ) return new     fel_imp_pet_t( sim, this );
  if ( pet_name == "shivarra"     ) return new    shivarra_pet_t( sim, this );
  if ( pet_name == "voidlord"     ) return new    voidlord_pet_t( sim, this );
  if ( pet_name == "abyssal"      ) return new     abyssal_pet_t( sim, this );
  if ( pet_name == "terrorguard"  ) return new terrorguard_pet_t( sim, this );

  if ( pet_name == "service_felguard"     ) return new    felguard_pet_t( sim, this, pet_name );
  if ( pet_name == "service_felhunter"    ) return new   felhunter_pet_t( sim, this, pet_name );
  if ( pet_name == "service_imp"          ) return new         imp_pet_t( sim, this, pet_name );
  if ( pet_name == "service_succubus"     ) return new    succubus_pet_t( sim, this, pet_name );
  if ( pet_name == "service_voidwalker"   ) return new  voidwalker_pet_t( sim, this, pet_name );
  if ( pet_name == "service_doomguard"    ) return new   doomguard_pet_t( sim, this, pet_name );
  if ( pet_name == "service_infernal"     ) return new    infernal_pet_t( sim, this, pet_name );

  return nullptr;
}

void warlock_t::create_pets()
{
  for ( size_t i = 0; i < pet_name_list.size(); ++i )
  {
    create_pet( pet_name_list[ i ] );
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    for ( size_t i = 0; i < pets.wild_imps.size(); i++ )
    {
      pets.wild_imps[ i ] = new pets::wild_imp_pet_t( sim, this );
      if ( i > 0 )
        pets.wild_imps[ i ] -> quiet = 1;
    }
    if ( sets.has_set_bonus( WARLOCK_DEMONOLOGY, T18, B4 ) )
    {
      for ( size_t i = 0; i < pets.t18_illidari_satyr.size(); i++ )
      {
        pets.t18_illidari_satyr[i] = new pets::t18_illidari_satyr_t( sim, this );
      }
      for ( size_t i = 0; i < pets.t18_prince_malchezaar.size(); i++ )
      {
        pets.t18_prince_malchezaar[i] = new pets::t18_prince_malchezaar_t( sim, this );
      }
      for ( size_t i = 0; i < pets.t18_vicious_hellhound.size(); i++ )
      {
        pets.t18_vicious_hellhound[i] = new pets::t18_vicious_hellhound_t( sim, this );
      }
    }

    if ( sets.has_set_bonus( WARLOCK_DEMONOLOGY, T17, B2 ) )
    {
      pets.inner_demon = new pets::inner_demon_t( this );
    }
  }
}

void warlock_t::init_spells()
{
  player_t::init_spells();

  // General
  spec.fel_armor   = find_spell( 104938 );
  spec.nethermancy = find_spell( 86091 );

  // Spezialization Spells
  spec.improved_demons        = find_specialization_spell( "Improved Demons" );
  spec.demonic_tactics        = find_specialization_spell( "Demonic Tactics" );
  spec.devastation            = find_specialization_spell( "Devastation" );
  spec.eradication            = find_specialization_spell( "Eradication" );
  spec.aftermath              = find_specialization_spell( "Aftermath" );
  spec.backdraft              = find_specialization_spell( "Backdraft" );
  spec.backlash               = find_specialization_spell( "Backlash" );
  spec.burning_embers         = find_specialization_spell( "Burning Embers" );
  spec.chaotic_energy         = find_specialization_spell( "Chaotic Energy" );
  spec.decimation             = find_specialization_spell( "Decimation" );
  spec.demonic_fury           = find_specialization_spell( "Demonic Fury" );
  spec.demonic_rebirth        = find_specialization_spell( "Demonic Rebirth" );
  spec.fire_and_brimstone     = find_specialization_spell( "Fire and Brimstone" );
  spec.immolation_aura        = find_specialization_spell( "Metamorphosis: Immolation Aura" );
  spec.imp_swarm              = find_specialization_spell( "Imp Swarm" );
  spec.improved_fear          = find_specialization_spell( "Improved Fear" );
  spec.immolate               = find_specialization_spell( "Immolate" );
  spec.metamorphosis          = find_specialization_spell( "Metamorphosis" );
  spec.molten_core            = find_specialization_spell( "Molten core" );
  spec.nightfall              = find_specialization_spell( "Nightfall" );
  spec.readyness_affliction   = find_specialization_spell( "Readyness: Affliction" );
  spec.readyness_demonology   = find_specialization_spell( "Readyness: Demonology" );
  spec.readyness_destruction  = find_specialization_spell( "Readyness: Destruction" );
  spec.wild_imps              = find_specialization_spell( "Wild Imps" );

  // Removed terniary for compat.
  spec.chaos_wave             = find_spell( 124916 );
  spec.doom                   = find_spell( 603 );
  spec.touch_of_chaos         = find_spell( 103964 );

  spec.dark_soul = find_specialization_spell( "Dark Soul: Instability", "dark_soul" );
  if ( ! spec.dark_soul -> ok() ) spec.dark_soul = find_specialization_spell( "Dark Soul: Knowledge", "dark_soul" );
  if ( ! spec.dark_soul -> ok() ) spec.dark_soul = find_specialization_spell( "Dark Soul: Misery", "dark_soul" );

  // Mastery
  mastery_spells.emberstorm          = find_mastery_spell( WARLOCK_DESTRUCTION );
  mastery_spells.potent_afflictions  = find_mastery_spell( WARLOCK_AFFLICTION );
  mastery_spells.master_demonologist = find_mastery_spell( WARLOCK_DEMONOLOGY );

  // Talents
  talents.dark_regeneration     = find_talent_spell( "Dark Regeneration" );
  talents.soul_leech            = find_talent_spell( "Soul Leech" );
  talents.harvest_life          = find_talent_spell( "Harvest Life" );

  talents.howl_of_terror        = find_talent_spell( "Howl of Terror" );
  talents.mortal_coil           = find_talent_spell( "Mortal Coil" );
  talents.shadowfury            = find_talent_spell( "Shadowfury" );

  talents.soul_link             = find_talent_spell( "Soul Link" );
  talents.sacrificial_pact      = find_talent_spell( "Sacrificial Pact" );
  talents.dark_bargain          = find_talent_spell( "Dark Bargain" );

  talents.blood_horror          = find_talent_spell( "Blood Horror" );
  talents.burning_rush          = find_talent_spell( "Burning Rush" );
  talents.unbound_will          = find_talent_spell( "Unbound Will" );

  talents.grimoire_of_supremacy = find_talent_spell( "Grimoire of Supremacy" );
  talents.grimoire_of_service   = find_talent_spell( "Grimoire of Service" );
  talents.grimoire_of_sacrifice = find_talent_spell( "Grimoire of Sacrifice" );
  talents.grimoire_of_synergy   = find_talent_spell( "Grimoire of Synergy" );

  talents.archimondes_darkness  = find_talent_spell( "Archimonde's Darkness" );
  talents.kiljaedens_cunning    = find_talent_spell( "Kil'jaeden's Cunning" );
  talents.mannoroths_fury       = find_talent_spell( "Mannoroth's Fury" );

  talents.soulburn_haunt        = find_talent_spell( "Soulburn: Haunt" );
  talents.demonbolt             = find_talent_spell( "Demonbolt" );
  talents.charred_remains       = find_talent_spell( "Charred Remains" );
  talents.cataclysm             = find_talent_spell( "Cataclysm" );
  talents.demonic_servitude     = find_talent_spell( "Demonic Servitude" );

  // Perks
  perk.empowered_demons             = find_perk_spell( "Empowered Demons" );
  perk.empowered_doom               = find_perk_spell( "Empowered Doom" );
  perk.empowered_drain_life         = find_perk_spell( "Empowered Drain Life" );
  perk.empowered_immolate           = find_perk_spell( "Empowered Immolate" );
  perk.enhanced_chaos_bolt          = find_perk_spell( "Enhanced Chaos Bolt" );
  perk.enhanced_corruption          = find_perk_spell( "Enhanced Corruption" );
  perk.enhanced_haunt               = find_perk_spell( "Enhanced Haunt" );
  perk.enhanced_havoc               = find_perk_spell( "Enhanced Havoc" );
  perk.empowered_corruption         = find_perk_spell( "Empowered Corruption" );
  perk.improved_drain_soul          = find_perk_spell( "Improved Drain Soul" );
  perk.improved_ember_tap           = find_perk_spell( "Improved Ember Tap" );

  // Glyphs
  glyphs.carrion_swarm          = find_glyph_spell( "Glyph of Carrion Swarm" );
  glyphs.conflagrate            = find_glyph_spell( "Glyph of Conflagrate" );
  glyphs.crimson_banish         = find_glyph_spell( "Glyph of Crimson Banish" );
  glyphs.curse_of_exhaustion    = find_glyph_spell( "Glyph of Curse of Exhaustion" );
  glyphs.curses                 = find_glyph_spell( "Glyph of Curses" );
  glyphs.dark_soul              = find_glyph_spell( "Glyph of Dark Soul" );
  glyphs.demon_hunting          = find_glyph_spell( "Glyph of Demon Hunting" );
  glyphs.demon_training         = find_glyph_spell( "Glyph of Demon Training" );
  glyphs.demonic_circle         = find_glyph_spell( "Glyph of Demonic Circle" );
  glyphs.drain_life             = find_glyph_spell( "Glyph of Drain Life" );
  glyphs.ember_tap              = find_glyph_spell( "Glyph of Ember Tap" );
  glyphs.enslave_demon          = find_glyph_spell( "Glyph of Enslave Demon" );
  glyphs.eternal_resolve        = find_glyph_spell( "Glyph of Eternal Resolve" );
  glyphs.eye_of_kilrogg         = find_glyph_spell( "Glyph of Eye of Kilrogg" );
  glyphs.falling_meteor         = find_glyph_spell( "Glyph of Falling Meteor" );
  glyphs.fear                   = find_glyph_spell( "Glyph of Fear" );
  glyphs.felguard               = find_glyph_spell( "Glyph of Felguard" );
  glyphs.flames_of_xoroth       = find_glyph_spell( "Glyph of Flames of Xoroth" );
  glyphs.gateway_attunement     = find_glyph_spell( "Glyph of Gateway Attunement" );
  glyphs.hand_of_guldan         = find_glyph_spell( "Glyph of Hand of Gul'dan" );
  glyphs.havoc                  = find_glyph_spell( "Glyph of Havoc" );
  glyphs.health_funnel          = find_glyph_spell( "Glyph of Health Funnel" );
  glyphs.healthstone            = find_glyph_spell( "Glyph of Healthstone" );
  glyphs.imp_swarm              = find_glyph_spell( "Glyph of Imp Swarm" );
  glyphs.life_pact              = find_glyph_spell( "Glyph of Life Pact" );
  glyphs.life_tap               = find_glyph_spell( "Glyph of Life Tap" );
  glyphs.metamorphosis          = find_glyph_spell( "Glyph of Metamorphosis" );
  glyphs.nightmares             = find_glyph_spell( "Glyph of Nightmares" );
  glyphs.shadow_bolt            = find_glyph_spell( "Glyph of Shadow Bolt" );
  glyphs.shadowflame            = find_glyph_spell( "Glyph of Shadowflame" );
  glyphs.siphon_life            = find_glyph_spell( "Glyph of Siphon Life" );
  glyphs.soul_consumption       = find_glyph_spell( "Glyph of Soul Consumption" );
  glyphs.soul_swap              = find_glyph_spell( "Glyph of Soul Swap" );
  glyphs.soulstone              = find_glyph_spell( "Glyph of Soulstone" );
  glyphs.soulwell               = find_glyph_spell( "Glyph of Soulwell" );
  glyphs.strengthened_resolve   = find_glyph_spell( "Glyph of Strengthened Resolve" );
  glyphs.subtlety               = find_glyph_spell( "Glyph of Subtlety" );
  glyphs.supernova              = find_glyph_spell( "Glyph of Supernova" );
  glyphs.twilight_ward          = find_glyph_spell( "Glyph of Twilight Ward" );
  glyphs.unending_breath        = find_glyph_spell( "Glyph of Unending Breath" );
  glyphs.unending_resolve       = find_glyph_spell( "Glyph of Unending Resolve" );
  glyphs.unstable_affliction    = find_glyph_spell( "Glyph of Unstable Affliction" );
  glyphs.verdant_spheres        = find_glyph_spell( "Glyph of Verdant Spheres" );

  spec.imp_swarm = ( glyphs.imp_swarm -> ok() ) ? find_spell( 104316 ) : spell_data_t::not_found();

  spells.tier15_2pc = find_spell( 138483 );
  rppm_chaotic_infusion.set_frequency( find_spell( 165452 ) -> real_ppm() );
}

void warlock_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility = 0.0;
  base.spell_power_per_intellect = 1.0;

  base.attribute_multiplier[ATTR_STAMINA] *= 1.0 + spec.fel_armor -> effectN( 1 ).percent();

  base.mana_regen_per_second = resources.base[RESOURCE_MANA] * 0.01;

  base.mana_regen_per_second *= 1.0 + spec.chaotic_energy -> effectN( 1 ).percent();

  if ( specialization() == WARLOCK_AFFLICTION )  resources.base[RESOURCE_SOUL_SHARD] = 4;
  if ( specialization() == WARLOCK_DEMONOLOGY )  resources.base[RESOURCE_DEMONIC_FURY] = 1000;
  if ( specialization() == WARLOCK_DESTRUCTION ) resources.base[RESOURCE_BURNING_EMBER] = 4;

  if ( default_pet.empty() )
  {
    if ( specialization() == WARLOCK_DEMONOLOGY )
      default_pet = "felguard";
    else
      default_pet = "felhunter";
  }
}

void warlock_t::init_scaling()
{
  player_t::init_scaling();
}

struct havoc_buff_t : public buff_t
{
  havoc_buff_t( warlock_t* p ) :
    buff_t( buff_creator_t( p, "havoc", p -> find_specialization_spell( "Havoc" ) ).cd( timespan_t::zero() ) )
  { }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( remaining_duration == timespan_t::zero() )
    {
      debug_cast<warlock_t*>( player ) -> procs.havoc_waste -> occur();
    }
  }
};

struct molten_core_t : public buff_t
{
  timespan_t illidari_satyr_duration;
  timespan_t vicious_hellhound_duration;
  timespan_t prince_malchezaar_duration;

  molten_core_t( warlock_t* p ) :
    buff_t( buff_creator_t( p, "molten_core", p -> find_spell( 122355 ) ).activated( false ).max_stack( 10 ) )
  { 
    prince_malchezaar_duration = p -> find_spell( 189296 ) -> duration();
    vicious_hellhound_duration = p -> find_spell( 189298 ) -> duration();
    illidari_satyr_duration = p -> find_spell( 189297 ) -> duration();
  }

  void execute( int a, double b, timespan_t t ) override
  {
    warlock_t* p = debug_cast<warlock_t*>( player );
    bool trigger_t18_4p = true;

    buff_t::execute( a, b, t );

    if ( trigger_t18_4p && rng().roll( p -> sets.set( WARLOCK_DEMONOLOGY, T18, B4 ) -> effectN( 1 ).percent() ) )
    {
      //Which pet will we spawn?
      double pet = rng().range( 0.0, 1.0 );
      if ( pet <= 0.6 ) // 45% chance to spawn hellhound
      {
        for ( size_t i = 0; i < p -> pets.t18_vicious_hellhound.size(); i++ )
        {
          if ( p -> pets.t18_vicious_hellhound[i] -> is_sleeping() )
          {
            p -> pets.t18_vicious_hellhound[i] -> summon( vicious_hellhound_duration );
            p -> procs.t18_vicious_hellhound -> occur();
            break;
          }
        }
      }
      else // 45% chance to spawn illidari
      {
        for ( size_t i = 0; i < p -> pets.t18_illidari_satyr.size(); i++ )
        {
          if ( p -> pets.t18_illidari_satyr[i] -> is_sleeping() )
          {
            p -> pets.t18_illidari_satyr[i] -> summon( illidari_satyr_duration );
            p -> procs.t18_illidari_satyr -> occur();
            break;
          }
        }
      }
    }
  }
};

void warlock_t::create_buffs()
{
  player_t::create_buffs();

  buffs.backdraft = buff_creator_t( this, "backdraft", spec.backdraft -> effectN( 1 ).trigger() ).max_stack( 6 );
  buffs.dark_soul = buff_creator_t( this, "dark_soul", spec.dark_soul ).add_invalidate( CACHE_CRIT ).add_invalidate( CACHE_HASTE ).add_invalidate( CACHE_MASTERY )
    .cd( timespan_t::zero() );
  buffs.metamorphosis = buff_creator_t( this, "metamorphosis", spec.metamorphosis ).add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.molten_core = new molten_core_t( this );
  buffs.soulburn = buff_creator_t( this, "soulburn", find_class_spell( "Soulburn" ) );
  buffs.grimoire_of_sacrifice = buff_creator_t( this, "grimoire_of_sacrifice", talents.grimoire_of_sacrifice );
  buffs.demonic_synergy = buff_creator_t( this, "demonic_synergy", find_spell( 171982 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .chance( 1 );

  buffs.demonic_calling = buff_creator_t( this, "demonic_calling", spec.wild_imps -> effectN( 1 ).trigger() ).duration( timespan_t::zero() );
  buffs.fire_and_brimstone = buff_creator_t( this, "fire_and_brimstone", spec.fire_and_brimstone )
    .cd( timespan_t::zero() );
  buffs.soul_swap = buff_creator_t( this, "soul_swap", find_spell( 86211 ) );
  buffs.havoc = new havoc_buff_t( this );
  buffs.demonic_rebirth = buff_creator_t( this, "demonic_rebirth", find_spell( 88448 ) ).cd( find_spell( 89140 ) -> duration() );
  buffs.mannoroths_fury = buff_creator_t( this, "mannoroths_fury", talents.mannoroths_fury );

  buffs.immolation_aura = buff_creator_t( this, "immolation_aura", find_spell( 104025 ) );

  buffs.haunting_spirits = buff_creator_t( this, "haunting_spirits", find_spell( 157698 ) )
    .chance( 1.0 )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );

  buffs.kiljaedens_cunning = buff_creator_t( this, "kiljaedens_cunning", talents.kiljaedens_cunning )
    .cd( timespan_t::zero() );

  buffs.demonbolt = buff_creator_t( this, "demonbolt", talents.demonbolt );

  buffs.tier16_4pc_ember_fillup = buff_creator_t( this, "ember_master", find_spell( 145164 ) )
    .cd( find_spell( 145165 ) -> duration() )
    .add_invalidate( CACHE_CRIT );

  buffs.chaotic_infusion = buff_creator_t( this, "chaotic_infusion", find_spell( 170000 ) )
    .default_value( find_spell( 170000 ) -> effectN( 1 ).base_value());

  buffs.tier16_2pc_destructive_influence = buff_creator_t( this, "destructive_influence", find_spell( 145075 ) )
    .chance( sets.set( SET_CASTER, T16, B2 ) -> effectN( 4 ).percent() )
    .duration( timespan_t::from_seconds( 10 ) )
    .default_value( find_spell( 145075 ) -> effectN( 1 ).percent() );

  buffs.tier16_2pc_empowered_grasp = buff_creator_t( this, "empowered_grasp", find_spell( 145082 ) )
    .chance( sets.set( SET_CASTER, T16, B2 ) -> effectN( 2 ).percent() )
    .default_value( find_spell( 145082 ) -> effectN( 2 ).percent() );
  buffs.tier16_2pc_fiery_wrath = buff_creator_t( this, "fiery_wrath", find_spell( 145085 ) )
    .chance( sets.set( SET_CASTER, T16, B2 ) -> effectN( 4 ).percent() )
    .duration( timespan_t::from_seconds( 10 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .default_value( find_spell( 145085 ) -> effectN( 1 ).percent() );
  buffs.tier18_2pc_demonology = buff_creator_t( this, "demon_rush", sets.set( WARLOCK_DEMONOLOGY, T18, B2 ) -> effectN( 1 ).trigger() )
    .default_value( sets.set( WARLOCK_DEMONOLOGY, T18, B2 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );
}

void warlock_t::init_rng()
{
  player_t::init_rng();
  grimoire_of_synergy.set_frequency( 1.333 );
  grimoire_of_synergy_pet.set_frequency( 1.333 );
}

void warlock_t::init_gains()
{
  player_t::init_gains();

  gains.life_tap = get_gain( "life_tap" );
  gains.soul_leech = get_gain( "soul_leech" );
  gains.nightfall = get_gain( "nightfall" );
  gains.incinerate = get_gain( "incinerate" );
  gains.incinerate_fnb = get_gain( "incinerate_fnb" );
  gains.incinerate_t15_4pc = get_gain( "incinerate_t15_4pc" );
  gains.conflagrate = get_gain( "conflagrate" );
  gains.conflagrate_fnb = get_gain( "conflagrate_fnb" );
  gains.rain_of_fire = get_gain( "rain_of_fire" );
  gains.immolate = get_gain( "immolate" );
  gains.immolate_fnb = get_gain( "immolate_fnb" );
  gains.immolate_t17_2pc = get_gain( "immolate_t17_2pc" );
  gains.shadowburn_ember = get_gain( "shadowburn_ember" );
  gains.miss_refund = get_gain( "miss_refund" );
  gains.siphon_life = get_gain( "siphon_life" );
  gains.seed_of_corruption = get_gain( "seed_of_corruption" );
  gains.haunt_tier16_4pc = get_gain( "haunt_tier16_4pc" );
  gains.shard_target_death = get_gain( "shard_target_death" );
}

// warlock_t::init_procs ===============================================

void warlock_t::init_procs()
{
  player_t::init_procs();

  procs.wild_imp = get_proc( "wild_imp" );
  procs.t17_2pc_demo = get_proc( "t17_2pc_demo" );
  procs.havoc_waste = get_proc( "Havoc: Buff expiration" );
  procs.fragment_wild_imp = get_proc( "fragment_wild_imp" );
  procs.t18_4pc_destruction = get_proc( "t18_4pc_destruction" );
  procs.t18_prince_malchezaar = get_proc( "t18_prince_malchezaar" );
  procs.t18_vicious_hellhound = get_proc( "t18_vicious_hellhound" );
  procs.t18_illidari_satyr = get_proc( "t18_illidari_satyr" );
}

void warlock_t::apl_precombat()
{
  std::string& precombat_list =
    get_action_priority_list( "precombat" )->action_list_str;

  if ( sim-> allow_flasks )
  {
    // Flask
    if ( true_level == 100 )
      precombat_list = "flask,type=greater_draenic_intellect_flask";
    else if ( true_level >= 85 )
      precombat_list = "flask,type=warm_sun";
  }

  if ( sim -> allow_food )
  {
    // Food
    if ( level() == 100 && specialization() == WARLOCK_DESTRUCTION )
      precombat_list += "/food,type=pickled_eel";
    else if ( level() == 100 && specialization() == WARLOCK_DEMONOLOGY)
      precombat_list += "/food,type=sleeper_sushi";
    else if ( level() == 100 && specialization() == WARLOCK_AFFLICTION )
      precombat_list += "/food,type=felmouth_frenzy";
    else if ( level() >= 85 )
      precombat_list += "/food,type=mogu_fish_stew";
  }

  add_action( "Dark Intent", "if=!aura.spell_power_multiplier.up",
              "precombat" );

  precombat_list += "/summon_pet,if=!talent.demonic_servitude.enabled&(!talent.grimoire_of_sacrifice.enabled|buff.grimoire_of_sacrifice.down)";

  precombat_list += "/summon_doomguard,if=talent.demonic_servitude.enabled&active_enemies<9";    
  precombat_list += "/summon_infernal,if=talent.demonic_servitude.enabled&active_enemies>=9";
  precombat_list += "/snapshot_stats";

  if ( specialization() != WARLOCK_DEMONOLOGY )
    precombat_list += "/grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled&!talent.demonic_servitude.enabled";

  if ( sim -> allow_potions )
  {
    // Pre-potion
  if ( true_level == 100 )
    precombat_list += "/potion,name=draenic_intellect";
  else if ( true_level >= 85 )
    precombat_list += "/potion,name=jade_serpent";
  }

  if ( specialization() == WARLOCK_DESTRUCTION )
    precombat_list += "/incinerate";

  if ( specialization() == WARLOCK_DEMONOLOGY )
    precombat_list += "/soul_fire";

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    precombat_list += "/soulburn,if=talent.soulburn_haunt.enabled";
    precombat_list += "/haunt";
  }

  add_action( "Summon Doomguard", "if=!talent.demonic_servitude.enabled&spell_targets.infernal_awakening<9" );
  add_action( "Summon Infernal", "if=!talent.demonic_servitude.enabled&spell_targets.infernal_awakening>=9" );

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    action_list_str += "/soulburn,if=!dot.agony.ticking&!dot.corruption.ticking&!dot.unstable_affliction.ticking&buff.soulburn.down&time<10&!talent.cataclysm.enabled";
    action_list_str += "/soul_swap,if=buff.soulburn.remains&!dot.agony.ticking&!dot.corruption.ticking&!dot.unstable_affliction.ticking&time<10&!talent.cataclysm.enabled";
   }

  if ( sim -> allow_potions )
  {
    // Potion
    if ( true_level == 100 && specialization() == WARLOCK_DEMONOLOGY )
    {
      if ( find_item( "nithramus_the_allseer" ) )
        action_list_str += "/potion,name=draenic_intellect,if=buff.bloodlust.remains>30|buff.nithramus.remains>4|(((buff.dark_soul.up&(trinket.proc.any.react|trinket.stacking_proc.any.react>6)&!buff.demonbolt.remains)|target.health.pct<20)&(!talent.grimoire_of_service.enabled|!talent.demonic_servitude.enabled|pet.service_doomguard.active))";
      else
        action_list_str += "/potion,name=draenic_intellect,if=buff.bloodlust.remains>30|(((buff.dark_soul.up&(trinket.proc.any.react|trinket.stacking_proc.any.react>6)&!buff.demonbolt.remains)|target.health.pct<20)&(!talent.grimoire_of_service.enabled|!talent.demonic_servitude.enabled|pet.service_doomguard.active))";
    }
    else if ( true_level == 100 && specialization() == WARLOCK_AFFLICTION )
    {
      if ( find_item( "nithramus_the_allseer" ) )
        action_list_str += "/potion,name=draenic_intellect,if=(target.health.pct<20&buff.nithramus.up)|target.time_to_die<=25";
      else
        action_list_str += "/potion,name=draenic_intellect,if=buff.dark_soul.up&(trinket.proc.any.react|trinket.stacking_proc.any.react)";
    }
    else if ( true_level == 100 )
    {
      if ( find_item( "nithramus_the_allseer" ) )
        action_list_str += "/potion,name=draenic_intellect,if=target.time_to_die<=25|buff.nithramus.remains>4|buff.dark_soul.remains>10|(glyph.dark_soul.enabled&buff.dark_soul.remains)";
      else
        action_list_str += "/potion,name=draenic_intellect,if=target.time_to_die<=25|buff.dark_soul.remains>10|(glyph.dark_soul.enabled&buff.dark_soul.remains)";
    }
  }

  action_list_str += init_use_profession_actions();
  if ( specialization() == WARLOCK_AFFLICTION && find_item( "nithramus_the_allseer" ) )
    action_list_str += "/berserking,if=(target.time_to_die<action.berserking.cooldown&target.health.pct<20&buff.nithramus.up)|target.time_to_die<=10|buff.dark_soul.up&target.health.pct>20";
  else if ( specialization() == WARLOCK_AFFLICTION )
    action_list_str += "/berserking,if=(target.time_to_die<=10|buff.dark_soul.up|set_bonus.tier18_4pc=0)";
  else
    action_list_str += "/berserking";
  action_list_str += "/blood_fury";
  action_list_str += "/arcane_torrent";
  action_list_str += "/mannoroths_fury";

  for ( int i = as< int >( items.size() ) - 1; i >= 0; i-- )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
        action_list_str += "/use_item,name=";
        action_list_str += items[i].name();
    }
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    action_list_str += "/felguard:felstorm";
    action_list_str += "/wrathguard:wrathstorm";
    action_list_str += "/wrathguard:mortal_cleave,if=pet.wrathguard.cooldown.wrathstorm.remains>5";
    action_list_str += "/call_action_list,name=opener,if=time<7&talent.demonic_servitude.enabled";
  }

  action_list_str += "/service_pet,if=talent.grimoire_of_service.enabled&(target.time_to_die>120|target.time_to_die<=25|(buff.dark_soul.remains&target.health.pct<20))";

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    action_list_str += "/dark_soul,if=talent.demonbolt.enabled&((time<=20&!buff.demonbolt.remains&demonic_fury>=360)|target.time_to_die<buff.demonbolt.remains|(!buff.demonbolt.remains&demonic_fury>=790))";
    if ( find_item(  "nithramus_the_allseer" ) )
    {
      action_list_str += "/dark_soul,if=!talent.demonbolt.enabled&((charges=2&(time>6|(debuff.shadowflame.stack=1&action.hand_of_guldan.in_flight)))|!talent.archimondes_darkness.enabled|(target.time_to_die<=20&!glyph.dark_soul.enabled)|target.time_to_die<=10|(target.time_to_die<=60&demonic_fury>400)|((trinket.proc.any.react|trinket.stacking_proc.any.react)&(demonic_fury>600|(glyph.dark_soul.enabled&demonic_fury>450))))|buff.nithramus.remains>4";
      action_list_str += "/imp_swarm,if=!talent.demonbolt.enabled&(buff.dark_soul.up|buff.nithramus.remains>4|(cooldown.dark_soul.remains>(120%(1%spell_haste)))|time_to_die<32)&time>3";
    }
    else
    {
      action_list_str += "/dark_soul,if=!talent.demonbolt.enabled&((charges=2&(time>6|(debuff.shadowflame.stack=1&action.hand_of_guldan.in_flight)))|!talent.archimondes_darkness.enabled|(target.time_to_die<=20&!glyph.dark_soul.enabled)|target.time_to_die<=10|(target.time_to_die<=60&demonic_fury>400)|((trinket.proc.any.react|trinket.stacking_proc.any.react)&(demonic_fury>600|(glyph.dark_soul.enabled&demonic_fury>450))))";
      action_list_str += "/imp_swarm,if=!talent.demonbolt.enabled&(buff.dark_soul.up|(cooldown.dark_soul.remains>(120%(1%spell_haste)))|time_to_die<32)&time>3";
    }
  }

  else if ( specialization() == WARLOCK_AFFLICTION )
  { 
  if ( find_item( "nithramus_the_allseer" ) )
    add_action( spec.dark_soul, "if=(set_bonus.tier18_4pc=1&dot.haunt.remains<=gcd)|!talent.archimondes_darkness.enabled|(talent.archimondes_darkness.enabled&(charges=2|buff.nithramus.remains>4|target.time_to_die<40|trinket.proc.any.react|trinket.stacking_proc.any.react))" );
  else
    add_action( spec.dark_soul, "if=(set_bonus.tier18_4pc=1&dot.haunt.remains<=gcd)|!talent.archimondes_darkness.enabled|(talent.archimondes_darkness.enabled&(charges=2|target.time_to_die<40|trinket.proc.any.react|trinket.stacking_proc.any.react))" );
  }
  else if ( find_item( "nithramus_the_allseer" ) )
    add_action( spec.dark_soul, "if=!talent.archimondes_darkness.enabled|(talent.archimondes_darkness.enabled&(charges=2|buff.nithramus.remains>4|target.time_to_die<40|trinket.proc.any.react|trinket.stacking_proc.any.react))" );
  else
    add_action( spec.dark_soul, "if=!talent.archimondes_darkness.enabled|(talent.archimondes_darkness.enabled&(charges=2|target.time_to_die<40|trinket.proc.any.react|trinket.stacking_proc.any.react))" );

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    action_list_str += "/hand_of_guldan,if=!in_flight&dot.shadowflame.remains<travel_time+action.shadow_bolt.cast_time&(((set_bonus.tier17_4pc=0&((charges=1&recharge_time<4)|charges=2))|(charges=3|(charges=2&recharge_time<13.8-travel_time*2))&((cooldown.cataclysm.remains>dot.shadowflame.duration)|!talent.cataclysm.enabled))|dot.shadowflame.remains>travel_time)";
    action_list_str += "/hand_of_guldan,if=!in_flight&dot.shadowflame.remains<travel_time+action.shadow_bolt.cast_time&talent.demonbolt.enabled&((set_bonus.tier17_4pc=0&((charges=1&recharge_time<4)|charges=2))|(charges=3|(charges=2&recharge_time<13.8-travel_time*2))|dot.shadowflame.remains>travel_time)";
    action_list_str += "/hand_of_guldan,if=!in_flight&dot.shadowflame.remains<3.7&time<5&buff.demonbolt.remains<gcd*2&(charges>=2|set_bonus.tier17_4pc=0)&action.dark_soul.charges>=1";
  }

}

void warlock_t::apl_global_filler()
{
  add_action( "Life Tap" );
}

void warlock_t::apl_default()
{
}

void warlock_t::apl_affliction()
{
  action_list_str += "/kiljaedens_cunning,if=(talent.cataclysm.enabled&!cooldown.cataclysm.remains)";
  action_list_str += "/kiljaedens_cunning,moving=1,if=!talent.cataclysm.enabled";
  action_list_str += "/cataclysm";
  add_action( "Agony", "cycle_targets=1,if=remains<=gcd" );
  add_action( "Corruption", "cycle_targets=1,if=remains<=gcd" );
  add_action( "Unstable Affliction", "cycle_targets=1,if=remains<=cast_time" );

  add_action( "Soulburn", "cycle_targets=1,if=!talent.soulburn_haunt.enabled&spell_targets.seed_of_corruption_aoe>2&dot.corruption.remains<=dot.corruption.duration*0.3" );
  add_action( "Seed of Corruption", "cycle_targets=1,if=!talent.soulburn_haunt.enabled&spell_targets.seed_of_corruption_aoe>2&!dot.seed_of_corruption.remains&buff.soulburn.remains" );
  
  add_action( "Soulburn", "if=shard_react&soul_shard>=2&talent.soulburn_haunt.enabled&buff.soulburn.down&(buff.haunting_spirits.remains-action.haunt.cast_time<=buff.haunting_spirits.duration*0.3)" );
   
  if ( find_item( "nithramus_the_allseer" ) )
    add_action( "Haunt", "if=shard_react&!talent.soulburn_haunt.enabled&!in_flight_to_target&(dot.haunt.remains<duration*0.3+cast_time+travel_time|soul_shard=4)&(buff.nithramus.remains>cast_time+travel_time|trinket.proc.any.react|trinket.stacking_proc.any.react>6|buff.dark_soul.up|soul_shard>2|soul_shard*14<=target.time_to_die)&(buff.dark_soul.down|set_bonus.tier18_4pc=0)" );
  else
    add_action( "Haunt", "if=shard_react&!talent.soulburn_haunt.enabled&!in_flight_to_target&(dot.haunt.remains<duration*0.3+cast_time+travel_time|soul_shard=4)&(trinket.proc.any.react|trinket.stacking_proc.any.react>6|buff.dark_soul.up|soul_shard>2|soul_shard*14<=target.time_to_die)&(buff.dark_soul.down|set_bonus.tier18_4pc=0)" );
  
  add_action( "Haunt", "cycle_targets=1,if=shard_react&!in_flight_to_target&buff.dark_soul.remains>cast_time+travel_time&!dot.haunt.ticking&set_bonus.tier18_4pc=1" );
  add_action( "Haunt", "if=shard_react&talent.soulburn_haunt.enabled&!in_flight_to_target&((buff.soulburn.up&((buff.haunting_spirits.remains-cast_time<=buff.haunting_spirits.duration*0.3&(dot.haunt.remains-cast_time<=dot.haunt.duration*0.3|set_bonus.tier18_4pc=1&buff.dark_soul.remains))|buff.haunting_spirits.down)))" );
 
  if ( find_item( "nithramus_the_allseer" ) )
    add_action( "Haunt", "if=shard_react&talent.soulburn_haunt.enabled&!in_flight_to_target&soul_shard>2&(dot.haunt.remains<duration*0.3+cast_time+travel_time|soul_shard=4)&(buff.nithramus.remains>cast_time+travel_time|trinket.proc.any.react|trinket.stacking_proc.any.react>6|buff.dark_soul.up|soul_shard>2|soul_shard*14<=target.time_to_die)&(buff.dark_soul.down|set_bonus.tier18_4pc=0)" );
  else
    add_action( "Haunt", "if=shard_react&talent.soulburn_haunt.enabled&!in_flight_to_target&soul_shard>2&(dot.haunt.remains<duration*0.3+cast_time+travel_time|soul_shard=4)&(trinket.proc.any.react|trinket.stacking_proc.any.react>6|buff.dark_soul.up|soul_shard>2|soul_shard*14<=target.time_to_die)&(buff.dark_soul.down|set_bonus.tier18_4pc=0)" );
  
  if ( find_item( "nithramus_the_allseer" ) )
  {
    add_action( "Agony", "cycle_targets=1,if=target.time_to_die>16&remains<=(duration*0.3)&(buff.nithramus.remains>8|buff.nithramus.down)&((talent.cataclysm.enabled&remains<=(cooldown.cataclysm.remains+action.cataclysm.cast_time))|!talent.cataclysm.enabled)" );
    add_action( "Unstable Affliction", "cycle_targets=1,if=target.time_to_die>10&remains-cast_time<=(duration*0.3)&(buff.nithramus.remains>5|buff.nithramus.down)&((talent.cataclysm.enabled&remains<=(cooldown.cataclysm.remains+action.cataclysm.cast_time))|!talent.cataclysm.enabled)" );
  }
  else
  {
    add_action( "Agony", "cycle_targets=1,if=target.time_to_die>16&remains<=(duration*0.3)&((talent.cataclysm.enabled&remains<=(cooldown.cataclysm.remains+action.cataclysm.cast_time))|!talent.cataclysm.enabled)" );
    add_action( "Unstable Affliction", "cycle_targets=1,if=target.time_to_die>10&remains-cast_time<=(duration*0.3)&((talent.cataclysm.enabled&remains<=(cooldown.cataclysm.remains+action.cataclysm.cast_time))|!talent.cataclysm.enabled)" );
  }

  add_action( "Seed of Corruption", "cycle_targets=1,if=!talent.soulburn_haunt.enabled&spell_targets.seed_of_corruption_aoe>3&!dot.seed_of_corruption.ticking" );

  if ( find_item( "nithramus_the_allseer" ) )
    add_action( "Corruption", "cycle_targets=1,if=target.time_to_die>12&remains<=(duration*0.3)&(buff.nithramus.remains>5|buff.nithramus.down)" );
  else
    add_action( "Corruption", "cycle_targets=1,if=target.time_to_die>12&remains<=(duration*0.3)" );

  add_action( "Drain Soul", "cycle_targets=1,interrupt=1,if=buff.dark_soul.remains&dot.haunt.ticking&dot.haunt.remains<=dot.haunt.duration*0.3&set_bonus.tier18_4pc=1");
  add_action( "Life Tap","if=mana.pct<30&buff.dark_soul.down" );
  add_action( "Seed of Corruption", "cycle_targets=1,if=spell_targets.seed_of_corruption_aoe>3&!dot.seed_of_corruption.ticking" );
  add_action( "Drain Soul", "interrupt=1,chain=1" );
  add_action( "Agony", "cycle_targets=1,moving=1,if=mana.pct>50" );
}

void warlock_t::apl_demonology()
{
  {

    action_list_str += "/call_action_list,name=db,if=talent.demonbolt.enabled";
    action_list_str += "/call_action_list,name=meta,if=buff.metamorphosis.up";

    get_action_priority_list( "opener" ) -> action_list_str += "hand_of_guldan,if=!in_flight&!dot.shadowflame.ticking";
    get_action_priority_list( "opener" ) -> action_list_str += "/service_pet,if=talent.grimoire_of_service.enabled";
    get_action_priority_list( "opener" ) -> action_list_str += "/corruption,if=!ticking";

    get_action_priority_list( "db" ) -> action_list_str += "/kiljaedens_cunning,moving=1,if=buff.demonbolt.stack=0|(buff.demonbolt.stack<4&buff.demonbolt.remains>=(40*spell_haste-execute_time))";
    get_action_priority_list( "db" ) -> action_list_str += "/call_action_list,name=db_meta,if=buff.metamorphosis.up";

    get_action_priority_list( "db_meta" ) -> action_list_str += "immolation_aura,if=demonic_fury>450&spell_targets.immolation_aura_tick>=5&buff.immolation_aura.down";
    get_action_priority_list( "db_meta" ) -> action_list_str += "/doom,cycle_targets=1,if=active_enemies_within.40>=6&target.time_to_die>=30*spell_haste&remains-gcd<=(duration*0.3)&(buff.dark_soul.down|!glyph.dark_soul.enabled)";
    get_action_priority_list( "db_meta" ) -> action_list_str += "/soul_fire,if=buff.molten_core.react&buff.demon_rush.remains<=4&set_bonus.tier18_2pc=1";
    get_action_priority_list( "db_meta" ) -> action_list_str += "/demonbolt,if=(buff.demonbolt.stack=0|(buff.demonbolt.stack<4&buff.demonbolt.remains>=(40*spell_haste-execute_time)))&(legendary_ring.cooldown.remains>=buff.demonbolt.duration|!legendary_ring.has_cooldown)";
    get_action_priority_list( "db_meta" ) -> action_list_str += "/doom,cycle_targets=1,if=target.time_to_die>=30*spell_haste&remains<=(duration*0.3)&(buff.dark_soul.down|!glyph.dark_soul.enabled)";
    get_action_priority_list( "db_meta" ) -> action_list_str += "/soul_fire,if=buff.molten_core.react&(buff.demon_rush.remains<=execute_time+travel_time+action.touch_of_chaos.execute_time)&set_bonus.tier18_2pc=1";

      if ( find_item( "prophecy_of_fear" ) )
    get_action_priority_list( "db_meta" ) -> action_list_str += "/touch_of_chaos,if=debuff.mark_of_doom.remains";
    get_action_priority_list( "db_meta" ) -> action_list_str += "/cancel_metamorphosis,if=buff.demonbolt.stack>3&demonic_fury<=600&target.time_to_die>buff.demonbolt.remains&buff.dark_soul.down";
    get_action_priority_list( "db_meta" ) -> action_list_str += "/chaos_wave,if=buff.dark_soul.up&spell_targets.chaos_wave>=2&demonic_fury>450";
    get_action_priority_list( "db_meta" ) -> action_list_str += "/soul_fire,if=buff.molten_core.react&(((buff.dark_soul.remains>execute_time)&demonic_fury>=175)|(target.time_to_die<buff.demonbolt.remains))";
    get_action_priority_list( "db_meta" ) -> action_list_str += "/soul_fire,if=buff.molten_core.react&target.health.pct<=25&(((demonic_fury-80)%800)>(buff.demonbolt.remains%40))&demonic_fury>=750";
    get_action_priority_list( "db_meta" ) -> action_list_str += "/soul_fire,if=buff.molten_core.react&buff.demon_rush.stack<5&set_bonus.tier18_2pc=1";
    get_action_priority_list( "db_meta" ) -> action_list_str += "/touch_of_chaos,cycle_targets=1,if=dot.corruption.remains<17.4&demonic_fury>750";
    get_action_priority_list( "db_meta" ) -> action_list_str += "/touch_of_chaos,if=(target.time_to_die<buff.demonbolt.remains|(demonic_fury>=750&buff.demonbolt.remains)|buff.dark_soul.up)";
    get_action_priority_list( "db_meta" ) -> action_list_str += "/touch_of_chaos,if=(((demonic_fury-40)%800)>(buff.demonbolt.remains%40))&demonic_fury>=750";
    get_action_priority_list( "db_meta" ) -> action_list_str += "/cancel_metamorphosis";

    get_action_priority_list( "db" ) -> action_list_str += "/corruption,cycle_targets=1,if=target.time_to_die>=6&remains<=(0.3*duration)&buff.metamorphosis.down";
    get_action_priority_list( "db" ) -> action_list_str += "/metamorphosis,if=buff.dark_soul.remains>gcd&(demonic_fury>=470|buff.dark_soul.remains<=action.demonbolt.execute_time*3)&(buff.demonbolt.down|target.time_to_die<buff.demonbolt.remains|(buff.dark_soul.remains>execute_time&demonic_fury>=175))";
    get_action_priority_list( "db" ) -> action_list_str += "/metamorphosis,if=buff.demonbolt.down&demonic_fury>=480&(action.dark_soul.charges=0|!talent.archimondes_darkness.enabled&cooldown.dark_soul.remains)&(legendary_ring.cooldown.remains>=buff.demonbolt.duration|!legendary_ring.has_cooldown)";
    get_action_priority_list( "db" ) -> action_list_str += "/metamorphosis,if=(demonic_fury%80)*2*spell_haste>=target.time_to_die&target.time_to_die<buff.demonbolt.remains";
    get_action_priority_list( "db" ) -> action_list_str += "/metamorphosis,if=target.time_to_die>=30*spell_haste&!dot.doom.ticking&buff.dark_soul.down&time>10";
    get_action_priority_list( "db" ) -> action_list_str += "/metamorphosis,if=demonic_fury>750&buff.demonbolt.remains>=action.metamorphosis.cooldown";
    get_action_priority_list( "db" ) -> action_list_str += "/metamorphosis,if=(((demonic_fury-120)%800)>(buff.demonbolt.remains%40))&buff.demonbolt.remains>=10&dot.doom.remains-gcd<=dot.doom.duration*0.3";
    get_action_priority_list( "db" ) -> action_list_str += "/metamorphosis,if=buff.demonbolt.remains&buff.demonbolt.remains<10&demonic_fury-((40*action.touch_of_chaos.gcd*buff.demonbolt.remains)+(6*buff.demonbolt.remains))>=800";
    
    if ( find_item( "prophecy_of_fear" ) )
      get_action_priority_list( "db" ) -> action_list_str += "/metamorphosis,if=buff.demonbolt.remains>10&debuff.mark_of_doom.remains&demonic_fury>=40*action.touch_of_chaos.gcd*debuff.mark_of_doom.remains";
    get_action_priority_list( "db" ) -> action_list_str += "/imp_swarm";
    get_action_priority_list( "db" ) -> action_list_str += "/hellfire,interrupt=1,if=spell_targets.hellfire_tick>=5";
    get_action_priority_list( "db" ) -> action_list_str += "/soul_fire,if=buff.molten_core.react&(buff.demon_rush.remains<=execute_time+travel_time+action.shadow_bolt.execute_time|buff.demon_rush.stack<5)&set_bonus.tier18_2pc=1";
    get_action_priority_list( "db" ) -> action_list_str += "/soul_fire,if=((set_bonus.tier18_2pc=0&buff.molten_core.react)|buff.molten_core.react>1)&(buff.dark_soul.remains<action.shadow_bolt.cast_time|buff.dark_soul.remains>cast_time)";
    get_action_priority_list( "db" ) -> action_list_str += "/life_tap,if=mana.pct<40&buff.dark_soul.down";
    get_action_priority_list( "db" ) -> action_list_str += "/hellfire,interrupt=1,if=spell_targets.hellfire_tick>=4";
    get_action_priority_list( "db" ) -> action_list_str += "/shadow_bolt";
    get_action_priority_list( "db" ) -> action_list_str += "/hellfire,moving=1,interrupt=1";
    get_action_priority_list( "db" ) -> action_list_str += "/life_tap";

    get_action_priority_list( "meta" ) -> action_list_str += "/kiljaedens_cunning,if=!cooldown.cataclysm.remains";
    get_action_priority_list( "meta" ) -> action_list_str += "/cataclysm,if=(active_enemies=1|spell_targets.cataclysm>1)";
    get_action_priority_list( "meta" ) -> action_list_str += "/immolation_aura,if=demonic_fury>450&spell_targets.immolation_aura_tick>=3&buff.immolation_aura.down";
    get_action_priority_list( "meta" ) -> action_list_str += "/doom,if=target.time_to_die>=30*spell_haste&remains<=(duration*0.3)&(remains<cooldown.cataclysm.remains|!talent.cataclysm.enabled)&trinket.stacking_proc.any.react<10";
    get_action_priority_list( "meta" ) -> action_list_str += "/soul_fire,if=buff.molten_core.react&(buff.demon_rush.remains<=execute_time+travel_time+action.touch_of_chaos.execute_time)&set_bonus.tier18_2pc=1";
    get_action_priority_list( "meta" ) -> action_list_str += "/chaos_wave,if=buff.dark_soul.remains&(trinket.proc.crit.react|trinket.proc.mastery.react|trinket.proc.intellect.react|trinket.proc.multistrike.react|trinket.stacking_proc.multistrike.react>7)";

    if ( find_item( "prophecy_of_fear" ) )
      get_action_priority_list( "meta" ) -> action_list_str += "/touch_of_chaos,if=debuff.mark_of_doom.remains";
    if ( find_item("draenic_philosophers_stone" ) )
    {
      get_action_priority_list( "meta" ) -> action_list_str += "/cancel_metamorphosis,if=((demonic_fury<650&!glyph.dark_soul.enabled)|demonic_fury<450)&buff.dark_soul.down&(trinket.stacking_proc.any.down&trinket.proc.any.down&buff.draenor_philosophers_stone_int.down|demonic_fury<(800-cooldown.dark_soul.remains*(10%spell_haste)))&target.time_to_die>20";
    }
    else
      get_action_priority_list( "meta" ) -> action_list_str += "/cancel_metamorphosis,if=((demonic_fury<650&!glyph.dark_soul.enabled)|demonic_fury<450)&buff.dark_soul.down&(trinket.stacking_proc.any.down&trinket.proc.any.down|demonic_fury<(800-cooldown.dark_soul.remains*(10%spell_haste)))&target.time_to_die>20";
    get_action_priority_list( "meta" ) -> action_list_str += "/cancel_metamorphosis,if=action.hand_of_guldan.charges>0&dot.shadowflame.remains<action.hand_of_guldan.travel_time+action.shadow_bolt.cast_time&((demonic_fury<100&buff.dark_soul.remains>10)|time<15)&!glyph.dark_soul.enabled";
    get_action_priority_list( "meta" ) -> action_list_str += "/cancel_metamorphosis,if=action.hand_of_guldan.charges=3&(!buff.dark_soul.remains>gcd|action.metamorphosis.cooldown<gcd)";
    get_action_priority_list( "meta" ) -> action_list_str += "/chaos_wave,if=buff.dark_soul.up&spell_targets.chaos_wave>=2|(charges=3|set_bonus.tier17_4pc=0&charges=2)";
    get_action_priority_list( "meta" ) -> action_list_str += "/soul_fire,if=buff.molten_core.react&(buff.dark_soul.remains>execute_time|target.health.pct<=25|trinket.proc.crit.react|trinket.proc.mastery.react|trinket.proc.intellect.react|trinket.proc.multistrike.react)";
    get_action_priority_list( "meta" ) -> action_list_str += "/soul_fire,if=buff.molten_core.react&trinket.stacking_proc.multistrike.react&trinket.stacking_proc.multistrike.remains<=buff.molten_core.react*cast_time&trinket.stacking_proc.multistrike.remains<=demonic_fury%(80%cast_time)";
    get_action_priority_list( "meta" ) -> action_list_str += "/soul_fire,if=buff.molten_core.react&buff.demon_rush.stack<5&set_bonus.tier18_2pc=1";
    get_action_priority_list( "meta" ) -> action_list_str += "/touch_of_chaos,cycle_targets=1,if=dot.corruption.remains<17.4&demonic_fury>750";
    get_action_priority_list( "meta" ) -> action_list_str += "/touch_of_chaos";
    get_action_priority_list( "meta" ) -> action_list_str += "/cancel_metamorphosis";

    action_list_str += "/corruption,cycle_targets=1,if=target.time_to_die>=6&remains<=(0.3*duration)&buff.metamorphosis.down";
    if ( find_item( "nithramus_the_allseer" ) )
      action_list_str += "/metamorphosis,if=buff.nithramus.remains>4&demonic_fury>=80*action.soul_fire.gcd*buff.nithramus.remains";
    if ( find_item( "prophecy_of_fear" ) )
      action_list_str += "/metamorphosis,if=debuff.mark_of_doom.remains&demonic_fury>=40*action.touch_of_chaos.gcd*debuff.mark_of_doom.remains";
    action_list_str += "/metamorphosis,if=buff.dark_soul.remains>gcd&(time>6|debuff.shadowflame.stack=2)&(demonic_fury>300|!glyph.dark_soul.enabled)&(demonic_fury>=80&buff.molten_core.stack>=1|demonic_fury>=40)";
    action_list_str += "/metamorphosis,if=(trinket.stacking_proc.any.react|trinket.proc.any.react)&((demonic_fury>450&action.dark_soul.recharge_time>=10&glyph.dark_soul.enabled)|(demonic_fury>650&cooldown.dark_soul.remains>=10))";
    action_list_str += "/metamorphosis,if=!cooldown.cataclysm.remains&talent.cataclysm.enabled";
    action_list_str += "/metamorphosis,if=!dot.doom.ticking&target.time_to_die>=30%(1%spell_haste)&demonic_fury>300";
    action_list_str += "/metamorphosis,if=(demonic_fury>750&(action.hand_of_guldan.charges=0|(!dot.shadowflame.ticking&!action.hand_of_guldan.in_flight_to_target)))|floor(demonic_fury%80)*action.soul_fire.execute_time>=target.time_to_die";
    action_list_str += "/metamorphosis,if=demonic_fury>=950";
    action_list_str += "/imp_swarm";
    action_list_str += "/hellfire,interrupt=1,if=spell_targets.hellfire_tick>=5";
    action_list_str += "/soul_fire,if=buff.molten_core.react&(buff.demon_rush.remains<=execute_time+travel_time+action.shadow_bolt.execute_time|buff.demon_rush.stack<5)&set_bonus.tier18_2pc=1";
    action_list_str += "/soul_fire,if=buff.molten_core.react&(buff.molten_core.stack>=7|target.health.pct<=25|(buff.dark_soul.remains&cooldown.metamorphosis.remains>buff.dark_soul.remains)|trinket.proc.any.remains>execute_time|trinket.stacking_proc.any.remains>execute_time)&(buff.dark_soul.remains<action.shadow_bolt.cast_time|buff.dark_soul.remains>execute_time)";
    action_list_str += "/soul_fire,if=buff.molten_core.react&target.time_to_die<(time+target.time_to_die)*0.25+cooldown.dark_soul.remains";
    action_list_str += "/life_tap,if=mana.pct<40&buff.dark_soul.down";
    action_list_str += "/hellfire,interrupt=1,if=spell_targets.hellfire_tick>=4";
    action_list_str += "/shadow_bolt";
    action_list_str += "/hellfire,moving=1,interrupt=1";
  }
}

void warlock_t::apl_destruction()
{
  action_priority_list_t* single_target       = get_action_priority_list( "single_target" );    
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe" );

  action_list_str +="/run_action_list,name=single_target,if=spell_targets.fire_and_brimstone<6&(!talent.charred_remains.enabled|spell_targets.rain_of_fire<4)";
  action_list_str +="/run_action_list,name=aoe,if=spell_targets.fire_and_brimstone>=6|(talent.charred_remains.enabled&spell_targets.rain_of_fire>=4)";

  single_target -> action_list_str += "/havoc,if=raid_event.adds.exists";
  single_target -> action_list_str += "/havoc,target=2,if=!raid_event.adds.exists";
  single_target -> action_list_str += "/shadowburn,cycle_targets=1,if=raid_event.adds.exists&sim.target!=target&talent.charred_remains.enabled&target.time_to_die<10";
  single_target -> action_list_str += "/shadowburn,if=talent.charred_remains.enabled&target.time_to_die<10";
  single_target -> action_list_str += "/kiljaedens_cunning,if=(talent.cataclysm.enabled&!cooldown.cataclysm.remains)";
  single_target -> action_list_str += "/kiljaedens_cunning,moving=1,if=!talent.cataclysm.enabled";
  single_target -> action_list_str += "/cataclysm,if=spell_targets.cataclysm>1";
  single_target -> action_list_str += "/fire_and_brimstone,if=buff.fire_and_brimstone.down&dot.immolate.remains<=action.immolate.cast_time&(cooldown.cataclysm.remains>action.immolate.cast_time|!talent.cataclysm.enabled)&spell_targets.fire_and_brimstone>4";
  
  if ( find_item( "fragment_of_the_dark_star" ) )
  {
    single_target -> action_list_str += "/immolate,cycle_targets=1,if=raid_event.adds.exists&(sim.target=target|!buff.havoc.remains)&remains<=cast_time&(cooldown.cataclysm.remains>cast_time|!talent.cataclysm.enabled)&debuff.flamelicked.remains>=(action.incinerate.execute_time+action.incinerate.travel_time+execute_time)";
    single_target -> action_list_str += "/immolate,cycle_targets=1,if=!raid_event.adds.exists&remains<=cast_time&(cooldown.cataclysm.remains>cast_time|!talent.cataclysm.enabled)";
  }
  else
    single_target -> action_list_str += "/immolate,cycle_targets=1,if=(sim.target=target|!buff.havoc.remains|!raid_event.adds.exists)&remains<=cast_time&(cooldown.cataclysm.remains>cast_time|!talent.cataclysm.enabled)";
  
  single_target -> action_list_str += "/cancel_buff,name=fire_and_brimstone,if=buff.fire_and_brimstone.up&dot.immolate.remains-action.immolate.cast_time>(dot.immolate.duration*0.3)";
  single_target -> action_list_str += "/shadowburn,cycle_targets=1,if=raid_event.adds.exists&sim.target!=target&buff.havoc.remains";
  single_target -> action_list_str += "/chaos_bolt,cycle_targets=1,if=raid_event.adds.exists&sim.target!=target&buff.havoc.remains>cast_time&buff.havoc.stack>=3&target.time_to_die>=12";
  single_target -> action_list_str += "/shadowburn,if=!raid_event.adds.exists&buff.havoc.remains";
  single_target -> action_list_str += "/chaos_bolt,if=!raid_event.adds.exists&buff.havoc.remains>cast_time&buff.havoc.stack>=3";
  single_target -> action_list_str += "/conflagrate,cycle_targets=1,if=raid_event.adds.exists&sim.target!=target&buff.havoc.remains<=gcd*3&charges=2";
  single_target -> action_list_str += "/conflagrate,if=charges=2";
  single_target -> action_list_str += "/cataclysm";
  single_target -> action_list_str += "/rain_of_fire,if=remains<=tick_time&(spell_targets.rain_of_fire>4|(buff.mannoroths_fury.up&spell_targets.rain_of_fire>2))";
  
  if ( find_item( "fragment_of_the_dark_star" ) )
    single_target -> action_list_str += "/incinerate,if=raid_event.adds.exists&debuff.flamelicked.remains<=execute_time+travel_time+action.chaos_bolt.execute_time";

  single_target -> action_list_str += "/chaos_bolt,if=talent.charred_remains.enabled&spell_targets.fire_and_brimstone>1&target.health.pct>20";
  single_target -> action_list_str += "/chaos_bolt,if=talent.charred_remains.enabled&buff.backdraft.stack<3&burning_ember>=2.5";

  if ( find_item( "nithramus_the_allseer" ) )
    single_target -> action_list_str += "/chaos_bolt,if=buff.backdraft.stack<3&(burning_ember>=3.5|buff.dark_soul.up|target.time_to_die<20|buff.nithramus.remains>cast_time+travel_time)";
  else
    single_target -> action_list_str += "/chaos_bolt,if=buff.backdraft.stack<3&(burning_ember>=3.5|buff.dark_soul.up|target.time_to_die<20)";

  single_target -> action_list_str += "/chaos_bolt,if=buff.backdraft.stack<3&set_bonus.tier17_2pc=1&burning_ember>=2.5";
  single_target -> action_list_str += "/chaos_bolt,if=buff.backdraft.stack<3&buff.archmages_greater_incandescence_int.react&buff.archmages_greater_incandescence_int.remains>cast_time";
  single_target -> action_list_str += "/chaos_bolt,if=buff.backdraft.stack<3&trinket.proc.intellect.react&trinket.proc.intellect.remains>cast_time";
  single_target -> action_list_str += "/chaos_bolt,if=buff.backdraft.stack<3&trinket.proc.crit.react&trinket.proc.crit.remains>cast_time";
  single_target -> action_list_str += "/chaos_bolt,if=buff.backdraft.stack<3&trinket.stacking_proc.multistrike.react>=8&trinket.stacking_proc.multistrike.remains>=cast_time";
  single_target -> action_list_str += "/chaos_bolt,if=buff.backdraft.stack<3&trinket.proc.multistrike.react&trinket.proc.multistrike.remains>cast_time";
  single_target -> action_list_str += "/chaos_bolt,if=buff.backdraft.stack<3&trinket.proc.versatility.react&trinket.proc.versatility.remains>cast_time";
  single_target -> action_list_str += "/chaos_bolt,if=buff.backdraft.stack<3&trinket.proc.mastery.react&trinket.proc.mastery.remains>cast_time";
  single_target -> action_list_str += "/fire_and_brimstone,if=buff.fire_and_brimstone.down&dot.immolate.remains-action.immolate.cast_time<=(dot.immolate.duration*0.3)&spell_targets.fire_and_brimstone>4";
  single_target -> action_list_str += "/immolate,cycle_targets=1,if=(sim.target=target|!buff.havoc.remains|!raid_event.adds.exists)&remains-cast_time<=(duration*0.3)";
  single_target -> action_list_str += "/conflagrate,cycle_targets=1,if=raid_event.adds.exists&sim.target!=target&buff.havoc.remains<=gcd*3&buff.backdraft.stack=0";
  single_target -> action_list_str += "/conflagrate,if=buff.backdraft.stack=0";
  single_target -> action_list_str += "/incinerate,cycle_targets=1,if=raid_event.adds.exists&sim.target!=target&buff.havoc.remains<=action.incinerate.cast_time*3";
  single_target -> action_list_str += "/incinerate";

  aoe -> action_list_str += "/rain_of_fire,if=!talent.charred_remains.enabled&remains<=tick_time";
  aoe -> action_list_str += "/havoc,target=2,if=!talent.charred_remains.enabled&buff.fire_and_brimstone.down";
  aoe -> action_list_str += "/shadowburn,if=!talent.charred_remains.enabled&buff.havoc.remains";
  aoe -> action_list_str += "/chaos_bolt,if=!talent.charred_remains.enabled&buff.havoc.remains>cast_time&buff.havoc.stack>=3";
  aoe -> action_list_str += "/kiljaedens_cunning,if=(talent.cataclysm.enabled&!cooldown.cataclysm.remains)";
  aoe -> action_list_str += "/kiljaedens_cunning,moving=1,if=!talent.cataclysm.enabled";
  aoe -> action_list_str += "/cataclysm";
  aoe -> action_list_str += "/fire_and_brimstone,if=buff.fire_and_brimstone.down";
  aoe -> action_list_str += "/immolate,if=buff.fire_and_brimstone.up&!dot.immolate.ticking&(burning_ember>=2|!talent.charred_remains.enabled)";
  aoe -> action_list_str += "/conflagrate,if=buff.fire_and_brimstone.up&charges=2&(burning_ember>=2|!talent.charred_remains.enabled)";
  aoe -> action_list_str += "/immolate,if=buff.fire_and_brimstone.up&dot.immolate.remains-action.immolate.cast_time<=(dot.immolate.duration*0.3)&(burning_ember>=2|!talent.charred_remains.enabled)";
  aoe -> action_list_str += "/chaos_bolt,if=talent.charred_remains.enabled&buff.fire_and_brimstone.up&burning_ember>=3";
  aoe -> action_list_str += "/incinerate";

}

void warlock_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    apl_precombat();

    switch ( specialization() )
    {
    case WARLOCK_AFFLICTION:
      apl_affliction();
      break;
    case WARLOCK_DESTRUCTION:
      apl_destruction();
      break;
    case WARLOCK_DEMONOLOGY:
      apl_demonology();
      break;
    default:
      apl_default();
      break;
    }

    apl_global_filler();

    use_default_action_list = true;
  }

  player_t::init_action_list();
}

void warlock_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[RESOURCE_BURNING_EMBER] = initial_burning_embers;
  resources.current[RESOURCE_DEMONIC_FURY] = initial_demonic_fury;

  if ( pets.active )
    pets.active -> init_resources( force );
}

void warlock_t::combat_begin()
{
  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    buffs.demonic_calling -> trigger();
    demonic_calling_event = new ( *sim ) demonic_calling_event_t( this, rng().range( timespan_t::zero(),
      timespan_t::from_seconds( ( spec.wild_imps -> effectN( 1 ).period().total_seconds() + spec.imp_swarm -> effectN( 3 ).base_value() ) * composite_spell_speed() ) ) );
  }

  player_t::combat_begin();
}

void warlock_t::reset()
{
  player_t::reset();

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    warlock_td_t* td = target_data[sim -> actor_list[i]];
    if ( td ) td -> reset();
  }

  pets.active = nullptr;
  ember_react = ( initial_burning_embers >= 1.0 ) ? timespan_t::zero() : timespan_t::max();
  shard_react = timespan_t::zero();
  event_t::cancel( demonic_calling_event );
  havoc_target = nullptr;
  double_nightfall = 0;

  grimoire_of_synergy.reset();
  grimoire_of_synergy_pet.reset();
  rppm_chaotic_infusion.reset();
}

void warlock_t::create_options()
{
  player_t::create_options();

  add_option( opt_int( "burning_embers", initial_burning_embers ) );
  add_option( opt_int( "demonic_fury", initial_demonic_fury ) );
  add_option( opt_string( "default_pet", default_pet ) );
}

std::string warlock_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  if ( stype == SAVE_ALL )
  {
    if ( initial_burning_embers != 1 ) profile_str += "burning_embers=" + util::to_string( initial_burning_embers ) + "\n";
    if ( initial_demonic_fury != 200 ) profile_str += "demonic_fury=" + util::to_string( initial_demonic_fury ) + "\n";
    if ( ! default_pet.empty() )       profile_str += "default_pet=" + default_pet + "\n";
  }

  return profile_str;
}

void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warlock_t* p = debug_cast<warlock_t*>( source );

  initial_burning_embers = p -> initial_burning_embers;
  initial_demonic_fury = p -> initial_demonic_fury;
  default_pet = p -> default_pet;
}

// warlock_t::convert_hybrid_stat ==============================================

stat_e warlock_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
    // This is all a guess at how the hybrid primaries will work, since they
    // don't actually appear on cloth gear yet. TODO: confirm behavior
  case STAT_STR_AGI_INT:
  case STAT_AGI_INT:
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_STR_AGI:
    return STAT_NONE;
  case STAT_SPIRIT:
    return STAT_NONE;
  case STAT_BONUS_ARMOR:
    return STAT_NONE;
  default: return s;
  }
}

void warlock_t::trigger_demonology_t17_2pc( const action_state_t* state ) const
{
  if ( ! sets.has_set_bonus( WARLOCK_DEMONOLOGY, T17, B2 ) )
    return;

  // Hand of Gul'Dan / Chaos Wave can be cast as a "free (and background) cast"
  // due to T16 4PC.  While in reality it's not possible to have T16 and T17
  // 4PC, the sim will allow force-overriding of options, where that scenario
  // can happen.
  if ( state -> action -> background )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  if ( cooldowns.t17_2pc_demonology -> down() )
    return;

  if ( true_level < 100 )
    return;

  if ( ! rng().roll( sets.set( WARLOCK_DEMONOLOGY, T17, B2 ) -> proc_chance() ) )
      return;

  pets.inner_demon -> summon( sets.set( WARLOCK_DEMONOLOGY, T17, B2 ) -> effectN( 1 ).trigger() -> duration() );

  procs.t17_2pc_demo -> occur();
  cooldowns.t17_2pc_demonology -> start( sets.set( WARLOCK_DEMONOLOGY, T17, B2 ) -> internal_cooldown() );
}

void warlock_t::trigger_demonology_t17_2pc_cast() const
{
  if ( ! pets.inner_demon || pets.inner_demon -> is_sleeping() )
    return;

  pets::inner_demon_t* demon = debug_cast<pets::inner_demon_t*>( pets.inner_demon );

  // Allow inner demon to start casting soul fire
  demon -> soul_fire -> background = false;
}

expr_t* warlock_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "ember_react" )
  {
    struct ember_react_expr_t: public expr_t
    {
      warlock_t& player;
      ember_react_expr_t( warlock_t& p ):
        expr_t( "ember_react" ), player( p ) { }
      virtual double evaluate() override { return player.resources.current[RESOURCE_BURNING_EMBER] >= 1 && player.sim -> current_time() >= player.ember_react; }
    };
    return new ember_react_expr_t( *this );
  }
  else if ( name_str == "shard_react" )
  {
    struct shard_react_expr_t: public expr_t
    {
      warlock_t& player;
      shard_react_expr_t( warlock_t& p ):
        expr_t( "shard_react" ), player( p ) { }
      virtual double evaluate() override { return player.resources.current[RESOURCE_SOUL_SHARD] >= 1 && player.sim -> current_time() >= player.shard_react; }
    };
    return new shard_react_expr_t( *this );
  }
  else if ( name_str == "felstorm_is_ticking" )
  {
    struct felstorm_is_ticking_expr_t: public expr_t
    {
      pets::warlock_pet_t* felguard;
      felstorm_is_ticking_expr_t( pets::warlock_pet_t* f ):
        expr_t( "felstorm_is_ticking" ), felguard( f ) { }
      virtual double evaluate() override { return ( felguard ) ? felguard -> special_action -> get_dot() -> is_ticking() : false; }
    };
    return new felstorm_is_ticking_expr_t( debug_cast<pets::warlock_pet_t*>( find_pet( "felguard" ) ) );
  }
  else
  {
    return player_t::create_expression( a, name_str );
  }
}


/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class warlock_report_t: public player_report_extension_t
{
public:
  warlock_report_t( warlock_t& player ):
    p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void)p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
    << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
    << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  warlock_t& p;
};

// WARLOCK MODULE INTERFACE =================================================

struct warlock_module_t: public module_t
{
  warlock_module_t(): module_t( WARLOCK ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new warlock_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new warlock_report_t( *p ) );
    return p;
  }

  virtual void static_init() const override
  {
    unique_gear::register_special_effect( 184922, affliction_trinket);
    unique_gear::register_special_effect( 184923, demonology_trinket);
    unique_gear::register_special_effect( 184924, destruction_trinket);
  }

  virtual void register_hotfixes() const override
  {
    hotfix::register_effect( "Warlock", "2015-07-20", "Chaos Bolt damage increased by 5%.", 132079 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.05 )
      .verification_value( 2.27500 );

    hotfix::register_effect( "Warlock", "2015-07-20-2", "Chaos Bolt damage increased by 5%.", 219895 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.05 )
      .verification_value( 2.27500 );

    hotfix::register_effect( "Warlock", "2015-07-20-", "Conflagrate damage increased by 5%.", 9553 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.05 )
      .verification_value( 2.04100 );

    hotfix::register_effect( "Warlock", "2015-07-20-2", "Conflagrate damage increased by 5%.", 119851 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.05 )
      .verification_value( 2.04100 );

    hotfix::register_effect( "Warlock", "2015-07-20", "Immolate damage increased by 5%.", 145 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.05 )
      .verification_value( 0.49500 );

    hotfix::register_effect( "Warlock", "2015-07-20-2", "Immolate damage increased by 5%.", 119854 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.05 )
      .verification_value( 0.49500 );

    hotfix::register_effect( "Warlock", "2015-07-20", "Incinerate damage increased by 5%.", 19297 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.05 )
      .verification_value( 1.43500 );

    hotfix::register_effect( "Warlock", "2015-07-20-2", "Incinerate damage increased by 5%.", 128057 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.05 )
      .verification_value( 1.43500 );

    hotfix::register_effect( "Warlock", "2015-07-20", "Shadowburn damage increased by 5%.", 9475 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.05 )
      .verification_value( 3.40000 );
  }

  virtual bool valid() const override { return true; }
  virtual void init( player_t* ) const override {}
  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};

} // end unnamed namespace

const module_t* module_t::warlock()
{
  static warlock_module_t m;
  return &m;
}
