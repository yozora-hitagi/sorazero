#include "Config/Config.h"
#include "../botpch.h"
#include "playerbot.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotFactory.h"
#include "AccountMgr.h"
#include "ObjectMgr.h"
#include "Database/DatabaseEnv.h"
#include "PlayerbotAI.h"
#include "Player.h"
#include "AiFactory.h"

INSTANTIATE_SINGLETON_1(RandomPlayerbotMgr);

RandomPlayerbotMgr::RandomPlayerbotMgr() : PlayerbotHolder(), processTicks(0)
{
	flag_bot = 0;
}

RandomPlayerbotMgr::~RandomPlayerbotMgr()
{
}

void RandomPlayerbotMgr::UpdateAIInternal(uint32 elapsed)
{
	//clock_t start_time = clock();

	//uint32 interval = urand(sPlayerbotAIConfig.randomBotUpdateMinInterval, sPlayerbotAIConfig.randomBotUpdateMaxInterval);
	SetNextCheckDelay(sPlayerbotAIConfig.randomBotUpdateStepInterval * 1000);

	if (!sPlayerbotAIConfig.randomBotAutologin || !sPlayerbotAIConfig.enabled)
		return;

	//sLog.outBasic("Processing random bots...");

	//这里count 会在时间后 重新换， 本着减少数据库操作的想法，先把这里干掉吧，以后再说。
	/*int maxAllowedBotCount = GetEventValue(0, "bot_count");
	if (!maxAllowedBotCount)
	{
		maxAllowedBotCount = urand(sPlayerbotAIConfig.minRandomBots, sPlayerbotAIConfig.maxRandomBots);
		SetEventValue(0, "bot_count", maxAllowedBotCount,
				urand(sPlayerbotAIConfig.randomBotCountChangeMinInterval, sPlayerbotAIConfig.randomBotCountChangeMaxInterval));
	}*/

	//list<uint32> bots = GetBots();
	//list<uint32> bots;//觉得这个位置没必要遍历拿出来， 后面然后又查找，虽然查找应该很快。
	int botCount = playerBots.size();
	//int allianceNewBots = 0, hordeNewBots = 0;
	/*   int randomBotsPerInterval = (int)urand(sPlayerbotAIConfig.minRandomBotsPerInterval, sPlayerbotAIConfig.maxRandomBotsPerInterval);
	   if (!processTicks)
	   {
		   if (sPlayerbotAIConfig.randomBotLoginAtStartup)
			   randomBotsPerInterval = bots.size();
	   }*/

	//轮询查看是否有联盟或者部落玩家在线，下面 会根据情况刷新机器人。
	//bool has_alliance = false;
	//bool has_horde = false;
	//SessionMap const& smap = sWorld.GetAllSessions();
	//SessionMap::const_iterator iter;
	//for (iter = smap.begin(); iter != smap.end(); ++iter)
	//{
	//	if (Player* player = iter->second->GetPlayer())
	//	{
	//		if (player->GetPlayerbotAI()) {
	//			continue;
	//		}
	//		if (IsAlliance(player->getRace()))
	//		{
	//			has_alliance = true;
	//		}
	//		else {
	//			has_horde = true;
	//		}
	//		if (has_alliance && has_horde) {
	//			break;
	//		}
	//	}
	//}


	while (botCount++ < sPlayerbotAIConfig.randomBotsCount)
	{
		bool alliance = botCount % 2;
		uint32 bot = AddRandomBot(alliance);
		if (bot)
		{
			/*if (alliance)
				allianceNewBots++;
			else
				hordeNewBots++;*/

			//bots.push_back(bot);

			//添加机器人的时候就初始化一下，免得下面还要遍历一遍
			ProcessBot(bot, NULL);
		}
		else
			break;
	}

	//int botProcessed = 0;
	//for (list<uint32>::iterator i = bots.begin(); i != bots.end(); ++i)
	//{
	//    uint32 bot = *i;
	//    if (ProcessBot(bot,NULL))
	//        botProcessed++;

	//    //if (botProcessed >= randomBotsPerInterval)//这代码是不是有问题， 这样不是每次都只能刷新前面的机器人， 后面的可能永远不会刷新
	//    //    break;
	//}

	//改成用 数字 每次查找，  用迭代器 指针， 如果 机器人登出，是不是 可能出问题。
	PlayerBotMap::const_iterator bot_it;
	uint32 bot_i;
	for (bot_it = GetPlayerBotsBegin(), bot_i = 0; bot_it != GetPlayerBotsEnd() && bot_i<flag_bot; bot_it++,bot_i++) {
		
	}

	//改成一次 一个step 更新一个机器人状态
	if (bot_it != GetPlayerBotsEnd()) {
		Player* bot = bot_it->second;
		ProcessBot(NULL, bot);
		sLog.outDetail("Bot update : %s", bot->GetName());
		flag_bot++;
		flag_bot = flag_bot % playerBots.size();
	}
	
	
	
	//已登录的直接遍历不好么
	//for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
	//{
	//	Player* bot = it->second;
	//	if (ProcessBot(NULL, bot))
	//		botProcessed++;
	//}

	//这里的 clock 貌似要 配合 CLOCKS_PER_SEC 一起使用， 在windows下 貌似返回的是 ms ，在别的地方就不一定了。
	//clock_t update_cost = clock() - start_time;

	//sLog.outString("%d bots processed. %d alliance and %d horde bots added. %d bots online. Next check in %d seconds",
	//	botProcessed, allianceNewBots, hordeNewBots, playerBots.size(), interval);

    if (processTicks++ == 1)
        PrintStats();


}

