#pragma once
// Frontier: Skyrim Settlements - interface for add-ons (SKSE plugins). Version 6 (Frontier 1.9.0; 5 = 1.8.1, 4 = 1.8.0, 3 = 1.7.2,
// 2 = 1.7.0, 1 = 1.0.3).
//
// Plain C types only, so an add-on built with any compiler / CommonLib can use it. This header (only this file) is under the
// MIT licence: copy it into your own add-on freely.
//
//   #include "FrontierAPI.h"
//   // at SKSE kPostLoad or later (Frontier must be loaded first):
//   auto* frontier = FrontierAPI::Get();   // nullptr: Frontier isn't installed or is too old
//
// Every call must be made on the GAME'S MAIN THREAD (an SKSE task, a game event, a menu sink...), never from your own
// thread. Form IDs are full (load-order) IDs. A settlement is named by its workbench reference.
//
// "Away": a settler you send somewhere (a quest, a trade run, a dungeon) is marked away under your DLL's name. Frontier
// then leaves them alone - doesn't walk them home, doesn't count them in its daily work, doesn't give them jobs. Hiding /
// moving the actor is up to you. Set them back (away = false) when they return. If your DLL isn't loaded when a save is
// loaded, Frontier brings them back itself (so uninstalling an add-on never loses settlers).

#include <cstdint>

namespace FrontierAPI
{
	constexpr std::uint32_t kVersion = 8;

	// jobs (same numbers in every version; new ones are only ever added at the end)
	enum Job : std::int32_t
	{
		kNone,
		kGatherer,
		kFarmer,
		kHunter,
		kMiner,
		kWoodcutter,
		kGuard,
		kShopkeeper,
		kTaxCollector,
		kSmelter,
		kAlchemist,
		kTanner,
		kAdventurer,
		kGuildLeader,   // Frontier 1.2.0
		kQuartermaster, // Frontier 1.2.0
		kHealer,        // Frontier 1.8.0
		kPriest,        // Frontier 1.8.0
		kCook,          // Frontier 1.8.1
		kTailor,        // Frontier 1.8.1
		kScribe,        // Frontier 1.8.1
		kCaravanMaster, // Frontier 1.8.1
		// 21-28: add-on jobs (version 6) - RegisterJobKind returns the number. Which add-on gets which number depends on the
		// order add-ons register in, so never save these numbers yourself: keep your job's key and ask again each game start
		kBookkeeper = 29,  // Frontier 1.16.0 (after the add-on places)
		kHousekeeper,      // Frontier 1.16.0
		kFisher,           // Frontier 1.16.0
		kBeekeeper,        // Frontier 1.16.0
		kBard              // Frontier 1.16.0
	};

	// (version 8) a settler's skills: Novice 1 ... Master 5. Numbers never change; new ones are only ever added at the end.
	enum Skill : std::int32_t
	{
		kFarming,
		kHunting,
		kMining,
		kWoodcutting,
		kGathering,
		kFighting,
		kTrading,
		kSmithing,
		kAlchemy,
		kTailoring,
		kCooking,
		kMagic,
		kFishing,
		kMusic,
		kStewardship
	};
	// (version 8) trait bits (TraitsOf)
	enum Trait : std::uint32_t
	{
		kHardy = 1u << 0,
		kSickly = 1u << 1,
		kKeenEyed = 1u << 2,
		kGlutton = 1u << 3,
		kFrugal = 1u << 4,
		kHardWorking = 1u << 5,
		kLazy = 1u << 6,
		kBrave = 1u << 7,
		kTimid = 1u << 8,
		kCheerful = 1u << 9,
		kGloomy = 1u << 10,
		kLoyal = 1u << 11,
		kFickle = 1u << 12,
		kGreenThumb = 1u << 13,
		kStrong = 1u << 14,
		kCharming = 1u << 15
	};

	// (version 5) the workshops of manufacturing. Numbers never change; new ones are only ever added at the end.
	enum Craft : std::int32_t
	{
		kCraftSmelter,
		kCraftTanner,
		kCraftAlchemy,
		kCraftForge,
		kCraftCooking,
		kCraftTailor,
		kCraftScribe
	};

