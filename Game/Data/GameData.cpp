#include "GameData.h"

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <set>
#include <algorithm>

#include "../../Engine/ThirdParty/nlohmann/json.hpp"

namespace
{
	using Json = nlohmann::json;

	void Require(bool valid, const std::string& field)
	{
		if (!valid) throw std::runtime_error("Invalid field: " + field);
	}

	float Number(const Json& value, const std::string& field, float minimum, float maximum)
	{
		Require(value.is_number(), field + " (number required)");
		const double number = value.get<double>();
		Require(std::isfinite(number) && number >= minimum && number <= maximum, field + " (out of range)");
		return static_cast<float>(number);
	}

	int Integer(const Json& value, const std::string& field, int minimum, int maximum)
	{
		Require(value.is_number_integer(), field + " (integer required)");
		return static_cast<int>(Number(value, field, static_cast<float>(minimum), static_cast<float>(maximum)));
	}

	Vector2 Vector(const Json& value, const std::string& field, float minimum, float maximum)
	{
		Require(value.is_array() && value.size() == 2, field + " (two numbers required)");
		return { Number(value[0], field + "[0]", minimum, maximum), Number(value[1], field + "[1]", minimum, maximum) };
	}

	AnimationClip ReadClip(const Json& clips, const char* name, bool looping)
	{
		const Json& value = clips.at(name);
		const std::string field = std::string("animations.") + name;
		AnimationClip clip;
		clip.frameCount = Integer(value.at("frameCount"), field + ".frameCount", 1, 4096);
		clip.frameDuration = Number(value.at("frameDuration"), field + ".frameDuration", 0.001f, 60.0f);
		Require(value.at("loop").is_boolean(), field + ".loop (boolean required)");
		clip.loop = value.at("loop").get<bool>();
		// 현재 FSM은 Attack/Hurt/Dead 등의 종료를 기다린다. 해당 상태의 무한 반복을 허용하지 않는다.
		Require(clip.loop == looping, field + ".loop (incompatible with current FSM)");
		clip.frameSizePixels = Vector(value.at("frameSizePixels"), field + ".frameSizePixels", 1.0f, 16384.0f);
		clip.renderHalfSize = Vector(value.at("renderHalfSize"), field + ".renderHalfSize", 0.0f, 10.0f);
		Require(clip.renderHalfSize.y > 0.0f, field + ".renderHalfSize[1]");
		clip.offset = Vector(value.at("offset"), field + ".offset", -10.0f, 10.0f);
		return clip;
	}

	PlayerDefinition ReadPlayer(const Json& value)
	{
		PlayerDefinition result;
		const Json& clips = value.at("animations");
		result.idle = ReadClip(clips, "idle", true);
		result.run = ReadClip(clips, "run", true);
		result.jumpStart = ReadClip(clips, "jumpStart", false);
		result.jumpEnd = ReadClip(clips, "jumpEnd", false);
		result.attack = ReadClip(clips, "attack", false);
		result.dead = ReadClip(clips, "dead", false);
		result.colliderHalfSize = Vector(value.at("colliderHalfSize"), "player.colliderHalfSize", 0.001f, 1.0f);
		result.startPosition = Vector(value.at("startPosition"), "player.startPosition", -100.0f, 100.0f);
		result.knockbackSpeed = Vector(value.at("knockbackSpeed"), "player.knockbackSpeed", 0.0f, 100.0f);
		result.renderOffsetY = Number(value.at("renderOffsetY"), "player.renderOffsetY", -10.0f, 10.0f);
		result.moveSpeed = Number(value.at("moveSpeed"), "player.moveSpeed", 0.0f, 100.0f);
		result.jumpSpeed = Number(value.at("jumpSpeed"), "player.jumpSpeed", 0.0f, 100.0f);
		result.invincibleDuration = Number(value.at("invincibleDuration"), "player.invincibleDuration", 0.0f, 60.0f);
		result.knockbackDuration = Number(value.at("knockbackDuration"), "player.knockbackDuration", 0.0f, 60.0f);
		result.blinkInterval = Number(value.at("blinkInterval"), "player.blinkInterval", 0.001f, 60.0f);
		return result;
	}