uint32 RandomPlayerbotMgr::AddRandomBot(bool alliance)
{
    vector<uint32> bots = GetFreeBots(alliance);
    if (bots.size() == 0)
        return 0;

    int index = urand(0, bots.size() - 1);
    uint32 bot = bots[index];

	//uint32 add = urand(sPlayerbotAIConfig.minRandomBotInWorldTime, sPlayerbotAIConfig.maxRandomBotInWorldTime);
    //SetEventValue(bot, "add", 1, add);
	//放到ScheduleRandomize，感觉是永远不会logout啊，所以放这里
	//根据下面的代码，add时间到了后就不会更新bot了。 下面的logout就不会做了。 这么说来 add 感觉应该有logout的功能。
	//SetEventValue(bot, "logout", 1, add + urand(sPlayerbotAIConfig.minRandomBotInWorldTime, sPlayerbotAIConfig.maxRandomBotInWorldTime));
    //uint32 randomTime = 30 + urand(sPlayerbotAIConfig.randomBotUpdateInterval, sPlayerbotAIConfig.randomBotUpdateInterval * 3);
    //ScheduleRandomize(bot, randomTime);
   // sLog.outDetail("Random bot %d added", bot);
    return bot;
}

//void RandomPlayerbotMgr::ScheduleRandomize(uint32 bot, uint32 time)
//{
//    SetEventValue(bot, "randomize", 1, time);
//	//这样岂不是永远不会logout了
//    //SetEventValue(bot, "logout", 1, time + 30 + urand(sPlayerbotAIConfig.randomBotUpdateInterval, sPlayerbotAIConfig.randomBotUpdateInterval * 3));
//}

//void RandomPlayerbotMgr::ScheduleTeleport(uint32 bot)
//{
//    SetEventValue(bot, "teleport", 1, 60 + urand(sPlayerbotAIConfig.randomBotUpdateInterval, sPlayerbotAIConfig.randomBotUpdateInterval * 3));
//}

