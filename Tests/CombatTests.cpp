#include "../Game/Data/GameData.h"
#include "../Game/Stats/PlayerStat.h"
#include "../Game/Stats/EnemyStat.h"
#include "../Game/Combat/WorldSlotPool.h"
#include "../Game/World/GameWorld.h"
#include "../Game/Player.h"
#include "../Game/Monster.h"
#include "../Engine/ThirdParty/nlohmann/json.hpp"
#include "../Engine/Core/UIManager.h"
#include "../Engine/Graphics/UIRadialFill.h"
#include <wincodec.h>
#include <iostream>
#include <fstream>
#include <functional>
#include <limits>

namespace
{
	int checks = 0;
	void Check(bool ok, const char* message)
	{
		++checks;
		if (!ok) throw std::runtime_error(message);
	}
	struct Pooled
	{
		bool active = false;
		void SetActive(bool value) { active = value; }
	};
	std::vector<Monster*> Monsters(GameWorld& world)
	{
		std::vector<Monster*> result;
		for (const auto& object : world.GetGameObjects())
			if (auto* monster = dynamic_cast<Monster*>(object.get())) result.push_back(monster);
		return result;
	}
	size_t Skills(GameWorld& world)
	{
		size_t result = 0;
		for (const auto& object : world.GetGameObjects())
			if (dynamic_cast<SkillObject*>(object.get()) && object->IsActive()) ++result;
		return result;
	}
	void Prepare(GameWorld& world)
	{
		auto* player = world.GetPlayer();
		player->physics.enabled = false;
		player->collider.enabled = false;
		for (auto* monster : Monsters(world))
		{
			monster->SetActive(false);
			monster->SetTarget(nullptr);
		}
	}
	void Place(Monster& monster, float x, unsigned level = 1)
	{
		monster.Spawn(level);
		monster.transform.position = {x, -0.12f};
		monster.physics.enabled = false;
		monster.SetTarget(nullptr);
	}
	void Tick(GameWorld& world, float dt)
	{
		Profiler profiler;
		profiler.BeginFrame();
		world.Update(dt, profiler);
		profiler.EndFrame();
	}
	void Stats(const GameData& data)
	{
		PlayerStat player(data.playerStat);
		Check(player.GetLevel() == 1 && player.GetExperience() == 0, "initial stats");
		player.AddExperience(9);
		Check(player.GetLevel() == 1 && player.GetExperience() == 9, "pre threshold");
		player.AddExperience(1);
		Check(player.GetLevel() == 2 && player.GetExperience() == 0, "exact threshold");
		player.Reset();
		player.AddExperience(45);
		Check(player.GetLevel() == 3 && player.GetExperience() == 5, "multi level carry");
		player.AddExperience(UINT64_MAX);
		Check(player.IsMaxLevel() && player.GetExperience() == 0, "overflow safe max");
		player.AddExperience(UINT64_MAX);
		Check(player.GetRequiredExperience() == 0, "max remains stable");
		player.Reset();
		Check(player.GetLevel() == 1, "reset progression");
		// 각 레벨에서 동일 레벨 적을 목표 횟수만큼 처치했을 때 정확히 한 레벨 상승.
		for (unsigned level = 1; level < data.playerStat.levels.size(); ++level)
		{
			auto definition = data.playerStat;
			definition.initialLevel = level;
			definition.initialExp = 0;
			PlayerStat progression(definition);
			const auto reward = data.enemyStat.levels[level - 1].killExp;
			for (unsigned kill = 0; kill < level; ++kill) progression.AddExperience(reward);
			Check(progression.GetLevel() == level, "one kill short of next level");
			progression.AddExperience(reward);
			Check(progression.GetLevel() == level + 1 && progression.GetExperience() == 0,
				"target kill count reaches next level exactly");
		}
		EnemyStat enemy(data.enemyStat);
		enemy.InitializeForSpawn(4);
		Check(enemy.GetLevel() == 4 && enemy.GetKillExperience() == 20, "enemy spawn snapshot");
	}
	void Pool()
	{
		Pooled object;
		WorldSlotPool<Pooled> pool, other;
		pool.Initialize({&object});
		auto a = pool.TryAcquire([](auto&) {});
		Check(a.has_value() && pool.Lookup(*a) == &object, "pool acquire");
		Check(!pool.TryAcquire([](auto&) {}), "pool capacity");
		Check(!pool.Lookup({}), "default handle");
		Check(!other.Lookup(*a), "foreign pool handle");
		auto invalid = *a;
		invalid.slot = 999;
		Check(!pool.Lookup(invalid), "invalid slot");
		Check(pool.Release(*a) && !pool.Release(*a), "duplicate release");
		auto b = pool.TryAcquire([](auto&) {});
		Check(b->slot == a->slot && !pool.Lookup(*a), "generation reuse");
		pool.Reset();
		Check(!pool.Lookup(*b), "reset invalidation");
		auto c = pool.TryAcquire([](auto&) {});
		pool.Initialize({&object});
		Check(!pool.Lookup(*c), "storage recreation");
	}
	void Combat(GameData data)
	{
		GameWorld world;
		world.Initialize(data);
		auto* player = world.GetPlayer();
		auto monsters = Monsters(world);
		Check(player->GetAttackHudData()[1].availability == AttackAvailability::Locked, "level1 Q locked");
		player->SubmitAttack(1);
		player->ConsumeAttack(world);
		Check(Skills(world) == 0 && player->GetAttackHudData()[1].remaining == 0,
			  "locked no cooldown or object");
		Check(world.ApplyDamageToMonster(*monsters[0], {3, 0}) == DamageResult::Killed,
			  "melee killed result");
		Check(player->GetStat().GetExperience() == 5, "kill exp immediately");
		world.ApplyDamageToMonster(*monsters[0], {3, 0});
		Check(player->GetStat().GetExperience() == 5, "dead duplicate no exp");
		Place(*monsters[0], 0.6f);
		world.ApplyDamageToMonster(*monsters[0], {3, 0});
		Check(player->GetStat().GetLevel() == 2 && player->GetStat().GetExperience() == 0, "two level1 kills reach level2");
		Check(player->GetStat().GetLevel() == 2, "unlock level2");
		player->SubmitAttack(1);
		player->ConsumeAttack(world);
		Check(Skills(world) == 1 && player->GetState() == PlayerState::SkillCast, "Q cast composition");
		Check(!player->IsAttackFrameActive(), "skill no melee hitbox");
		Check(player->GetAttackHudData()[1].remaining == 3, "Q cooldown starts on success");
		world.Reset();
		Check(Skills(world) == 0 && player->GetStat().GetLevel() == 1,
			  "world reset clears skills and growth");
		Check(player->GetAttackHudData()[4].availability == AttackAvailability::Locked, "reset R locked");
		world.Initialize(data);
		Check(Monsters(world).size() == 3, "world reinitialize references");
	}
	void SkillsAndTicks(GameData data)
	{
		// Deterministic fixtures change static data, never product unlock checks.
		data.playerStat.initialLevel = 10;
		GameWorld world;
		world.Initialize(data);
		Prepare(world);
		auto monsters = Monsters(world);
		auto* player = world.GetPlayer();
		Place(*monsters[0], 0.75f);
		Place(*monsters[1], 0.35f);
		world.SpawnSkill(AttackSlot::Q, *player);
		Tick(world, 0.8f);
		Check(monsters[1]->GetHP() == 2 && monsters[0]->GetHP() == 3,
			  "Q closest swept hit despite registration order");
		Check(Skills(world) == 0, "Q returned on contact");
		world.Reset();
		Prepare(world);
		Place(*monsters[0], 0.35f);
		Place(*monsters[1], 0.7f);
		world.SpawnSkill(AttackSlot::W, *player);
		Tick(world, 0.6f);
		Check(monsters[0]->GetHP() == 2 && monsters[1]->GetHP() == 2, "W long frame piercing");
		Tick(world, 0.1f);
		Check(monsters[1]->GetHP() == 2, "W one attempt per spawn");
		world.Reset();
		Prepare(world);
		Place(*monsters[0], 0.24f);
		world.SpawnSkill(AttackSlot::E, *player);
		Tick(world, 0.7f);
		Check(monsters[0]->GetHP() == 1, "E crossed active frames");
		world.Reset();
		Prepare(world);
		player->transform.position.x = 2.0f;
		player->SubmitAttack(3);
		player->ConsumeAttack(world);
		Check(Skills(world) == 0 && player->GetAttackHudData()[3].remaining == 0, "E no ground no cooldown");
		world.Reset();
		Prepare(world);
		data.attacks[4].speed = 0.0f;
		data.enemyStat.levels[0].maxHp = 20;
		Place(*monsters[0], 0.15f);
		monsters[0]->TakeDamage({1, 0});
		world.SpawnSkill(AttackSlot::R, *player);
		Tick(world, 4.0f);
		Check(monsters[0]->GetHP() == 11, "R eight ticks including t0 excluding expiry, hurt bypass");
		Check(Skills(world) == 0, "R expiry returns");
		world.Reset();
		Prepare(world);
		world.SpawnSkill(AttackSlot::Q, *player);
		player->TakeDamage({999, 0});
		Tick(world, 0.01f);
		Check(Skills(world) == 0, "death clears active skills");
		data.skillPoolSize = 1;
		world.Initialize(data);
		Prepare(world);
		world.SpawnSkill(AttackSlot::Q, *world.GetPlayer());
		world.GetPlayer()->FinishAttack();
		world.GetPlayer()->SubmitAttack(2);
		world.GetPlayer()->ConsumeAttack(world);
		Check(Skills(world) == 1 && world.GetPlayer()->GetAttackHudData()[2].remaining == 0,
			  "pool full no cooldown");
	}
	void Loader()
	{
		namespace fs = std::filesystem;
		const auto directory = fs::path("Tests/build/fixtures");
		fs::create_directories(directory);
		for (const char* name : {"GameData.json", "PlayerStat.json", "EnemyStat.json"})
			fs::copy_file(fs::path("Assets/Data") / name, directory / name,
						  fs::copy_options::overwrite_existing);
		using Json = nlohmann::json;
		auto read = [](const fs::path& path) {
			std::ifstream input(path);
			return Json::parse(input);
		};
		auto write = [](const fs::path& path, const Json& value) {
			std::ofstream output(path);
			output << value.dump(2);
		};
		auto fails = [&]() {
			try
			{
				LoadGameData(directory / "GameData.json");
				return false;
			}
			catch (const std::exception&)
			{
				return true;
			}
		};
		const auto player = read(directory / "PlayerStat.json");
		auto broken = player;
		broken["levels"][0]["expToNext"] = -1;
		write(directory / "PlayerStat.json", broken);
		Check(fails(), "negative exp rejected");
		broken = player;
		broken["levels"][1]["level"] = 1;
		write(directory / "PlayerStat.json", broken);
		Check(fails(), "duplicate level rejected");
		broken = player;
		broken["initialExp"] = 10;
		write(directory / "PlayerStat.json", broken);
		Check(fails(), "invalid initial exp rejected");
		write(directory / "PlayerStat.json", player);
		const auto game = read(directory / "GameData.json");
		broken = game;
		broken["schemaVersion"] = 1;
		write(directory / "GameData.json", broken);
		Check(fails(), "old schema rejected");
		broken = game;
		broken["attacks"][1]["inputKey"] = "Ctrl";
		write(directory / "GameData.json", broken);
		Check(fails(), "duplicate binding rejected");
		write(directory / "GameData.json", game);
		Check(LoadGameData(directory / "GameData.json").attacks[4].requiredLevel == 10,
			  "three file load recovers");
	}
	void InputContracts()
	{
		AttackInput input;
		Check(input.Sample({true, true, true, true, true}, true, true, false).slot == 1,
			  "Q priority over Ctrl and other skills");
		Check(input.Sample({false, true, true, true, true}, true, true, false).slot == -1,
			  "held skills not repeated");
		input.Sample({}, false, true, false);
		Check(input.Sample({true, false, false, false, false}, false, true, false).slot == 0, "Ctrl press");
		Check(input.Sample({true, false, false, false, false}, false, true, false).slot == 0,
			  "Ctrl held repeat");
		Check(input.Sample({false, false, true, false, false}, false, false, false).slot == -1,
			  "unfocused no request");
		Check(input.Sample({false, false, true, false, false}, false, true, false).slot == -1,
			  "focus regain no held skill");
		input.Sample({}, false, true, false);
		const auto restart = input.Sample({false, false, false, false, true}, true, true, true);
		Check(restart.restart && restart.slot == -1, "death R only restart");
		Check(input.Sample({false, false, false, false, true}, true, true, false).slot == -1,
			  "restart consumes held R");
		input.Sample({}, false, true, false);
		Check(input.Sample({false, false, false, false, true}, true, true, false).slot == 4,
			  "fresh R after restart");
	}