	MonsterDefinition ReadMonster(const Json& value)
	{
		MonsterDefinition result;
		const Json& clips = value.at("animations");
		result.idle = ReadClip(clips, "idle", true);
		result.chase = ReadClip(clips, "chase", true);
		result.hurt = ReadClip(clips, "hurt", false);
		result.dead = ReadClip(clips, "dead", false);
		result.colliderHalfSize = Vector(value.at("colliderHalfSize"), "monster.colliderHalfSize", 0.001f, 1.0f);
		result.startPosition = Vector(value.at("startPosition"), "monster.startPosition", -100.0f, 100.0f);
		result.renderOffsetY = Number(value.at("renderOffsetY"), "monster.renderOffsetY", -10.0f, 10.0f);
		result.chaseSpeed = Number(value.at("chaseSpeed"), "monster.chaseSpeed", 0.0f, 100.0f);
		result.chaseRange = Number(value.at("chaseRange"), "monster.chaseRange", 0.0f, 100.0f);
		result.knockbackSpeed = Number(value.at("knockbackSpeed"), "monster.knockbackSpeed", 0.0f, 100.0f);
		return result;
	}

	void ReadTextures(const Json& value, GameData& result)
	{
		// 이름과 enum의 대응은 코드 계약이고, 실제 파일 경로는 JSON의 데이터다.
		const std::pair<const char*, SpriteId> names[] = {
			{ "PlayerIdle", SpriteId::PlayerIdle }, { "PlayerRun", SpriteId::PlayerRun },
			{ "PlayerJumpStart", SpriteId::PlayerJumpStart }, { "PlayerJumpEnd", SpriteId::PlayerJumpEnd },
			{ "PlayerAttack", SpriteId::PlayerAttack }, { "PlayerDead", SpriteId::PlayerDead },
			{ "MonsterIdle", SpriteId::MonsterIdle }, { "MonsterChase", SpriteId::MonsterChase },
			{ "MonsterHurt", SpriteId::MonsterHurt }, { "MonsterDead", SpriteId::MonsterDead },
			{ "Ground", SpriteId::Ground }, { "Tree", SpriteId::Tree }, { "Background", SpriteId::Background },
			{ "Coin", SpriteId::Coin }, { "Potion", SpriteId::Potion },
			{ "Heart", SpriteId::Heart }, { "GameOver", SpriteId::GameOver }
		};
		Require(value.is_object() && value.size() == std::size(names), "textures (all known SpriteIds required)");
		for (const auto& [name, id] : names)
		{
			const std::string path = value.at(name).get<std::string>();
			Require(!path.empty() && path.find('\0') == std::string::npos, std::string("textures.") + name);
			result.textures.emplace(id, path);
		}
	}
	Json ReadDocument(const std::filesystem::path& path)
	{
		try
		{
			std::ifstream file(path, std::ios::binary | std::ios::ate);
			if (!file) throw std::runtime_error("Cannot open file");
			const auto size = file.tellg();
			Require(size > 0 && size <= 1024 * 1024, "file size");
			std::string source(static_cast<size_t>(size), '\0');
			file.seekg(0);
			if (!file.read(source.data(), size)) throw std::runtime_error("Incomplete read");
			return Json::parse(source);
		}
		catch (const std::exception& error) { throw std::runtime_error(path.string() + ": " + error.what()); }
	}

	uint64_t Unsigned(const Json& value, const std::string& field)
	{
		Require(value.is_number_integer(), field + " (unsigned integer required)");
		Require(value.is_number_unsigned() || value.get<int64_t>() >= 0, field + " (negative)");
		return value.get<uint64_t>();
	}