bool RandomPlayerbotMgr::ProcessBot(uint32 bot,Player* _player)
{
	Player* player;
	if (bot) {
		player = GetPlayerBot(bot);
		if (!player)
		{
			sLog.outDetail("Bot %d logged in", bot);
			AddPlayerBot(bot, 0);
			return true;
		}
	}
	else if (_player) {
		player = _player;
	}
	else {
		return false;
	}
    

    PlayerbotAI* ai = player->GetPlayerbotAI();
    if (!ai)
        return false;

	

	if (!player->GetGroup()) {
		uint32 t = time(0);
		uint32 t1 =player->m_logintime +sPlayerbotAIConfig.minRandomBotInWorldTime;
		if (t > t1) {
			uint32 t2 = player->m_logintime + sPlayerbotAIConfig.maxRandomBotInWorldTime;
			if (t > t2) {
				LogoutPlayerBot(bot);
				sLog.outDetail("Bot %d logged out", bot);
				return true;
			}

			if (urand(0, 100) < 5) {//当到了登出的时间范围的时候， 每次检查有百分之5的几率下线
				LogoutPlayerBot(bot);
				sLog.outDetail("Bot %d logged out", bot);
				return true;
			}
		}

		//未组队的机器人，全部就不刷新了。减少CPU，全部挂机。
		return true;
	}

	//update时间 改成了 范围的，这里改成每次 都随机一下。
	Randomize(player);


    if (player->IsDead())
    {
		//直接 信春哥
		PlayerbotChatHandler ch(player);
		ch.revive(*player);
		player->GetPlayerbotAI()->ResetStrategies();
    }

    return false;
}

void RandomPlayerbotMgr::RandomTeleport(Player* bot, vector<WorldLocation> &locs)
{
    if (bot->IsBeingTeleported())
        return;

    if (locs.empty())
    {
        sLog.outError("Cannot teleport bot %s - no locations available", bot->GetName());
        return;
    }

    for (int attemtps = 0; attemtps < 10; ++attemtps)
    {
        int index = urand(0, locs.size() - 1);
        WorldLocation loc = locs[index];
        float x = loc.coord_x + urand(0, sPlayerbotAIConfig.grindDistance) - sPlayerbotAIConfig.grindDistance / 2;
        float y = loc.coord_y + urand(0, sPlayerbotAIConfig.grindDistance) - sPlayerbotAIConfig.grindDistance / 2;
        float z = loc.coord_z;

        Map* map = sMapMgr.FindMap(loc.mapid);
        if (!map)
            continue;

        const TerrainInfo * terrain = map->GetTerrain();
        if (!terrain)
            continue;

        AreaTableEntry const* area = sAreaStore.LookupEntry(terrain->GetAreaId(x, y, z));
        if (!area)
            continue;

        if (!terrain->IsOutdoors(x, y, z) ||
                terrain->IsUnderWater(x, y, z) ||
                terrain->IsInWater(x, y, z))
            continue;

        sLog.outDetail("Random teleporting bot %s to %s %f,%f,%f", bot->GetName(), area->area_name[0], x, y, z);
        float height = map->GetTerrain()->GetHeightStatic(x, y, 0.5f + z, true, MAX_HEIGHT);
        if (height <= INVALID_HEIGHT)
            continue;

        z = 0.05f + map->GetTerrain()->GetHeightStatic(x, y, 0.05f + z, true, MAX_HEIGHT);

        bot->GetMotionMaster()->Clear();
        bot->TeleportTo(loc.mapid, x, y, z, 0);
        return;
    }

    sLog.outError("Cannot teleport bot %s - no locations available", bot->GetName());
}

void RandomPlayerbotMgr::RandomTeleportForLevel(Player* bot)
{
    vector<WorldLocation> locs;
    QueryResult* results = WorldDatabase.PQuery("select map, position_x, position_y, position_z "
        "from (select map, position_x, position_y, position_z, avg(t.maxlevel), avg(t.minlevel), "
        "%u - (avg(t.maxlevel) + avg(t.minlevel)) / 2 delta "
        "from creature c inner join creature_template t on c.id = t.entry group by t.entry) q "
        "where delta >= 0 and delta <= %u and map in (%s)",
        bot->getLevel(), sPlayerbotAIConfig.randomBotTeleLevel, sPlayerbotAIConfig.randomBotMapsAsString.c_str());
    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            uint32 mapId = fields[0].GetUInt32();
            float x = fields[1].GetFloat();
            float y = fields[2].GetFloat();
            float z = fields[3].GetFloat();
            WorldLocation loc(mapId, x, y, z, 0);
            locs.push_back(loc);
        } while (results->NextRow());
        delete results;
    }

    RandomTeleport(bot, locs);

	//下面的这个调用的refresh的方法没地方调用了。 所以先把refresh 方法在这里掉一下， 感觉改的有些问题了。 先思考一下。
	Refresh(bot);
}

