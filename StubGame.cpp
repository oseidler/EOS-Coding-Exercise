// Copyright Epic Games, Inc. All Rights Reserved.

#include <Windows.h>
#include <iostream>
#include <Windows/eos_Windows.h>
#include <eos_init.h>
#include <eos_logging.h>
#include <eos_common.h>
#include <eos_auth.h>
#include <eos_achievements.h>
#include <eos_stats.h>
#include <eos_connect.h>
#include <eos_sdk.h>
#include <cassert>
#include <string.h>
#include "SDKConfig.h"
#include "PlatformHandler.h"
#include "AuthHandler.h"


//global bools to track test completion - program ends once all are true
bool manualAchievementUnlocked = false;
bool achievementNotificationReceived = false;
bool statIngested = false;
bool successfulQuery = false;


//callback function for notifying about unlocked achievement
void EOS_CALL AchievementNotifyCallback(EOS_Achievements_OnAchievementsUnlockedCallbackV2Info const* data)
{
	assert(data != NULL);

	std::cout << std::endl << "Received a notification that an achievement was unlocked. Verify that the following info is correct:" << std::endl;
	std::cout << "User ID: " << data->UserId << std::endl;
	std::cout << "Achievement ID: " << data->AchievementId << std::endl;
	std::cout << "Unlock Time: " << data->UnlockTime << std::endl << std::endl;

	achievementNotificationReceived = true;

	return;
}


//callback for querying about achievement progress
void EOS_CALL AchievementQueryCallback(EOS_Achievements_OnQueryPlayerAchievementsCompleteCallbackInfo const* data)
{
	assert(data != NULL);

	if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		std::cout << std::endl << "Successfully queried achievement progress!" << std::endl;

		successfulQuery = true;
	}
	else
	{
		std::cout << std::endl << "Failed to query achievement progress" << std::endl << std::endl;
	}

	return;
}


//callback function for achievement unlocking
void EOS_CALL AchievementUnlockCallback(EOS_Achievements_OnUnlockAchievementsCompleteCallbackInfo const* data)
{
	assert(data != NULL);

	if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		std::cout << std::endl << "Achievement unlocked! Verify that the following data is correct:" << std::endl;
		std::cout << "User ID: " << data->UserId << std::endl;
		std::cout << "Achievement Count: " << data->AchievementsCount << std::endl;
		char const* retrievedAchievementName = (char const*)data->ClientData;
		std::cout << "Achievement ID: " << retrievedAchievementName << std::endl << std::endl;
		
		manualAchievementUnlocked = true;
	}
	else
	{
		std::cout << std::endl << "Achievement failed to unlock." << std::endl << std::endl;
	}

	return;
}


//callback function for stat ingestion
void EOS_CALL StatIngestCallback(EOS_Stats_IngestStatCompleteCallbackInfo const* data)
{
	assert(data != NULL);

	if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		std::cout << std::endl << "Stat ingested successfully! Verify that the following data is correct:" << std::endl;
		std::cout << "User ID: " << data->TargetUserId << std::endl;
		char const* retrievedStatName = (char const*)data->ClientData;
		std::cout << "Stat Name: " << retrievedStatName << std::endl << std::endl;

		statIngested = true;
	}
	else
	{
		std::cout << std::endl << "Stat ingestion failed." << std::endl << std::endl;
	}

	return;
}


