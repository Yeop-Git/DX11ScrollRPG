#include "GameData.h"

#include <cmath>
#include <fstream>
#include <stdexcept>

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
		result.maxHp = Integer(value.at("maxHp"), "player.maxHp", 1, 1000);
		result.colliderHalfSize = Vector(value.at("colliderHalfSize"), "player.colliderHalfSize", 0.001f, 1.0f);
		result.startPosition = Vector(value.at("startPosition"), "player.startPosition", -100.0f, 100.0f);
		result.knockbackSpeed = Vector(value.at("knockbackSpeed"), "player.knockbackSpeed", 0.0f, 100.0f);
		result.attackExtent = Vector(value.at("attackExtent"), "player.attackExtent", 0.001f, 10.0f);
		result.renderOffsetY = Number(value.at("renderOffsetY"), "player.renderOffsetY", -10.0f, 10.0f);
		result.moveSpeed = Number(value.at("moveSpeed"), "player.moveSpeed", 0.0f, 100.0f);
		result.jumpSpeed = Number(value.at("jumpSpeed"), "player.jumpSpeed", 0.0f, 100.0f);
		result.invincibleDuration = Number(value.at("invincibleDuration"), "player.invincibleDuration", 0.0f, 60.0f);
		result.knockbackDuration = Number(value.at("knockbackDuration"), "player.knockbackDuration", 0.0f, 60.0f);
		result.blinkInterval = Number(value.at("blinkInterval"), "player.blinkInterval", 0.001f, 60.0f);
		result.attackFirstFrame = Integer(value.at("attackFirstFrame"), "player.attackFirstFrame", 0, result.attack.frameCount - 1);
		result.attackLastFrame = Integer(value.at("attackLastFrame"), "player.attackLastFrame", result.attackFirstFrame, result.attack.frameCount - 1);
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
		result.maxHp = Integer(value.at("maxHp"), "monster.maxHp", 1, 1000);
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
		Require(Integer(root.at("schemaVersion"), "schemaVersion", 1, 1) == 1, "schemaVersion");
		GameData result;
		result.player = ReadPlayer(root.at("player"));
		result.monster = ReadMonster(root.at("monster"));
		ReadTextures(root.at("textures"), result);
		return result;
	}
	catch (const std::exception& error)
	{
		throw std::runtime_error(filePath.string() + ": " + error.what());
	}
}