/******************************
这个位置，先随机到一个地图， 然后 在 refresh ， 在bot更新线程里面死了后会通过这个方法 刷成活的。
*****/
void RandomPlayerbotMgr::RandomTeleport(Player* bot, uint32 mapId, float teleX, float teleY, float teleZ)
{
    vector<WorldLocation> locs;
    QueryResult* results = WorldDatabase.PQuery("select position_x, position_y, position_z from creature where map = '%u' and abs(position_x - '%f') < '%u' and abs(position_y - '%f') < '%u'",
            mapId, teleX, sPlayerbotAIConfig.randomBotTeleportDistance / 2, teleY, sPlayerbotAIConfig.randomBotTeleportDistance / 2);
    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            float x = fields[0].GetFloat();
            float y = fields[1].GetFloat();
            float z = fields[2].GetFloat();
            WorldLocation loc(mapId, x, y, z, 0);
            locs.push_back(loc);
        } while (results->NextRow());
        delete results;
    }

    RandomTeleport(bot, locs);
    Refresh(bot);
}

//
void RandomPlayerbotMgr::Randomize(Player* bot)
{
    //if (bot->getLevel() == 1)
    //    RandomizeFirst(bot);
    //else
        IncreaseLevel(bot);
}

void RandomPlayerbotMgr::IncreaseLevel(Player* bot)
{
  /*  uint32 maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);
    uint32 level = min(bot->getLevel() + 1, maxLevel);*/

	//个人修改为一段时间获取多少经验，另外去掉factory里面的setlevel方法
	//看代码，改等级应该用givelevel方法，而不是set， givelevel 会发出等级变化的通知。
	//所有等级经验都差不多不太好。

	if (!bot->GetGroup()) {
		//在队伍中没必要额外加经验
		uint16 xp = urand(20, 100) + bot->getLevel();
		bot->GiveXP(xp, NULL);
	}
	bot->ModifyMoney(urand(0, 100));

    PlayerbotFactory factory(bot, bot->getLevel());
    /*if (bot->GetGuildId())
        factory.Refresh();
    else
        factory.Randomize();*/
	factory.Supply();

	//每次加经验就随机，感觉没必要。在 levelupaction 里面调用了一下， 这里就不调用了。
	/*if (!bot->GetGroup())
		RandomTeleportForLevel(bot);*/
}