int main()
{
	// Initialize config and platform
	SDKConfig* config = new SDKConfig;
	PlatformHandler* platformInitializer = new PlatformHandler();
	EOS_HPlatform platformHandle = nullptr;
	platformHandle = platformInitializer->InitializePlatform(config);
	assert(platformHandle != nullptr);

	// Login and retrieve a PUID
	AuthHandler* auth = new AuthHandler();
	EOS_ProductUserId puid = nullptr;
	puid = auth->Login(config, platformHandle);
	assert(puid != nullptr);

	std::cout << "Logged in and PUID is: " << puid << std::endl;


	//get the achievements interface
	EOS_HAchievements achievementInterface = EOS_Platform_GetAchievementsInterface(platformHandle);
	assert(achievementInterface != nullptr);
	std::cout << std::endl << "Got the achievements interface!" << std::endl;

	//get the stats interface
	EOS_HStats statInterface = EOS_Platform_GetStatsInterface(platformHandle);
	assert(statInterface != nullptr);
	std::cout << std::endl << "Got the stats interface!" << std::endl << std::endl;

	//set up notifications for achievements
	EOS_Achievements_AddNotifyAchievementsUnlockedV2Options notifyOptions = {};
	notifyOptions.ApiVersion = EOS_ACHIEVEMENTS_ADDNOTIFYACHIEVEMENTSUNLOCKEDV2_API_LATEST;
	EOS_NotificationId notifyID = EOS_Achievements_AddNotifyAchievementsUnlockedV2(achievementInterface, &notifyOptions, nullptr, AchievementNotifyCallback);
	assert(notifyID != 0);

	//main loop - keep ticking until all tasks have been completed
	bool isFinished = false;
	bool queryReported = false;
	bool achievementUnlocksFinished = false;
	int achievementUnlockTimer = 0;
	while (!isFinished)
	{
		EOS_Platform_Tick(platformHandle);

		achievementUnlockTimer++;

		if (achievementUnlockTimer == 500000)	//this is definitely hacky, but I needed to delay unlocking achievements and ingesting stats until the achievement notification was set up
		{			
			//ingest stat to unlock stat achievement
			EOS_Stats_IngestData stats = {};
			stats.ApiVersion = EOS_STATS_INGESTDATA_API_LATEST;
			stats.StatName = "Stat1";
			stats.IngestAmount = 1;
			EOS_Stats_IngestStatOptions statOptions = {};
			statOptions.ApiVersion = EOS_STATS_INGESTSTAT_API_LATEST;
			statOptions.LocalUserId = puid;
			statOptions.Stats = &stats;
			statOptions.StatsCount = 1;
			statOptions.TargetUserId = puid;
			EOS_Stats_IngestStat(statInterface, &statOptions, (void*)stats.StatName, StatIngestCallback);

			//ingest stat to partially unlock other stat achievement
			EOS_Stats_IngestData statsPartial = {};
			statsPartial.ApiVersion = EOS_STATS_INGESTDATA_API_LATEST;
			statsPartial.StatName = "Stat2";
			statsPartial.IngestAmount = 1;
			EOS_Stats_IngestStatOptions statPartialOptions = {};
			statPartialOptions.ApiVersion = EOS_STATS_INGESTSTAT_API_LATEST;
			statPartialOptions.LocalUserId = puid;
			statPartialOptions.Stats = &statsPartial;
			statPartialOptions.StatsCount = 1;
			statPartialOptions.TargetUserId = puid;
			EOS_Stats_IngestStat(statInterface, &statPartialOptions, (void*)statsPartial.StatName, StatIngestCallback);

			//unlock manual achievement
			char const* achieveID = "Manual";
			EOS_Achievements_UnlockAchievementsOptions unlockOptions = {};
			unlockOptions.ApiVersion = EOS_ACHIEVEMENTS_UNLOCKACHIEVEMENTS_API_LATEST;
			unlockOptions.UserId = puid;
			unlockOptions.AchievementIds = &achieveID;
			unlockOptions.AchievementsCount = 1;
			EOS_Achievements_UnlockAchievements(achievementInterface, &unlockOptions, (void*)achieveID, AchievementUnlockCallback);
		}

		//once all achievements have been supposedly unlocked, query achievement progress
		if (statIngested && manualAchievementUnlocked && !achievementUnlocksFinished)
		{
			//query current achievement progress
			EOS_Achievements_QueryPlayerAchievementsOptions queryOptions = {};
			queryOptions.ApiVersion = EOS_ACHIEVEMENTS_QUERYPLAYERACHIEVEMENTS_API_LATEST;
			queryOptions.LocalUserId = puid;
			queryOptions.TargetUserId = puid;
			EOS_Achievements_QueryPlayerAchievements(achievementInterface, &queryOptions, nullptr, AchievementQueryCallback);

			achievementUnlocksFinished = true;
		}

		//once the query has returned, check the data
		if (successfulQuery && !queryReported)
		{
			EOS_Achievements_CopyPlayerAchievementByAchievementIdOptions copyOptionsManual = {};
			copyOptionsManual.ApiVersion = EOS_ACHIEVEMENTS_QUERYPLAYERACHIEVEMENTS_API_LATEST;
			copyOptionsManual.LocalUserId = puid;
			copyOptionsManual.TargetUserId = puid;
			copyOptionsManual.AchievementId = "Manual";
			EOS_Achievements_PlayerAchievement* manualUnlockAchievement = nullptr;
			EOS_EResult copyResultManual = EOS_Achievements_CopyPlayerAchievementByAchievementId(achievementInterface, &copyOptionsManual, &manualUnlockAchievement);
			if (copyResultManual == EOS_EResult::EOS_Success)
			{
				std::cout << std::endl << "Got results for manual unlock achievement!" << std::endl;
				std::cout << "Achievement ID: " << manualUnlockAchievement->AchievementId << std::endl;
				std::cout << "Achievement Progress: " << manualUnlockAchievement->Progress << std::endl;
				std::cout << "Unlock Time: " << manualUnlockAchievement->UnlockTime << std::endl;
			}
			else
			{
				std::cout << std::endl << "Failed to copy results for manual unlock achievement" << std::endl << std::endl;
			}

			EOS_Achievements_CopyPlayerAchievementByAchievementIdOptions copyOptionsStat = {};
			copyOptionsStat.ApiVersion = EOS_ACHIEVEMENTS_QUERYPLAYERACHIEVEMENTS_API_LATEST;
			copyOptionsStat.LocalUserId = puid;
			copyOptionsStat.TargetUserId = puid;
			copyOptionsStat.AchievementId = "Stat";
			EOS_Achievements_PlayerAchievement* statUnlockAchievement = nullptr;
			EOS_EResult copyResultStat = EOS_Achievements_CopyPlayerAchievementByAchievementId(achievementInterface, &copyOptionsStat, &statUnlockAchievement);
			if (copyResultStat == EOS_EResult::EOS_Success)
			{
				std::cout << std::endl << "Got results for stat unlock achievement!" << std::endl;
				std::cout << "Achievement ID: " << statUnlockAchievement->AchievementId << std::endl;
				std::cout << "Achievement Progress: " << statUnlockAchievement->Progress << std::endl;
				std::cout << "Unlock Time: " << statUnlockAchievement->UnlockTime << std::endl;
			}
			else
			{
				std::cout << std::endl << "Failed to copy results for stat unlock achievement" << std::endl << std::endl;
			}

			EOS_Achievements_CopyPlayerAchievementByAchievementIdOptions copyOptionsStatPartial = {};
			copyOptionsStatPartial.ApiVersion = EOS_ACHIEVEMENTS_QUERYPLAYERACHIEVEMENTS_API_LATEST;
			copyOptionsStatPartial.LocalUserId = puid;
			copyOptionsStatPartial.TargetUserId = puid;
			copyOptionsStatPartial.AchievementId = "StatPartial";
			EOS_Achievements_PlayerAchievement* statPartialUnlockAchievement = nullptr;
			EOS_EResult copyResultStatPartial = EOS_Achievements_CopyPlayerAchievementByAchievementId(achievementInterface, &copyOptionsStatPartial, &statPartialUnlockAchievement);
			if (copyResultStatPartial == EOS_EResult::EOS_Success)
			{
				std::cout << std::endl << "Got results for stat partial unlock achievement!" << std::endl;
				std::cout << "Achievement ID: " << statPartialUnlockAchievement->AchievementId << std::endl;
				std::cout << "Achievement Progress: " << statPartialUnlockAchievement->Progress << std::endl;
				std::cout << "Unlock Time: " << statPartialUnlockAchievement->UnlockTime << std::endl;
			}
			else
			{
				std::cout << std::endl << "Failed to copy results for stat partial unlock achievement" << std::endl << std::endl;
			}
			
			EOS_Achievements_CopyPlayerAchievementByAchievementIdOptions copyOptionsLocked = {};
			copyOptionsLocked.ApiVersion = EOS_ACHIEVEMENTS_QUERYPLAYERACHIEVEMENTS_API_LATEST;
			copyOptionsLocked.LocalUserId = puid;
			copyOptionsLocked.TargetUserId = puid;
			copyOptionsLocked.AchievementId = "PermanentlyLocked";
			EOS_Achievements_PlayerAchievement* permanentLockAchievement = nullptr;
			EOS_EResult copyResultLocked = EOS_Achievements_CopyPlayerAchievementByAchievementId(achievementInterface, &copyOptionsLocked, &permanentLockAchievement);
			if (copyResultLocked == EOS_EResult::EOS_Success)
			{
				std::cout << std::endl << "Got results for permanently locked achievement!" << std::endl;
				std::cout << "Achievement ID: " << permanentLockAchievement->AchievementId << std::endl;
				std::cout << "Achievement Progress: " << permanentLockAchievement->Progress << std::endl;
				std::cout << "Unlock Time: " << permanentLockAchievement->UnlockTime << std::endl;
			}
			else
			{
				std::cout << std::endl << "Failed to copy results for permanently locked achievement" << std::endl << std::endl;
			}

			EOS_Achievements_PlayerAchievement_Release(manualUnlockAchievement);		//release memory once we're done with it, to be a good programmer
			EOS_Achievements_PlayerAchievement_Release(permanentLockAchievement);
			EOS_Achievements_PlayerAchievement_Release(statUnlockAchievement);
			EOS_Achievements_PlayerAchievement_Release(statPartialUnlockAchievement);

			queryReported = true;
		}

		//once we've seen the results of the query, end the loop
		if (queryReported)
		{
			isFinished = true;
		}
	}

	EOS_Achievements_RemoveNotifyAchievementsUnlocked(achievementInterface, notifyID);	//unsubscribe from achievement notifications as part of shutdown cleanup
	std::cout << "Closing program." << std::endl;
}