	void ReadStats(const std::filesystem::path& directory, GameData& result)
	{
		const auto player = ReadDocument(directory / "PlayerStat.json");
		const auto enemy = ReadDocument(directory / "EnemyStat.json");
		Require(Integer(player.at("schemaVersion"), "PlayerStat.schemaVersion", 1, 1) == 1, "PlayerStat.version");
		Require(Integer(enemy.at("schemaVersion"), "EnemyStat.schemaVersion", 1, 1) == 1, "EnemyStat.version");
		const auto count = Integer(player.at("maxLevel"), "PlayerStat.maxLevel", 10, 1000);
		Require(player.at("levels").is_array() && player.at("levels").size() == count, "PlayerStat.levels");
		Require(enemy.at("levels").is_array() && enemy.at("levels").size() == count, "EnemyStat.levels");
		uint64_t previousExp = 0, previousReward = 0;
		for (int i = 0; i < count; ++i)
		{
			const auto& p = player.at("levels").at(i);
			const auto& e = enemy.at("levels").at(i);
			const auto field = "levels[" + std::to_string(i) + "]";
			Integer(p.at("level"), "PlayerStat." + field + ".level", i + 1, i + 1);
			Integer(e.at("level"), "EnemyStat." + field + ".level", i + 1, i + 1);
			const auto exp = Unsigned(p.at("expToNext"), "PlayerStat." + field + ".expToNext");
			const auto reward = Unsigned(e.at("killExp"), "EnemyStat." + field + ".killExp");
			Require(i == count - 1 ? exp == 0 : exp > 0 && exp >= previousExp, "PlayerStat." + field + ".expToNext");
			Require(reward > previousReward, "EnemyStat." + field + ".killExp (must increase)");
			result.playerStat.levels.push_back({ exp, Integer(p.at("maxHp"), "PlayerStat." + field + ".maxHp", 1, 1000) });
			result.enemyStat.levels.push_back({ reward, Integer(e.at("maxHp"), "EnemyStat." + field + ".maxHp", 1, 1000) });
			previousExp = exp; previousReward = reward;
		}
		result.playerStat.initialLevel = Integer(player.at("initialLevel"), "PlayerStat.initialLevel", 1, count);
		result.playerStat.initialExp = Unsigned(player.at("initialExp"), "PlayerStat.initialExp");
		const auto required = result.playerStat.levels[result.playerStat.initialLevel - 1].expToNext;
		Require(required ? result.playerStat.initialExp < required : result.playerStat.initialExp == 0, "PlayerStat.initialExp (out of range)");
	}

	std::vector<SpriteId> ReadFrames(const Json& value, GameData& result, int& nextId)
	{
		Require(value.is_array() && value.size() <= 128, "frames (array, up to 128)");
		std::vector<SpriteId> frames;
		for (const auto& frame : value)
		{
			const auto path = frame.get<std::string>();
			Require(!path.empty() && path.find('\0') == std::string::npos, "frames.path");
			const auto id = static_cast<SpriteId>(nextId++);
			result.textures.emplace(id, path);
			frames.push_back(id);
		}
		return frames;
	}