//因为组队的机器人才会刷新， 而组队时会 根据玩家等级处理，  这个方法似乎多余了
void RandomPlayerbotMgr::RandomizeFirst(Player* bot)
{


    uint32 maxLevel = sPlayerbotAIConfig.randomBotMaxLevel;
    if (maxLevel > sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
        maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);

	//简化第一次等级随机的策略，因为地图高等级比低等级多，所以，随意等级会有倾向性，完全随机个人觉得理论表现应该更好。
	uint32 level = urand(sPlayerbotAIConfig.randomBotMinLevel, maxLevel);
	if (urand(0, 100) < 100 * sPlayerbotAIConfig.randomBotMaxLevelChance)
		level = maxLevel;

	bot->GiveLevel(level);

	/*PlayerbotFactory factory(bot, level);
	factory.RandomizeFirst();*/

	RandomTeleportForLevel(bot);

  /*  for (int attempt = 0; attempt < 100; ++attempt)
    {
        int index = urand(0, sPlayerbotAIConfig.randomBotMaps.size() - 1);
        uint32 mapId = sPlayerbotAIConfig.randomBotMaps[index];

        vector<GameTele const*> locs;
        GameTeleMap const & teleMap = sObjectMgr.GetGameTeleMap();
        for(GameTeleMap::const_iterator itr = teleMap.begin(); itr != teleMap.end(); ++itr)
        {
            GameTele const* tele = &itr->second;
            if (tele->mapId == mapId)
                locs.push_back(tele);
        }

        index = urand(0, locs.size() - 1);
        if (index >= locs.size())
            return;
        GameTele const* tele = locs[index];
        uint32 level = GetZoneLevel(tele->mapId, tele->position_x, tele->position_y, tele->position_z);
        if (level > maxLevel + 5)
            continue;

        level = min(level, maxLevel);
        if (!level) level = 1;

        if (urand(0, 100) < 100 * sPlayerbotAIConfig.randomBotMaxLevelChance)
            level = maxLevel;

        if (level < sPlayerbotAIConfig.randomBotMinLevel)
            continue;

        PlayerbotFactory factory(bot, level);
        factory.CleanRandomize();
        RandomTeleport(bot, tele->mapId, tele->position_x, tele->position_y, tele->position_z);
        break;
    }*/
}

uint32 RandomPlayerbotMgr::GetZoneLevel(uint32 mapId, float teleX, float teleY, float teleZ)
{
    uint32 maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);

    uint32 level;
    QueryResult *results = WorldDatabase.PQuery("select avg(t.minlevel) minlevel, avg(t.maxlevel) maxlevel from creature c "
            "inner join creature_template t on c.id = t.entry "
            "where map = '%u' and minlevel > 1 and abs(position_x - '%f') < '%u' and abs(position_y - '%f') < '%u'",
            mapId, teleX, sPlayerbotAIConfig.randomBotTeleportDistance / 2, teleY, sPlayerbotAIConfig.randomBotTeleportDistance / 2);

    if (results)
    {
        Field* fields = results->Fetch();
        uint32 minLevel = fields[0].GetUInt32();
        uint32 maxLevel = fields[1].GetUInt32();
        level = urand(minLevel, maxLevel);
        if (level > maxLevel)
            level = maxLevel;
        delete results;
    }
    else
    {
        level = urand(1, maxLevel);
    }

    return level;
}

void RandomPlayerbotMgr::Refresh(Player* bot)
{
    if (bot->IsDead())
    {
        PlayerbotChatHandler ch(bot);
        ch.revive(*bot);
        bot->GetPlayerbotAI()->ResetStrategies();
    }

    bot->GetPlayerbotAI()->Reset();

    HostileReference *ref = bot->GetHostileRefManager().getFirst();
    while( ref )
    {
        ThreatManager *threatManager = ref->getSource();
        Unit *unit = threatManager->getOwner();
        float threat = ref->getThreat();

        unit->RemoveAllAttackers();
        unit->ClearInCombat();

        ref = ref->next();
    }

    bot->RemoveAllAttackers();
    bot->ClearInCombat();

    bot->DurabilityRepairAll(false, 1.0f);
    bot->SetHealthPercent(100);
    bot->SetPvP(true);

    if (bot->GetMaxPower(POWER_MANA) > 0)
        bot->SetPower(POWER_MANA, bot->GetMaxPower(POWER_MANA));

    if (bot->GetMaxPower(POWER_ENERGY) > 0)
        bot->SetPower(POWER_ENERGY, bot->GetMaxPower(POWER_ENERGY));
}


bool RandomPlayerbotMgr::IsRandomBot(Player* bot)
{
    return IsRandomBot(bot->GetObjectGuid());
}

bool RandomPlayerbotMgr::IsRandomBot(uint32 bot)
{
    return sPlayerbotAIConfig.GetEventValue(bot, "add");
}