	struct Settlement
	{
		std::uint32_t bench;    // the workbench reference
		std::uint32_t world;    // worldspace (0 inside)
		std::uint32_t cell;     // inside: the cell
		std::uint32_t storage;  // the settlement's storage chest (0: not made yet)
		float         x, y, z;  // the middle of its area
		float         radius;   // its area (game units)
		char          name[64]; // UTF-8
	};

	struct Settler
	{
		std::uint32_t ref;       // the actor
		std::uint32_t bench;     // their settlement
		std::uint32_t home;      // their home's outside door (0: no home)
		std::int32_t  job;       // Job
		std::int32_t  stars;     // what they're good at, 1-5
		std::int32_t  jobStars;  // at their current job, 1-5
		std::int32_t  mood;      // 0-100 (50 content)
		std::uint8_t  away;      // marked away (by anyone)
		char          name[64];  // UTF-8
	};

	using MorningFn = void (*)(std::uint32_t a_bench, float a_gameDay, void* a_user);  // 6:00 each game day, per settlement

	// (version 2) crime and the law. Numbers never change; new ones are only ever added at the end.
	enum Crime : std::int32_t
	{
		kTheft,       // from the settlement storage
		kShopTheft,   // from a shop's stock
		kPickpocket,
		kVandalism,
		kBrawl,
		kAssault,
		kMurder,
		kJailbreak
	};
	enum Sentence : std::int32_t
	{
		kWarning,
		kFine,
		kLabour,   // unpaid work
		kStocks,
		kJail,
		kBanish,
		kDeath,
		kPardon,
		kNoSentence = 8  // (in events that aren't a sentence)
	};
	enum LawEvent : std::int32_t
	{
		kCrimeDone = 1,  // a_who did a_crime (a_who may be the player, or 0 for a stranger long gone)
		kArrested = 2,   // a_who was arrested (by you or a guard) for a_crime
		kSentenced = 3,  // a_who was sentenced to a_sentence
		kEscaped = 4,    // a_who got away from the law
		kExecuted = 5    // a_who was put to death
	};
	using LawEventFn = void (*)(std::int32_t a_event, std::uint32_t a_bench, std::uint32_t a_who, std::uint32_t a_victim, std::int32_t a_crime,
		std::int32_t a_sentence, void* a_user);

	struct IFrontier1
	{
		std::uint32_t version;          // the interface version you got (>= what you asked for)
		std::uint32_t frontierVersion;  // major << 16 | minor << 8 | patch

		// Settlements of the game that's loaded right now. While a save is being loaded (SKSE kPreLoadGame until
		// kPostLoadGame) there are none, and a settlement whose workbench no longer exists is never listed (Frontier 1.10.1+).
		std::uint32_t (*SettlementCount)();
		bool (*GetSettlement)(std::uint32_t a_index, Settlement* a_out);
		std::uint32_t (*SettlementOf)(std::uint32_t a_ref);  // the settlement whose area a reference stands in (0: none)

		std::uint32_t (*SettlerCount)(std::uint32_t a_bench);  // a_bench 0: every hired settler
		bool (*GetSettler)(std::uint32_t a_bench, std::uint32_t a_index, Settler* a_out);

		bool (*SetAway)(std::uint32_t a_ref, bool a_away, const char* a_ownerDll);  // a_ownerDll: "MyAddon.dll"
		bool (*IsAway)(std::uint32_t a_ref);

		std::int32_t (*CountInStorage)(std::uint32_t a_bench, std::uint32_t a_item);
		bool (*AddToStorage)(std::uint32_t a_bench, std::uint32_t a_item, std::int32_t a_count);
		std::int32_t (*RemoveFromStorage)(std::uint32_t a_bench, std::uint32_t a_item, std::int32_t a_count);  // how many were taken

		void (*Notify)(const char* a_text);  // top-left message
		void (*AddMorningHook)(MorningFn a_fn, void* a_user);
	};

