/*
*	Logging component for COALITION games
*	Component overrides base game mode so it always runs
*
*	Note that write files seem weird because they are parsed by an external program
*	which splits strings via colons
*
*	Server only
*/
[ComponentEditorProps(category: "CRF Logging Component", description: "")]
class CRF_LoggingServerComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_LoggingServerComponent: SCR_BaseGameModeComponent
{	
	string m_sLogPath = "$profile:COAServerLog.txt";
	string m_sMissionName;
	string m_sKillerName;
	string m_sKillerFaction;
	string m_sKilledName;
	string m_sKilledFaction;
	string m_sTime;
	string m_sWeaponName;
	
	int m_iPlayerCount;
	int m_iBluforCount;
	int m_iOpforCount;
	int m_iIndforCount;
	int m_iTotalSeconds;
	
	float m_fRange;
	float m_fTotalTime;
	
	SCR_FactionManager m_FM;
	BaseWeaponManagerComponent m_WMC;
	FileHandle m_handle;
	
	static CRF_LoggingServerComponent GetInstance() 
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_LoggingServerComponent.Cast(gameMode.FindComponent(CRF_LoggingServerComponent));
		else
			return null;
	}
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Only run if in a real game and always in workbench
		/*#ifdef WORKBENCH
			Print("CRF::Workbench");
		#else 
			if (GetGame().GetPlayerManager().GetPlayerCount() < 10)
				return;
		#endif*/
	}

	// Setup
	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		if (!Replication.IsServer())
			return;
		
		m_handle = FileIO.OpenFile(m_sLogPath, FileMode.APPEND);

		PrintFormat("CRF Handle: %1",m_handle);
		m_sMissionName = GetGame().GetMissionName();
		m_handle.WriteLine("mission,beginning," + m_sMissionName);	
	}
	
	// Player Connected
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		if (!Replication.IsServer())
			return;
		
		// Get player name
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		
		// Log to file
		m_handle.WriteLine("connect," + playerName);
	}
	
	// Player Disconnected 
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		if (!Replication.IsServer())
			return;
		
		// Get player name
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		
		m_handle.WriteLine("disconnect," + playerName + "," + cause);
	}
	
	// Mission status messages 
	override void OnGameStateChanged(SCR_EGameModeState state)
	{
		super.OnGameStateChanged(state);
		if (!Replication.IsServer())
			return;
		
		m_iPlayerCount = GetGame().GetPlayerManager().GetPlayerCount();
		
		switch (state)
		{
			case SCR_EGameModeState.SLOTSELECTION:
			{
				m_handle.WriteLine("mission,slotting," + m_sMissionName + "," + m_iPlayerCount);
				break;
			}
			case SCR_EGameModeState.BRIEFING:
			{
				m_handle.WriteLine("mission,briefing," + m_sMissionName + "," + m_iPlayerCount);
				break;
			}
			case SCR_EGameModeState.GAME:
			{
				m_handle.WriteLine("mission,safestart," + m_sMissionName + "," + m_iPlayerCount);
				break;
			}
			case SCR_EGameModeState.DEBRIEFING:
			{
				m_handle.WriteLine("mission,ended," + m_sMissionName + "," + m_iPlayerCount);
				break;
			}
		}
	}
	
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		super.OnGameModeEnd(data);
		if (!Replication.IsServer())
			return;
		
		m_handle.Close(); // lets avoid a mem leak
	}
	
	// Method called from safestart to annotate a game has begun
	// TODO: Mission stats get logged here
	void GameStarted()
	{
		if (!Replication.IsServer())
			return;
		// Collect mission data 
		m_iPlayerCount = GetGame().GetPlayerManager().GetPlayerCount();
		//m_iBluforCount = m_FM.SGetFactionPlayerCount(Faction
		
		// log
		m_handle.WriteLine("mission,started," + m_sMissionName + "," + m_iPlayerCount);
	}
	
	// Killfeed log
    override void OnPlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
    {
		super.OnPlayerKilled(playerId, playerEntity, killerEntity, killer);
		
		m_FM = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		
		// Killer
		// Check if killer is AI
		if (GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(killerEntity) == 0)
		{
			m_sKillerName = "AI";
			m_sKillerFaction = "AI";
		} else {
			m_sKillerName = GetGame().GetPlayerManager().GetPlayerName(killer.GetInstigatorPlayerID());
			m_sKillerFaction = m_FM.GetPlayerFaction(killer.GetInstigatorPlayerID()).GetFactionName();
		}
		
		// Killed
		m_sKilledName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		m_sKilledFaction = m_FM.GetPlayerFaction(playerId).GetFactionName();
		
		// Range
		m_fRange = vector.Distance(playerEntity.GetOrigin(),killerEntity.GetOrigin());
		
		// Killer Weapon 
		m_WMC = BaseWeaponManagerComponent.Cast(killerEntity.FindComponent(BaseWeaponManagerComponent));
		m_sWeaponName = m_WMC.GetCurrentWeapon().GetUIInfo().GetName();	
		if (m_sWeaponName == "")
			m_sWeaponName = "Unknown Weapon";
			
		// Time
		m_fTotalTime = GetGame().GetWorld().GetWorldTime();
  		m_iTotalSeconds = (m_fTotalTime / 1000);
		m_sTime = SCR_FormatHelper.FormatTime(m_iTotalSeconds);
		
		Print("kill," + m_sKilledName + "," + m_sKilledFaction + "," + m_sKillerName + "," + m_sKillerFaction + "," + m_fRange + "," + m_sWeaponName + "," + m_sTime);
		m_handle.WriteLine("kill," + m_sKilledName + "," + m_sKilledFaction + "," + m_sKillerName + "," + m_sKillerFaction + "," + m_fRange + "," + m_sWeaponName + "," + m_sTime);
	}
}