/**
之前是查询的ai_playerbot_random_bots表，想稍微提高性能，想干掉这张表，所以这里从已登录里面拿数据，更直接。

表里面的意思应该是添加进来需要登录的机器人，这里这是获取在线机器人
*/
list<uint32> RandomPlayerbotMgr::GetBots()
{
    list<uint32> bots;

	for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
	{
		Player* bot = it->second;
		bots.push_back(bot->GetObjectGuid().GetRawValue());
	}


  /*  QueryResult* results = CharacterDatabase.Query(
            "select bot from ai_playerbot_random_bots where owner = 0 and event = 'add'");

    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            uint32 bot = fields[0].GetUInt32();
            bots.push_back(bot);
        } while (results->NextRow());
        delete results;
    }*/

    return bots;
}

/**
上面getbots 方法变了， 这里也要变
*/
vector<uint32> RandomPlayerbotMgr::GetFreeBots(bool alliance)
{
//    set<uint32> bots;
  /*  QueryResult* results = CharacterDatabase.PQuery(
            "select `bot` from ai_playerbot_random_bots where event = 'add'");

    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            uint32 bot = fields[0].GetUInt32();
            bots.insert(bot);
        } while (results->NextRow());
        delete results;
    }
*/
    vector<uint32> guids;
	for (list<uint32>::iterator i = sPlayerbotAIConfig.randomBotAccounts.begin(); i != sPlayerbotAIConfig.randomBotAccounts.end(); i++)
	{
		uint32 accountId = *i;
		if (!sAccountMgr.GetCharactersCount(accountId))
			continue;

		QueryResult *result = CharacterDatabase.PQuery("SELECT guid, race FROM characters WHERE account = '%u'", accountId);
		if (!result)
			continue;

		do
		{
			Field* fields = result->Fetch();
			uint32 guid = fields[0].GetUInt32();
			uint32 race = fields[1].GetUInt32();
			if (playerBots.find(guid) == playerBots.end() &&
				(( alliance && IsAlliance(race) ) || (( !alliance && !IsAlliance(race) ))))
				guids.push_back(guid);
		} while (result->NextRow());
		delete result;
	}


    return guids;
}



bool ChatHandler::HandlePlayerbotConsoleCommand(char* args)
{
    if (!sPlayerbotAIConfig.enabled)
    {
        PSendSysMessage("Playerbot system is currently disabled!");
        SetSentErrorMessage(true);
        return false;
    }

    if (!args || !*args)
    {
        sLog.outError("Usage: rndbot stats (or with .bot cmd args)");
        return false;
    }

    string cmd = args;

    //if (cmd == "reset")
    //{
    //    CharacterDatabase.PExecute("delete from ai_playerbot_random_bots");
    //    sLog.outBasic("Random bots were reset for all players");
    //    return true;
    //}
    //else 
	if (cmd == "stats")
    {
        sRandomPlayerbotMgr.PrintStats();
        return true;
    }
    //else if (cmd == "update")
	//{
    //    sRandomPlayerbotMgr.UpdateAIInternal(0);
    //    return true;
    //}
    //else if (cmd == "init" || cmd == "refresh")
    //{
    //    sLog.outString("Randomizing bots for %d accounts", sPlayerbotAIConfig.randomBotAccounts.size());
    //    BarGoLink bar(sPlayerbotAIConfig.randomBotAccounts.size());
    //    for (list<uint32>::iterator i = sPlayerbotAIConfig.randomBotAccounts.begin(); i != sPlayerbotAIConfig.randomBotAccounts.end(); ++i)
    //    {
    //        uint32 account = *i;
    //        bar.step();
    //        if (QueryResult *results = CharacterDatabase.PQuery("SELECT guid FROM characters where account = '%u'", account))
    //        {
    //            do
    //            {
    //                Field* fields = results->Fetch();
    //                ObjectGuid guid = ObjectGuid(fields[0].GetUInt64());
    //                Player* bot = sObjectMgr.GetPlayer(guid, true);
    //                if (!bot)
    //                    continue;

    //                if (cmd == "init")
    //                {
    //                    sLog.outDetail("Randomizing bot %s for account %u", bot->GetName(), account);
    //                    sRandomPlayerbotMgr.RandomizeFirst(bot);
    //                }
    //                else
    //                {
    //                    sLog.outDetail("Refreshing bot %s for account %u", bot->GetName(), account);
    //                    //bot->SetLevel(bot->getLevel() - 1);
    //                    sRandomPlayerbotMgr.IncreaseLevel(bot);
    //                }
    //               /* uint32 randomTime = urand(sPlayerbotAIConfig.minRandomBotRandomizeTime, sPlayerbotAIConfig.maxRandomBotRandomizeTime);
    //                CharacterDatabase.PExecute("update ai_playerbot_random_bots set validIn = '%u' where event = 'randomize' and bot = '%u'",
    //                        randomTime, bot->GetGUIDLow());
    //                CharacterDatabase.PExecute("update ai_playerbot_random_bots set validIn = '%u' where event = 'logout' and bot = '%u'",
    //                        sPlayerbotAIConfig.maxRandomBotInWorldTime, bot->GetGUIDLow());*/
    //            } while (results->NextRow());

    //            delete results;
    //        }
    //    }
    //    return true;
    //}
    else
    {
        list<string> messages = sRandomPlayerbotMgr.HandlePlayerbotCommand(args, NULL);
        for (list<string>::iterator i = messages.begin(); i != messages.end(); ++i)
        {
            sLog.outString(i->c_str());
        }
        return true;
    }

    return false;
}

