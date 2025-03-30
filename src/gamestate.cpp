/*
 * This file is part of OpenOrion2
 * Copyright (C) 2021 Daniel Donisa, Martin Doucha
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <cstring>
#include <stdexcept>
#include "lang.h"
#include "lbx.h"
#include "tech.h"
#include "gamestate.h"

#define COLONY_COUNT_OFFSET 0x25b

const unsigned galaxySizeFactors[GALAXY_ZOOM_LEVELS] = {10, 15, 20, 30};

static const unsigned mineralProductionTable[PLANET_MINERALS_COUNT] = {
	1, 2, 3, 5, 8
};

static const unsigned climatePopFactors[PLANET_CLIMATE_COUNT] = {
	25,	// Toxic
	25,	// Radiated
	25,	// Barren
	25,	// Desert
	25,	// Tundra
	25,	// Ocean
	40,	// Swamp
	60,	// Arid
	80,	// Terran
	100,	// Gaia
};

static const unsigned aquaticPopFactors[PLANET_CLIMATE_COUNT] = {
	25,	// Toxic
	25,	// Radiated
	25,	// Barren
	25,	// Desert
	80,	// Tundra
	100,	// Ocean
	80,	// Swamp
	60,	// Arid
	100,	// Terran
	100,	// Gaia
};

// gravityPelanties[player_homeworld][dest_planet]
static const int gravityPenalties[GRAVITY_LEVEL_COUNT][GRAVITY_LEVEL_COUNT] = {
	{  0, -25, -50},	// low-G homeworld
	{-25,   0, -50},	// normal-G homeworld
	{-50,   0,   0},	// heavy-G homeworld
};

static const unsigned leaderExpThresholds[] = {60, 150, 300, 500, 0};

static const int navigatorSkillValues[][MAX_LEADER_LEVELS] = {
	{1, 1, 2, 2, 3},
	{1, 1, 3, 3, 4}
};

static const int baseSkillValues[MAX_SKILL_TYPES][MAX_COMMON_SKILLS] = {
	{2, 2, 10, -60, 10, 2, 5, 2, 2, 10},
	{2, 5, 5, 5, 1, 5, 2, 5},
	{-10, 10, 10, 1, 10, 10, 10, 5, 2}
};

static const unsigned skillNameTable[MAX_SKILL_TYPES][2*MAX_COMMON_SKILLS] = {
	{
		ESTR_SKILLNAME_ASSASSIN,
		ESTR_SKILLNAME_ASSASSIN_ADV,
		ESTR_SKILLNAME_COMMANDO,
		ESTR_SKILLNAME_COMMANDO_ADV,
		ESTR_SKILLNAME_DIPLOMAT,
		ESTR_SKILLNAME_DIPLOMAT_ADV,
		ESTR_SKILLNAME_FAMOUS,
		ESTR_SKILLNAME_FAMOUS_ADV,
		ESTR_SKILLNAME_MEGAWEALTH,
		ESTR_SKILLNAME_MEGAWEALTH_ADV,
		ESTR_SKILLNAME_OPERATIONS,
		ESTR_SKILLNAME_OPERATIONS_ADV,
		ESTR_SKILLNAME_RESEARCHER,
		ESTR_SKILLNAME_RESEARCHER_ADV,
		ESTR_SKILLNAME_SPY_MASTER,
		ESTR_SKILLNAME_SPY_MASTER_ADV,
		ESTR_SKILLNAME_TELEPATH,
		ESTR_SKILLNAME_TELEPATH_ADV,
		ESTR_SKILLNAME_TRADER,
		ESTR_SKILLNAME_TRADER_ADV,
	},
	{
		ESTR_SKILLNAME_ENGINEER,
		ESTR_SKILLNAME_ENGINEER_ADV,
		ESTR_SKILLNAME_FIGHTER_PILOT,
		ESTR_SKILLNAME_FIGHTER_PILOT_ADV,
		ESTR_SKILLNAME_GALACTIC_LORE,
		ESTR_SKILLNAME_GALACTIC_LORE_ADV,
		ESTR_SKILLNAME_HEMLSMAN,
		ESTR_SKILLNAME_HEMLSMAN_ADV,
		ESTR_SKILLNAME_NAVIGATOR,
		ESTR_SKILLNAME_NAVIGATOR_ADV,
		ESTR_SKILLNAME_ORDNANCE,
		ESTR_SKILLNAME_ORDNANCE_ADV,
		ESTR_SKILLNAME_SECURITY,
		ESTR_SKILLNAME_SECURITY_ADV,
		ESTR_SKILLNAME_WEAPONRY,
		ESTR_SKILLNAME_WEAPONRY_ADV,
	},
	{
		ESTR_SKILLNAME_ENVIRONMENTALIST,
		ESTR_SKILLNAME_ENVIRONMENTALIST_ADV,
		ESTR_SKILLNAME_FARMING_LEADER,
		ESTR_SKILLNAME_FARMING_LEADER_ADV,
		ESTR_SKILLNAME_FINANCIAL_LEADER,
		ESTR_SKILLNAME_FINANCIAL_LEADER_ADV,
		ESTR_SKILLNAME_INSTRUCTOR,
		ESTR_SKILLNAME_INSTRUCTOR_ADV,
		ESTR_SKILLNAME_LABOR_LEADER,
		ESTR_SKILLNAME_LABOR_LEADER_ADV,
		ESTR_SKILLNAME_MEDICINE,
		ESTR_SKILLNAME_MEDICINE_ADV,
		ESTR_SKILLNAME_SCIENCE_LEADER,
		ESTR_SKILLNAME_SCIENCE_LEADER_ADV,
		ESTR_SKILLNAME_SPIRITUAL_LEADER,
		ESTR_SKILLNAME_SPIRITUAL_LEADER_ADV,
		ESTR_SKILLNAME_TACTICS,
		ESTR_SKILLNAME_TACTICS_ADV,
	}
};

static const unsigned skillNumTable[MAX_SKILL_TYPES][MAX_COMMON_SKILLS] = {
	{0, 1, 2, 5, 14, 16, 18, 22, 24, 25},
	{3, 7, 9, 10, 15, 17, 20, 26},
	{4, 6, 8, 11, 12, 13, 19, 21, 23}
};

static const unsigned computerHPTable[MAX_COMBAT_SHIP_CLASSES] = {
	1, 2, 5, 7, 10, 20
};

static const unsigned driveHPTable[MAX_COMBAT_SHIP_CLASSES] = {
	2, 5, 10, 15, 20, 40
};

static const unsigned computerBonusTable[MAX_SHIP_COMPUTER_TYPES] = {
	0, 25, 50, 75, 100, 125
};

static const int shipCrewOffenseBonuses[MAX_SHIP_CREW_LEVELS] = {
	0, 15, 30, 50, 75
};

static const int shipCrewDefenseBonuses[MAX_SHIP_CREW_LEVELS] = {
	0, 15, 30, 50, 75
};

static const unsigned npcFleetOwnerNames[NPC_FLEET_OWNERS] = {
	ESTR_MONSTER_ANTARANS, ESTR_MONSTER_GUARDIAN, ESTR_MONSTER_AMOEBA,
	ESTR_MONSTER_CRYSTAL, ESTR_MONSTER_DRAGON, ESTR_MONSTER_EEL,
	ESTR_MONSTER_HYDRA
};

GameConfig::GameConfig(void) {
	version = 0;
	memset(saveGameName, 0, SAVE_GAME_NAME_SIZE);
	stardate = 0;
	multiplayer = 0;
	endOfTurnSummary = 0;
	endOfTurnWait = 0;
	randomEvents = 0;
	enemyMoves = 0;
	expandingHelp = 0;
	autoSelectShips = 0;
	animations = 0;
	autoSelectColony = 0;
	showRelocationLines = 0;
	showGNNReport = 0;
	autoDeleteTradeGoodHousing = 0;
	showOnlySeriousTurnSummary = 0;
	shipInitiative = 0;
}

void GameConfig::load(ReadStream &stream) {
	version = stream.readUint32LE();

	if (version != 0xe0) {
		throw std::runtime_error("Invalid savegame version");
	}

	stream.read(saveGameName, SAVE_GAME_NAME_SIZE);
	saveGameName[SAVE_GAME_NAME_SIZE - 1] = '\0';
	stardate = stream.readUint32LE();
	multiplayer = stream.readUint8();
	endOfTurnSummary = stream.readUint8();
	endOfTurnWait = stream.readUint8();
	randomEvents = stream.readUint8();
	enemyMoves = stream.readUint8();
	expandingHelp = stream.readUint8();
	autoSelectShips = stream.readUint8();
	animations = stream.readUint8();
	autoSelectColony = stream.readUint8();
	showRelocationLines = stream.readUint8();
	showGNNReport = stream.readUint8();
	autoDeleteTradeGoodHousing = stream.readUint8();
	showOnlySeriousTurnSummary = stream.readUint8();
	shipInitiative = stream.readUint8();
}

Nebula::Nebula(void) : x(0), y(0), type(0) {

}

void Nebula::load(ReadStream &stream) {
	x = stream.readUint16LE();
	y = stream.readUint16LE();
	type = stream.readUint8();
}

void Nebula::validate(void) const {
	if (type >= NEBULA_TYPE_COUNT) {
		throw std::runtime_error("Invalid nebula type");
	}
}

Galaxy::Galaxy(void) : sizeFactor(0), width(0), height(0), nebulaCount(0) {

}

void Galaxy::load(ReadStream &stream) {
	sizeFactor = stream.readUint8();
	stream.readUint32LE(); // Skip unknown data
	width = stream.readUint16LE();
	height = stream.readUint16LE();
	stream.readUint16LE(); // Skip unknown data

	for (int i = 0; i < MAX_NEBULAS; i++) {
		nebulas[i].load(stream);
	}

	nebulaCount = stream.readUint8();
}

void Galaxy::validate(void) const {
	unsigned i;

	if (nebulaCount > MAX_NEBULAS) {
		throw std::runtime_error("Invalid nebula count");
	}

	for (i = 0; i < GALAXY_ZOOM_LEVELS; i++) {
		if (sizeFactor == galaxySizeFactors[i]) {
			break;
		}
	}

	if (i >= GALAXY_ZOOM_LEVELS) {
		throw std::runtime_error("Invalid galaxy size factor");
	}

	for (i = 0; i < nebulaCount; i++) {
		nebulas[i].validate();

		if (nebulas[i].x >= width || nebulas[i].y >= height) {
			throw std::runtime_error("Nebula outside galaxy area");
		}
	}
}

Colonist::Colonist(void) {
	race = 0;
	loyalty = 0;
	job = 0;
	flags = 0;
}

void Colonist::load(ReadStream &stream) {
	uint32_t raw_data = stream.readUint32LE();

	race = raw_data & 0xf;
	loyalty = (raw_data >> 4) & 0x7;
	job = (raw_data >> 7) & 0x3;
	flags = raw_data >> 9;
}

Colony::Colony(void) {
	size_t i;

	owner = -1;
	unknown1 = -1;
	planet = -1;
	unknown2 = -1;
	is_outpost = 0;
	morale = 0;
	pollution = 0;
	population = 0;
	colony_type = 0;

	memset(race_population, 0, MAX_RACES * sizeof(uint16_t));
	memset(pop_growth, 0, MAX_RACES * sizeof(int16_t));

	age = 0;
	food_per_farmer = 0;
	industry_per_worker = 0;
	research_per_scientist = 0;
	max_farms = 0;
	max_population = 0;
	climate = 0;
	ground_strength = 0;
	space_strength = 0;
	total_food = 0;
	net_industry = 0;
	total_research = 0;
	total_revenue = 0;
	food_consumption = 0;
	industry_consumption = 0;
	research_consumption = 0;
	upkeep = 0;
	food_imported = 0;
	industry_consumed = 0;
	research_imported = 0;
	budget_deficit = 0;
	recycled_industry = 0;
	food_consumption_citizens = 0;
	food_consumption_aliens = 0;
	food_consumption_prisoners = 0;
	food_consumption_natives = 0;
	industry_consumption_citizens = 0;
	industry_consumption_androids = 0;
	industry_consumption_aliens = 0;
	industry_consumption_prisoners = 0;

	memset(food_consumption_races, 0, MAX_PLAYERS * sizeof(uint8_t));
	memset(industry_consumption_races, 0, MAX_PLAYERS * sizeof(uint8_t));

	replicated_food = 0;

	for (i = 0; i < MAX_BUILD_QUEUE; i++) {
		build_queue[i] = -1;
	}

	finished_production = -1;
	build_progress = 0;
	tax_revenue = 0;
	autobuild = 0;
	unknown3 = 0;
	bought_progress = 0;
	assimilation_progress = 0;
	prisoner_policy = 0;
	soldiers = 0;
	tanks = 0;
	tank_progress = 0;
	soldier_progress = 0;

	memset(buildings, 0, MAX_BUILDINGS * sizeof(uint8_t));

	status = 0;
}

void Colony::load(ReadStream &stream) {
	size_t i;

	owner = stream.readUint8();
	unknown1 = stream.readSint8();
	planet = stream.readSint16LE();
	unknown2 = stream.readSint16LE();
	is_outpost = stream.readUint8();
	morale = stream.readSint8();
	pollution = stream.readUint16LE();
	population = stream.readUint8();
	colony_type = stream.readUint8();

	for (i = 0; i < MAX_POPULATION; i++) {
		colonists[i].load(stream);
	}

	for (i = 0; i < MAX_RACES; i++) {
		race_population[i] = stream.readUint16LE();
	}

	for (i = 0; i < MAX_RACES; i++) {
		pop_growth[i] = stream.readSint16LE();
	}

	age = stream.readUint8();
	food_per_farmer = stream.readUint8();
	industry_per_worker = stream.readUint8();
	research_per_scientist = stream.readUint8();
	max_farms = stream.readSint8();
	max_population = stream.readUint8();
	climate = stream.readUint8();
	ground_strength = stream.readUint16LE();
	space_strength = stream.readUint16LE();
	total_food = stream.readUint16LE();
	net_industry = stream.readUint16LE();
	total_research = stream.readUint16LE();
	total_revenue = stream.readUint16LE();
	food_consumption = stream.readUint8();
	industry_consumption = stream.readUint8();
	research_consumption = stream.readUint8();
	upkeep = stream.readUint8();
	food_imported = stream.readSint16LE();
	industry_consumed = stream.readUint16LE();
	research_imported = stream.readSint16LE();
	budget_deficit = stream.readSint16LE();
	recycled_industry = stream.readUint8();
	food_consumption_citizens = stream.readUint8();
	food_consumption_aliens = stream.readUint8();
	food_consumption_prisoners = stream.readUint8();
	food_consumption_natives = stream.readUint8();
	industry_consumption_citizens = stream.readUint8();
	industry_consumption_androids = stream.readUint8();
	industry_consumption_aliens = stream.readUint8();
	industry_consumption_prisoners = stream.readUint8();

	for (i = 0; i < MAX_PLAYERS; i++) {
		food_consumption_races[i] = stream.readUint8();
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		industry_consumption_races[i] = stream.readUint8();
	}

	replicated_food = stream.readUint8();

	for (i = 0; i < MAX_BUILD_QUEUE; i++) {
		build_queue[i] = stream.readSint16LE();
	}

	finished_production = stream.readSint16LE();
	build_progress = stream.readUint16LE();
	tax_revenue = stream.readUint16LE();
	autobuild = stream.readUint8();
	unknown3 = stream.readUint16LE();
	bought_progress = stream.readUint16LE();
	assimilation_progress = stream.readUint8();
	prisoner_policy = stream.readUint8();
	soldiers = stream.readUint16LE();
	tanks = stream.readUint16LE();
	tank_progress = stream.readUint8();
	soldier_progress = stream.readUint8();

	for (i = 0; i < MAX_BUILDINGS; i++) {
		buildings[i] = stream.readUint8();
	}

	status = stream.readUint16LE();
}

void Colony::validate(void) const {
	unsigned i;

	if (population > MAX_POPULATION) {
		throw std::out_of_range("Colony population too high");
	}

	if (climate > GAIA) {
		throw std::out_of_range("Colony has invalid climate");
	}

	for (i = 0; i < population; i++) {
		if (colonists[i].job > SCIENTIST) {
			throw std::out_of_range("Invalid colonist job");
		}
	}
}

Planet::Planet(void) {
	colony = -1;
	star = 0;
	orbit = 0;
	type = 0;
	size = 0;
	gravity = 0;
	unknown1 = 0;
	climate = 0;
	bg = 0;
	minerals = 0;
	foodbase = 0;
	terraforms = 0;
	unknown2 = 0;
	max_pop = 0;
	special = 0;
	flags = 0;
}

void Planet::load(ReadStream &stream) {
	colony = stream.readSint16LE();
	star = stream.readUint8();
	orbit = stream.readUint8();
	type = stream.readUint8();
	size = stream.readUint8();
	gravity = stream.readUint8();
	unknown1 = stream.readUint8();
	climate = stream.readUint8();
	bg = stream.readUint8();
	minerals = stream.readUint8();
	foodbase = stream.readUint8();
	terraforms = stream.readUint8();
	unknown2 = stream.readUint8();
	max_pop = stream.readUint8();
	special = stream.readUint8();
	flags = stream.readUint8();
}

unsigned Planet::baseProduction(void) const {
	return mineralProductionTable[minerals];
}

void Planet::validate(void) const {
	if (orbit >= MAX_ORBITS) {
		throw std::out_of_range("Planet has invalid orbit");
	}

	if (type < ASTEROIDS || type > HABITABLE) {
		throw std::out_of_range("Planet has invalid type");
	}

	if (size > HUGE_PLANET) {
		throw std::out_of_range("Planet has invalid size");
	}

	if (gravity > HEAVY_G) {
		throw std::out_of_range("Planet has invalid gravity value");
	}

	if (type == HABITABLE && climate > GAIA) {
		throw std::out_of_range("Planet has invalid climate");
	}

	if (bg >= MAX_PLANET_BGS) {
		throw std::out_of_range("Planet has invalid surface image");
	}

	if (minerals > ULTRA_RICH) {
		throw std::out_of_range("Planet has invalid mineral value");
	}

	if (special == BAD_SPECIAL1 || special == BAD_SPECIAL2 ||
		special > ORION_SPECIAL) {
		throw std::out_of_range("Planet has invalid special treasure");
	}
}

Leader::Leader(void) {
	int i;

	memset(name, 0, LEADER_NAME_SIZE);
	memset(title, 0, LEADER_TITLE_SIZE);
	type = 0;
	experience = 0;
	commonSkills = 0;
	specialSkills = 0;

	for (i = 0; i < MAX_LEADER_TECH_SKILLS; i++) {
		techs[i] = 0;
	}

	picture = 0;
	skillValue = 0;
	level = 0;
	location = -1;
	eta = 0;
	displayLevelUp = 0;
	status = 0;
	playerIndex = -1;
}

void Leader::load(ReadStream &stream) {
	int i;

	stream.read(name, LEADER_NAME_SIZE);
	name[LEADER_NAME_SIZE - 1] = '\0';
	stream.read(title, LEADER_TITLE_SIZE);
	title[LEADER_TITLE_SIZE - 1] = '\0';
	type = stream.readUint8();
	experience = stream.readUint16LE();
	commonSkills = stream.readUint32LE();
	specialSkills = stream.readUint32LE();

	for (i = 0; i < MAX_LEADER_TECH_SKILLS; i++) {
		techs[i] = stream.readUint8();
	}

	picture = stream.readUint8();
	skillValue = stream.readUint16LE();
	level = stream.readUint8();
	location = stream.readSint16LE();
	eta = stream.readUint8();
	displayLevelUp = stream.readUint8();
	status = stream.readSint8();
	playerIndex = stream.readSint8();
}

unsigned Leader::expLevel(void) const {
	unsigned i;

	for (i = 0; leaderExpThresholds[i]; i++) {
		if (experience < leaderExpThresholds[i]) {
			break;
		}
	}

	return i;
}

const char *Leader::rank(void) const {
	unsigned base;

	if (type) {
		base = ESTR_LEADER_LEVEL_ADMIN;
	} else {
		base = ESTR_LEADER_LEVEL_COMMANDER;
	}

	return gameLang->estrings(base + expLevel());
}

int Leader::hasSkill(unsigned id) const {
       uint32_t skills = 0;
       unsigned max, skillnum;

       switch (id & SKILLTYPE_MASK) {
       case COMMON_SKILLS_TYPE:
               skills = commonSkills;
               max = MAX_COMMON_SKILLS;
               break;

       case CAPTAIN_SKILLS_TYPE:
               skills = (type == LEADER_TYPE_CAPTAIN) ? specialSkills : 0;
               max = MAX_CAPTAIN_SKILLS;
               break;

       case ADMIN_SKILLS_TYPE:
               skills = (type == LEADER_TYPE_ADMIN) ? specialSkills : 0;
               max = MAX_ADMIN_SKILLS;
               break;

       default:
               throw std::invalid_argument("Invalid skill ID");
       }

       skillnum = id & SKILLCODE_MASK;

       if (skillnum >= max) {
               return 0;
       }

       return (skills >> (2 * skillnum)) & 0x3;
}

int Leader::skillBonus(unsigned id) const {
       unsigned tier;
       int ret;

       tier = hasSkill(id);

       if (!tier) {
               return 0;
       }

       // Navigator skill has special tier/level progression
       if (id == SKILL_NAVIGATOR) {
               return navigatorSkillValues[(tier > 1) ? 1 : 0][expLevel()];
       }

       ret = baseSkillValues[SKILLTYPE(id)][id & SKILLCODE_MASK];

       // Megawealth bonus is the same for all levels
       if (id != SKILL_MEGAWEALTH) {
               ret *= (expLevel() + 1);
       }

       // +50% bonus for advanced skill
       if (tier > 1) {
               ret += ret / 2;
       }

       return ret;
}

int Leader::isEmployed(void) const {
	return (status == LeaderState::Idle ||
		status == LeaderState::Working ||
		status == LeaderState::Unassigned);
}

unsigned Leader::hireCost(int modifier) const {
	int ret = 10 * skillValue * (expLevel() + 1) + modifier;

	return MAX(ret, 0);
}

void Leader::validate(void) const {
	if (picture >= LEADER_COUNT) {
		throw std::out_of_range("Invalid leader picture");
	}

	if (status == LeaderState::Working && location < 0) {
		throw std::runtime_error("Working officer has no location");
	}
}

unsigned Leader::skillCount(unsigned type) {
	static const unsigned skillCounts[MAX_SKILL_TYPES] = {
		MAX_COMMON_SKILLS, MAX_CAPTAIN_SKILLS, MAX_ADMIN_SKILLS
	};

	if (type >= MAX_SKILL_TYPES) {
		throw std::out_of_range("Invalid skill type");
	}

	return skillCounts[type];
}

const char *Leader::skillName(unsigned id, unsigned advanced) {
	unsigned code = id & SKILLCODE_MASK;

	if (code >= skillCount(SKILLTYPE(id))) {
		throw std::out_of_range("Invalid skill ID");
	}

	code *= 2;
	code = advanced ? code + 1 : code;
	return gameLang->estrings(skillNameTable[SKILLTYPE(id)][code]);
}

unsigned Leader::skillNum(unsigned id) {
	unsigned code = id & SKILLCODE_MASK;

	if (code >= skillCount(SKILLTYPE(id))) {
		throw std::out_of_range("Invalid skill ID");
	}

	return skillNumTable[SKILLTYPE(id)][code];
}

void ShipWeapon::load(ReadStream &stream) {
	type = stream.readSint16LE();
	maxCount = stream.readUint8();
	workingCount = stream.readUint8();
	arc = stream.readUint8();
	mods = stream.readUint16LE();
	ammo = stream.readUint8();
}

unsigned ShipWeapon::arcID(void) const {
	unsigned i;

	if (arc == WeaponArc::ARC_MONSTER_360) {
		return 4;
	}

	for (i = 0; i < MAX_SHIP_WEAPON_ARCS; i++) {
		if (arc == (1 << i)) {
			return i;
		}
	}

	throw std::runtime_error("Invalid weapon arc");
}

const char *ShipWeapon::arcAbbr(void) const {
	if (arc == WeaponArc::ARC_360 || arc == WeaponArc::ARC_MONSTER_360) {
		return gameLang->hstrings(HSTR_WEAPON_ARC_360);
	}

	return gameLang->misctext(TXT_MISC_KENTEXT,
		KEN_WEAPON_ARC_FWD + arcID());
}

ShipDesign::ShipDesign(void) {
	memset(name, 0, SHIP_NAME_SIZE);
	size = 0;
	type = 0;
	shield = 0;
	drive = 0;
	speed = 0;
	computer = 0;
	armor = 0;
	memset(specials, 0, (MAX_SHIP_SPECIALS + 7) / 8);
	memset(weapons, 0, MAX_SHIP_WEAPONS * sizeof(ShipWeapon));
	picture = 0;
	builder = 0;
	cost = 0;
	baseCombatSpeed = 0;
	buildDate = 0;
}

void ShipDesign::load(ReadStream &stream) {
	int i;

	stream.read(name, SHIP_NAME_SIZE);
	name[SHIP_NAME_SIZE - 1] = '\0';
	size = stream.readUint8();
	type = stream.readUint8();
	shield = stream.readUint8();
	drive = stream.readUint8();
	speed = stream.readUint8();
	computer = stream.readUint8();
	armor = stream.readUint8();
	stream.read(specials, (MAX_SHIP_SPECIALS + 7) / 8);

	for (i = 0; i < MAX_SHIP_WEAPONS; i++) {
		weapons[i].load(stream);
	}

	picture = stream.readUint8();
	builder = stream.readUint8();
	cost = stream.readUint16LE();
	baseCombatSpeed = stream.readUint8();
	buildDate = stream.readUint16LE();
}

int ShipDesign::hasSpecial(unsigned id) const {
	if (id >= MAX_SHIP_SPECIALS) {
		throw std::out_of_range("Invalid ship special device ID");
	}

	return checkBitfield(specials, id);
}

int ShipDesign::hasWorkingSpecial(unsigned id,
	const uint8_t *specDamage) const {

	return hasSpecial(id) && !checkBitfield(specDamage, id);
}

unsigned ShipDesign::maxComputerHP(void) const {
	return computerHPTable[size];
}

unsigned ShipDesign::computerHP(unsigned compDamage) const {
	unsigned maxHP = maxComputerHP();

	return compDamage < maxHP ? maxHP - compDamage : 0;
}

unsigned ShipDesign::maxDriveHP(const uint8_t *specDamage) const {
	unsigned ret = driveHPTable[size];

	if (hasWorkingSpecial(SPEC_REINFORCED_HULL, specDamage)) {
		ret *= 3;
	}

	return ret;
}

unsigned ShipDesign::driveHP(unsigned driveDamage,
	const uint8_t *specDamage) const {

	if (driveDamage >= 100) {
		return 0;
	}

	return (maxDriveHP(specDamage) * (100 - driveDamage)) / 100;
}

unsigned ShipDesign::combatSpeed(int transDimensional, unsigned driveDamage,
	const uint8_t *specDamage) const {

	unsigned hp, minHP, maxHP, ret = baseCombatSpeed;

	if (hasWorkingSpecial(SPEC_AUGMENTED_ENGINES, specDamage)) {
		ret += 5;
	}

	// drive damage higher than 33% will disable the ship in combat
	// driveDamage value is percentage (0-100%) but damage penalty
	// is recalculated from hitpoints, not the direct percentage value
	maxHP = maxDriveHP(specDamage);
	hp = driveHP(driveDamage, specDamage);
	minHP = (2 * maxHP) / 3;

	if (minHP < hp) {
		hp -= minHP;
		maxHP -= minHP;
		ret = (ret * hp) / maxHP;
	} else {
		ret = 0;
	}

	// transdimensional bonus applies regardless of damage
	if (transDimensional) {
		ret += 4;
	}

	return ret;
}

int ShipDesign::beamOffense(unsigned compDamage,
	const uint8_t *specDamage) const {
	int ret = 0;

	if (compDamage < maxComputerHP()) {
		ret += computerBonusTable[computer];
	}

	if (hasWorkingSpecial(SPEC_BATTLE_SCANNER, specDamage)) {
		ret += 50;
	}

	return ret;
}

int ShipDesign::beamDefense(int transDimensional, unsigned driveDamage,
	const uint8_t *specDamage) const {

	int ret = combatSpeed(transDimensional, driveDamage, specDamage) * 5;

	if (hasWorkingSpecial(SPEC_INERTIAL_NULLIFIER, specDamage)) {
		ret += 100;
	}

	if (hasWorkingSpecial(SPEC_INERTIAL_STABILIZER, specDamage)) {
		ret += 50;
	}

	return ret;
}

void ShipDesign::validate(void) const {
	unsigned i, monster;

	if (type >= MAX_SHIP_TYPES || type == BAD_SHIP_TYPE) {
		throw std::out_of_range("Invalid ship type");
	}

	if (type == COMBAT_SHIP && size >= MAX_COMBAT_SHIP_CLASSES) {
		throw std::out_of_range("Invalid combat ship size");
	}

	if (shield >= MAX_SHIP_SHIELD_TYPES) {
		throw std::out_of_range("Invalid ship shield type");
	}

	if (drive >= MAX_SHIP_DRIVE_TYPES) {
		throw std::out_of_range("Invalid ship drive type");
	}

	if (computer >= MAX_SHIP_COMPUTER_TYPES) {
		throw std::out_of_range("Invalid ship computer type");
	}

	if (armor >= MAX_SHIP_ARMOR_TYPES) {
		throw std::out_of_range("Invalid ship armor type");
	}

	// Non-combat ships use weapon attributes to store special data
	for (i = 0; type == COMBAT_SHIP && i < MAX_SHIP_WEAPONS; i++) {
		if (weapons[i].type >= MAX_SHIP_WEAPON_TYPES) {
			throw std::out_of_range("Invalid ship weapon type");
		}

		if (weapons[i].type > 0) {
			// arcID() also validates arc value
			weapons[i].arcID();
		}
	}

	if (builder >= MAX_FLEET_OWNERS) {
		throw std::out_of_range("Invalid ship builder");
	}

	if (picture >= MAX_SHIP_SPRITES) {
		throw std::out_of_range("Invalid ship sprite");
	}

	if (builder == MAX_PLAYERS && picture >= MAX_SHIPTYPES_ANTARAN) {
		throw std::out_of_range("Invalid antaran ship sprite");
	}

	if (builder > MAX_PLAYERS) {
		monster = builder - MAX_PLAYERS - 1;

		if ((picture != SHIPSPRITE_GUARDIAN + monster) && (!monster ||
			picture != SHIPSPRITE_MINIMONSTER + monster - 1)) {
			throw std::runtime_error("Invalid monster sprite");
		}
	}
}

void SettlerInfo::load(ReadStream &stream) {
	BitStream data(stream);

	sourceColony = data.readBitsLE(8);
	destinationPlanet = data.readBitsLE(8);
	player = data.readBitsLE(4);
	eta = data.readBitsLE(4);
	job = data.readBitsLE(2);
	//data.readBitsLE(6);
}

Player::Player(void) {
	memset(name, 0, PLAYER_NAME_SIZE);
	memset(race, 0, PLAYER_RACE_SIZE);
	picture = 0;
	color = 0;
	personality = 0;
	objective = 0;

	homePlayerId = 0;
	networkPlayerId = 0;
	playerDoneFlags = 0;
	researchBreakthrough = 0;
	taxRate = 0;
	BC = 0;
	totalFreighters = 0;
	surplusFreighters = 0;
	commandPoints = 0;
	usedCommandPoints = 0;
	foodFreighted = 0;
	settlersFreighted = 0;

	totalPop = 0;
	foodProduced = 0;
	industryProduced = 0;
	researchProduced = 0;
	bcProduced = 0;

	surplusFood = 0;
	surplusBC = 0;

	totalMaintenance = 0;
	buildingMaintenance = 0;
	freighterMaintenance = 0;
	shipMaintenance = 0;
	spyMaintenance = 0;
	tributeCost = 0;
	officerMaintenance = 0;

	memset(researchTopics, 1, MAX_RESEARCH_TOPICS * sizeof(uint8_t));
	memset(techs, 0, MAX_TECHNOLOGIES * sizeof(uint8_t));

	researchProgress = 0;
	researchTopic = 0;

	memset(hyperTechLevels, 0, MAX_RESEARCH_AREAS * sizeof(uint8_t));

	researchItem = 0;

	memset(playerContacts, 0, MAX_PLAYERS * sizeof(uint8_t));
	memset(playerRelations, 0, MAX_PLAYERS * sizeof(int8_t));
	memset(foreignPolicies, 0, MAX_PLAYERS * sizeof(uint8_t));
	memset(tradeTreaties, 0, MAX_PLAYERS * sizeof(uint8_t));
	memset(researchTreaties, 0, MAX_PLAYERS * sizeof(uint8_t));
	memset(fleetHistory, 0, MAX_HISTORY_LENGTH * sizeof(uint8_t));
	memset(techHistory, 0, MAX_HISTORY_LENGTH * sizeof(uint8_t));
	memset(populationHistory, 0, MAX_HISTORY_LENGTH * sizeof(uint8_t));
	memset(buildingHistory, 0, MAX_HISTORY_LENGTH * sizeof(uint8_t));
	memset(spies, 0, MAX_PLAYERS * sizeof(uint8_t));

	galaxyCharted = 0;
}

void Player::load(SeekableReadStream &stream) {
	int i;

	stream.readUint8();	// FIXME: unknown data
	stream.read(name, PLAYER_NAME_SIZE);
	name[PLAYER_NAME_SIZE - 1] = '\0';
	stream.read(race, PLAYER_RACE_SIZE);
	race[PLAYER_RACE_SIZE - 1] = '\0';
	eliminated = stream.readUint8();
	picture = stream.readUint8();
	color = stream.readUint8();
	// 100 = Human player
	personality = stream.readUint8();
	objective = stream.readUint8();

	homePlayerId = stream.readUint16LE();
	networkPlayerId = stream.readUint16LE();
	playerDoneFlags = stream.readUint8();
	stream.readUint16LE(); // Dead field
	researchBreakthrough = stream.readUint8();
	taxRate = stream.readUint8();
	BC = stream.readSint32LE();
	totalFreighters = stream.readUint16LE();
	surplusFreighters = stream.readSint16LE();
	commandPoints = stream.readUint16LE();
	usedCommandPoints = stream.readSint16LE();
	foodFreighted = stream.readUint16LE();
	settlersFreighted = stream.readUint16LE();

	for (i = 0; i < MAX_SETTLERS; i++) {
		settlers[i].load(stream);
	}

	totalPop = stream.readUint16LE();
	foodProduced = stream.readUint16LE();
	industryProduced = stream.readUint16LE();
	researchProduced = stream.readUint16LE();
	bcProduced = stream.readUint16LE();

	surplusFood = stream.readSint16LE();
	surplusBC = stream.readSint16LE();

	totalMaintenance = stream.readSint32LE();
	buildingMaintenance = stream.readUint16LE();
	freighterMaintenance = stream.readUint16LE();
	shipMaintenance = stream.readUint16LE();
	spyMaintenance = stream.readUint16LE();
	tributeCost = stream.readUint16LE();
	officerMaintenance = stream.readUint16LE();

	for (i = 0; i < MAX_RESEARCH_TOPICS; i++) {
		researchTopics[i] = stream.readUint8();
	}

	for (i = 0; i < MAX_TECHNOLOGIES; i++) {
		techs[i] = stream.readUint8();
	}

	researchProgress = stream.readUint32LE();

	stream.seek(45, SEEK_CUR);

	for (i = 0; i < MAX_RESEARCH_AREAS; i++) {
		hyperTechLevels[i] = stream.readUint8();
	}

	stream.seek(253, SEEK_CUR);

	researchTopic = stream.readUint8();
	researchItem = stream.readUint8();

	stream.readUint8();	// FIXME: Unknown data
	stream.readUint8();	// FIXME: Unknown data
	stream.readUint8();	// FIXME: Unknown data

	for (i = 0; i < MAX_PLAYER_BLUEPRINTS; i++) {
		blueprints[i].load(stream);
	}

	selectedBlueprint.load(stream);

	stream.seek(12, SEEK_CUR);

	for (i = 0; i < MAX_PLAYERS; i++) {
		playerContacts[i] = stream.readUint8();
	}

	stream.seek(139, SEEK_CUR);

	for (i = 0; i < MAX_PLAYERS; i++) {
		playerRelations[i] = stream.readSint8();
	}

	stream.seek(8, SEEK_CUR);

	for (i = 0; i < MAX_PLAYERS; i++) {
		foreignPolicies[i] = stream.readUint8();
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		tradeTreaties[i] = stream.readUint8();
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		researchTreaties[i] = stream.readUint8();
	}

	stream.seek(608, SEEK_CUR);

	for (i = 0; i < TRAITS_COUNT; i++) {
		traits[i] = stream.readSint8();
	}

	stream.seek(33, SEEK_CUR);

	for (i = 0; i < MAX_HISTORY_LENGTH; i++) {
		fleetHistory[i] = stream.readUint8();
	}

	for (i = 0; i < MAX_HISTORY_LENGTH; i++) {
		techHistory[i] = stream.readUint8();
	}

	for (i = 0; i < MAX_HISTORY_LENGTH; i++) {
		populationHistory[i] = stream.readUint8();
	}

	for (i = 0; i < MAX_HISTORY_LENGTH; i++) {
		buildingHistory[i] = stream.readUint8();
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		spies[i] = stream.readUint8();
	}

	infoPanel = stream.readUint8();

	stream.seek(21, SEEK_CUR);

	galaxyCharted = stream.readUint8();

	stream.seek(51, SEEK_CUR);
}

int Player::gravityPenalty(unsigned gravity) const {
	unsigned homegrav;

	if (gravity >= GRAVITY_LEVEL_COUNT) {
		throw std::out_of_range("Invalid gravity level");
	}

	if (traits[TRAIT_LOW_G]) {
		homegrav = PlanetGravity::LOW_G;
	} else if (traits[TRAIT_HIGH_G]) {
		homegrav = PlanetGravity::HEAVY_G;
	} else {
		homegrav = PlanetGravity::NORMAL_G;
	}

	return gravityPenalties[homegrav][gravity];
}

unsigned Player::knowsTechnology(unsigned tech_id) const {
	if (tech_id >= MAX_TECHNOLOGIES) {
		throw std::out_of_range("Invalid technology ID");
	}

	if (tech_id < MAX_APPLIED_TECHS) {
		return techs[tech_id] == RSTATE_KNOWN;
	}

	return hyperTechLevels[tech_id - MAX_APPLIED_TECHS];
}

int Player::canResearchTopic(unsigned topic_id) const {
	if (topic_id >= MAX_RESEARCH_TOPICS) {
		throw std::out_of_range("Invalid research topic ID");
	}

	return researchTopics[topic_id] == RSTATE_READY;
}

unsigned Player::researchCost(unsigned topic_id, int full) const {
	unsigned cost;

	if (topic_id >= MAX_RESEARCH_TOPICS) {
		throw std::out_of_range("Invalid research topic ID");
	}

	cost = research_choices[topic_id].cost;

	if (isHyperTopic(topic_id)) {
		Technology tech = research_choices[topic_id].choices[0];

		cost += 10000 * knowsTechnology(tech);
	}

	if (full) {
		return cost;
	}

	return researchProgress < cost ? cost - researchProgress : 0;
}

int Player::canResearchTech(unsigned tech_id) const {
	if (tech_id >= MAX_TECHNOLOGIES) {
		throw std::out_of_range("Invalid technology ID");
	}

	return techs[tech_id] == RSTATE_RESEARCHABLE;
}

unsigned Player::blueprintCombatSpeed(unsigned id) const {
	if (id >= MAX_PLAYER_BLUEPRINTS) {
		throw std::out_of_range("Invalid ship ID");
	}

	return blueprintCombatSpeed(blueprints + id);
}

unsigned Player::blueprintCombatSpeed(const ShipDesign *design) const {
	return design->combatSpeed(traits[TRAIT_TRANS_DIMENSIONAL]);
}

int Player::blueprintBeamOffense(unsigned id) const {
	if (id >= MAX_PLAYER_BLUEPRINTS) {
		throw std::out_of_range("Invalid ship ID");
	}

	return blueprintBeamOffense(blueprints + id);
}

int Player::blueprintBeamOffense(const ShipDesign *design) const {
	return design->beamOffense() + traits[TRAIT_SHIP_ATTACK];
}

int Player::blueprintBeamDefense(unsigned id) const {
	if (id >= MAX_PLAYER_BLUEPRINTS) {
		throw std::out_of_range("Invalid ship ID");
	}

	return blueprintBeamDefense(blueprints + id);
}

int Player::blueprintBeamDefense(const ShipDesign *design) const {
	int ret = design->beamDefense(traits[TRAIT_TRANS_DIMENSIONAL]);

	return ret + traits[TRAIT_SHIP_DEFENSE];
}

int Player::isPlayerVisible(unsigned player_id) const {
	if (player_id >= MAX_PLAYERS) {
		throw std::out_of_range("Invalid player ID");
	}

	return traits[TRAIT_OMNISCIENCE] || galaxyCharted ||
		playerContacts[player_id];
}

void Player::validate(void) const {
	unsigned i;

	if (picture >= RACE_COUNT) {
		throw std::out_of_range("Player has invalid race ID");
	}

	if (color >= MAX_PLAYERS) {
		throw std::out_of_range("Player has invalid color ID");
	}

	for (i = 0; i < MAX_PLAYER_BLUEPRINTS; i++) {
		blueprints[i].validate();
	}
}

Star::Star(void) {
	size_t i;

	memset(name, 0, STARS_NAME_SIZE);
	x = 0;
	y = 0;
	size = StarSize::Large;
	owner = 0;
	pictureType = 0;
	spectralClass = SpectralClass::Blue;

	memset(lastPlanetSelected, 0, sizeof(lastPlanetSelected));
	memset(blackHoleBlocks, 0, sizeof(blackHoleBlocks));

	special = NO_SPECIAL;
	wormhole = 0;
	blockaded = 0;

	memset(blockadedBy, 0, sizeof(blockadedBy));

	visited = 0;
	justVisited = 0;
	ignoreColonyShips = 0;
	ignoreCombatShips = 0;
	colonizePlayer = 0;
	hasColony = 0;
	hasWarpFieldInterdictor = 0;
	nextWFIInList = 0;
	hasTachyon = 0;
	hasSubspace = 0;
	hasStargate = 0;
	hasJumpgate = 0;
	hasArtemisNet = 0;
	hasDimensionalPortal = 0;
	isStagepoint = 0;

	for (i = 0; i < MAX_ORBITS; i++) {
		planetIndex[i] = -1;
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		officerIndex[i] = -1;
	}

	memset(relocateShipTo, 0, sizeof(relocateShipTo));
	memset(surrenderTo, 0, sizeof(surrenderTo));

	inNebula = 0;
	artifactsGaveApp = 0;

	// Initialize fleet lists
	_firstOrbitingFleet.insert_before(&_lastOrbitingFleet);
	_firstLeavingFleet.insert_before(&_lastLeavingFleet);
}

Star::~Star(void) {
	BilistNode<Fleet> *ptr, *next;

	ptr = _firstOrbitingFleet.next();
	_firstOrbitingFleet.unlink();

	for (; ptr && ptr != &_lastOrbitingFleet; ptr = next) {
		next = ptr->next();
		delete ptr->data;
		delete ptr;
	}

	ptr = _firstLeavingFleet.next();
	_firstLeavingFleet.unlink();

	for (; ptr && ptr != &_lastLeavingFleet; ptr = next) {
		next = ptr->next();
		delete ptr->data;
		delete ptr;
	}
}

void Star::load(ReadStream &stream) {
	int i;

	stream.read(name, STARS_NAME_SIZE);
	name[STARS_NAME_SIZE - 1] = '\0';
	x = stream.readUint16LE();
	y = stream.readUint16LE();
	size = stream.readUint8();
	owner = stream.readSint8();
	pictureType = stream.readUint8();
	spectralClass = stream.readUint8();

	for (i = 0; i < MAX_PLAYERS; i++) {
		lastPlanetSelected[i] = stream.readUint8();
	}

	for (i = 0; i < (MAX_STARS + 7)/8; i++) {
		blackHoleBlocks[i] = stream.readUint8();
	}

	special = stream.readUint8();
	wormhole = stream.readSint8();
	blockaded = stream.readUint8();

	for (i = 0; i < MAX_PLAYERS; i++) {
		blockadedBy[i] = stream.readUint8();
	}

	visited = stream.readUint8();
	justVisited = stream.readUint8();
	ignoreColonyShips = stream.readUint8();
	ignoreCombatShips = stream.readUint8();
	colonizePlayer = stream.readSint8();
	hasColony = stream.readUint8();
	hasWarpFieldInterdictor = stream.readUint8();
	nextWFIInList = stream.readUint8();
	hasTachyon = stream.readUint8();
	hasSubspace = stream.readUint8();
	hasStargate = stream.readUint8();
	hasJumpgate = stream.readUint8();
	hasArtemisNet = stream.readUint8();
	hasDimensionalPortal = stream.readUint8();
	isStagepoint = stream.readUint8();

	for (i = 0; i < MAX_PLAYERS; i++) {
		officerIndex[i] = stream.readSint8();
	}

	for (i = 0; i < MAX_ORBITS; i++) {
		planetIndex[i] = stream.readSint16LE();
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		relocateShipTo[i] = stream.readUint16LE();
	}

	stream.readUint8();
	stream.readUint8();
	stream.readUint8();

	for (i = 0; i < MAX_PLAYERS; i++) {
		surrenderTo[i] = stream.readUint8();
	}

	inNebula = stream.readUint8();
	artifactsGaveApp = stream.readUint8();
}

void Star::addFleet(Fleet *f) {
	unsigned status = f->getStatus();

	if (status != ShipState::InOrbit && status != ShipState::LeavingOrbit) {
		throw std::invalid_argument("Cannot add moving fleet to star");
	}

	if (status == ShipState::InOrbit) {
		_lastOrbitingFleet.insert(f);
	} else {
		_lastLeavingFleet.insert(f);
	}
}

BilistNode<Fleet> *Star::getOrbitingFleets(void) {
	return _firstOrbitingFleet.next();
}

BilistNode<Fleet> *Star::getLeavingFleets(void) {
	return _firstLeavingFleet.next();
}

const BilistNode<Fleet> *Star::getOrbitingFleets(void) const {
	return _firstOrbitingFleet.next();
}

const BilistNode<Fleet> *Star::getLeavingFleets(void) const {
	return _firstLeavingFleet.next();
}

unsigned Star::planetSeq(unsigned orbit) const {
	unsigned i, ret;

	for (i = 0, ret = 0; i < MAX_ORBITS && i < orbit; i++) {
		if (planetIndex[i] >= 0) {
			ret++;
		}
	}

	return ret;
}

void Star::validate(void) const {
	if (size > StarSize::Small) {
		throw std::out_of_range("Invalid star size");
	}

	if (spectralClass > SpectralClass::BlackHole) {
		throw std::out_of_range("Invalid star spectral class");
	}

	if (special == BAD_SPECIAL1 || special == BAD_SPECIAL2 ||
		special > ORION_SPECIAL) {
		throw std::out_of_range("Star has invalid special treasure");
	}

	if ((visited & hasColony) != hasColony) {
		throw std::runtime_error("Player who hasn't visited star has colony?!");
	}
}

Ship::Ship(void) {
	owner = 0;
	status = 0;
	star = 0;
	x = 0;
	y = 0;
	groupHasNavigator = 0;
	warpSpeed = 0;
	eta = 0;
	shieldDamage = 0;
	driveDamage = 0;
	computerDamage = 0;
	crewLevel = 0;
	crewExp = 0;
	officer = 0;
	memset(damagedSpecials, 0, (MAX_SHIP_SPECIALS + 7) / 8);
	armorDamage = 0;
	structureDamage = 0;
	mission = 0;
	justBuilt = 0;
}

bool Ship::operator<(const Ship &other) const {
	if (design.type != other.design.type) {
		return design.type < other.design.type;
	}

	if (design.size != other.design.size) {
		return design.size > other.design.size;
	}

	if (design.builder != other.design.builder) {
		return design.builder < other.design.builder;
	}

	return design.picture > other.design.picture;
}

bool Ship::operator<=(const Ship &other) const {
	if (design.type != other.design.type) {
		return design.type < other.design.type;
	}

	if (design.size != other.design.size) {
		return design.size > other.design.size;
	}

	if (design.builder != other.design.builder) {
		return design.builder < other.design.builder;
	}

	return design.picture >= other.design.picture;
}

bool Ship::operator>(const Ship &other) const {
	return !(*this <= other);
}

bool Ship::operator>=(const Ship &other) const {
	return !(*this < other);
}

void Ship::load(ReadStream &stream) {
	design.load(stream);
	owner = stream.readUint8();
	status = stream.readUint8();
	star = stream.readSint16LE();
	x = stream.readUint16LE();
	y = stream.readUint16LE();
	groupHasNavigator = stream.readUint8();
	warpSpeed = stream.readUint8();
	eta = stream.readUint8();
	shieldDamage = stream.readUint8();
	driveDamage = stream.readUint8();
	computerDamage = stream.readUint8();
	crewLevel = stream.readUint8();
	crewExp = stream.readUint16LE();
	officer = stream.readSint16LE();
	stream.read(damagedSpecials, (MAX_SHIP_SPECIALS + 7) / 8);
	armorDamage = stream.readUint16LE();
	structureDamage = stream.readUint16LE();
	mission = stream.readUint8();
	justBuilt = stream.readUint8();
}

unsigned Ship::getStarID(void) const {
	return star - (status <= ShipState::LeavingOrbit ? 500 * status : 0);
}

int Ship::isActive(void) const {
	return status <= ShipState::LeavingOrbit;
}

int Ship::exists(void) const {
	return status <= ShipState::UnderConstruction &&
		status != ShipState::Unknown && status != ShipState::Destroyed;
}

int Ship::hasSpecial(unsigned id) const {
	return design.hasSpecial(id);
}

int Ship::hasWorkingSpecial(unsigned id) const {
	return design.hasWorkingSpecial(id, damagedSpecials);
}

int Ship::isSpecialDamaged(unsigned id) const {
	if (id >= MAX_SHIP_SPECIALS) {
		throw std::out_of_range("Invalid ship special device ID");
	}

	return checkBitfield(damagedSpecials, id);
}

unsigned Ship::maxComputerHP(void) const {
	return design.maxComputerHP();
}

unsigned Ship::computerHP(void) const {
	return design.computerHP(computerDamage);
}

unsigned Ship::maxDriveHP(void) const {
	return design.maxDriveHP(damagedSpecials);
}

unsigned Ship::driveHP(void) const {
	return design.driveHP(driveDamage, damagedSpecials);
}

int Ship::combatSpeed(int transDimensional, int ignoreDamage) const {
	if (ignoreDamage) {
		return design.combatSpeed(transDimensional);
	} else {
		return design.combatSpeed(transDimensional, driveDamage,
			damagedSpecials);
	}
}

int Ship::beamOffense(int ignoreDamage) const {
	int ret;

	if (ignoreDamage) {
		ret = design.beamOffense();
	} else {
		ret = design.beamOffense(computerDamage, damagedSpecials);
	}

	return ret + shipCrewOffenseBonuses[crewLevel];
}

int Ship::beamDefense(int transDimensional, int ignoreDamage) const {
	int ret;

	if (ignoreDamage) {
		ret = design.beamDefense(transDimensional);
	} else {
		ret = design.beamDefense(transDimensional, driveDamage,
			damagedSpecials);
	}

	return ret + shipCrewDefenseBonuses[crewLevel];
}

void Ship::validate(void) const {
	design.validate();

	if (status > ShipState::UnderConstruction) {
		throw std::out_of_range("Invalid ship status");
	}

	if (crewLevel >= MAX_SHIP_CREW_LEVELS) {
		throw std::out_of_range("Invalid ship crew level");
	}

	if (officer >= LEADER_COUNT) {
		throw std::out_of_range("Ship has invalid officer");
	}
}

GameState::GameState(void) {
	_firstMovingFleet.insert_before(&_lastMovingFleet);
}

GameState::~GameState(void) {
	BilistNode<Fleet> *next, *ptr = _firstMovingFleet.next();

	// prevent array scans in removeFleet() called by fleet destructor
	_firstMovingFleet.unlink();

	for (; ptr && ptr != &_lastMovingFleet; ptr = next) {
		next = ptr->next();
		delete ptr->data;
		delete ptr;
	}
}

Fleet *GameState::findFleet(unsigned owner, unsigned status, unsigned x,
	unsigned y, unsigned star_id) {

	BilistNode<Fleet> *node;
	Fleet *f;
	Star *dest;
	unsigned old_star;

	if (star_id > _starSystemCount) {
		throw std::out_of_range("Invalid star ID");
	}

	switch (status) {
	case ShipState::InOrbit:
		node = _starSystems[star_id].getOrbitingFleets();
		dest = NULL;
		break;

	case ShipState::InTransit:
		node = _firstMovingFleet.next();
		dest = _starSystems + star_id;
		break;

	case ShipState::LeavingOrbit:
		old_star = findStar(x, y);
		node = _starSystems[old_star].getLeavingFleets();
		dest = _starSystems + star_id;
		break;

	default:
		return NULL;
	}

	for (; node; node = node->next()) {
		if (!node->data) {
			continue;
		}

		f = node->data;

		if (owner == f->getOwner() && x == f->getX() &&
			y == f->getY() && dest == f->getDestStar()) {
			return f;
		}
	}

	return NULL;
}

void GameState::createFleets(void) {
	unsigned i;
	Ship *ptr;
	Fleet *flt;

	for (i = 0, ptr = _ships; i < _shipCount; i++, ptr++) {
		if (!ptr->isActive()) {
			continue;
		}

		flt = findFleet(ptr->owner, ptr->status, ptr->x, ptr->y,
			ptr->getStarID());

		if (flt) {
			flt->addShip(i);
			continue;
		}

		flt = new Fleet(this, i);

		try {
			addFleet(flt);
		} catch (...) {
			delete flt;
			throw;
		}
	}
}

void GameState::addFleet(Fleet *flt) {
	if (flt->getStatus() != ShipState::InTransit) {
		flt->getOrbitedStar()->addFleet(flt);
		return;
	}

	_lastMovingFleet.insert(flt);
}

void GameState::removeFleet(Fleet *flt) {
	BilistNode<Fleet> *ptr = _firstMovingFleet.next();

	for (; ptr && ptr != &_lastMovingFleet; ptr = ptr->next()) {
		if (ptr->data && ptr->data == flt) {
			ptr->discard();
			return;
		}
	}
}

void GameState::load(SeekableReadStream &stream) {
	int i;

	// FIXME: get rid of seeks
	_gameConfig.load(stream);
	stream.seek(0x31be4, SEEK_SET);
	_galaxy.load(stream);
	stream.seek(COLONY_COUNT_OFFSET, SEEK_SET);
	_colonyCount = stream.readUint16LE();

	for (i = 0; i < MAX_COLONIES; i++) {
		_colonies[i].load(stream);
	}

	_planetCount = stream.readUint16LE();

	for (i = 0; i < MAX_PLANETS; i++) {
		_planets[i].load(stream);
	}

	_starSystemCount = stream.readUint16LE();

	for (i = 0; i < MAX_STARS; i++) {
		_starSystems[i].load(stream);
	}

	for (i = 0; i < LEADER_COUNT; i++) {
		_leaders[i].load(stream);
	}

	_playerCount = stream.readUint16LE();

	for (i = 0; i < MAX_PLAYERS; i++) {
		_players[i].load(stream);
	}

	_shipCount = stream.readUint16LE();

	for (i = 0; i < MAX_SHIPS; i++) {
		_ships[i].load(stream);
	}

	validate();
	createFleets();
}

void GameState::load(const char *filename) {
	File fr;

	if (!fr.open(filename)) {
		throw std::runtime_error("Cannot open savegame file");
	}

	load(fr);
}

void GameState::validate(void) const {
	int i, j, tmp;

	if (_colonyCount > MAX_COLONIES) {
		throw std::out_of_range("Invalid colony count");
	}

	if (_planetCount > MAX_PLANETS) {
		throw std::out_of_range("Invalid planet count");
	}

	if (_starSystemCount > MAX_STARS) {
		throw std::out_of_range("Invalid star system count");
	}

	if (_playerCount > MAX_PLAYERS) {
		throw std::out_of_range("Invalid player count");
	}

	if (_shipCount > MAX_SHIPS) {
		throw std::out_of_range("Invalid star system count");
	}

	_galaxy.validate();

	// Validate star systems
	for (i = 0; i < _starSystemCount; i++) {
		const Star *ptr = _starSystems + i;

		ptr->validate();

		if (ptr->x >= _galaxy.width || ptr->y >= _galaxy.height) {
			throw std::out_of_range("Star outside galaxy area");
		}

		if (ptr->owner >= (int)_playerCount ||
			(ptr->owner >= 0 && _players[ptr->owner].eliminated)) {
			throw std::out_of_range("Invalid star owner");
		}

		if (ptr->wormhole >= _starSystemCount) {
			throw std::out_of_range("Invalid wormhole index");
		}

		if (ptr->wormhole >= 0 &&
			_starSystems[ptr->wormhole].wormhole != i) {
			throw std::logic_error("One-way wormholes not allowed");
		}

		for (j = 0; j < MAX_ORBITS; j++) {
			const Planet *planet;

			if (ptr->planetIndex[j] < 0) {
				continue;
			} else if (ptr->planetIndex[j] >= _planetCount) {
				throw std::out_of_range("Star references invalid planet ID");
			}

			planet = _planets + ptr->planetIndex[j];

			if (planet->star != i) {
				throw std::logic_error("Planet referenced by wrong star");
			}

			if (planet->orbit != j) {
				throw std::logic_error("Planet is on wrong orbit");
			}
		}
	}

	for (i = 0; i < LEADER_COUNT; i++) {
		_leaders[i].validate();

		if (_leaders[i].playerIndex >= _playerCount) {
			throw std::out_of_range("Leader is at invalid player");
		}
	}

	// Validate players
	for (i = 0; i < _playerCount; i++) {
		_players[i].validate();

		for (j = 0; j < MAX_PLAYER_BLUEPRINTS; j++) {
			if (_players[i].blueprints[j].builder != i) {
				throw std::runtime_error("Wrong blueprint builder");
			}
		}

		for (j = 0; j < MAX_PLAYERS; j++) {
			if ((j < _playerCount && !_players[i].eliminated) ||
				!(_players[i].spies[j] & ~SPY_MISSION_MASK)) {
				continue;
			}

			fprintf(stderr, "%s spying on invalid player %d\n",
				_players[i].name, j);
		}

		for (j = i + 1; j < _playerCount; j++) {
			if (_players[i].playerContacts[j] !=
				_players[j].playerContacts[i]) {
				throw std::runtime_error("Player contact mismatch");
			}

			if (!_players[i].playerContacts[j]) {
				break;
			}

			if (_players[i].playerRelations[j] !=
				_players[j].playerRelations[i]) {
				throw std::runtime_error("Player relations mismatch");
			}

			if (_players[i].foreignPolicies[j] !=
				_players[j].foreignPolicies[i]) {
				throw std::runtime_error("Player diplomatic state mismatch");
			}

			if (_players[i].tradeTreaties[j] !=
				_players[j].tradeTreaties[i]) {
				throw std::runtime_error("Player trade treaty mismatch");
			}

			if (_players[i].researchTreaties[j] !=
				_players[j].researchTreaties[i]) {
				throw std::runtime_error("Player research treaty mismatch");
			}
		}
	}

	for (i = 0; i < _planetCount; i++) {
		const Planet *ptr = _planets + i;

		ptr->validate();

		if (ptr->star >= _starSystemCount) {
			throw std::out_of_range("Planet has invalid star ID");
		}

		if (_starSystems[ptr->star].planetIndex[ptr->orbit] != i) {
			// Yes, this can happen in the original game
			fprintf(stderr, "Warning: Planet %d not referenced by parent star\n",
				i);
		}

		if (ptr->colony >= _colonyCount) {
			throw std::out_of_range("Planet has invalid colony ID");
		}

		if (ptr->colony >= 0 && _colonies[ptr->colony].planet != i) {
			throw std::logic_error("Colony referenced by wrong planet");
		}
	}

	// Validate colonies
	for (i = 0; i < _colonyCount; i++) {
		const Colony *ptr = _colonies + i;

		if (ptr->planet < 0) {
			continue;	// Colony was destroyed, skip
		}

		ptr->validate();

		if (ptr->owner < 0 || ptr->owner >= _playerCount ||
			_players[ptr->owner].eliminated) {
			throw std::logic_error("Colony owned by invalid player");
		}

		if (ptr->planet >= _planetCount) {
			throw std::out_of_range("Colony is on invalid planet");
		}

		if (_planets[ptr->planet].colony != (int)i) {
			throw std::logic_error("Colony not referenced by parent planet");
		}

		if (ptr->climate != _planets[ptr->planet].climate &&
			(_planets[ptr->planet].climate != RADIATED ||
			ptr->climate != BARREN)) {
			throw std::logic_error("Climate mismatch between planet and colony");
		}

		for (j = 0; j < ptr->population; j++) {
			tmp = ptr->colonists[j].race;

			if ((tmp >= _playerCount && tmp < MAX_PLAYERS) ||
				tmp >= MAX_RACES) {
				throw std::logic_error("Invalid colonist race");
			}

			if (ptr->colonists[j].loyalty >= _playerCount) {
				throw std::out_of_range("Colonist loyal to invalid player");
			}
		}
	}

	// Validate ships
	for (i = 0; i < _shipCount; i++) {
		const Ship *ptr = _ships + i;

		if (ptr->status == ShipState::Destroyed) {
			continue;
		}

		ptr->validate();

		if (ptr->x >= _galaxy.width || ptr->y >= _galaxy.height) {
			throw std::out_of_range("Ship outside galaxy area");
		}

		tmp = ptr->getStarID();

		if (tmp >= _starSystemCount &&
			(ptr->status != ShipState::LeavingOrbit ||
			tmp != _starSystemCount)) {

			throw std::out_of_range("Ship has invalid star ID");
		}

		if (ptr->design.type == ShipType::COLONY_SHIP ||
			ptr->design.type == ShipType::OUTPOST_SHIP) {

			// Destination planet ID
			tmp = ptr->design.weapons[0].type;

			if (tmp >= _planetCount) {
				throw std::out_of_range(
					"Invalid destination planet");
			}

			if ((ptr->status == ShipState::LeavingOrbit ||
				ptr->status == ShipState::InTransit) &&
				tmp >= 0 &&
				_planets[tmp].star != ptr->getStarID()) {

				throw std::out_of_range(
					"Invalid destination planet");
			}
		}
	}
}

void GameState::setActivePlayer(unsigned player_id) {
	unsigned i, j;
	Player *pptr;
	Star *sptr;

	if (player_id >= _playerCount) {
		throw std::out_of_range("Invalid player ID");
	}

	pptr = _players + player_id;

	// update star system ownership cache
	for (i = 0; i < _starSystemCount; i++) {
		sptr = _starSystems + i;
		sptr->owner = -1;

		if (!sptr->hasColony || !isStarExplored(sptr, player_id)) {
			continue;
		}

		if (sptr->hasColony & (1 << player_id)) {
			sptr->owner = player_id;
			continue;
		}

		for (j = 0; j < _playerCount; j++) {
			if (sptr->hasColony & (1 << j) &&
				pptr->isPlayerVisible(j)) {

				sptr->owner = j;
				break;
			}
		}
	}
}

unsigned GameState::findStar(int x, int y) const {
	unsigned i;

	for (i = 0; i < _starSystemCount; i++) {
		if (_starSystems[i].x == x && _starSystems[i].y == y) {
			return i;
		}
	}

	throw std::runtime_error("No star at given coordinates");
}

int GameState::getOrbitingPlanetID(unsigned star_id, unsigned orbit) const {
	if (star_id >= _starSystemCount) {
		throw std::out_of_range("Invalid star ID");
	}

	if (orbit >= MAX_ORBITS) {
		throw std::out_of_range("Invalid orbit ID");
	}

	return _starSystems[star_id].planetIndex[orbit];
}

Planet *GameState::getOrbitingPlanet(unsigned star_id, unsigned orbit) {
	int id = getOrbitingPlanetID(star_id, orbit);

	return id >= 0 ? _planets + id : NULL;
}

BilistNode<Fleet> *GameState::getMovingFleets(void) {
	return _firstMovingFleet.next();
}

const BilistNode<Fleet> *GameState::getMovingFleets(void) const {
	return _firstMovingFleet.next();
}

Fleet *GameState::findFleet(const Ship *s) {
	Fleet *ret;

	if (!s->isActive()) {
		return NULL;
	}

	ret = findFleet(s->owner, s->status, s->x, s->y, s->getStarID());

	if (!ret) {
		throw std::runtime_error("Active ship without fleet");
	}

	return ret;
}

StarKnowledge GameState::isStarExplored(unsigned star_id,
	unsigned player_id) const {

	if (star_id >= _starSystemCount) {
		throw std::out_of_range("Invalid star ID");
	}

	return isStarExplored(_starSystems + star_id, player_id);
}

StarKnowledge GameState::isStarExplored(const Star *s,
	unsigned player_id) const {

	unsigned i;
	const Player *p;

	if (s->visited & (1 << player_id)) {
		return STAR_VISITED;
	}

	if (player_id >= _playerCount) {
		throw std::out_of_range("Invalid player ID");
	}

	p = _players + player_id;

	if (p->galaxyCharted || p->traits[TRAIT_OMNISCIENCE]) {
		return STAR_CHARTED;
	}

	for (i = 0; i < _playerCount; i++) {
		if (s->hasColony & (1 << i) && p->playerContacts[i]) {
			return STAR_NAME_ONLY;
		}
	}

	return STAR_UNEXPLORED;
}

unsigned GameState::planetClimate(unsigned planet_id) const {
	const Planet *ptr;

	ptr = _planets + planet_id;

	if (ptr->colony >= 0) {
		return _colonies[ptr->colony].climate;
	}

	return ptr->climate;
}

unsigned GameState::planetMaxPop(unsigned planet_id, unsigned player_id) const {
	unsigned ret, climate, climateFactor;
	const Planet *ptr;
	const Colony *cptr = NULL;
	const Player *pptr;

	if (planet_id >= _planetCount) {
		throw std::out_of_range("Invalid planet ID");
	}

	if (player_id >= _playerCount) {
		throw std::out_of_range("Invalid player ID");
	}

	ptr = _planets + planet_id;
	climate = planetClimate(planet_id);

	if (ptr->colony >= 0) {
		cptr = _colonies + ptr->colony;
		player_id = cptr->owner;
	}

	pptr = _players + player_id;

	if (pptr->traits[TRAIT_AQUATIC]) {
		climateFactor = aquaticPopFactors[climate];
	} else {
		climateFactor = climatePopFactors[climate];
	}

	if (pptr->traits[TRAIT_TOLERANT]) {
		climateFactor += 25;
	}

	climateFactor = climateFactor > 100 ? 100 : climateFactor;
	ret = ((ptr->size + 1) * 5 * climateFactor + 50) / 100;

	if (pptr->traits[TRAIT_SUBTERRANEAN]) {
		ret += 2 * (ptr->size + 1);
	}

	if (cptr && cptr->buildings[BUILDING_BIOSPHERES]) {
		ret += 2;
	}

	if (pptr->knowsTechnology(TECH_ADVANCED_CITY_PLANNING)) {
		ret += 5;
	}

	return ret;
}

unsigned GameState::shipCombatSpeed(unsigned ship_id, int ignoreDamage) const {
	if (ship_id >= _shipCount) {
		throw std::out_of_range("Invalid ship ID");
	}

	return shipCombatSpeed(_ships + ship_id, ignoreDamage);
}
unsigned GameState::shipCombatSpeed(const Ship *sptr, int ignoreDamage) const {
	int td = 0;

	if (sptr->owner < _playerCount) {
		td = _players[sptr->owner].traits[TRAIT_TRANS_DIMENSIONAL];
	}

	return sptr->combatSpeed(td, ignoreDamage);
}

int GameState::shipBeamOffense(unsigned ship_id, int ignoreDamage) const {
	if (ship_id >= _shipCount) {
		throw std::out_of_range("Invalid ship ID");
	}

	return shipBeamOffense(_ships + ship_id, ignoreDamage);
}

int GameState::shipBeamOffense(const Ship *sptr, int ignoreDamage) const {
	int ret = sptr->beamOffense(ignoreDamage);

	if (sptr->owner < _playerCount) {
		ret += _players[sptr->owner].traits[TRAIT_SHIP_ATTACK];
	}

	if (sptr->officer >= 0) {
		ret += _leaders[sptr->officer].skillBonus(SKILL_WEAPONRY);
	}

	return ret;
}

int GameState::shipBeamDefense(unsigned ship_id, int ignoreDamage) const {
	if (ship_id >= _shipCount) {
		throw std::out_of_range("Invalid ship ID");
	}

	return shipBeamDefense(_ships + ship_id, ignoreDamage);
}

int GameState::shipBeamDefense(const Ship *sptr, int ignoreDamage) const {
	int ret;

	if (sptr->owner < _playerCount) {
		const Player *owner = _players + sptr->owner;

		ret = sptr->beamDefense(owner->traits[TRAIT_TRANS_DIMENSIONAL],
			ignoreDamage);
		ret += owner->traits[TRAIT_SHIP_DEFENSE];
	} else {
		ret = sptr->beamDefense(0, ignoreDamage);
	}

	if (sptr->officer >= 0) {
		ret += _leaders[sptr->officer].skillBonus(SKILL_HELMSMAN);
	}

	return ret;
}

int GameState::leaderHireModifier(unsigned player_id) const {
	unsigned i;
	int tmp, ret = 0;
	const Leader *ptr;

	for (i = 0; i < LEADER_COUNT; i++) {
		ptr = _leaders + i;

		if (ptr->playerIndex != (int)player_id || !ptr->isEmployed()) {
			continue;
		}

		// The bonus is not cumulative, only the leader with
		// the highest effect counts
		tmp = ptr->skillBonus(SKILL_FAMOUS);
		ret = MIN(tmp, ret);
	}

	return ret;
}

unsigned GameState::leaderMaintenanceCost(unsigned leader_id,
	int modifier) const {
	unsigned ret;

	if (leader_id >= LEADER_COUNT) {
		throw std::out_of_range("Invalid leader ID");
	} else if (leader_id == LEADER_ID_LOKNAR ||
		_leaders[leader_id].hasSkill(SKILL_MEGAWEALTH)) {
		return 0;
	}

	ret = (_leaders[leader_id].hireCost(modifier) + 99) / 100;
	return MAX(ret, 1);
}

void GameState::sort_ids(unsigned *id_list, unsigned length, int player,
	gamestate_cmp_func cmp) {

	unsigned plist[3];
	unsigned tmp, pivot, i, j;

	if (length <= 1) {
		return;
	} else if (length < 6) {
		pivot = id_list[length / 2];
	} else {
		plist[0] = id_list[0];
		plist[1] = id_list[length / 2];
		plist[2] = id_list[length - 1];
		sort_ids(plist, 3, player, cmp);
		pivot = plist[1];
	}

	for (i = 0, j = length - 1; i <= j; i++, j--) {
		for (; cmp(this, player, id_list[i], pivot) < 0; i++);
		for (; cmp(this, player, id_list[j], pivot) > 0; j--);

		if (i > j) {
			break;
		}

		tmp = id_list[i];
		id_list[i] = id_list[j];
		id_list[j] = tmp;
	}

	j++;
	sort_ids(id_list, j, player, cmp);
	sort_ids(id_list + j, length - j, player, cmp);
}

void GameState::dump(void) const {
	const char *trait_names[] = {
		"Government", "Population", "Farming", "Industry", "Science",
		"Money", "Ship defense", "Ship attack", "Ground combat",
		"Spying", "Low G homeworld", "High G homeworld", "Aquatic",
		"Subterranean", "Large homeworld", "Rich homeworld",
		"Artifacts homeworld", "Cybernetic", "Lithovore", "Repulsive",
		"Charismatic", "Uncreative", "Creative", "Tolerant",
		"Fantastic traders", "Telepathic", "Lucky", "Omniscience",
		"Stealthy ships", "Trans-dimensional", "Warlord"
	};

	fprintf(stdout, "=== Config ===\n");
	fprintf(stdout, "Version: %d\n", _gameConfig.version);
	fprintf(stdout, "Save game name: %s\n", _gameConfig.saveGameName);
	fprintf(stdout, "Stardate: %d\n", _gameConfig.stardate);
	fprintf(stdout, "End of turn summary: %d\n", _gameConfig.endOfTurnSummary);
	fprintf(stdout, "End of turn wait: %d\n", _gameConfig.endOfTurnWait);
	fprintf(stdout, "Random events: %d\n", _gameConfig.randomEvents);
	fprintf(stdout, "Enemy moves: %d\n", _gameConfig.enemyMoves);
	fprintf(stdout, "Expanding help: %d\n", _gameConfig.expandingHelp);
	fprintf(stdout, "Autoselect ships: %d\n", _gameConfig.autoSelectShips);
	fprintf(stdout, "Animations: %d\n", _gameConfig.animations);
	fprintf(stdout, "Auto select colony: %d\n", _gameConfig.autoSelectColony);
	fprintf(stdout, "Show relocation lines: %d\n", _gameConfig.showRelocationLines);
	fprintf(stdout, "Show GNN Report: %d\n", _gameConfig.showGNNReport);
	fprintf(stdout, "Auto delete trade good housing: %d\n", _gameConfig.autoDeleteTradeGoodHousing);
	fprintf(stdout, "Show only serious turn summary: %d\n", _gameConfig.showOnlySeriousTurnSummary);
	fprintf(stdout, "Ship initiative: %d\n", _gameConfig.shipInitiative);

	fprintf(stdout, "=== Galaxy ===\n");
	fprintf(stdout, "Size factor: %d\n", _galaxy.sizeFactor);
	fprintf(stdout, "width: %d\n", _galaxy.width);
	fprintf(stdout, "height: %d\n", _galaxy.height);

	fprintf(stdout, "\n=== Hero ===\n");
	for (int i = 0; i < LEADER_COUNT; i++) {
		fprintf(stdout, "Name: %s\n", _leaders[i].name);
		fprintf(stdout, "Title: %s\n", _leaders[i].title);
		fprintf(stdout, "Type: %d\n", _leaders[i].type);
		fprintf(stdout, "Experience: %d\n", _leaders[i].experience);
		fprintf(stdout, "Common skills: %u\n", _leaders[i].commonSkills);
		fprintf(stdout, "Special skills: %u\n", _leaders[i].specialSkills);
		for (int j = 0; j < MAX_LEADER_TECH_SKILLS; j++) {
			fprintf(stdout, "Tech: %d\n", _leaders[i].techs[j]);
		}
		fprintf(stdout, "Picture: %d\n", _leaders[i].picture);
		fprintf(stdout, "Skill value: %d\n", _leaders[i].skillValue);
		fprintf(stdout, "Level: %d\n", _leaders[i].level);
		fprintf(stdout, "Location: %d\n", _leaders[i].location);
		fprintf(stdout, "ETA: %d\n", _leaders[i].eta);
		fprintf(stdout, "Level up: %d\n", _leaders[i].displayLevelUp);
		fprintf(stdout, "Status: %d\n", _leaders[i].status);
		fprintf(stdout, "Player: %d\n", _leaders[i].playerIndex);
	}

	fprintf(stdout, "\n=== Players (%d) ===\n", _playerCount);
	for (int i = 0; i < _playerCount; i++) {
		fprintf(stdout, "Name:\t%s\tRace:\t%s\n",
			_players[i].name, _players[i].race);
		fprintf(stdout, "Picture:\t\t%d\tColor:\t\t\t%d\tPersonality:\t\t%d\n",
			_players[i].picture, _players[i].color, _players[i].personality);
		fprintf(stdout, "Objective:\t\t%d\tTax rate:\t\t%d\tBC:\t\t\t%d\n",
			_players[i].objective, _players[i].taxRate, _players[i].BC);
		fprintf(stdout, "Total freighters:\t%d\tUsed freighters:\t%d\tCommand points:\t\t%d\n",
			_players[i].totalFreighters, _players[i].surplusFreighters, _players[i].commandPoints);
		fprintf(stdout, "Total production:\t%d\tRP:\t\t\t%d\tFood:\t\t\t%d\n",
			_players[i].industryProduced, _players[i].researchProduced, _players[i].surplusFood);
		fprintf(stdout, "Yearly BC:\t\t%d\tResearch progress:\t%u\tResearch Topic:\t\t%d\n",
			_players[i].surplusBC, _players[i].researchProgress, (int)_players[i].researchTopic);
		fprintf(stdout, "Research Item:\t\t%d\n",
			_players[i].researchItem);

		fprintf(stdout, "--- Traits ---\n");
		printf("- Government: %d\n",
			_players[i].traits[TRAIT_GOVERNMENT]);

		for (int j = 1; j < TRAITS_COUNT; j++) {
			if (_players[i].traits[j]) {
				printf("- %s: %d\n", trait_names[j],
					_players[i].traits[j]);
			}
		}
	}

	fprintf(stdout, "Number of stars: %d\n", _starSystemCount);
	for (int i = 0; i < _starSystemCount; i++) {
		const Star &star = _starSystems[i];
		fprintf(stdout, "\nName:\t%s (%d)\n", star.name, i);
		fprintf(stdout, "Class:\t\t%x\t\tSize:\t\t%x\t\tPicture:\t%x\n", (unsigned)star.spectralClass, (unsigned)star.size, star.pictureType);
		fprintf(stdout, "Position:\t%d,%d\tPrimary owner:\t%d\n", star.x, star.y, star.owner);
		fprintf(stdout, "Special:\t%d\t\tWormhole:\t%d\n", (int)star.special, star.wormhole);
	}
}

Fleet::Fleet(GameState *parent, unsigned flagship) : _parent(parent),
	_shipCount(0), _maxShips(8), _orbitedStar(-1), _destStar(-1) {

	Ship *fs;

	if (flagship >= _parent->_shipCount ||
		!_parent->_ships[flagship].isActive()) {
		throw std::invalid_argument("Invalid fleet flagship");
	}

	memset(_shipTypeCounts, 0, MAX_SHIP_TYPES * sizeof(size_t));
	memset(_combatCounts, 0, MAX_COMBAT_SHIP_CLASSES * sizeof(size_t));
	fs = _parent->_ships + flagship;

	switch (fs->status) {
	case ShipState::InOrbit:
		_orbitedStar = fs->getStarID();
		break;

	case ShipState::LeavingOrbit:
		_orbitedStar = _parent->findStar(fs->x, fs->y);
		// fall through

	case ShipState::InTransit:
		_destStar = fs->getStarID();
		break;

	default:
		throw std::logic_error("Fleet flagship has disallowed state");
	}

	_owner = fs->owner;
	_status = fs->status;
	_x = fs->x;
	_y = fs->y;
	// FIXME: recalculate properly and update ships
	_hasNavigator = fs->groupHasNavigator;
	_warpSpeed = fs->warpSpeed;
	_eta = fs->eta;

	_ships = new unsigned[_maxShips];
	_ships[_shipCount++] = flagship;
	_shipTypeCounts[fs->design.type]++;

	if (fs->design.type == COMBAT_SHIP) {
		_combatCounts[fs->design.size]++;
	}
}

Fleet::Fleet(const Fleet &other) : _parent(other._parent), _ships(NULL),
	_shipCount(other._shipCount), _orbitedStar(other._orbitedStar),
	_destStar(other._destStar), _owner(other._owner),
	_status(other._status), _x(other._x), _y(other._y),
	_hasNavigator(other._hasNavigator), _warpSpeed(other._warpSpeed),
	_eta(other._eta) {

	_maxShips = _shipCount > 8 ? _shipCount : 8;
	_ships = new unsigned[_maxShips];
	memcpy(_ships, other._ships, _shipCount * sizeof(unsigned));
	memcpy(_shipTypeCounts, other._shipTypeCounts,
		MAX_SHIP_TYPES * sizeof(size_t));
	memcpy(_combatCounts, other._combatCounts,
		MAX_COMBAT_SHIP_CLASSES * sizeof(size_t));
}

Fleet::~Fleet(void) {
	delete[] _ships;
}

void Fleet::addShip(unsigned ship_id) {
	Ship *s;
	int dest, i = 0, j = _shipCount, pos;

	if (ship_id >= _parent->_shipCount) {
		throw std::out_of_range("Invalid ship ID");
	}

	s = _parent->_ships + ship_id;

	if (s->getStarID() > _parent->_starSystemCount) {
		throw std::out_of_range("Ship has invalid star ID");
	}

	dest = s->status == ShipState::InOrbit ? -1 : s->getStarID();

	if (s->owner != _owner || s->status != _status || dest != _destStar ||
		s->x != _x || s->y != _y) {
		throw std::runtime_error("Ship state does not match fleet");
	}

	if (_shipCount >= _maxShips) {
		unsigned *tmp;
		size_t size = 2 * _maxShips;

		tmp = new unsigned[size];
		memcpy(tmp, _ships, _shipCount * sizeof(unsigned));
		delete[] _ships;
		_ships = tmp;
		_maxShips = size;
	}

	while (i < j) {
		pos = (i + j) / 2;

		if (*s < _parent->_ships[_ships[pos]]) {
			j = pos;
		} else {
			i = pos + 1;
		}
	}

	for (pos = i, i = _shipCount; i > pos; i--) {
		_ships[i] = _ships[i - 1];
	}

	_ships[pos] = ship_id;

	if (s->design.type == COMBAT_SHIP) {
		_combatCounts[s->design.size]++;
	}

	_shipCount++;
	_shipTypeCounts[s->design.type]++;
	// FIXME: update _hasNavigator, recalculate speed, eta and update ships
}

void Fleet::removeShip(size_t pos) {
	size_t i;
	const Ship *s;

	if (pos >= _shipCount) {
		throw std::out_of_range("Invalid ship index");
	}

	s = _parent->_ships + _ships[pos];

	for (i = pos; i < _shipCount - 1; i++) {
		_ships[i] = _ships[i + 1];
	}

	_shipCount--;
	_shipTypeCounts[s->design.type]--;

	if (s->design.type == COMBAT_SHIP) {
		_combatCounts[s->design.size]--;
	}
}

unsigned Fleet::getShipID(size_t pos) const {
	if (pos >= _shipCount) {
		throw std::out_of_range("Invalid ship index");
	}

	return _ships[pos];
}

Ship *Fleet::getShip(size_t pos) {
	return _parent->_ships + getShipID(pos);
}

const Ship *Fleet::getShip(size_t pos) const {
	return _parent->_ships + getShipID(pos);
}

Star *Fleet::getOrbitedStar(void) {
	return _orbitedStar >= 0 ? _parent->_starSystems + _orbitedStar : NULL;
}

const Star *Fleet::getOrbitedStar(void) const {
	return _orbitedStar >= 0 ? _parent->_starSystems + _orbitedStar : NULL;
}

Star *Fleet::getDestStar(void) {
	return _destStar >= 0 ? _parent->_starSystems + _destStar : NULL;
}

const Star *Fleet::getDestStar(void) const {
	return _destStar >= 0 ? _parent->_starSystems + _destStar : NULL;
}

size_t Fleet::shipCount(void) const {
	return _shipCount;
}

size_t Fleet::combatCount(void) const {
	return _shipTypeCounts[COMBAT_SHIP];
}

size_t Fleet::supportCount(void) const {
	return _shipCount - _shipTypeCounts[COMBAT_SHIP];
}

size_t Fleet::shipTypeCount(unsigned type) const {
	if (type >= MAX_SHIP_TYPES || type == BAD_SHIP_TYPE) {
		throw std::runtime_error("Invalid ship type");
	}

	return _shipTypeCounts[type];
}

size_t Fleet::combatClassCount(unsigned cls) const {
	if (cls >= MAX_COMBAT_SHIP_CLASSES) {
		throw std::runtime_error("Invalid combat ship class");
	}

	return _combatCounts[cls];
}

uint8_t Fleet::getOwner(void) const {
	return _owner;
}

uint8_t Fleet::getColor(void) const {
	return _owner < MAX_PLAYERS ? _parent->_players[_owner].color : _owner;
}

const char *Fleet::getRace(void) const {
	unsigned idx;

	if (_owner < MAX_PLAYERS) {
		idx = ESTR_RACENAME_ALKARI + _parent->_players[_owner].picture;
	} else {
		idx = npcFleetOwnerNames[_owner - MAX_PLAYERS];
	}

	return gameLang->estrings(idx);
}

uint8_t Fleet::getStatus(void) const {
	return _status;
}

uint16_t Fleet::getX(void) const {
	return _x;
}

uint16_t Fleet::getY(void) const {
	return _y;
}

int cmpPlanetClimate(const GameState *game, int player, unsigned a,
	unsigned b) {

	return int(game->planetClimate(b)) - int(game->planetClimate(a));
}

int cmpPlanetMinerals(const GameState *game, int player, unsigned a,
	unsigned b) {

	int mineralsA, mineralsB;

	mineralsA = game->_planets[a].minerals;
	mineralsB = game->_planets[b].minerals;
	return mineralsB - mineralsA;
}

int cmpPlanetMaxPop(const GameState *game, int player, unsigned a,
	unsigned b) {

	int popA, popB;

	popA = game->planetMaxPop(a, player);
	popB = game->planetMaxPop(b, player);
	return popB - popA;
}