	// version 2 (Frontier 1.7.0): everything in version 1, in the same places, then the law
	struct IFrontier2 : IFrontier1
	{
		void (*AddLawHook)(LawEventFn a_fn, void* a_user);  // told about every crime, arrest, sentence, escape, execution
		// your add-on's own crime: it goes into the settlement's Crime Ledger like any other (a_what: "a stolen horse")
		bool (*ReportCrime)(std::uint32_t a_bench, std::uint32_t a_culprit, std::uint32_t a_victim, std::int32_t a_crime, const char* a_what);
		std::int32_t (*LawAndOrder)(std::uint32_t a_bench);  // 0-100 (60 at the start)
		bool (*IsHeldByLaw)(std::uint32_t a_ref);           // under arrest, in jail, in the stocks...
	};

	// (version 3) settlement news. Numbers never change; new ones are only ever added at the end.
	enum FrontierEvent : std::int32_t
	{
		kSettlerArrived = 1,  // a_who came to live there (a_text: "banner" / "event")
		kSettlerLeft = 2,     // a_who left (a_text: "sent away", "unhappy", "hungry", "gave up waiting", "left")
		kSettlerDied = 3,     // a_who died (a_text: "attack", "contract", "died")
		kAttackWarned = 4,    // attackers seen nearby (a_value: attack kind number, a_text: its name)
		kAttackBegins = 5,    // the attack starts (a_value: how many, a_text: the kind's name)
		kAttackOver = 6,      // a_value 1: driven off, 0: they got away with something (a_text: the kind's name)
		kEventHappened = 7,   // a text-file event happened (a_text: its id)
		// version 4 (Frontier 1.8.0)
		kMarried = 8,         // a_who married a_value (the other's reference; a_text: their name)
		kChildBorn = 9,       // a_who was born (a_value 1: a daughter, 0: a son; a_text: the child's name)
		kGrewUp = 10,         // a_who grew up and is a settler now (a_text: their name)
		kFellIll = 11,        // a_who fell ill (a_value: Disease, a_text: its name)
		kOutbreak = 12,       // an outbreak in the settlement (a_value: Disease, a_text: its name)
		kFuneral = 13,        // a_who was laid to rest (a_value: how many came, a_text: their name)
		// (1.8.1) caravans
		kCaravanBack = 14,    // a_who (the caravan master) is back (a_value: gold spent, a_text: the city)
		kCaravanRobbed = 15   // the caravan was robbed on the road (a_value: gold of goods lost, a_text: the city)
	};

	// (version 4) the diseases that go round settlements (Skyrim's own)
	enum Disease : std::int32_t
	{
		kAtaxia,
		kBoneBreakFever,
		kBrainRot,
		kRattles,
		kRockjoint,
		kWitbane
	};
	using FrontierEventFn = void (*)(std::int32_t a_event, std::uint32_t a_bench, std::uint32_t a_who, std::int32_t a_value, const char* a_text,
		void* a_user);

	// version 3 (Frontier 1.7.2): everything in version 2, in the same places, then events
	struct IFrontier3 : IFrontier2
	{
		void (*AddEventHook)(FrontierEventFn a_fn, void* a_user);  // told about arrivals, departures, deaths, attacks, events
		// a text-file event (Events/*.txt) by its id, now - no chance roll, no cooldown, no 'min ...' checks
		bool (*TriggerEvent)(std::uint32_t a_bench, const char* a_eventId);
		// an attack on the settlement: warning an hour before, then the attack. a_kind: "Bandits", "Wolves"... or one a mod
		// added in Attacks/*.txt ("" or nullptr: whoever is about). a_hours: from now (1 or more).
		bool (*StartAttack)(std::uint32_t a_bench, const char* a_kind, float a_hours);
		// that NPC comes to live there as a settler (at the recruitment banner, else the workbench). Any NPC (Actor) works;
		// one not listed in a People file gets a settler's day the moment it comes. false: already a settler / no such place.
		bool (*BringSettler)(std::uint32_t a_bench, std::uint32_t a_npcBase);
		// a mood reason on everyone in the settlement for a while (-50..50 points, shown on the Wellbeing page)
		void (*AddMood)(std::uint32_t a_bench, const char* a_text, std::int32_t a_points, float a_days);
		std::uint32_t (*AttackKindCount)();
		bool (*GetAttackKind)(std::uint32_t a_index, char* a_out, std::uint32_t a_outSize);  // its name (UTF-8)
	};

