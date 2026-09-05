#pragma once

enum class SpriteId
{
	//None
	None,

	//Player
	PlayerIdle,
	PlayerRun,
	PlayerJumpStart,
	PlayerJumpEnd,
	PlayerAttack,
	PlayerDead,

	//Monster
	MonsterIdle,
	MonsterChase,
	MonsterHurt,
	MonsterDead,

	//Map
	Background,
	Tree,
	Ground,

	// Item
	Coin,
	Potion,

	//UI
	Heart,
	GameOver
};