void RandomPlayerbotMgr::HandleCommand(uint32 type, const string& text, Player& fromPlayer)
{
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        bot->GetPlayerbotAI()->HandleCommand(type, text, fromPlayer);
    }
}


void RandomPlayerbotMgr::HandleChannelMessage(Channel* chn, uint32 type, const string& text, Player& fromPlayer)
{

}

void RandomPlayerbotMgr::OnPlayerLogout(Player* player)
{
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        PlayerbotAI* ai = bot->GetPlayerbotAI();
        if (player == ai->GetMaster())
        {
            ai->SetMaster(NULL);
            ai->ResetStrategies();
        }
    }

    if (!player->GetPlayerbotAI())
    {
        vector<Player*>::iterator i = find(players.begin(), players.end(), player);
        if (i != players.end())
            players.erase(i);
    }
}

void RandomPlayerbotMgr::OnPlayerLogin(Player* player)
{
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        if (player == bot || player->GetPlayerbotAI())
            continue;

        Group* group = bot->GetGroup();
        if (!group)
            continue;

        for (GroupReference *gref = group->GetFirstMember(); gref; gref = gref->next())
        {
            Player* member = gref->getSource();
            PlayerbotAI* ai = bot->GetPlayerbotAI();
            if (member == player && (!ai->GetMaster() || ai->GetMaster()->GetPlayerbotAI()))
            {
                ai->SetMaster(player);
                ai->ResetStrategies();
				ai->TellMaster("Hello, welcome back, " + string(player->GetName()) + "!");
                break;
            }
        }
    }

    if (!player->GetPlayerbotAI())
        players.push_back(player);
}

Player* RandomPlayerbotMgr::GetRandomPlayer()
{
    if (players.empty())
        return NULL;

    uint32 index = urand(0, players.size() - 1);
    return players[index];
}