	// version 4 (Frontier 1.8.0): everything in version 3, in the same places, then families and sickness
	struct IFrontier4 : IFrontier3
	{
		std::uint32_t (*SpouseOf)(std::uint32_t a_ref);     // the settler they're married to (0: nobody)
		std::int32_t (*ChildCount)(std::uint32_t a_ref);    // their children living now (not grown up yet)
		bool (*IsChild)(std::uint32_t a_ref);               // one of Frontier's settlement children
		bool (*IsSick)(std::uint32_t a_ref);
		// a settler or child falls ill with a Disease now (false: sickness is off, already ill, or not one of Frontier's people);
		// a_disease -1 cures them (true if they were ill)
		bool (*MakeSick)(std::uint32_t a_ref, std::int32_t a_disease);
	};

	// version 5 (Frontier 1.8.1): everything in version 4, in the same places, then manufacturing
	struct IFrontier5 : IFrontier4
	{
		void (*SetShopsMadeOnly)(std::uint32_t a_bench, bool a_on);  // 'shops sell only what we make'
		bool (*ShopsMadeOnly)(std::uint32_t a_bench);
		// an order at a workshop (false: that workshop can't make it). a_keep: keep a_count in the storage; else make a_count
		bool (*AddOrder)(std::uint32_t a_bench, std::int32_t a_craft, std::uint32_t a_item, std::int32_t a_count, bool a_keep);
		bool (*CaravanOut)(std::uint32_t a_bench);  // the settlement's caravan is on the road
	};

	// (version 6) an add-on's own shop type. It shows in the Homes page's shop list like Frontier's own; a settler you make
	// its Shopkeeper runs it. The strings are copied when you register. Up to 8 add-on shop types in all.
	struct ShopKind
	{
		const char*   key;      // stable and unique, e.g. "MyAddon.dll:shop" - saves remember the shop by this
		const char*   name;     // in the menus: "Fishmonger"
		const char*   title;    // a shop named after its keeper: "<first name>'s <title>" ("Fish Stall")
		std::uint32_t outfit;   // the keeper's clothes (an Outfit form ID; 0: merchant clothes)
		std::uint32_t buyList;  // what the keeper buys from you: a FormList of keywords (0: they buy nothing from you); may be one
		                        // your add-on made in game
		bool (*fits)(std::uint32_t a_item, void* a_user);                         // does this item belong in the shop
		std::int32_t (*restock)(std::uint32_t a_chest, std::int32_t a_stars, void* a_user);  // every 2 days: put the shop's goods in
		                        // its stock chest (Frontier empties it first and adds the gold); return how many you put in. 0: none
		void*         user;
	};

	// (version 6) an add-on's own job, with its own workshop on the Production page (orders, stars, materials, 'shops sell
	// only what we make' - all as Frontier's own). What the workshop can make: AddCraftItem. Up to 8 add-on jobs in all.
	struct JobKind
	{
		const char*          key;           // stable and unique, e.g. "MyAddon.dll:job"
		const char*          name;          // "Fishmonger"
		const char*          help;          // a sentence for the job list
		std::uint32_t        outfit;        // their work clothes (an Outfit form ID; 0: farm clothes)
		std::uint32_t        stationBase;   // the furniture a work spot is made of when none is placed (0: they need one placed)
		const std::uint32_t* stationKeywords;  // furniture with any of these keywords counts as their station (can be nullptr)
		std::int32_t         stationKeywordCount;
		const char*          stationName;   // "a tanning rack or a forge"
		const char*          shopKey;       // the shop type their goods are for (a ShopKind key, or "" for the storage only)
	};
	struct Need
	{
		std::uint32_t item;   // a material
		std::int32_t  count;
	};
	// (version 6) a list your add-on wants the player to tick in Frontier's menu (Production page - Add-ons), with a search box
	struct TickItem
	{
		char         label[128];  // UTF-8
		char         note[128];   // a short note on the right ("12 items")
		bool         ticked;
		bool         hint;        // drawn picked out (a suggestion - it's still only on when ticked)
	};
	struct TickList
	{
		const char* key;          // stable and unique
		const char* title;        // "SEX SHOP - MODS"
		const char* help;         // one or two sentences above the list
		std::uint32_t (*count)(void* a_user);
		bool (*get)(std::uint32_t a_index, TickItem* a_out, void* a_user);
		void (*set)(std::uint32_t a_index, bool a_on, void* a_user);  // called on the game's main thread
		void*       user;
	};

