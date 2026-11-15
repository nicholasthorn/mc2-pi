#ifndef CONTROLGUI_H
#define CONTROLGUI_H
/*************************************************************************************************\
ControlGui.h			: Interface for the ControlGui component.  This thing holds the tac map
							and everything else on the left hand side of the screen.
//---------------------------------------------------------------------------//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//
\*************************************************************************************************/

#ifndef GAMETACMAP_H
#include"gametacmap.h"
#endif

#ifndef FORCEGROUPBAR_H
#include"forcegroupbar.h"
#endif

#ifndef MOVER_H
#include"mover.h"
#endif

#ifndef UTILITIES_H
#include"utilities.h"
#endif

#ifndef PAUSEWINDOW_H
#include"pausewindow.h"
#endif

#ifndef AFONT_H
#include"afont.h"
#endif

#ifndef AEDIT_H
#include"aedit.h"
#endif

#ifndef MC2movie_H
#include"mc2movie.h"
#endif

#ifndef MPSTATS_H
#include"mpstats.h"
#endif
class CObjective;
class InfoWindow;
class PauseWindow;
//*************************************************************************************************


#define LAST_VEHICLE MAX_VEHICLE - LARGE_AIRSTRIKE
#define OBJECTVE_MOVE_COUNT 2
#define RESULTS_MOVE_COUNT 3
#define MAX_CHAT_COUNT 5
#define RPTOTAL_CALLOUT	21185
/**************************************************************************************************
CLASS DESCRIPTION
ControlGui:
**************************************************************************************************/


struct	ButtonData
{
	long			ID;
	long			helpTextHeader;
	long			helpTextID;
	long			textID;
	long			textColors[4];
	aFont			textFont;

	char			fileName[32];
	long			stateCoords[4][2];	
	long			textureWidth;
	long			textureHeight;
	int				fileWidth;
	int				fileHeight;
	unsigned long	textureHandle;
	bool			textureRotated;
	
};

class ControlButton
{
	public:
	gos_VERTEX		location[4];
	long ID;

	ButtonData*			data;
	int					state;

	void render();
	void press( bool );
	void toggle();
	void disable( bool );
	bool isEnabled();
	void makeAmbiguous( bool bAmbiguous );
	void hide( bool );
	void move( float deltaX, float deltaY );
	void setColor( unsigned long newColor );
	static void makeUVs( gos_VERTEX* vertices, int State, ButtonData& data );

	// fileIndex: the block number to read for each button (NULL: i), for
	// wolfman-MC2X's support buttons, which are in another order
	static void initButtons( FitIniFile& file, long buttonCount, 
			ControlButton* buttons, ButtonData* buttonData, const char* str, aFont* font = 0,
			const int* fileIndex = 0 );


	enum States
	{
		ENABLED = 0,
		PRESSED,
		DISABLED,
		AMBIGUOUS,
		HIDDEN
	};

};


class ControlGui
{
	public:

		static ControlGui*	instance;

		static long			hiResOffsetX;
		static long			hiResOffsetY;

		ControlGui();
		~ControlGui();
		// owns heap buttons and statics, freed by the destructor: not copyable
		ControlGui( const ControlGui& ) = delete;
		ControlGui& operator=( const ControlGui& ) = delete;
		bool inRegion( int mouseX, int mouseY, bool bPaused );
		void render( bool bPaused );
		void update( bool bPaused, bool bLOS );
		void initTacMapBuildings( unsigned char* data, int size ){ tacMap.initBuildings( data, size ); }
		void initTacMap( unsigned char* data, int size ){ tacMap.init( data, size ); }
		void initMechs();
		void unPressAllVehicleButtons();
		void disableAllVehicleButtons();
		void addMover (MoverPtr mover);
		void removeMover (MoverPtr mover);
		int  updateChat();

		void beginPause();
		void endPause();

		bool resultsDone();

		void startObjectives( bool bStart );
		bool objectivesStarted() { return renderObjectives; }

		void setInfoWndMover( Mover* pMover );
		void setVehicleCommand( bool );
		bool getVehicleCommand( );

		void playMovie( const char* fileName );
		bool isMoviePlaying();

		bool playPilotVideo( MechWarrior* pPilot, char movieCode );
		void endPilotVideo();
		bool isSelectingInfoObject();

		bool isOverTacMap();

		bool isChatting() { return bChatting; }
		
		//TUTORIAL
		bool animateTacMap (int buttonId,float timeToScroll,long numFlashes);
		bool pushButton (long buttonId);
		bool flashRPTotal (long numFlashes);

		const char* getVehicleName( long& ID );
		const char* getVehicleNameFromID (long ID);
		void swapResolutions( int newResolutionX, int resolutionY );

		GameTacMap						tacMap;
		ForceGroupBar					forceGroupBar;

		bool				mouseInVehicleStopButton;