void RandomPlayerbotMgr::PrintStats()
{
    sLog.outString("%d Random Bots online", playerBots.size());

    map<uint32, int> alliance, horde;
    for (uint32 i = 0; i < 10; ++i)
    {
        alliance[i] = 0;
        horde[i] = 0;
    }

    map<uint8, int> perRace, perClass;
    for (uint8 race = RACE_HUMAN; race < MAX_RACES; ++race)
    {
        perRace[race] = 0;
    }
    for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
    {
        perClass[cls] = 0;
    }

    int dps = 0, heal = 0, tank = 0;
    for (PlayerBotMap::iterator i = playerBots.begin(); i != playerBots.end(); ++i)
    {
        Player* bot = i->second;
        if (IsAlliance(bot->getRace()))
            alliance[bot->getLevel() / 10]++;
        else
            horde[bot->getLevel() / 10]++;

        perRace[bot->getRace()]++;
        perClass[bot->getClass()]++;

        int spec = AiFactory::GetPlayerSpecTab(bot);
        switch (bot->getClass())
        {
        case CLASS_DRUID:
            if (spec == 2)
                heal++;
            else
                dps++;
            break;
        case CLASS_PALADIN:
            if (spec == 1)
                tank++;
            else if (spec == 0)
                heal++;
            else
                dps++;
            break;
        case CLASS_PRIEST:
            if (spec != 2)
                heal++;
            else
                dps++;
            break;
        case CLASS_SHAMAN:
            if (spec == 2)
                heal++;
            else
                dps++;
            break;
        case CLASS_WARRIOR:
            if (spec == 2)
                tank++;
            else
                dps++;
            break;
        default:
            dps++;
            break;
        }
    }

    sLog.outString("Per level:");
    uint32 maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);
    for (uint32 i = 0; i < 10; ++i)
    {
        if (!alliance[i] && !horde[i])
            continue;

        uint32 from = i*10;
        uint32 to = min(from + 9, maxLevel);
        if (!from) from = 1;
        sLog.outString("    %d..%d: %d alliance, %d horde", from, to, alliance[i], horde[i]);
    }
    sLog.outString("Per race:");
    for (uint8 race = RACE_HUMAN; race < MAX_RACES; ++race)
    {
        if (perRace[race])
            sLog.outString("    %s: %d", ChatHelper::formatRace(race).c_str(), perRace[race]);
    }
    sLog.outString("Per class:");
    for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
    {
        if (perClass[cls])
            sLog.outString("    %s: %d", ChatHelper::formatClass(cls).c_str(), perClass[cls]);
    }
    sLog.outString("Per role:");
    sLog.outString("    tank: %d", tank);
    sLog.outString("    heal: %d", heal);
    sLog.outString("    dps: %d", dps);
}

double RandomPlayerbotMgr::GetBuyMultiplier(Player* bot)
{
    uint32 id = bot->GetObjectGuid();
    uint32 value = sPlayerbotAIConfig.GetEventValue(id, "buymultiplier");
    if (!value)
    {
        value = urand(1, 120);
        uint32 validIn = urand(sPlayerbotAIConfig.minRandomBotsPriceChangeInterval, sPlayerbotAIConfig.maxRandomBotsPriceChangeInterval);
		sPlayerbotAIConfig.SetEventValue(id, "buymultiplier", value, validIn);
    }

    return (double)value / 100.0;
}

double RandomPlayerbotMgr::GetSellMultiplier(Player* bot)
{
    uint32 id = bot->GetObjectGuid();
    uint32 value = sPlayerbotAIConfig.GetEventValue(id, "sellmultiplier");
    if (!value)
    {
        value = urand(80, 250);
        uint32 validIn = urand(sPlayerbotAIConfig.minRandomBotsPriceChangeInterval, sPlayerbotAIConfig.maxRandomBotsPriceChangeInterval);
		sPlayerbotAIConfig.SetEventValue(id, "sellmultiplier", value, validIn);
    }

    return (double)value / 100.0;
}

uint32 RandomPlayerbotMgr::GetLootAmount(Player* bot)
{
    uint32 id = bot->GetObjectGuid();
    return sPlayerbotAIConfig.GetEventValue(id, "lootamount");
}

void RandomPlayerbotMgr::SetLootAmount(Player* bot, uint32 value)
{
    uint32 id = bot->GetObjectGuid();
	sPlayerbotAIConfig.SetEventValue(id, "lootamount", value, 24 * 3600);
}

uint32 RandomPlayerbotMgr::GetTradeDiscount(Player* bot)
{
    Group* group = bot->GetGroup();
    return GetLootAmount(bot) / (group ? group->GetMembersCount() : 10);
}