	// version 6 (Frontier 1.9.0): everything in version 5, in the same places, then add-on shops, jobs and tick lists.
	// Register at SKSE kPostLoad (then mods' Recipes/*.txt files can name your workshop too) or kDataLoaded. Craft items can be
	// added (and cleared) at any time after kDataLoaded.
	// CALLBACKS: fits() and restock() run on the game's main thread while Frontier holds its own lock - never call Frontier
	// from inside them (it would wait for itself). A tick list's count() and get() run while Frontier's menu draws (its own
	// thread, every frame): keep them quick and safe to call while set() may change things; set() runs on the main thread.
	struct IFrontier6 : IFrontier5
	{
		std::int32_t (*RegisterShopKind)(const ShopKind* a_kind);  // its shop number (-1: full, or the key is taken)
		std::int32_t (*RegisterJobKind)(const JobKind* a_kind);    // its job number (-1: full, or the key is taken)
		std::int32_t (*CraftOfJob)(std::int32_t a_job);            // the workshop of an add-on job (for AddOrder)
		void (*ClearCraftItems)(std::int32_t a_job);
		// something that job's workshop can make: a_value (gold) sets the stars it needs and the shop price Frontier uses
		bool (*AddCraftItem)(std::int32_t a_job, std::uint32_t a_item, std::int32_t a_value, const Need* a_needs, std::int32_t a_needCount);
		bool (*RegisterTickList)(const TickList* a_list);
		void (*RestockShops)(std::int32_t a_shop);  // every shop of that type restocks on the next tick (after your list changed)
	};

	// version 7 (Frontier 1.10.0): everything in version 6, in the same places, then the steward. A settlement a steward runs
	// fills its own gaps every few seconds: settlers with no home get a free bed, settlers with no job get the job the
	// settlement needs most, ready-made shops / inns / smithies nobody lives in become shops. What the player set is never
	// changed. Call from the game's main thread.
	struct IFrontier7 : IFrontier6
	{
		void (*SetSteward)(std::uint32_t a_bench, bool a_on);
		bool (*Steward)(std::uint32_t a_bench);
	};

	// version 8 (Frontier 1.16.0): everything in version 7, in the same places, then skills, traits, loyalty, hiring people
	// already in the world, and settlement upgrades. Call from the game's main thread.
	struct IFrontier8 : IFrontier7
	{
		std::int32_t (*SkillOf)(std::uint32_t a_settler, std::int32_t a_skill);  // 1-5 (0: not a settler)
		std::int32_t (*LoyaltyOf)(std::uint32_t a_settler);                      // 0-100 (-1: not a settler)
		std::uint32_t (*TraitsOf)(std::uint32_t a_settler);                      // Trait bits
		// hire someone already in the world (a named NPC): they keep their own record, packages underneath Frontier's, and get
		// their own life back if they ever leave. false (and nothing done) if they can't be hired. a_job: Job (0 none)
		bool (*HireFromWorld)(std::uint32_t a_actor, std::uint32_t a_bench, std::int32_t a_job);
		// an upgrade built in that settlement - keys: cellar, granary, kitchen, smokehouse, irrigation, apiary, toolshed, tools,
		// benches, masterbenches, market, tradehall, records, woodshed, hearths, bathhouse, garden, meetinghall, townhall, bell,
		// training, barracks, roads, stables
		bool (*UpgradeBuilt)(std::uint32_t a_bench, const char* a_key);
	};