		enum Commands
		{
			DEFAULT_RANGE = 0,
			SHORT_RANGE ,
			MED_RANGE,
			LONG_RANGE,
			JUMP_COMMAND,
			RUN_COMMAND,
			GUARD_COMMAND,
			FIRE_FROM_CURRENT_POS,
			STOP_COMMAND,
			INFO_COMMAND,
			OBJECTIVES_COMMAND,
			SAVE_COMMAND,
			TACMAP_TAB,
			INFO_TAB,
			VEHICLE_TAB,
			LAYMINES,
			REPAIR,
			SALVAGE,
			GUARDTOWER,
			CUR_RANGE, // fire from right where I AM
			LAST_COMMAND

		};

		enum VehicleCommands
		{
			LARGE_AIRSTRIKE = 100,
			GUARD_TOWER,
			SENSOR_PROBE,
			REPAIR_VEHICLE,
			PEGASUS_SCOUT,
			MINELAYER,
			RECOVERY_TEAM,	
			STOP_VEHICLE,
			MAX_VEHICLE
		};

		bool	isDefaultSpeed();
		void	toggleDefaultSpeed( );
		void	toggleJump( );
		bool	getJump( );
		bool	getWalk( );
		bool	getRun( );
		void	toggleGuard( );
		bool	getGuard( );
		void	setDefaultSpeed();
		void	toggleHoldPosition();

		void	setRange( int Range );
		void	doStop();
		void	toggleFireFromCurrentPos();
		bool	getFireFromCurrentPos(){ return fireFromCurrentPos; }
		void    setFireFromCurrentPos( bool bset ) { fireFromCurrentPos = bset; }
		bool	isAddingVehicle(){ return addingVehicle; }
		bool	isAddingAirstrike() { return addingArtillery; }
		bool	isAddingSalvage() { return addingSalvage; }
		bool	isButtonPressed( int ID ) { return getButton( ID )->state & ControlButton::PRESSED; }
		bool	getMines();
		bool	getSalvage();
		bool	getRepair();
		bool	getGuardTower();
		void	switchTabs( int direction );
		void	renderObjective( CObjective* pObjective, long xPos, long yPos, bool bDrawTotal );
		void	renderMissionStatus( bool bRender){ renderStatusInfo = bRender; }		
		
		int		getCurrentRange();
		void	pressInfoButton( ){ handleClick( INFO_COMMAND ); }
		bool	infoButtonPressed() { return getButton( INFO_COMMAND )->state & ControlButton::PRESSED; }
		// wolfman-MC2X's move formation (formation.h): kNone (0) unless its
		// HUD's palette chose one
		int		getFormation() const { return formationKind; }
		// how many units the support delivery under way brings (MC2X's
		// kamikaze swarm: 5; every other: 1)
		int		getVehicleCount();

		void	showServerMissing();

		void	pressAirstrikeButton() 
		{ 
			for ( int i = 0; i < LAST_VEHICLE; i++ )
			{
				if (( vehicleButtons[i].ID == LARGE_AIRSTRIKE) &&
					!( vehicleButtons[i].state & ControlButton::PRESSED ))
				{
					handleVehicleClick( LARGE_AIRSTRIKE ); 
				}
			}
		}		
		void	pressLargeAirstrikeButton() { handleVehicleClick( LARGE_AIRSTRIKE ); }
		void	pressVehicleButton( int ID ) { handleVehicleClick( ID ); }
		// resource points for a support vehicle's type number or an artillery type; -1 if none
		static long supportCost( long vehicleTypeID );
		static long strikeCost( long strikeType );
		void	pressSensorStrikeButton() 
		{ 
			for ( int i = 0; i < LAST_VEHICLE; i++ )
			{
				if (( vehicleButtons[i].ID == SENSOR_PROBE) &&
					!( vehicleButtons[i].state & ControlButton::PRESSED ))
				{
					handleVehicleClick( SENSOR_PROBE ); 
				}
			}
		}		

		void	setRolloverHelpText( unsigned long textID );


		void	setChatText( const char* playerName, const char* message, unsigned long backgroundColor, 
			unsigned long textColor );
		// a line saying which force group was chosen: replaces the last line
		// when that was one too, so stepping through groups adds one line
		void	setForceGroupText( const char* message );
		void	toggleChat( bool setTeamOnly );
		void eatChatKey();
		void	cancelInfo();
		
		ControlButton*		getButton( int ID );

		struct RectInfo
		{
			GUI_RECT rect;
			long color;
		};

	private:

		struct ChatInfo
		{
			char playerName[32];
			char message[512];			// mission text messages run to 300 characters (Omnitech)
			unsigned long backgroundColor;
			unsigned long time;
			unsigned long messageLength; // number of lines
			unsigned long chatTextColor;
		};

		ChatInfo		chatInfos[MAX_CHAT_COUNT]; // max five lines -- could change

		RectInfo*		rectInfos;
		long			rectCount;
		
		//static	ButtonFile		vehicleFileData[LAST_VEHICLE];
		static unsigned long RUN;
		static unsigned long WALK;
		static unsigned long GUARD;
		static unsigned long JUMP;

		ControlButton*		buttons;
		ControlButton*		vehicleButtons;
		static ButtonData*	buttonData;
		static ButtonData*	vehicleData;
		static const char*	vehicleNames[5];
		static long			vehicleIDs[5];
		static const char*	vehiclePilots[5];