	void AdditionalContracts(GameData data)
	{
		// Verify every unlock boundary through the same availability query used by input and HUD.
		for (unsigned level = 1; level <= 10; ++level)
		{
			data.playerStat.initialLevel = level;
			Player player(data);
			const auto hud = player.GetAttackHudData();
			for (size_t slot = 1; slot < 5; ++slot)
				Check((hud[slot].availability == AttackAvailability::Locked) ==
						  (level < data.attacks[slot].requiredLevel),
					  "all unlock boundaries");
		}
		data.playerStat.initialLevel = 6;
		data.effectPoolSize = 1;
		GameWorld world;
		world.Initialize(data);
		Prepare(world);
		auto monsters = Monsters(world);
		auto* player = world.GetPlayer();
		Place(*monsters[0], 0.24f);
		Place(*monsters[1], 0.24f);
		monsters[0]->TakeDamage({1, 0});
		monsters[0]->FinishHit();
		monsters[1]->TakeDamage({1, 0});
		monsters[1]->FinishHit();
		world.SpawnSkill(AttackSlot::E, *player);
		Tick(world, 0.4f);
		Check(monsters[0]->IsDead() && monsters[1]->IsDead(),
			  "area multiple kills despite effect pool limit");
		Check(player->GetStat().GetExperience() == 10, "two targets pay once each");
		Tick(world, 0.5f);
		Check(player->GetStat().GetExperience() == 10, "animation completion no duplicate reward");
		world.Reset();
		Prepare(world);
		Place(*monsters[0], 0.22f);
		monsters[0]->TakeDamage({1, 0});
		world.SpawnSkill(AttackSlot::Q, *player);
		Tick(world, 0.01f);
		Check(monsters[0]->GetHP() == 2 && Skills(world) == 0, "Q rejected damage still ends on contact");
		world.Reset();
		Prepare(world);
		data.attacks[2].speed = 0.0f;
		Place(*monsters[0], 0.20f);
		world.SpawnSkill(AttackSlot::W, *player);
		Tick(world, 0.01f);
		Check(monsters[0]->GetHP() == 2, "W initial contact");
		monsters[0]->FinishHit();
		Tick(world, 0.01f);
		Check(monsters[0]->GetHP() == 2, "W no second hit after hurt ends");
		Place(*monsters[0], 0.20f);
		Tick(world, 0.01f);
		Check(monsters[0]->GetHP() == 2, "W can hit new spawn at same address");
		world.Reset();
		Prepare(world);
		player->SetCombatEnabled(false);
		player->SubmitAttack(1);
		player->ConsumeAttack(world);
		Check(Skills(world) == 0, "stress gate rejects creation");
		player->SetCombatEnabled(true);
		player->physics.isGrounded = false;
		player->SubmitAttack(1);
		player->ConsumeAttack(world);
		Check(Skills(world) == 0 && player->GetAttackHudData()[1].remaining == 0,
			  "airborne rejected without cooldown");
		player->physics.isGrounded = true;
		player->SubmitAttack(1);
		player->ConsumeAttack(world);
		player->FinishAttack();
		player->SubmitAttack(1);
		player->ConsumeAttack(world);
		Check(Skills(world) == 1, "cooldown prevents repeated Q");
		Tick(world, 3.0f);
		Check(player->GetAttackHudData()[1].remaining == 0, "cooldown elapsed");
		// Animator uses bounded arithmetic even for an extreme elapsed time.
		Animator animator;
		AnimationClip clip;
		clip.frameCount = 4;
		clip.frameDuration = 0.1f;
		clip.loop = false;
		animator.Play(clip);
		animator.Update(1000000.0f);
		Check(animator.IsFinished() && animator.GetCurrentFrame() == 3, "long frame animation completes");
		clip.loop = true;
		animator.Play(clip);
		animator.Update(1000000.0f);
		Check(!animator.IsFinished() && animator.GetCurrentFrame() < 4, "long frame looping bounded");
	}