	// exported by FrontierSettlements.dll
	using RequestFn = void* (*)(std::uint32_t a_version);

	// asks Frontier for the interface (include <Windows.h> before this header to get this helper)
#ifdef _WINDOWS_
	inline IFrontier1* Get()
	{
		const HMODULE dll = ::GetModuleHandleA("FrontierSettlements.dll");
		if (!dll) {
			return nullptr;
		}
		const auto request = reinterpret_cast<RequestFn>(::GetProcAddress(dll, "FrontierAPI_Request"));
		return request ? static_cast<IFrontier1*>(request(1)) : nullptr;
	}
	// version 2 (Frontier 1.7.0 or later; nullptr on an older Frontier - then use Get())
	inline IFrontier2* Get2()
	{
		const HMODULE dll = ::GetModuleHandleA("FrontierSettlements.dll");
		if (!dll) {
			return nullptr;
		}
		const auto request = reinterpret_cast<RequestFn>(::GetProcAddress(dll, "FrontierAPI_Request"));
		return request ? static_cast<IFrontier2*>(request(2)) : nullptr;
	}
	// version 3 (Frontier 1.7.2 or later; nullptr on an older Frontier - then use Get2() / Get())
	inline IFrontier3* Get3()
	{
		const HMODULE dll = ::GetModuleHandleA("FrontierSettlements.dll");
		if (!dll) {
			return nullptr;
		}
		const auto request = reinterpret_cast<RequestFn>(::GetProcAddress(dll, "FrontierAPI_Request"));
		return request ? static_cast<IFrontier3*>(request(3)) : nullptr;
	}
	// version 4 (Frontier 1.8.0 or later; nullptr on an older Frontier - then use Get3() and the others)
	inline IFrontier4* Get4()
	{
		const HMODULE dll = ::GetModuleHandleA("FrontierSettlements.dll");
		if (!dll) {
			return nullptr;
		}
		const auto request = reinterpret_cast<RequestFn>(::GetProcAddress(dll, "FrontierAPI_Request"));
		return request ? static_cast<IFrontier4*>(request(4)) : nullptr;
	}
	// version 5 (Frontier 1.8.1 or later; nullptr on an older Frontier - then use Get4() and the others)
	inline IFrontier5* Get5()
	{
		const HMODULE dll = ::GetModuleHandleA("FrontierSettlements.dll");
		if (!dll) {
			return nullptr;
		}
		const auto request = reinterpret_cast<RequestFn>(::GetProcAddress(dll, "FrontierAPI_Request"));
		return request ? static_cast<IFrontier5*>(request(5)) : nullptr;
	}
	// version 6 (Frontier 1.9.0 or later; nullptr on an older Frontier - then use Get5() and the others)
	inline IFrontier6* Get6()
	{
		const HMODULE dll = ::GetModuleHandleA("FrontierSettlements.dll");
		if (!dll) {
			return nullptr;
		}
		const auto request = reinterpret_cast<RequestFn>(::GetProcAddress(dll, "FrontierAPI_Request"));
		return request ? static_cast<IFrontier6*>(request(6)) : nullptr;
	}
	// version 7 (Frontier 1.10.0 or later; nullptr on an older Frontier)
	inline IFrontier7* Get7()
	{
		const HMODULE dll = ::GetModuleHandleA("FrontierSettlements.dll");
		if (!dll) {
			return nullptr;
		}
		const auto request = reinterpret_cast<RequestFn>(::GetProcAddress(dll, "FrontierAPI_Request"));
		return request ? static_cast<IFrontier7*>(request(7)) : nullptr;
	}
	// version 8 (Frontier 1.16.0 or later; nullptr on an older Frontier)
	inline IFrontier8* Get8()
	{
		const HMODULE dll = ::GetModuleHandleA("FrontierSettlements.dll");
		if (!dll) {
			return nullptr;
		}
		const auto request = reinterpret_cast<RequestFn>(::GetProcAddress(dll, "FrontierAPI_Request"));
		return request ? static_cast<IFrontier8*>(request(8)) : nullptr;
	}
#endif
}