	void ReadAttacks(const Json& root, GameData& result)
	{
		const auto& values = root.at("attacks");
		Require(values.is_array() && values.size() == 5, "attacks (five slots required)");
		const char* slots[] = { "Normal", "Q", "W", "E", "R" };
		int nextId = 100; // 기존 SpriteId 영역과 분리한 개별 효과 프레임 ID.
		std::set<int> keys;
		for (size_t i = 0; i < values.size(); ++i)
		{
			const auto& value = values.at(i);
			auto& a = result.attacks[i];
			const auto field = std::string("attacks.") + slots[i];
			Require(value.at("slot") == slots[i], field + ".slot (ordered, unique)");
			a.name = value.at("name").get<std::string>();
			const auto iconPath = value.at("icon").get<std::string>();
			Require(!iconPath.empty() && iconPath.find('\0') == std::string::npos, field + ".icon");
			// 기존 월드 프레임 ID(100~867)와 겹치지 않는 HUD 아이콘 영역.
			a.icon = static_cast<SpriteId>(1000 + i);
			result.textures.emplace(a.icon, iconPath);
			a.inputKey = value.at("inputKey").get<std::string>();
			Require(!a.name.empty() && a.name.size() <= 24, field + ".name");
			Require(a.inputKey == "Ctrl" || a.inputKey == "Q" || a.inputKey == "W" || a.inputKey == "E" || a.inputKey == "R", field + ".inputKey");
			a.virtualKey = a.inputKey == "Ctrl" ? 0x11 : a.inputKey[0];
			Require(keys.insert(a.virtualKey).second, field + ".inputKey (duplicate)");
			a.requiredLevel = Integer(value.at("requiredLevel"), field + ".requiredLevel", 1, static_cast<int>(result.playerStat.levels.size()));
			a.cooldown = Number(value.at("cooldown"), field + ".cooldown", 0.0f, 600.0f);
			a.damage = Integer(value.at("damage"), field + ".damage", 1, 1000);
			a.speed = Number(value.at("speed"), field + ".speed", 0.0f, 20.0f);
			a.lifetime = Number(value.at("lifetime"), field + ".lifetime", i ? 0.001f : 0.0f, 30.0f);
			a.hitInterval = Number(value.at("hitInterval"), field + ".hitInterval", 0.01f, 30.0f);
			Require(a.lifetime / a.hitInterval <= 128.0f, field + ".hitInterval (too many ticks)");
			a.hitBox = Vector(value.at("hitBox"), field + ".hitBox", 0.001f, 2.0f);
			a.spawnOffset = Vector(value.at("spawnOffset"), field + ".spawnOffset", -2.0f, 2.0f);
			a.renderHalfSize = Vector(value.at("renderHalfSize"), field + ".renderHalfSize", 0.001f, 2.0f);
			a.frameDuration = Number(value.at("frameDuration"), field + ".frameDuration", 0.001f, 5.0f);
			a.frames = ReadFrames(value.at("frames"), result, nextId);
			Require(i == 0 || !a.frames.empty(), field + ".frames");
			Require(value.at("loop").is_boolean(), field + ".loop");
			a.loop = value.at("loop").get<bool>();
			Require(a.loop == (i == 1 || i == 2 || i == 4), field + ".loop");
			const int frames = i ? static_cast<int>(a.frames.size()) : result.player.attack.frameCount;
			a.firstActiveFrame = Integer(value.at("firstActiveFrame"), field + ".firstActiveFrame", 0, frames - 1);
			a.lastActiveFrame = Integer(value.at("lastActiveFrame"), field + ".lastActiveFrame", a.firstActiveFrame, frames - 1);
			if (i == 3) Require(a.lifetime >= (a.lastActiveFrame + 1) * a.frameDuration, field + ".lifetime");
		}
		const auto& effect = root.at("hitEffect");
		result.hitEffect.frames = ReadFrames(effect.at("frames"), result, nextId);
		Require(!result.hitEffect.frames.empty(), "hitEffect.frames");
		result.hitEffect.frameDuration = Number(effect.at("frameDuration"), "hitEffect.frameDuration", 0.001f, 5.0f);
		result.hitEffect.renderHalfSize = Vector(effect.at("renderHalfSize"), "hitEffect.renderHalfSize", 0.001f, 2.0f);
		result.skillPoolSize = Integer(root.at("pools").at("skills"), "pools.skills", 1, 512);
		result.effectPoolSize = Integer(root.at("pools").at("effects"), "pools.effects", 1, 1024);
	}

}

GameData LoadGameData(const std::filesystem::path& filePath)
{
	try
	{
		// 크기 제한은 잘못 지정한 대형 파일을 설정으로 통째로 읽는 실수를 막는다.
		std::ifstream file(filePath, std::ios::binary | std::ios::ate);
		if (!file) throw std::runtime_error("Cannot open file");
		const auto size = file.tellg();
		Require(size > 0 && size <= 1024 * 1024, "file size (1 byte to 1 MiB)");
		std::string source(static_cast<std::size_t>(size), '\0');
		file.seekg(0);
		if (!file.read(source.data(), static_cast<std::streamsize>(source.size())))
			throw std::runtime_error("Cannot read complete file");
		const Json root = Json::parse(source);
		Require(Integer(root.at("schemaVersion"), "schemaVersion", 3, 3) == 3, "schemaVersion");
		GameData result;
		result.player = ReadPlayer(root.at("player"));
		result.monster = ReadMonster(root.at("monster"));
		ReadTextures(root.at("textures"), result);
		ReadStats(filePath.parent_path(), result);
		ReadAttacks(root, result);
		return result;
	}
	catch (const std::exception& error)
	{
		throw std::runtime_error(filePath.string() + ": " + error.what());
	}
}