	void RenderChecks(const GameData& data)
	{
		using Microsoft::WRL::ComPtr;
		ComPtr<ID3D11Device> device;
		ComPtr<ID3D11DeviceContext> context;
		D3D_FEATURE_LEVEL feature;
		Check(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
										  D3D11_SDK_VERSION, &device, &feature, &context)),
			  "WARP device");
		ResourceManager resources;
		Check(resources.Initialize(device.Get(), data.textures), "all sprite files decode/load");
		Profiler profiler;
		Renderer renderer;
		Check(renderer.Initialize(device.Get(), context.Get(), &resources, &profiler, {1280, 720}),
			  "real renderer/shaders");
		UIManager ui;
		Check(ui.Initialize(device.Get(), context.Get(), {1280, 720}), "GDI/DX11 HUD initialize");
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = 1280;
		desc.Height = 720;
		desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET;
		ComPtr<ID3D11Texture2D> target;
		ComPtr<ID3D11RenderTargetView> rtv;
		Check(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &target)) &&
				  SUCCEEDED(device->CreateRenderTargetView(target.Get(), nullptr, &rtv)),
			  "test render target");
		desc.BindFlags = 0;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		ComPtr<ID3D11Texture2D> readback;
		Check(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &readback)), "test readback");
		D3D11_VIEWPORT viewport{0, 0, 1280, 720, 0, 1};
		context->RSSetViewports(1, &viewport);
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		ComPtr<IWICImagingFactory> factory;
		Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
										 IID_PPV_ARGS(&factory))),
			  "PNG encoder");
		std::filesystem::create_directories("Tests/build/render");
		D3D11_TEXTURE2D_DESC depthDesc{};
		depthDesc.Width = 1280; depthDesc.Height = 720;
		depthDesc.MipLevels = depthDesc.ArraySize = depthDesc.SampleDesc.Count = 1;
		depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		ComPtr<ID3D11Texture2D> depth;
		ComPtr<ID3D11DepthStencilView> depthView;
		Check(SUCCEEDED(device->CreateTexture2D(&depthDesc, nullptr, &depth)) &&
			SUCCEEDED(device->CreateDepthStencilView(depth.Get(), nullptr, &depthView)), "depth target");
		std::vector<BYTE> depthReference;
		for (int fixture = 0; fixture < 7; ++fixture)
		{
			GameData sceneData = data;
			sceneData.playerStat.initialLevel = fixture == 0 ? 1 : fixture == 1 ? 4 : 10;
			GameWorld world;
			world.Initialize(sceneData);
			auto* player = world.GetPlayer();
			if (fixture == 1) player->GainExperience(15);
			if (fixture == 2 || fixture >= 4)
			{
				for (int i = 1; i < 5; ++i)
				{
					player->FinishAttack();
					player->SubmitAttack(i);
					player->ConsumeAttack(world);
				}
				// Different actor positions expose all four original skill sprites in one fixture.
				int i = 0;
				for (const auto& object : world.GetGameObjects())
					if (auto* skill = dynamic_cast<SkillObject*>(object.get()); skill && skill->IsActive())
					{
						skill->transform.position = {-0.65f + i * 0.40f, 0.35f};
						skill->age = 0.32;
						++i;
					}
			}
			if (fixture == 3) player->TakeDamage({999, 0});
			UIFrameData frame;
			frame.playerHp = player->GetHP();
			frame.playerDead = player->IsDead();
			frame.combat = {player->GetStat().GetLevel(),
							player->GetStat().GetExperience(),
							player->GetStat().GetRequiredExperience(),
							player->GetStat().IsMaxLevel(),
							player->IsDead(),
							player->GetAttackHudData()};
			if (fixture == 1)
			{
				frame.combat.attacks[1].remaining = 1.8f;
				frame.combat.attacks[1].availability = AttackAvailability::Cooldown;
			}
			ui.Update(0.016f, frame);
			if (ui.IsProfilerVisible() != (fixture == 3)) ui.ToggleProfilerPanel();
			ID3D11RenderTargetView* output = rtv.Get();
			const bool useDepth = fixture >= 4;
			context->OMSetRenderTargets(1, &output, useDepth ? depthView.Get() : nullptr);
			context->ClearDepthStencilView(depthView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
			const float clear[] = {0.02f, 0.025f, 0.04f, 1};
			context->ClearRenderTargetView(rtv.Get(), clear);
			renderer.SetRenderQueueEnabled(fixture == 5);
			renderer.Begin(useDepth, fixture == 5);
			for (auto layer : {RenderLayer::Background, RenderLayer::Environment, RenderLayer::TransparentItem,
				RenderLayer::Monster, RenderLayer::Player, RenderLayer::TransparentSkill})
			{
				const bool transparent = layer == RenderLayer::TransparentItem || layer == RenderLayer::TransparentSkill;
				if (transparent) renderer.FlushRenderQueue();
				for (const auto& object : world.GetRenderLayers()[static_cast<size_t>(layer)])
					if (object.object->IsActive())
					{
						auto info = object.object->GetRenderInfo();
						info.depth = object.depth;
						renderer.Draw(info, static_cast<size_t>(layer), useDepth && !transparent);
					}
			}
			renderer.FlushRenderQueue();
			renderer.Flush();
			renderer.Begin(false, false);
			ui.Render(renderer);
			if (fixture == 6)
			{
				renderer.Flush();
				context->ClearRenderTargetView(rtv.Get(), clear);
				for (int i = 0; i < 5; ++i)
				{
					Vector2 center{-0.8f + i * 0.4f, 0};
					renderer.DrawUIRect(center, {0.1f, 0.2f}, {1, 1, 1, 1});
					renderer.DrawUICooldown(center, {0.1f, 0.2f}, 1.0f - i * 0.25f, {0, 0, 0, 1});
				}
			}
			renderer.Flush();
			context->CopyResource(readback.Get(), target.Get());
			D3D11_MAPPED_SUBRESOURCE pixels{};
			Check(SUCCEEDED(context->Map(readback.Get(), 0, D3D11_MAP_READ, 0, &pixels)), "render readback");
			ComPtr<IWICStream> stream;
			ComPtr<IWICBitmapEncoder> encoder;
			ComPtr<IWICBitmapFrameEncode> image;
			const auto filename = L"Tests/build/render/hud-" + std::to_wstring(fixture) + L".png";
			Check(SUCCEEDED(factory->CreateStream(&stream)) &&
					  SUCCEEDED(stream->InitializeFromFilename(filename.c_str(), GENERIC_WRITE)),
				  "render output file");
			Check(SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) &&
					  SUCCEEDED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache)),
				  "PNG initialize");
			Check(SUCCEEDED(encoder->CreateNewFrame(&image, nullptr)) &&
					  SUCCEEDED(image->Initialize(nullptr)) && SUCCEEDED(image->SetSize(1280, 720)),
				  "PNG frame");
			std::vector<BYTE> bgra(1280 * 720 * 4);
			for (unsigned y = 0; y < 720; ++y)
			{
				const auto* row = static_cast<const BYTE*>(pixels.pData) + y * pixels.RowPitch;
				for (unsigned x = 0; x < 1280; ++x)
				{
					auto* pixel = bgra.data() + (y * 1280 + x) * 4;
					pixel[0] = row[x * 4 + 2];
					pixel[1] = row[x * 4 + 1];
					pixel[2] = row[x * 4];
					pixel[3] = row[x * 4 + 3];
				}
			}
			if (fixture == 4) depthReference = bgra;
			if (fixture == 5) Check(depthReference == bgra, "transparent skills identical with render queue on/off");
			if (fixture == 6)
			{
				// Quadrants in clockwise order: top-right, bottom-right, bottom-left, top-left.
				const int dx[] = {30, 30, -30, -30}, dy[] = {-35, 35, 35, -35};
				for (int i = 0; i < 5; ++i)
					for (int quadrant = 0; quadrant < 4; ++quadrant)
					{
						const int x = 128 + i * 256 + dx[quadrant], y = 360 + dy[quadrant];
						const auto value = bgra[(y * 1280 + x) * 4];
						Check(i > quadrant ? value > 250 : value < 5,
							"cooldown clockwise quadrants and zero removes overlay");
					}
			}
			WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
			Check(SUCCEEDED(image->SetPixelFormat(&format)) &&
					  IsEqualGUID(format, GUID_WICPixelFormat32bppBGRA),
				  "PNG RGBA format");
			Check(SUCCEEDED(image->WritePixels(720, 1280 * 4, static_cast<UINT>(bgra.size()), bgra.data())) &&
					  SUCCEEDED(image->Commit()) && SUCCEEDED(encoder->Commit()),
				  "PNG complete");
			context->Unmap(readback.Get(), 0);
		}
		context->ClearState();
		factory.Reset();
		CoUninitialize();
	}

} // namespace
int main()
{
	try
	{
		const auto data = LoadGameData("Assets/Data/GameData.json");
		Check(UIRadialFill::Remaining(0).empty() && UIRadialFill::Remaining(-1).empty(), "expired mask empty");
		Check(UIRadialFill::Remaining(std::numeric_limits<float>::quiet_NaN()).empty(), "invalid mask empty");
		for (float fraction : {1.0f, 0.75f, 0.5f, 0.25f})
		{
			float area = 0;
			for (const auto& triangle : UIRadialFill::Remaining(fraction))
				area += (triangle[1].x * triangle[2].y - triangle[1].y * triangle[2].x) * 0.5f;
			Check(std::abs(area - fraction * 4) < 0.0001f, "quarter mask area and winding");
		}
		Check(data.textures.size() == 55, "world textures and five icons loaded");
		Stats(data);
		Pool();
		Combat(data);
		SkillsAndTicks(data);
		Loader();
		InputContracts();
		AdditionalContracts(data);
		RenderChecks(data);
		std::cout << "PASS: " << checks << " combat/progression/handle/loader checks\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL after " << checks << ": " << error.what() << "\n";
		return 1;
	}
}
