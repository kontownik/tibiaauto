	enum alarmType {
		SKILL, RESOURCE, CHARACTER_STATUS, PROXIMITY, ONLINE, ITEMS, MESSAGE, EVENT
	};

	enum skills {
		ALL, LEVEL, MAGICLVL, FISHING, FIST, CLUB, SWORDSKILL, AXE, DISTANCE, SHIELD
	};

	enum resources {
		HP, MP, XP, SP, STAMINA, CAPACITY, SPACE
	};

	enum statuses {
		POISONED, BURNING, ELECTRIFIED, DRUNK, MANASHIELD, SLOWED, HASTED, LOGOUTBLOCK, DROWNING, FREEZING, DAZZLED, CURSED, STRENGTHENED, PZBLOCK, INPZ, BLEEDING
	};

	enum proximities {
		PLAYER, MONSTER, GM, BATTLELIST, BLACKSKULL, REDSKULL, GREENSKULL, YELLOWSKULL, WHITESKULL, ATTACKINGPLAYER
	};

	enum online {
		 CURRENTPLAYERONLINE, VIPPLAYERONLINE, NONEONLINE, HEARTONLINE, SKULLONLINE, LIGHTNINGONLINE, RETICLEONLINE, STARONLINE, YINYANGONLINE, TRIPOINTONLINE, XONLINE, DOLLARSIGNONLINE, IRONCROSSONLINE
	};

	enum message {
		ALLMESSAGES, PUBLICMESSAGES, PRIVATEMESSAGES
	};

	enum general {
		CHARACTERMOVED, CHARACTERNOTMOVED, CHARACTERHIT, WAYPOINTREACHED
	};

	enum spells {
		HASTE, GREATHASTE, MAGICSHIELD, CUSTOM
	};

	enum conditions1 {
		LEVELUP, PERCLVLACHIEVED, PERCLVLREMAINING
	};

	enum conditions2 {
		EQUAL, LESS, MORE
	};

	enum conditions3 {
		NEARBY, DISAPPEARS, ISONSCREENFOR
	};

	enum conditions4 {
		FROMALL, FROM, CONTAINS
	};

	enum conditions5 {
		LOGON, LOGOFF
	};

	enum conditions6 {
		PRESENT, ABSENT
	};

	enum skulls {
		NO_SKULL, YELLOW_SKULL, GREEN_SKULL, WHITE_SKULL, RED_SKULL, BLACK_SKULL
	};

	enum options {
		OPTIONS_BATTLE_PARANOIA		=		0x0001,
		OPTIONS_BATTLE_ANXIETY		=		0x0002,
		OPTIONS_IGNORE_SPELLS		=		0x0004,
		OPTIONS_MAKE_BLACKLIST		=		0x0008,
		OPTIONS_FLASHONALARM		=		0x0010,
	};

	enum triggerType{
		UNDEFINED, STRING, INTEGER, PointTRIGGER, DURATIONMIN, DURATIONSEC
	};