		static long			vehicleCosts[LAST_VEHICLE];

		// wolfman-MC2X's formation buttons, its layout's 24 to 43 (the palette,
		// the Formation button, the indicators); NULL with other layouts
		ControlButton*		formationButtons;
		ButtonData*			formationData;
		bool				formationMenuOpen;
		int					formationKind;
		void				initFormationButtons( FitIniFile& buttonFile, bool mc2xHud );
		void				updateFormationButtons( bool bShow, float mouseX, float mouseY );
		// wolfman-MC2X's own support units (mc2xsupport.h): its layout's
		// [Vehicle7] to [Vehicle20] over two support tabs, and the second tab
		// (Button49); NULL with other layouts
		ControlButton*		mc2xSupportButtons;
		ButtonData*			mc2xSupportData;
		ControlButton*		support2Tab;
		ButtonData*			support2TabData;
		int					supportPage;		// 1, or 2: the second tab
		int					mc2xPendingBlock;	// the unit being brought in, or -1
		void				initMC2XSupport( FitIniFile& buttonFile, bool mc2xHud );
		void				updateMC2XSupport( float mouseX, float mouseY );
		void				renderMC2XSupport();
		bool				stockVehicleShown( int i );

		// wolfman-MC2X's exchange and spy map buttons (mc2xhud.h): its
		// layout's Button22, Button23 and Button53; NULL with other layouts
		ControlButton*		mc2xCommandButtons;
		ButtonData*			mc2xCommandData;
		float				spyMapTime;			// seconds of the spy map left
		void				initMC2XCommands( FitIniFile& buttonFile, bool mc2xHud );
		void				updateMC2XCommands( bool bPaused, float mouseX, float mouseY );

		InfoWindow*			infoWnd;
		PauseWindow*		pauseWnd;

		StaticInfo*			staticInfos;
		long				staticCount;

		StaticInfo*			objectiveInfos; //2nd to last one is check, last is x
		long				objectiveInfoCount;

		StaticInfo*			missionStatusInfos;
		long				missionStatusInfoCount;
		RectInfo			missionStatusRect;

		bool				renderStatusInfo;
		float				resultsTime;
		bool				renderObjectives;
		float				objectiveTime;
		
		float				tabFlashTime;
		static long OBJECTIVESTOP;
		static long OBJECTIVESLEFT;
		static long OBJECTIVESSKIP;
		static long OBJECTIVESTOTALRIGHT;
		static long OBJEECTIVESHEADERSKIP;
		static long OBJECTIVESHEADERTOP;
		static long OBJECTIVEBOXX;
		static long OBJECTIVEBOXSKIP;
		static long OBJECTIVECHECKSKIP;
		static long OBJECTIVEHEADERLEFT;
		static long HELPAREA_LEFT;
		static long HELPAREA_BOTTOM;
		static long RPLEFT;
		static long RPTOP;

		static MoveInfo objectiveMoveInfo[OBJECTVE_MOVE_COUNT];
		static MoveInfo missionResultsMoveInfo[RESULTS_MOVE_COUNT];

		StaticInfo*			videoInfos;
		long				videoInfoCount;
		GUI_RECT			videoRect;
		GUI_RECT			videoTextRect;
		MC2MoviePtr			bMovie;

		StaticInfo*			timerInfos;
		long				timerInfoCount;
		RectInfo			timerRect;

		//TUTORIAL!!
		RectInfo			rpCallout;
		long				rpNumFlashes;
		float				rpFlashTime;
		long				buttonToPress;

		unsigned long		curOrder;
		bool				fireFromCurrentPos;
		bool				addingVehicle;
		bool				addingArtillery;
		bool				addingSalvage;
		bool				wasLayingMines;

		bool				moviePlaying;
		bool				twoMinWarningPlayed;
		bool				thirtySecondWarningPlayed;
		bool				bChatting;
		bool				lastChatIsForceGroup;

		long					idToUnPress;
		aFont					guiFont;
		aFont					helpFont;
		aFont					vehicleFont;
		aFont					timerFont;
		aFont					missionResultsFont;
		
		aText					chatEdit;
		aEdit					playerNameEdit;
		aEdit					personalEdit;

		void handleClick( int ID );
		void updateVehicleTab( int mouseX, int mouseY, bool bLOS );
		void renderVehicleTab();
		void RenderObjectives();
		void renderResults();
		void handleVehicleClick( int ID );
		void renderHelpText();
		void renderInfoTab();
		void renderChatText();

		void initStatics( FitIniFile& file );
		void initRects( FitIniFile& file );

		void renderPlayerStatus( float delta);

		MPStatsEntry			mpStats[9];

		bool					chatIsTeamOnly;
		bool					bServerWarningShown;

		public:

		RectInfo *getRect (long id)
		{
			if ((id >= 0) && (id < rectCount))
			{
				return &(rectInfos[id]);
			}

			if (id == RPTOTAL_CALLOUT)
			{
				return &rpCallout;
			}

			return NULL;
		}

};


//*************************************************************************************************
#endif  // end of file ( ControlGui.h )
