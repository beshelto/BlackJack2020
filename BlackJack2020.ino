/**************************************************************************
 *     This file is part of BlackJack2020.

    I, Dick Hamill, the author of this program disclaim all copyright
    in order to make this program freely available in perpetuity to
    anyone who would like to use it. Dick Hamill, 6/1/2020

    BlackJack2020 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BlackJack2020 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
 */

// intermediate credits 
// new sounds

#include "RPU_Config.h"
#include "RPU.h"
#include "BlackJack2020.h"
#include "SelfTestAndAudit.h"
#include <EEPROM.h>

#define DEBUG_MESSAGES  0

// MachineState
//  0 - Attract Mode
//  negative - self-test modes
//  positive - game play
int MachineState = 0;
boolean MachineStateChanged = true;
#define MACHINE_STATE_ATTRACT         0
#define MACHINE_STATE_INIT_GAMEPLAY   1
#define MACHINE_STATE_INIT_NEW_BALL   2
#define MACHINE_STATE_UNVALIDATED     3
#define MACHINE_STATE_NORMAL_GAMEPLAY 4
#define MACHINE_STATE_COUNTDOWN_BONUS 90
#define MACHINE_STATE_MATCH_MODE      95
#define MACHINE_STATE_BALL_OVER       100
#define MACHINE_STATE_GAME_OVER       110

/* TODO -- adjust this to be based on MACHINE_STATE_TEST_DONE */
#define MACHINE_STATE_ADJUST_FREEPLAY           -20
#define MACHINE_STATE_ADJUST_BALLSAVE           -21
#define MACHINE_STATE_ADJUST_TILT_WARNING       -22
#define MACHINE_STATE_ADJUST_TOURNAMENT_SCORING -23
#define MACHINE_STATE_ADJUST_MUSIC_LEVEL        -24
#define MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD   -25
#define MACHINE_STATE_ADJUST_SPECIAL_AWARD      -26
#define MACHINE_STATE_ADJUST_AWARD_OVERRIDE     -27
#define MACHINE_STATE_ADJUST_BALLS_PER_GAME     -28
#define MACHINE_STATE_ADJUST_RANDOMIZE_DECK     -29
#define MACHINE_STATE_ADJUST_HIT_OVER_16        -30
#define MACHINE_STATE_ADJUST_SHOW_DEALER_HITS   -31
#define MACHINE_STATE_ADJUST_PLAYER_LOSES_TIES  -32
#define MACHINE_STATE_ADJUST_ONE_SPECIAL_PER_BALL -33
#define MACHINE_STATE_ADJUST_NO_RESET_DURING_PLAY  -34
#define MACHINE_STATE_ADJUST_DONE               -35


#define CLUB_BUMPER_INDEX     0
#define SPADE_BUMPER_INDEX    1
#define RED_BUMPER_INDEX      2

#define TILT_WARNING_DEBOUNCE_TIME    1000

#define MAX_DISPLAY_BONUS     69

#define SOUND_EFFECT_ADD_CREDIT           1
#define SOUND_EFFECT_BONUS_ADD            2
#define SOUND_EFFECT_BONUS_SUBTRACT       3
#define SOUND_EFFECT_SKILL_SHOT           4
#define SOUND_EFFECT_TILT_WARNING         5
#define SOUND_EFFECT_OUTLANE_UNLIT        6
#define SOUND_EFFECT_OUTLANE_LIT          7
#define SOUND_EFFECT_INLANE               8
#define SOUND_EFFECT_CHANGE_PLAYER        9
#define SOUND_EFFECT_CHANGE_DEALER        10
#define SOUND_EFFECT_BONUS_ROLLOVER       11
#define SOUND_EFFECT_TOPLANE_LIGHT_PLUS   12
#define SOUND_EFFECT_TOPLANE_LIGHT        13
#define SOUND_EFFECT_TOPLANE_FLASH        14
#define SOUND_EFFECT_BUMPER_10            15
#define SOUND_EFFECT_BUMPER_100           16
#define SOUND_EFFECT_BUMPER_1000          17
#define SOUND_EFFECT_PLAYER_WINS          18
#define SOUND_EFFECT_DEAL_CARD_UP         19
#define SOUND_EFFECT_DEAL_CARD_DOWN       20
#define SOUND_EFFECT_RANDOM_SAUCER        21
#define SOUND_EFFECT_SPINNER_BONUS        22
#define SOUND_EFFECT_ADD_PLAYER           30
#define SOUND_EFFECT_BALL_OVER            35
#define SOUND_EFFECT_GAME_OVER            36
#define SOUND_EFFECT_MACHINE_START        37
#define SOUND_EFFECT_EXTRA_BALL           38
#define SOUND_EFFECT_SLING                39
#define SOUND_EFFECT_COIN_DROP            40
#define SOUND_EFFECT_MATCH_SPIN           50
#define SOUND_EFFECT_BONUS_COLLECT_HURRY_UP 51
#define SOUND_EFFECT_BONUS_COUNTDOWN_BASE 100
#define SOUND_EFFECT_BONUS_COUNTDOWN_1K   101
#define SOUND_EFFECT_BONUS_COUNTDOWN_2K   102
#define SOUND_EFFECT_BONUS_COUNTDOWN_3K   103
#define SOUND_EFFECT_BONUS_COUNTDOWN_5K   105


#define EEPROM_BALL_SAVE_BYTE       100
#define EEPROM_FREE_PLAY_BYTE       101
#define EEPROM_MUSIC_LEVEL_BYTE     102
#define EEPROM_SKILL_SHOT_BYTE      103
#define EEPROM_TILT_WARNING_BYTE    104
#define EEPROM_AWARD_OVERRIDE_BYTE  105
#define EEPROM_BALLS_OVERRIDE_BYTE  106
#define EEPROM_TOURNAMENT_SCORING_BYTE  107
#define EEPROM_EXTRA_BALL_SCORE_BYTE    108
#define EEPROM_SPECIAL_SCORE_BYTE       112
#define EEPROM_CLEAR_SUITS_BYTE         116
#define EEPROM_RANDOMIZE_DECK_BYTE      117
#define EEPROM_HIT_OVER_16_BYTE         118
#define EEPROM_NUM_HITS_TO_SHOW_DEALER_BYTE   119
#define EEPROM_PLAYER_LOSES_TIES_BYTE         120
#define EEPROM_ONE_SPECIAL_PER_BALL_BYTE      121
#define EEPROM_NO_RESET_DURING_PLAY         122




// Game/machine global variables
unsigned long HighScore = 0;
unsigned long AwardScores[3];
byte AwardScoresOverride = 99;
int Credits = 0;
int MaximumCredits = 20;
boolean FreePlayMode = false;
byte CreditsPerCoin1, CreditsPerCoin2, CreditsPerCoin3;

byte ChuteCoinsInProgress[3];

// Game mechanics
byte CurrentPlayer = 0;
byte CurrentBallInPlay = 1;
byte CurrentNumPlayers = 0;
unsigned long CurrentTime = 0;
unsigned long BallTimeInTrough = 0;
unsigned long LastTimeSaucerEmpty = 0;
unsigned long BallFirstSwitchHitTime = 0;
unsigned long TimeToWaitForBall = 100;
unsigned long ShowingSurrenderSpins = 0;
unsigned long ShowBetUntilThisTime = 0;
boolean UsingTreeToShowBet = false;
boolean BallSaveUsed = false;
byte BallSaveNumSeconds = 0;
byte BallsPerGame = 3;
boolean MatchFeature = true;
boolean NoResetDuringPlay = true;

byte NumTiltWarnings = 0;
unsigned long LastTiltWarningTime = 0;

// Game play parameters (stored)
byte MaxTiltWarnings = 2;
byte MusicLevel = 2;
byte RandomizeDeck = 0;
boolean NoHitsOver16 = true;
//boolean ResetSuitsPerBall = false;
boolean HighScoreReplay = true;
boolean PlayerLosesOnTies = false;
byte NoveltyScoring = 0;
boolean CreditDisplay = true;
byte NumberOfDealerHitsToShow = 3;
unsigned long ExtraBallValue = 0;
unsigned long SpecialValue = 0;
boolean OneSpecialPerBall = true;
byte NumSecondsForBonusCollect = 45;

// Game state
byte SuitsComplete[4];
byte PlayersTopCard;
byte DealersTopCard;
byte PlayersTally;
byte DealersTally;
boolean PlayerHasAce = false;
boolean DealerHasAce = false;
byte Deck[26] = {0x4D, 0xD7, 0xBC, 0x6C, 0x6A, 0x53, 0xC7, 0x15, 0x72, 0x43, 0xB1, 0x29, 0x18, 0xAA, 0x26, 0xB5, 0x3D, 0x64, 0x7A, 0xD4, 0x38, 0x92, 0x9C, 0x88, 0x59, 0x1B};
byte PlayersDeckPtr[4];
byte ShowDealersCardCountdown = 3;
boolean PlayerHits = false;
boolean PlayerCheats = false;
byte SurrenderSpins = 0;
byte RegularSpins = 0;
boolean Spinner1kLit = false;
unsigned long CurrentScores[4];
byte Bonus;
byte BonusX[4];
boolean SamePlayerShootsAgain = false;
boolean LeftOutlaneLit = false;
boolean RightOutlaneLit = false;
boolean SpecialCollectedThisBall = false;

unsigned long SpinnerMadnessEndTime = 0;

#define BETTING_STAGE_BUILDING_STAKES   0
#define BETTING_STAGE_BET_SWEEP         1
#define BETTING_STAGE_DEAL              2
#define BETTING_STAGE_SHOW_DEAL         3
#define BETTING_STAGE_NATURAL_21        4
#define BETTING_STAGE_PLAYER_BUST       5
#define BETTING_STAGE_WAIT_FOR_COLLECT  6
#define BETTING_STAGE_END_OF_ROUND      7
#define BETTING_STAGE_WAIT_FOR_BONUS_COLLECT  8
#define BETTING_STAGE_BONUS_COLLECT           9

byte BettingStage = BETTING_STAGE_BUILDING_STAKES;

byte dipBank0, dipBank1, dipBank2, dipBank3;

void GetDIPSwitches() {
  dipBank0 = RPU_GetDipSwitches(0);
  dipBank1 = RPU_GetDipSwitches(1);
  dipBank2 = RPU_GetDipSwitches(2);
  dipBank3 = RPU_GetDipSwitches(3);
}

void DecodeDIPSwitchParameters() {

  CreditsPerCoin1 = ((dipBank0)&0x1F);
  HighScoreReplay = ((dipBank0&0x60)>>5)?true:false;
  MusicLevel = (dipBank0&0x80)?2:0;
  
  CreditsPerCoin3 = ((dipBank1)&0x1F);
  NoveltyScoring = ((dipBank1&60)>>5);
  BallsPerGame = (dipBank1&80)?5:3;
  
  MaximumCredits = (dipBank2&0x07)*5 + 5;
  CreditDisplay = (dipBank2&0x08)?true:false;
  MatchFeature = (dipBank2&0x10)?true:false;

  CreditsPerCoin2 = ((dipBank3)&0x0F);
//  ResetSuitsPerBall = ((dipBank3)&0x60)?false:true;
  PlayerLosesOnTies = ((dipBank3)&0x80)?true:false;
  
}

void GetStoredParameters() {

  for (byte count=0; count<3; count++) {
    ChuteCoinsInProgress[count] = 0;
  }

  byte ballsOverride = ReadSetting(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  if (ballsOverride==3 || ballsOverride==5) {
    BallsPerGame = ballsOverride;
  } else {
    if (ballsOverride!=99) EEPROM.write(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  }
  
  ReadSetting(EEPROM_FREE_PLAY_BYTE, 0);
  FreePlayMode = (EEPROM.read(EEPROM_FREE_PLAY_BYTE)) ? true : false;
    
  BallSaveNumSeconds = ReadSetting(EEPROM_BALL_SAVE_BYTE, 16);
  MaxTiltWarnings = ReadSetting(EEPROM_TILT_WARNING_BYTE, 2);
  MusicLevel = ReadSetting(EEPROM_MUSIC_LEVEL_BYTE, 2);
  RandomizeDeck = ReadSetting(EEPROM_RANDOMIZE_DECK_BYTE, 2);
  NoHitsOver16 = ReadSetting(EEPROM_HIT_OVER_16_BYTE, true);
  NumberOfDealerHitsToShow = ReadSetting(EEPROM_NUM_HITS_TO_SHOW_DEALER_BYTE, 2);
  ExtraBallValue = RPU_ReadULFromEEProm(EEPROM_EXTRA_BALL_SCORE_BYTE);
  SpecialValue = RPU_ReadULFromEEProm(EEPROM_SPECIAL_SCORE_BYTE);
  OneSpecialPerBall = ReadSetting(EEPROM_ONE_SPECIAL_PER_BALL_BYTE, false);

  NoResetDuringPlay = ReadSetting(EEPROM_NO_RESET_DURING_PLAY, true)?true:false;

  byte noveltyOverride = ReadSetting(EEPROM_TOURNAMENT_SCORING_BYTE, 99);
  if (noveltyOverride!=99) NoveltyScoring = noveltyOverride;

  byte playerTiesOverride = ReadSetting(EEPROM_PLAYER_LOSES_TIES_BYTE, false);
  if (playerTiesOverride!=99) PlayerLosesOnTies = playerTiesOverride;

  AwardScores[0] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_1_EEPROM_START_BYTE);
  AwardScores[1] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_2_EEPROM_START_BYTE);
  AwardScores[2] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_3_EEPROM_START_BYTE);
  AwardScoresOverride = ReadSetting(EEPROM_AWARD_OVERRIDE_BYTE, 99);

  HighScore = RPU_ReadULFromEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, 10000);
  Credits = RPU_ReadByteFromEEProm(RPU_CREDITS_EEPROM_BYTE);
  if (Credits>MaximumCredits) Credits = MaximumCredits;

}


byte ReadSetting(byte setting, byte defaultValue) {
    byte value = EEPROM.read(setting);
    if (value == 0xFF) {
        EEPROM.write(setting, defaultValue);
        return defaultValue;
    }
    return value;
}




void PlaySoundEffect(byte soundEffectNum, unsigned long timeOffset=0) {
  if (MusicLevel==0) return;

  int count;

  switch (soundEffectNum) {
    case SOUND_EFFECT_ADD_PLAYER:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime, true);
      if (MusicLevel>1) {
        RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+200, true);
        RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+400, true);
      }    
    break;
    case SOUND_EFFECT_COIN_DROP:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
    break;
    case SOUND_EFFECT_BALL_OVER:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+166, true);
      if (MusicLevel>1) {
        RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+250, true);
        RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+500, true);
        RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+750, true);
        RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+1000, true);
        RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+1166, true);
        RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+1250, true);
      }
    break;
    case SOUND_EFFECT_GAME_OVER:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+166, true);
      if (MusicLevel>1) {
        RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+250, true);
        RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+500, true);
        RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+666, true);
        RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+750, true);
      }
    break;
/*    
    case SOUND_EFFECT_MACHINE_START:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 1, CurrentTime+500, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+600, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+900, true);
    break;
*/    
    case SOUND_EFFECT_MACHINE_START:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+200, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+300, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+600, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+800, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+900, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+1200, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+1400, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+1500, true);
    break;
    case SOUND_EFFECT_ADD_CREDIT:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime + timeOffset, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+75 + timeOffset, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+150 + timeOffset, true);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+225 + timeOffset, true);
    break;
    case SOUND_EFFECT_PLAYER_WINS:
      for (count=0; count<(MusicLevel*2+1); count++) {
        RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+count*100);
      }
    break;
    case SOUND_EFFECT_BONUS_COUNTDOWN_1K:
    case SOUND_EFFECT_DEAL_CARD_DOWN:
    case SOUND_EFFECT_MATCH_SPIN:
    case SOUND_EFFECT_BUMPER_10:
    case SOUND_EFFECT_BONUS_SUBTRACT:
    case SOUND_EFFECT_SLING:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
    break;
    case SOUND_EFFECT_BONUS_COUNTDOWN_2K:
    case SOUND_EFFECT_BONUS_COUNTDOWN_3K:
    case SOUND_EFFECT_DEAL_CARD_UP:
    case SOUND_EFFECT_BUMPER_100:
    case SOUND_EFFECT_OUTLANE_UNLIT:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
    break;
    case SOUND_EFFECT_BONUS_COLLECT_HURRY_UP:
    case SOUND_EFFECT_SPINNER_BONUS:
    case SOUND_EFFECT_BUMPER_1000:
    case SOUND_EFFECT_BONUS_ADD:
    case SOUND_EFFECT_BONUS_COUNTDOWN_5K:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime);
    break;
    case SOUND_EFFECT_OUTLANE_LIT:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+100);
      if (MusicLevel>1) {
        RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+200);
        RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+300);
      }
    break;
    case SOUND_EFFECT_CHANGE_PLAYER:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      if (MusicLevel>1) {
        RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+200);
        RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+300);
      }
    break;
    case SOUND_EFFECT_BONUS_ROLLOVER:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime);
      if (MusicLevel>1) {
        RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+250);
        RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+250);
      }
    break;
    case SOUND_EFFECT_CHANGE_DEALER:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime);
      if (MusicLevel>1) {
        RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+200);
        RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+300);
      }
    break;
    case SOUND_EFFECT_INLANE:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+100);
    break;
    case SOUND_EFFECT_EXTRA_BALL:
    case SOUND_EFFECT_SKILL_SHOT:
      for (count=0; count<2; count++) {
        RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime + count*150);
        if (MusicLevel>1) {
          RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+25 + count*150);
          RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+50 + count*150);
        }
      }
    break;
    case SOUND_EFFECT_RANDOM_SAUCER:
    case SOUND_EFFECT_TOPLANE_LIGHT:
      for (count=0; count<(MusicLevel); count++) {
        RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+count*200);
      }
    break;
    case SOUND_EFFECT_TOPLANE_LIGHT_PLUS:
      for (count=0; count<(MusicLevel+1); count++) {
        RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime+count*200);
      }
    break;
    case SOUND_EFFECT_TOPLANE_FLASH:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_1000, 3, CurrentTime);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+175);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+350);
    break;
    case SOUND_EFFECT_TILT_WARNING:
      RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime);
      RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+100);
      if (MusicLevel>1) {
        RPU_PushToTimedSolenoidStack(SOL_CHIME_100, 3, CurrentTime+200);
        RPU_PushToTimedSolenoidStack(SOL_CHIME_10, 3, CurrentTime+300);
      }
    break;
  }    
  
}


void setup() {  

  // If using an Alltek & experiencing boot problems, 
  // try uncommenting the following delay:
  //delay(2000);
  
  if (DEBUG_MESSAGES) {
    Serial.begin(57600);
    Serial.write("Setup begin.\n");
  }

  // Tell the OS about game-specific lights and switches
  RPU_SetupGameSwitches(NUM_SWITCHES_WITH_TRIGGERS, NUM_PRIORITY_SWITCHES_WITH_TRIGGERS, TriggeredSwitches);

  // Set up the chips and interrupts
  RPU_InitializeMPU(RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_PERFORM_MPU_TEST,
                    SW_CREDIT_RESET);
  RPU_DisableSolenoidStack();
  RPU_SetDisableFlippers(true);

  // Use dip switches to set up game variables
  GetDIPSwitches();
  DecodeDIPSwitchParameters();

  // Read parameters from EEProm
  GetStoredParameters();
  
  CurrentTime = millis();
  PlaySoundEffect(SOUND_EFFECT_MACHINE_START);  
  if (DEBUG_MESSAGES) {
    Serial.write("Kicking out saucer\n");
  }
  RPU_PushToSolenoidStack(SOL_SAUCER, 5, true);

}

void PulseTopLights(byte topLightPulse, byte suitsComplete) {
  byte shiftNum = 1;
  for (byte count=0; count<4; count++) {
    SetTopLampState(CLUBS_TOP_LANE-count, (topLightPulse/4)==count, topLightPulse%4, suitsComplete&shiftNum);
    shiftNum *= 2;
  }
}

void AddToBonus(byte bonusAddition) {
  Bonus += bonusAddition;
  if (Bonus>MAX_DISPLAY_BONUS) Bonus = MAX_DISPLAY_BONUS;
}


void SetPlayerLamps(byte numPlayers, int flashPeriod=0) {
  for (byte count=PLAYER_1_UP; count<(PLAYER_4_UP+1); count++) RPU_SetLampState(count, (numPlayers==(1+count-PLAYER_1_UP))?1:0, 0, flashPeriod);
}


void ShowSuitsComplete(byte suitsComplete) {
  byte shiftNum = 8;
  int baseFlash=0, fastFlash=0;
  boolean solidOn = false;
  if ((suitsComplete/16)>1) {
    if (suitsComplete/16) baseFlash = 3200 / (suitsComplete/16);
    fastFlash = baseFlash/2; 
  } else {
    if ((suitsComplete/16)) {
      fastFlash = 1600;
      solidOn = true;
    }
  }

  for (byte count=0; count<4; count++) {
    RPU_SetLampState(HEARTS_TOP_LANE+count, (solidOn || baseFlash || (suitsComplete&shiftNum))?1:0, 0, (((suitsComplete)&shiftNum)?fastFlash:baseFlash));
    shiftNum /= 2;
  }

  RPU_SetLampState(CLUB_BUMPER, (solidOn || baseFlash || (suitsComplete&0x01))?1:0, 0, (suitsComplete&0x01)?fastFlash:baseFlash);
  RPU_SetLampState(RED_BUMPER, (solidOn || baseFlash || ((suitsComplete&0x0A)==0x0A))?1:0, 0, ((suitsComplete&0x0A)==0x0A)?fastFlash:baseFlash);
  RPU_SetLampState(SPADE_BUMPER, (solidOn || baseFlash || (suitsComplete&0x04))?1:0, 0, (suitsComplete&0x04)?fastFlash:baseFlash);  

  LeftOutlaneLit = ((suitsComplete&0x03)==0x03);
  RightOutlaneLit = ((suitsComplete&0x0C)==0x0C);
  RPU_SetLampState(LEFT_OUTLANE, LeftOutlaneLit);
  RPU_SetLampState(RIGHT_OUTLANE, RightOutlaneLit);

  if (SpinnerMadnessEndTime==0) {
    Spinner1kLit = ((suitsComplete&0x0F)==0x00 && suitsComplete>0x00);
    RPU_SetLampState(SPINNER_1000, Spinner1kLit);
  } else {
    if (CurrentTime<SpinnerMadnessEndTime) {
      RPU_SetLampState(SPINNER_1000, 1, 0, 125);
    } else {
      SpinnerMadnessEndTime = 0;
    }
  }
}


void ShowBetOnTree(byte betAmount) {

  if (betAmount>=20) {
    RPU_SetLampState(BONUS_TREE_20, 1, 1);
    betAmount -= 20;
  }
  else RPU_SetLampState(BONUS_TREE_20, 0);

  if (betAmount>=10) {
    RPU_SetLampState(BONUS_TREE_10, 1, 1);
    betAmount -= 10;
  }
  else RPU_SetLampState(BONUS_TREE_10, 0);
  
  for (byte count=0; count<9; count++) {
    if ((count+1)<=betAmount) RPU_SetLampState(BONUS_TREE_1+count, 1, 1);
    else RPU_SetLampState(BONUS_TREE_1+count, 0);
  }


}

void ShowPlayerWinsPulse(boolean turnOff=false) {

  if (turnOff) {
    RPU_SetLampState(PLAYER_WINS, 0);
  } else {
    int period;
/*    if (PlayersTally<17) period = 1500;
    if (PlayersTally==17) period = 1000;
    if (PlayersTally==18) period = 500;
    if (PlayersTally==19) period = 200;
    if (PlayersTally==20) period = 100;
    if (PlayersTally==21) period = 50;
*/    
    period = 1500;
    if (PlayersTally>16 && PlayersTally<22) period /= (PlayersTally-16);
    if (DealersTopCard==10 || DealersTopCard==1) period += 500;
    
    byte phase = (CurrentTime/period)%4;
    RPU_SetLampState(PLAYER_WINS, (phase!=0), phase%2);
  }
}


void ShowBonusOnTree(byte bonus, byte dim) {
  if (bonus>MAX_DISPLAY_BONUS) bonus = MAX_DISPLAY_BONUS;

  if (bonus>=60) {
    RPU_SetLampState(BONUS_TREE_10, 1, dim, 250);
    bonus -= 20;
  } else if ( ((bonus/10)%2)==1 ) {
    RPU_SetLampState(BONUS_TREE_10, 1, dim);
    bonus -= 10;
  } else {
    RPU_SetLampState(BONUS_TREE_10, 0, dim, 250);
  }

  if (bonus>=40) {
    RPU_SetLampState(BONUS_TREE_20, 1, dim, 250);
    bonus -= 40;
  } else if (bonus>=20) {
    RPU_SetLampState(BONUS_TREE_20, 1, dim);
    bonus -= 20;   
  } else {
    RPU_SetLampState(BONUS_TREE_20, 0, dim);
  }
 
  for (byte count=0; count<9; count++) {
    if (count==(bonus-1)) RPU_SetLampState(BONUS_TREE_1+count, 1, dim);
    else RPU_SetLampState(BONUS_TREE_1+count, 0, dim);
  }

}


unsigned long TimeToRevertDisplays = 0;
byte DisplayOverridden = 0;
unsigned long ScrollStartTime = 0;
void ShowPlayerDisplays(boolean pauseScrolls=false, boolean showHighScores=false, byte numDisplaysToShow=0) {
  int count;

  if (pauseScrolls) ScrollStartTime = CurrentTime + 5000;

  if (TimeToRevertDisplays!=0 && CurrentTime>TimeToRevertDisplays) {
    DisplayOverridden = 0;
  }

  if (numDisplaysToShow==0) numDisplaysToShow = CurrentNumPlayers;

  // Loop on player displays
  for (count=0; count<4; count++) {

    // If this display is currently overriden, don't change it here
    if ((DisplayOverridden>>count) & 0x01) continue;

    unsigned long scoreToShow = CurrentScores[count];
    if (showHighScores) scoreToShow = HighScore;
    
    if (!showHighScores && count==CurrentPlayer) {
      // For current player, flash the score if nothing has been hit
      if (BallFirstSwitchHitTime==0) {
        RPU_SetDisplay(count, scoreToShow);
        RPU_SetDisplayFlash(count, scoreToShow, CurrentTime, 500, 2);
      } else {
        RPU_SetDisplay(count, scoreToShow, true);
      }
    } else {
      // For non-current player, only show display if it's in use
      if (count<numDisplaysToShow) {
        RPU_SetDisplay(count, scoreToShow, true);
      } else {
        RPU_SetDisplayBlank(count, 0x00);        
      }
    }
  }
}

void OverridePlayerDisplays(byte displayNum, unsigned long overrideUntilTime = 0 /* zero = forever */) {
  DisplayOverridden |= 1<<displayNum;
  TimeToRevertDisplays = overrideUntilTime;
}

void ClearOverridePlayerDisplays() {
  DisplayOverridden = 0;
}


void AddCredit(byte numCredits = 1, boolean silent = false) {
  unsigned long timeOffset = 0;
  for (byte count=0; count<numCredits; count++) {
    if (Credits<MaximumCredits) {
      Credits += 1;
      RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
      if (!silent) PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT, timeOffset);    
    } else {
    }
    timeOffset += 500;
  }

  RPU_SetDisplayCredits(Credits, CreditDisplay && !FreePlayMode);
  RPU_SetCoinLockout((Credits<MaximumCredits && !FreePlayMode)?false:true);
}


void AddCoinToAudit(byte switchHit) {

  unsigned short coinAuditStartByte = 0;

  switch (switchHit) {
    case SW_COIN_3: coinAuditStartByte = RPU_CHUTE_3_COINS_START_BYTE; break;
    case SW_COIN_2: coinAuditStartByte = RPU_CHUTE_2_COINS_START_BYTE; break;
    case SW_COIN_1: coinAuditStartByte = RPU_CHUTE_1_COINS_START_BYTE; break;
  }

  if (coinAuditStartByte) {
    RPU_WriteULToEEProm(coinAuditStartByte, RPU_ReadULFromEEProm(coinAuditStartByte) + 1);
  }

}

boolean AddCoin(byte chuteNum) {
  boolean creditAdded = false;
  if (chuteNum>2) return false;
  byte cpcSelection = GetCPCSelection(chuteNum);

  // Find the lowest chute num with the same ratio selection
  // and use that ChuteCoinsInProgress counter
  byte chuteNumToUse;
  for (chuteNumToUse=0; chuteNumToUse<=chuteNum; chuteNumToUse++) {
    if (GetCPCSelection(chuteNumToUse) == cpcSelection)
      break;
  }

  byte cpcCoins = GetCPCCoins(cpcSelection);
  byte cpcCredits = GetCPCCredits(cpcSelection);
  byte coinProgressBefore = ChuteCoinsInProgress[chuteNumToUse];
  ChuteCoinsInProgress[chuteNumToUse] += 1;

  if (ChuteCoinsInProgress[chuteNumToUse]==cpcCoins) {
    if (cpcCredits>cpcCoins) AddCredit(cpcCredits - (coinProgressBefore));
    else AddCredit(cpcCredits);
    ChuteCoinsInProgress[chuteNumToUse] = 0;
    creditAdded = true;
  } else {
    if (cpcCredits>cpcCoins) {
      AddCredit(1);
      creditAdded = true;
    } else {
      PlaySoundEffect(SOUND_EFFECT_COIN_DROP);
    }
  }

  return creditAdded;
}


boolean AddPlayer(boolean resetNumPlayers=false) {

  if (Credits<1 && !FreePlayMode) return false;
  if (resetNumPlayers) CurrentNumPlayers = 0;
  if (CurrentNumPlayers>=4) return false;

  CurrentNumPlayers += 1;
//  RPU_SetDisplay(CurrentNumPlayers-1, 0);
//  RPU_SetDisplayBlank(CurrentNumPlayers-1, 0x30);
  RPU_SetDisplay(CurrentNumPlayers-1, 0, true);

  if (!FreePlayMode) {
    Credits -= 1;
    RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
  }

  RPU_SetDisplayCredits(Credits, CreditDisplay && !FreePlayMode);
  RPU_WriteULToEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE) + 1);
  PlaySoundEffect(SOUND_EFFECT_ADD_PLAYER);

  return true;
}


void ShuffleDeck() {
  for (byte deckPtr=0; deckPtr<52; deckPtr++) {
    byte cardByte1 = Deck[deckPtr/2];
    byte randomPtr = (micros() + random(0, 100))%52;
    byte cardByte2 = Deck[randomPtr/2];
    byte card1, card2;

    if (deckPtr%2) card1 = cardByte1/16;
    else card1 = cardByte1 & 0x0F;
    if (randomPtr%2) card2 = cardByte2/16;
    else card2 = cardByte2 & 0x0F;
    
    if (deckPtr%2) cardByte1 = (cardByte1 & 0x0F) | (card2 * 16);
    else cardByte1 = (cardByte1 & 0xF0) | card2;
    if (randomPtr%2) cardByte2 = (cardByte2 & 0x0F) | (card1 * 16);
    else cardByte2 = (cardByte2 & 0xF0) | card1;

    Deck[deckPtr/2] = cardByte1;
    Deck[randomPtr/2] = cardByte2;
  }
}


int InitGamePlay(boolean curStateChanged) {
  int returnState = MACHINE_STATE_INIT_GAMEPLAY;

  returnState = ClearSwitchBuffer(returnState, false);

  if (curStateChanged) {
    CurrentTime = millis();

    RPU_SetCoinLockout((Credits>=MaximumCredits || FreePlayMode)?true:false);
    RPU_SetDisableFlippers(true);
    RPU_DisableSolenoidStack();
    RPU_TurnOffAllLamps();
    RPU_SetLampState(CREDIT_LIGHT, Credits||FreePlayMode);
    RPU_SetDisplayBallInPlay(1);

    DisplayOverridden = 0;
    TimeToRevertDisplays = 0;
    
    // Set up general game variables
    CurrentPlayer = 0;
    CurrentNumPlayers = 1;
    CurrentBallInPlay = 1;
    SamePlayerShootsAgain = false;
    if (RandomizeDeck>1) ShuffleDeck();
    for (int count=0; count<4; count++) {
      CurrentScores[count] = 0;
      SuitsComplete[count] = 0;
      PlayersDeckPtr[count] = 0;
      RPU_SetDisplay(count, 0);
      RPU_SetDisplayBlank(count, 0);
      BonusX[count] = 1;
    }
    RPU_SetDisplay(0, 0, true, 2);

    // if the ball is in the outhole, then we can move on
    if (RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
      RPU_EnableSolenoidStack();
      RPU_SetDisableFlippers(false);
      returnState = MACHINE_STATE_INIT_NEW_BALL;
    } else {

      // Otherwise, let's see if it's in a spot where it could get trapped,
      // for instance, a saucer (if the game has one)
      if (RPU_ReadSingleSwitchState(SW_SAUCER)) {
        RPU_PushToSolenoidStack(SOL_SAUCER, 5, true);
        // if it was in the saucer - kick it out and wait five seconds
        TimeToWaitForBall = CurrentTime + 8000;
      } else {
        // if it wasn't in the saucer, just wait 100ms
        TimeToWaitForBall = CurrentTime + 100;
      }

    }
  }

  // Wait for TimeToWaitForBall seconds, or until the ball appears
  // The reason to bail out after TIME_TO_WAIT_FOR_BALL is just
  // in case the ball is already in the shooter lane.
  if (CurrentTime>TimeToWaitForBall || RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
    RPU_EnableSolenoidStack();
    RPU_SetDisableFlippers(false);
    returnState = MACHINE_STATE_INIT_NEW_BALL;
  }
  
  return returnState;  
}




int InitNewBall(bool curStateChanged, byte playerNum, int ballNum) {  

  if (curStateChanged) {
    BallFirstSwitchHitTime = 0;

    // if we came here as an extra ball, set the same player lights
    if (SamePlayerShootsAgain) {
      RPU_SetLampState(SAME_PLAYER, 1);
      RPU_SetLampState(HEAD_SAME_PLAYER, 1);
    }
    SamePlayerShootsAgain = false;
    BallSaveUsed = false;
    NumTiltWarnings = 0;
    LastTiltWarningTime = CurrentTime;
    SpecialCollectedThisBall = false;
    
    RPU_SetDisableFlippers(false);
    RPU_EnableSolenoidStack(); 
    RPU_SetDisplayCredits(Credits, CreditDisplay && !FreePlayMode);
    SetPlayerLamps(playerNum+1, 500);
    
    if (RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
      RPU_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
      // Maybe just give it half a second to eject the ball?
    }

    RPU_SetDisplayBallInPlay(ballNum);
    RPU_SetLampState(BALL_IN_PLAY, 1);
    RPU_SetLampState(TILT, 0);

    if (BallSaveNumSeconds>0) {
      RPU_SetLampState(SAME_PLAYER, 1, 0, 500);
    }
    
    BettingStage = BETTING_STAGE_BUILDING_STAKES;
    Bonus = 1;
    SpinnerMadnessEndTime = 0;

    PlayersTopCard = 0;
    PlayersTally = 0;
    DealersTopCard = 0;
    DealersTally = 0;
    PlayerHasAce = false;
    DealerHasAce = false;
    if (BonusX[CurrentPlayer]>5) BonusX[CurrentPlayer] = 5;
//    ShowSuitsComplete(SuitsComplete[playerNum]);
  }
  
  // We should only consider the ball initialized when 
  // the ball is no longer triggering the SW_OUTHOLE
  if (RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
    return MACHINE_STATE_INIT_NEW_BALL;
  } else {
    return MACHINE_STATE_NORMAL_GAMEPLAY;
  }
  
}


#define ADJ_TYPE_LIST                 1
#define ADJ_TYPE_MIN_MAX              2
#define ADJ_TYPE_MIN_MAX_DEFAULT      3
#define ADJ_TYPE_SCORE                4
#define ADJ_TYPE_SCORE_WITH_DEFAULT   5
byte AdjustmentType = 0;
byte NumAdjustmentValues = 0;
byte AdjustmentValues[8];
unsigned long AdjustmentScore;
byte *CurrentAdjustmentByte = NULL;
unsigned long *CurrentAdjustmentUL = NULL;
byte CurrentAdjustmentStorageByte = 0;

int RunSelfTest(int curState, boolean curStateChanged) {
  int returnState = curState;
  CurrentNumPlayers = 0;

  // Any state that's greater than CHUTE_3 is handled by the Base Self-test code
  // Any that's less, is machine specific, so we handle it here.
  if (curState>=MACHINE_STATE_TEST_CHUTE_3_COINS) {
    returnState = RunBaseSelfTest(returnState, curStateChanged, CurrentTime, SW_CREDIT_RESET, SW_SLAM);
    if (returnState==MACHINE_STATE_ATTRACT) {
      DecodeDIPSwitchParameters();
      GetStoredParameters();
      returnState = MACHINE_STATE_ATTRACT;     
    }
  } else {
    byte curSwitch = RPU_PullFirstFromSwitchStack();

    if (curSwitch==SW_SELF_TEST_SWITCH && (CurrentTime-GetLastSelfTestChangedTime())>250) {
      SetLastSelfTestChangedTime(CurrentTime);
      returnState -= 1;
    }

    if (curStateChanged) {
      RPU_ReadContinuousSolenoids();
      for (int count=0; count<4; count++) {
        RPU_SetDisplay(count, 0);
        RPU_SetDisplayBlank(count, 0x00);        
      }
      RPU_SetDisplayCredits(MACHINE_STATE_TEST_SOUNDS - curState);
      RPU_SetDisplayBallInPlay(0, false);
      CurrentAdjustmentByte = NULL;
      CurrentAdjustmentUL = NULL;
      CurrentAdjustmentStorageByte = 0;

      AdjustmentType = ADJ_TYPE_MIN_MAX;
      AdjustmentValues[0] = 0;
      AdjustmentValues[1] = 1;

      if (curState==MACHINE_STATE_ADJUST_FREEPLAY) {
        CurrentAdjustmentByte = (byte *)&FreePlayMode;
        CurrentAdjustmentStorageByte = EEPROM_FREE_PLAY_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_BALLSAVE) {
        AdjustmentType = ADJ_TYPE_LIST;
        NumAdjustmentValues = 5;
        AdjustmentValues[1] = 5;
        AdjustmentValues[2] = 10;
        AdjustmentValues[3] = 15;
        AdjustmentValues[4] = 20;
        CurrentAdjustmentByte = &BallSaveNumSeconds;
        CurrentAdjustmentStorageByte = EEPROM_BALL_SAVE_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_TILT_WARNING) {
        AdjustmentValues[1] = 2;
        CurrentAdjustmentByte = &MaxTiltWarnings;
        CurrentAdjustmentStorageByte = EEPROM_TILT_WARNING_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_TOURNAMENT_SCORING) {
        AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
        CurrentAdjustmentByte = &NoveltyScoring;
        CurrentAdjustmentStorageByte = EEPROM_TOURNAMENT_SCORING_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_MUSIC_LEVEL) {
        AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
        AdjustmentValues[1] = 2;
        CurrentAdjustmentByte = &MusicLevel;
        CurrentAdjustmentStorageByte = EEPROM_MUSIC_LEVEL_BYTE;      
      } else if (curState==MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD) {
        AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
        CurrentAdjustmentUL = &ExtraBallValue;
        CurrentAdjustmentStorageByte = EEPROM_EXTRA_BALL_SCORE_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_SPECIAL_AWARD) {
        AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
        CurrentAdjustmentUL = &SpecialValue;
        CurrentAdjustmentStorageByte = EEPROM_SPECIAL_SCORE_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_AWARD_OVERRIDE) {
        AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
        AdjustmentValues[1] = 7;
        CurrentAdjustmentByte = &AwardScoresOverride;
        CurrentAdjustmentStorageByte = EEPROM_AWARD_OVERRIDE_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_BALLS_PER_GAME) {
        AdjustmentType = ADJ_TYPE_LIST;
        NumAdjustmentValues = 3;
        AdjustmentValues[0] = 3;
        AdjustmentValues[1] = 5;
        AdjustmentValues[2] = 99;
        CurrentAdjustmentByte = &BallsPerGame;
        CurrentAdjustmentStorageByte = EEPROM_BALLS_OVERRIDE_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_RANDOMIZE_DECK) {
        AdjustmentValues[1] = 3;
        CurrentAdjustmentByte = &RandomizeDeck;
        CurrentAdjustmentStorageByte = EEPROM_RANDOMIZE_DECK_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_HIT_OVER_16) {
        CurrentAdjustmentByte = (byte *)&NoHitsOver16;
        CurrentAdjustmentStorageByte = EEPROM_HIT_OVER_16_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_SHOW_DEALER_HITS) {
        AdjustmentType = ADJ_TYPE_LIST;
        NumAdjustmentValues = 7;
        AdjustmentValues[2] = 2;
        AdjustmentValues[3] = 3;
        AdjustmentValues[4] = 4;
        AdjustmentValues[5] = 5;
        AdjustmentValues[6] = 99;
        CurrentAdjustmentByte = &NumberOfDealerHitsToShow;
        CurrentAdjustmentStorageByte = EEPROM_NUM_HITS_TO_SHOW_DEALER_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_PLAYER_LOSES_TIES) {
        AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
        CurrentAdjustmentByte = (byte *)&PlayerLosesOnTies;
        CurrentAdjustmentStorageByte = EEPROM_PLAYER_LOSES_TIES_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_ONE_SPECIAL_PER_BALL) {
        CurrentAdjustmentByte = (byte *)&OneSpecialPerBall;
        CurrentAdjustmentStorageByte = EEPROM_ONE_SPECIAL_PER_BALL_BYTE;
      } else if (curState==MACHINE_STATE_ADJUST_NO_RESET_DURING_PLAY) {
        CurrentAdjustmentByte = (byte *)&NoResetDuringPlay;
        CurrentAdjustmentStorageByte = EEPROM_NO_RESET_DURING_PLAY;
      } else if (curState==MACHINE_STATE_ADJUST_DONE) {
        DecodeDIPSwitchParameters();
        GetStoredParameters();
        returnState = MACHINE_STATE_ATTRACT;
      } else {
        returnState = curState;
      }
    }

    // Change value, if the switch is hit
    if (curSwitch==SW_CREDIT_RESET) {

      if (CurrentAdjustmentByte && (AdjustmentType==ADJ_TYPE_MIN_MAX || AdjustmentType==ADJ_TYPE_MIN_MAX_DEFAULT)) {
        byte curVal = *CurrentAdjustmentByte;
        curVal += 1;
        if (curVal>AdjustmentValues[1]) {
          if (AdjustmentType==ADJ_TYPE_MIN_MAX) curVal = AdjustmentValues[0];
          else {
            if (curVal>99) curVal = AdjustmentValues[0];
            else curVal = 99;
          }
        }
        *CurrentAdjustmentByte = curVal;
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, curVal);
      } else if (CurrentAdjustmentByte && AdjustmentType==ADJ_TYPE_LIST) {
        byte valCount=0;
        byte curVal = *CurrentAdjustmentByte;
        byte newIndex = 0;
        for (valCount=0; valCount<(NumAdjustmentValues-1); valCount++) {
          if (curVal==AdjustmentValues[valCount]) newIndex = valCount+1;
        }
        *CurrentAdjustmentByte = AdjustmentValues[newIndex];
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, AdjustmentValues[newIndex]);
      } else if (CurrentAdjustmentUL && AdjustmentType==ADJ_TYPE_SCORE_WITH_DEFAULT) {
        unsigned long curVal = *CurrentAdjustmentUL;
        curVal += 5000;
        if (curVal>100000) curVal = 0;
        *CurrentAdjustmentUL = curVal;
        if (CurrentAdjustmentStorageByte) RPU_WriteULToEEProm(CurrentAdjustmentStorageByte, curVal);
      }

      if (curState==MACHINE_STATE_ADJUST_FREEPLAY) {
        RPU_SetCoinLockout((Credits<MaximumCredits && !FreePlayMode)?false:true);
      }
    }

    if (curSwitch==SW_SLAM) {
      DecodeDIPSwitchParameters();
      GetStoredParameters();
      returnState = MACHINE_STATE_ATTRACT;     
    }

    // Show current value
    if (CurrentAdjustmentByte!=NULL) {
      if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_1 || curState==MACHINE_STATE_ADJUST_CPC_CHUTE_2 || curState==MACHINE_STATE_ADJUST_CPC_CHUTE_3) {
        byte curSelection = *CurrentAdjustmentByte;
        RPU_SetDisplay(0, GetCPCCoins(curSelection), true);
        RPU_SetDisplay(2, GetCPCCredits(curSelection), true);
      } else {
        RPU_SetDisplay(0, (unsigned long)(*CurrentAdjustmentByte), true);
        RPU_SetDisplayBlank(2, 0x00);
      }
    } else if (CurrentAdjustmentUL!=NULL) {
      RPU_SetDisplay(0, (*CurrentAdjustmentUL), true);
    }

  }

  return returnState;
}





void SetTopLampState(byte lampNum, boolean lampSelected, byte lampPhase, boolean lampBaseLevelOn) {
  byte lampIsOn = (lampSelected || lampBaseLevelOn) ? 1 : 0;
  byte lampIsDim = 1;
  if ( lampSelected && (lampPhase==1 || lampPhase==2) ) lampIsDim = 0;
  RPU_SetLampState(lampNum, lampIsOn, lampIsDim);
}

boolean PlayerUpLightBlinking = false;
byte CurrentSkillShotSuit = 0;
unsigned long maxSweepBet = 0;
unsigned long BettingModeStart = 0;
byte CurrentSweepingBet = 0;
unsigned long LastTimeInfoUpdated = 0;



int CountdownBonus(boolean curStateChanged) {
  int returnState = MACHINE_STATE_COUNTDOWN_BONUS;

  if (curStateChanged) {
    // Turn off displays & lights we don't want for bonus countdown
    ClearOverridePlayerDisplays();
    ShowPlayerWinsPulse(true);
    SetBonusXLights(BonusX[CurrentPlayer]);
    LastTimeInfoUpdated = 0;
  }

  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>125) {
    if (Bonus>1) {
      if (NumTiltWarnings<=MaxTiltWarnings) {
        unsigned long maxBonusX = BonusX[CurrentPlayer];
        if (maxBonusX>5) maxBonusX = 5;
        CurrentScores[CurrentPlayer] += 1000 * maxBonusX;
        PlaySoundEffect(SOUND_EFFECT_BONUS_COUNTDOWN_BASE + maxBonusX);
      }
      Bonus -= 1;
    } else {
      Bonus = 0;
      returnState = MACHINE_STATE_BALL_OVER;
    }
    ShowBonusOnTree(Bonus, 0);
    LastTimeInfoUpdated = CurrentTime;    
  }

  return returnState;
}


void CalculateAndShowBetSweep(byte minBet, byte maxBet) {
  unsigned long gap = (unsigned long)(maxBet-minBet);
  unsigned long timeRunning = ((CurrentTime-BettingModeStart)/1000)%10;
  if (gap!=0) {
    if (timeRunning<5) {
      // bet is climbing up
      gap = (timeRunning*gap)/4;
    } else {
      // bet is ramping down
      timeRunning = 9 - timeRunning;
      gap = (timeRunning*gap)/4;
    }    
  }
  CurrentSweepingBet = minBet + ((byte)gap);
  if (CurrentSweepingBet>20) CurrentSweepingBet = 20;

  // Update the sweep 2x a second
  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>500) {
    LastTimeInfoUpdated = CurrentTime;
    ShowBetOnTree(CurrentSweepingBet);
    UsingTreeToShowBet = true;
    ShowBetUntilThisTime = CurrentTime+2000;
  }
}


void SetTallyLamps(byte tally, boolean withAce, byte topLamp) {


  RPU_SetLampState(topLamp+4, ((tally==17)||(withAce&&tally==7)||(tally>0&&tally<17&&(!withAce || tally<7 || tally>11)))?1:0, 0, (tally<17)?250:0 + (withAce&&tally==7)?500:0);
  RPU_SetLampState(topLamp+3, ((tally==18)||(withAce&&tally==8))?1:0, 0, (withAce&&tally==8)?750:0);
  RPU_SetLampState(topLamp+2, ((tally==19)||(withAce&&tally==9))?1:0, 0, (withAce&&tally==9)?750:0);
  RPU_SetLampState(topLamp+1, ((tally==20)||(withAce&&tally==10))?1:0, 0, (withAce&&tally==10)?750:0);
  RPU_SetLampState(topLamp, ((tally>=21)||(withAce&&tally==11))?1:0, 0, ((tally>21)?250:0) + ((withAce&&tally==11)?500:0));

/*  
  if (tally==0) {
    RPU_SetLampState(topLamp+4, 0);
  } else if (tally>21) {
    RPU_SetLampState(topLamp+4, 0);
//    RPU_SetLampState(topLamp, 1, 0, 250);
  } else if (tally>=17) {
    RPU_SetLampState(topLamp+4, (tally==17));
//    RPU_SetLampState(topLamp+3, (tally==18));
//    RPU_SetLampState(topLamp+2, (tally==19));
//    RPU_SetLampState(topLamp+1, (tally==20));
//    RPU_SetLampState(topLamp, (tally==21));
  } else {
    if (!withAce || tally<7 || tally>11) {
      RPU_SetLampState(topLamp+4, 1, 0, 250);
//      RPU_SetLampState(topLamp, 0);
    } else {
      RPU_SetLampState(topLamp+4, (tally==7), 0, 750);
//      RPU_SetLampState(topLamp+3, (tally==8), 0, 750);
//      RPU_SetLampState(topLamp+2, (tally==9), 0, 750);
//      RPU_SetLampState(topLamp+1, (tally==10), 0, 750);
//      RPU_SetLampState(topLamp, (tally==11), 0, 750);
    }
  }
*/  
}

void SlideCards(unsigned long slideStart, byte displayNum, byte top, byte tally, boolean hasAce) {
  unsigned long digitMultiplier = 10000;
  unsigned long elapsed = (CurrentTime-slideStart);

  if (elapsed<1000) {
    byte blank = 0x03;
    for (unsigned long count=0; count<(elapsed/250); count++) {
      digitMultiplier /= 10;
      blank = blank << 1;
    }
    blank |= 0x30;
    unsigned long topDigit = top;
    unsigned long bottomDigit = tally;
    if (digitMultiplier==10) topDigit -= (topDigit%10);

    bottomDigit += topDigit*digitMultiplier;

    OverridePlayerDisplays(displayNum);
    RPU_SetDisplay(displayNum, bottomDigit);
    RPU_SetDisplayBlank(displayNum, blank); 
  } else {
    unsigned long newValue = top + tally;
    if (hasAce) {
      if ((elapsed/500)%2 && newValue<11) newValue += 10;
      OverridePlayerDisplays(displayNum);
      RPU_SetDisplay(displayNum, newValue);
      RPU_SetDisplayBlank(displayNum, 0x30); 
    } else {
      OverridePlayerDisplays(displayNum);
      RPU_SetDisplay(displayNum, newValue);
      RPU_SetDisplayBlank(displayNum, 0x30); 
    }
  }
  
}

byte cardShown = 0;
boolean Natural21;

boolean ShowCards() {
  boolean allCardsShown = false;
  byte displayForPlayer=0, displayForDealer=2;
  if ((CurrentPlayer%2)==0) {
    displayForPlayer = 1;
    displayForDealer = 3;
  }
  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>100) {
    if ((CurrentTime-BettingModeStart)<800) {
      OverridePlayerDisplays(displayForPlayer);
      OverridePlayerDisplays(displayForDealer);

      RPU_SetDisplay(displayForPlayer, ((unsigned long)PlayersTopCard)*10000 + (unsigned long)PlayersTally);
      if (NumberOfDealerHitsToShow) {
        RPU_SetDisplay(displayForDealer, ((unsigned long)DealersTopCard)*10000 + (unsigned long)88);
      } else {
        RPU_SetDisplay(displayForDealer, ((unsigned long)DealersTopCard)*10000 + (unsigned long)DealersTally);
      }
      RPU_SetDisplayBlank(displayForPlayer, 0x00);
      RPU_SetDisplayBlank(displayForDealer, 0x00);
      SetTallyLamps(0, false, PLAYER_21);
      SetTallyLamps(0, false, DEALER_21);
      cardShown = 0;
    } else if ((CurrentTime-BettingModeStart)<1400) {
      if (cardShown==0) {
        // Show player's first card
        RPU_SetDisplayBlank(displayForPlayer, 0x03);
        PlaySoundEffect(SOUND_EFFECT_DEAL_CARD_UP);
        cardShown = 1;
      }
    } else if ((CurrentTime-BettingModeStart)<2000) {
      if (cardShown==1) {
        // Show dealer's first card
        RPU_SetDisplayBlank(displayForDealer, 0x03);
        PlaySoundEffect(SOUND_EFFECT_DEAL_CARD_UP);
        cardShown = 2;
      }
    } else if ((CurrentTime-BettingModeStart)<2600) {
      if (cardShown==2) {
        // Show player's second card
        RPU_SetDisplayBlank(displayForPlayer, 0x33);
        PlaySoundEffect(SOUND_EFFECT_DEAL_CARD_UP);
        cardShown = 3;
      }
    } else if ((CurrentTime-BettingModeStart)<3200) {
      if (cardShown==3) {
        // Show dealer's second card
        RPU_SetDisplayBlank(displayForDealer, 0x33);
        PlaySoundEffect(SOUND_EFFECT_DEAL_CARD_DOWN);
        cardShown = 4;
      }
    } else if ((CurrentTime-BettingModeStart)<5200) {
      SlideCards(BettingModeStart+3200, displayForPlayer, PlayersTopCard, PlayersTally, PlayerHasAce);
      if ((CurrentTime-BettingModeStart)<4000) {
        byte newTally = PlayersTally+PlayersTopCard;
        SetTallyLamps(newTally, PlayerHasAce, PLAYER_21);
      }
    } else {
      if (PlayersTally==1 || PlayersTopCard==1) PlayerHasAce = true;
      PlayersTally += PlayersTopCard;
      PlayersTopCard = 0;

      // check for immediate win (10 + A)
      if (PlayersTally==11 && PlayerHasAce) {
        Natural21 = true;
      } else {
        Natural21 = false;
      }
      
      SetTallyLamps(PlayersTally, PlayerHasAce, PLAYER_21);
      if (DealersTopCard<6 && DealersTopCard!=0) {
        SetTallyLamps(16, false, DEALER_21);
      }
      allCardsShown = true;
    }
    LastTimeInfoUpdated = CurrentTime;
  }

  return allCardsShown;
}


void SweepSaucerLights(boolean allOff = false) {
  byte SaucerLightIndex = (CurrentTime/80)%4 + 1;
  if (allOff) SaucerLightIndex = 0;

  RPU_SetLampState(B_2XFEATURE_BONUS, (SaucerLightIndex==1)?1:0);
  RPU_SetLampState(SPECIAL, (SaucerLightIndex==1)?1:0);
  RPU_SetLampState(B_2X_3XFEATURE, (SaucerLightIndex==2)?1:0);
  RPU_SetLampState(B_3X_5XFEATURE, (SaucerLightIndex==3)?1:0);
  RPU_SetLampState(B_5X_BONUS, (SaucerLightIndex==4)?1:0);
  RPU_SetLampState(EXTRA_BALL, (SaucerLightIndex==4)?1:0);
  
}

void SetBonusXLights(byte bonusX) {
  RPU_SetLampState(B_2XFEATURE_BONUS, (bonusX==1)?1:0);
  RPU_SetLampState(B_2X_3XFEATURE, (bonusX==2)?1:0);
  RPU_SetLampState(B_3X_5XFEATURE, (bonusX==3)?1:0);
  RPU_SetLampState(B_5X_BONUS, (bonusX>=5)?1:0);
  RPU_SetLampState(EXTRA_BALL, (bonusX==5)?1:0);
  RPU_SetLampState(SPECIAL, (bonusX>=6)?1:0, 0, 150 * ((bonusX>6)?1:0));
}


void SweepSpinnerLights(boolean allOff = false) {
  byte id = (CurrentTime/100)%10 + 1;
  if (allOff) id = 0;

  for (int count=0; count<9; count++) {
    RPU_SetLampState(BONUS_1+count, (id==(count+1)||id==(count+2)), (id==(count+2)));
  }
  RPU_SetLampState(BONUS_1+9, (id==(10)||id==1), (id==1));
  
}

void ShowSpinnerLights(byte lightNum, boolean single=true) {
  for (int count=0; count<10; count++) {
    RPU_SetLampState(BONUS_1+count, (single && count==lightNum) || (!single && (count<(10-lightNum))), !single);
  }
}

boolean ShowPlayerHit() {
  boolean slideDone = false;
  byte displayForPlayer=0;
  if ((CurrentPlayer%2)==0) {
    displayForPlayer = 1;
  }

  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>125) {
    if (BettingModeStart!=0 && (CurrentTime-BettingModeStart)<2000) {
      SlideCards(BettingModeStart, displayForPlayer, PlayersTopCard, PlayersTally, PlayerHasAce);
    } else {
      if (PlayersTopCard==1) PlayerHasAce = true;
      PlayersTally += PlayersTopCard;
      PlayersTopCard = 0;
      SetTallyLamps(PlayersTally, PlayerHasAce, PLAYER_21);      
      slideDone = true;
    }
    LastTimeInfoUpdated = CurrentTime;
  }

  return slideDone;
}


void CheckHighScores() {
  

  unsigned long highestScore = 0;
  int highScorePlayerNum = 0;
  for (int count=0; count<CurrentNumPlayers; count++) {
    if (CurrentScores[count]>highestScore) highestScore = CurrentScores[count];
    highScorePlayerNum = count;
  }

  if (highestScore>HighScore) {
    HighScore = highestScore;
    if (HighScoreReplay) {
      Credits+=3;
      if (Credits>MaximumCredits) Credits = MaximumCredits;
      RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
      RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 3);
      RPU_SetDisplayCredits(Credits, CreditDisplay && !FreePlayMode);
    }
    RPU_WriteULToEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, highestScore);
    RPU_WriteULToEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE) + 1);

    for (int count=0; count<4; count++) {
      if (count==highScorePlayerNum) {
        RPU_SetDisplay(count, CurrentScores[count], true, 2);
      } else {
        RPU_SetDisplayBlank(count, 0x00);
      }
    }

    for (int count=0; count<3; count++) {
      RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime + count*300, true);
    }    
  }
}




byte GetNextCard() {

  byte playerNum;

  if (RandomizeDeck%2) playerNum = 0;
  else playerNum = CurrentPlayer;

  byte deckPtr = PlayersDeckPtr[playerNum];
  byte nextCard;
  byte cardByte = Deck[deckPtr/2];
  if ((deckPtr%2)==0) nextCard = cardByte&0x0F;
  else nextCard = cardByte/16;  
  
  if (nextCard>10) nextCard = 10;

  PlayersDeckPtr[playerNum]+=1;
  if (PlayersDeckPtr[playerNum]==52) PlayersDeckPtr[playerNum] = 0;
  return nextCard;
}




void ToggleAce() {

  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>500) {
    byte displayForPlayer=0;
    if ((CurrentPlayer%2)==0) {
      displayForPlayer = 1;
    }

    byte plus10 = 0;
    if (LastTimeInfoUpdated) {
      if (PlayersTally<11) plus10 = 10*((CurrentTime/500)%2);
      else if (PlayersTally==11) plus10 = 10;
    }
    

    OverridePlayerDisplays(displayForPlayer);
    RPU_SetDisplay(displayForPlayer, PlayersTally + plus10);
    RPU_SetDisplayBlank(displayForPlayer, 0x30);

    LastTimeInfoUpdated = CurrentTime;
  }
}


byte LastCreditButtonState;
byte CreditButtonSequenceGate;
unsigned long LastTimeCreditButtonPressed;
unsigned long LastTimeCreditButtonReleased;


int ClearSwitchBuffer(int curState, boolean ResetNumPlayers) {
  int returnState = curState;
  byte switchHit;
  while ( (switchHit=RPU_PullFirstFromSwitchStack())!=SWITCH_STACK_EMPTY ) {
    if (switchHit==SW_CREDIT_RESET) {
      if (AddPlayer(ResetNumPlayers)) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    if (switchHit==SW_COIN_1 || switchHit==SW_COIN_2 || switchHit==SW_COIN_3) {
      byte chuteNum = 0;
      if (switchHit==SW_COIN_2) chuteNum = 1;
      if (switchHit==SW_COIN_3) chuteNum = 2;
      AddCoin(chuteNum);
      AddCoinToAudit(switchHit);
    }
    if (switchHit==SW_SELF_TEST_SWITCH && (CurrentTime-GetLastSelfTestChangedTime())>500) {
      returnState = MACHINE_STATE_TEST_LAMPS;
      SetLastSelfTestChangedTime(CurrentTime);
    }
  }
  return returnState;
}

byte AttractLastHeadMode = 255;
byte AttractLastPlayfieldMode = 255;

int RunAttractMode(int curState, boolean curStateChanged) {

  int returnState = curState;

  // If this is the first time in the attract mode loop
  if (curStateChanged) {
    RPU_DisableSolenoidStack();
    RPU_TurnOffAllLamps();
    RPU_SetLampState(CREDIT_LIGHT, Credits||FreePlayMode);
    if (Credits>=MaximumCredits || FreePlayMode) RPU_SetCoinLockout(true);
    else RPU_SetCoinLockout(false);
    RPU_SetDisableFlippers(true);
    if (DEBUG_MESSAGES) {
      Serial.write("Entering Attract Mode\n\r");
    }
    for (int count=0; count<4; count++) {
      RPU_SetDisplayBlank(count, 0x00);     
    }
    RPU_SetDisplayCredits(Credits, CreditDisplay && !FreePlayMode);
    RPU_SetDisplayBallInPlay(CreditButtonSequenceGate);
    AttractLastHeadMode = 255;
    AttractLastPlayfieldMode = 255;

    LastCreditButtonState = 0;
    CreditButtonSequenceGate = 0;
    LastTimeCreditButtonPressed = 0;
    LastTimeCreditButtonReleased = 0;  
  }

  // Alternate displays between high score and blank
  if ((CurrentTime/6000)%2==0) {

    if (AttractLastHeadMode!=1) {
      if (DEBUG_MESSAGES) {
        Serial.write("Showing high score\n\r");
      }
      RPU_SetLampState(HIGH_SCORE, 1, 0, 500);
      RPU_SetLampState(GAME_OVER, 0);
      SetPlayerLamps(0);

      ShowPlayerDisplays(false, true, 4);
      RPU_SetDisplayCredits(Credits, CreditDisplay && !FreePlayMode);
      RPU_SetDisplayBallInPlay(CreditButtonSequenceGate, true);
    }
    AttractLastHeadMode = 1;
    
  } else {
    if (AttractLastHeadMode!=2) {
      RPU_SetLampState(HIGH_SCORE, 0);
      RPU_SetLampState(GAME_OVER, 1);
      RPU_SetDisplayCredits(Credits, CreditDisplay && !FreePlayMode);
      RPU_SetDisplayBallInPlay(CreditButtonSequenceGate, true);
      for (int count=0; count<4; count++) {
        if (CurrentNumPlayers>0) {
          ShowPlayerDisplays();
        } else {
          RPU_SetDisplayBlank(count, 0x30);
          RPU_SetDisplay(count, 0);          
        }
      }
    }
    RPU_SetLampState(GAME_OVER, 1);
    SetPlayerLamps(((CurrentTime/250)%4) + 1);
    AttractLastHeadMode = 2;
  }

  if ((CurrentTime/6000)%3==0) {
    // This attract mode shows the skill shot on top lanes
    // and sweeps the bonus tree  
    if (AttractLastPlayfieldMode!=1) {
      RPU_SetLampState(CREDIT_LIGHT, Credits||FreePlayMode);
      for (int count=0; count<5; count++) {
        RPU_SetLampState(PLAYER_21+count, 0);
        RPU_SetLampState(DEALER_21+count, 0);
      }
      RPU_SetLampState(LEFT_OUTLANE, 0);
      RPU_SetLampState(RIGHT_OUTLANE, 0);
      RPU_SetLampState(SPINNER_1000, 0);
      RPU_SetLampState(PLAYER_WINS, 0);
    }

    PulseTopLights((CurrentTime/175)%16, 0);
    ShowBonusOnTree((CurrentTime/100)%39, 0);

    byte bumper = (CurrentTime/500)%3;
    RPU_SetLampState(RED_BUMPER, bumper==0);
    RPU_SetLampState(SPADE_BUMPER, bumper==1);
    RPU_SetLampState(CLUB_BUMPER, bumper==2);
    
    AttractLastPlayfieldMode = 1;
  } else if ((CurrentTime/6000)%3==1) {
    if (AttractLastPlayfieldMode!=2) {
      RPU_SetLampState(CREDIT_LIGHT, Credits||FreePlayMode);
      RPU_SetLampState(RED_BUMPER, 0);
      RPU_SetLampState(SPADE_BUMPER, 0);
      RPU_SetLampState(CLUB_BUMPER, 0);
      ShowBonusOnTree(0, 0);
    }

    // Sweep the Spinner lights
    PulseTopLights((CurrentTime/175)%16, 0);
    SweepSpinnerLights();
    SweepSaucerLights();

    AttractLastPlayfieldMode = 2;
  } else {
    if (AttractLastPlayfieldMode!=3) {
      RPU_SetLampState(CREDIT_LIGHT, Credits||FreePlayMode);

      RPU_SetLampState(LEFT_OUTLANE, 1, 0, 400);
      RPU_SetLampState(RIGHT_OUTLANE, 1, 0, 400);
  
      RPU_SetLampState(SPINNER_1000, 1, 0, 170);
      RPU_SetLampState(PLAYER_WINS, 1, 0, 170);
      SweepSpinnerLights(true);
      SweepSaucerLights(true);
    }

    PulseTopLights((CurrentTime/175)%16, 0);
    byte id = (CurrentTime/100)%8;

    for (int count=0; count<4; count++) {
      RPU_SetLampState(PLAYER_17-count, (id==(count)||id==(count+1)), (id==(count+1)));
      RPU_SetLampState(DEALER_21+count, (id==(count)||id==(count+1)), (id==(count+1)));
    }
    RPU_SetLampState(PLAYER_17-4, (id==(4)||id==0), (id==0));
    RPU_SetLampState(DEALER_21+4, (id==(4)||id==0), (id==0));

    AttractLastPlayfieldMode = 3;
  }

  returnState = ClearSwitchBuffer(returnState, true);

  if (returnState==MACHINE_STATE_ATTRACT) {
    if (CurrentTime>(LastTimeCreditButtonPressed+3000) && CurrentTime>(LastTimeCreditButtonReleased+3000)) {
      LastTimeCreditButtonPressed = 0;
      LastTimeCreditButtonReleased = 0;
      CreditButtonSequenceGate = 0;  
      RPU_SetDisplayBallInPlay(CreditButtonSequenceGate);
    }
    
    if (RPU_ReadSingleSwitchState(SW_CREDIT_RESET)) {
      // We must be out of credits
      if (LastCreditButtonState==0) LastTimeCreditButtonPressed = CurrentTime;
      LastCreditButtonState = 1;
    } else {
      if (LastCreditButtonState==1) LastTimeCreditButtonReleased = CurrentTime;
      LastCreditButtonState = 0;      
    }

    if (LastTimeCreditButtonPressed && (LastTimeCreditButtonReleased!=0 || CreditButtonSequenceGate==0)) {
      switch (CreditButtonSequenceGate) {
        case 0:
          if (LastCreditButtonState==0) {
            if (CurrentTime>(LastTimeCreditButtonPressed+1000) && CurrentTime<(LastTimeCreditButtonReleased+1000)) {
              CreditButtonSequenceGate = 1;
            }
          } else {
            if (CurrentTime>(LastTimeCreditButtonPressed+1500)) {
              LastTimeCreditButtonPressed = 0;
              LastTimeCreditButtonReleased = 0;
              CreditButtonSequenceGate = 0;  
            }
          }
          break;
        case 1:
          if (LastCreditButtonState==1) {
            if (CurrentTime>(LastTimeCreditButtonReleased+1000)) {
              CreditButtonSequenceGate = 2;
            } else {
              LastTimeCreditButtonPressed = 0;
              LastTimeCreditButtonReleased = 0;
              CreditButtonSequenceGate = 0;  
            }
          }
          break;
        case 2:
          if (LastCreditButtonState==0) {
            if (CurrentTime>(LastTimeCreditButtonPressed+1000) && CurrentTime<(LastTimeCreditButtonReleased+1000)) {
              CreditButtonSequenceGate = 3;
            }
          } else {
            if (CurrentTime>(LastTimeCreditButtonPressed+1500)) {
              LastTimeCreditButtonPressed = 0;
              LastTimeCreditButtonReleased = 0;
              CreditButtonSequenceGate = 0;  
            }
          }
          break;
        case 3:
          if (LastCreditButtonState==1) {
            if (CurrentTime>(LastTimeCreditButtonReleased+1000)) {
              CreditButtonSequenceGate = 4;
              RPU_SetDisplayBallInPlay(CreditButtonSequenceGate);
            } else {
              LastTimeCreditButtonPressed = 0;
              LastTimeCreditButtonReleased = 0;
              CreditButtonSequenceGate = 0;  
            }
          }
          break;
        case 4:
          if (LastCreditButtonState==0) {
            if (CurrentTime>(LastTimeCreditButtonPressed+500) && CurrentTime<(LastTimeCreditButtonReleased+1000)) {
              CreditButtonSequenceGate = 5;
              RPU_SetDisplayBallInPlay(CreditButtonSequenceGate);
            }
          } else {
            if (CurrentTime>(LastTimeCreditButtonPressed+1000)) {
              LastTimeCreditButtonPressed = 0;
              LastTimeCreditButtonReleased = 0;
              CreditButtonSequenceGate = 0;  
            }
          }
          break;
        case 5:
          if (LastCreditButtonState==1) {
            CreditButtonSequenceGate = 6;
            RPU_SetDisplayBallInPlay(CreditButtonSequenceGate);
          }
          break;
        case 6:
          if (LastCreditButtonState==0) {
            if (CurrentTime>(LastTimeCreditButtonPressed+1000) && CurrentTime<(LastTimeCreditButtonReleased+1000)) {
              AddCredit(10, true);
              LastTimeCreditButtonPressed = 0;
              LastTimeCreditButtonReleased = 0;
              CreditButtonSequenceGate = 0;  
            }
          } else {
            if (CurrentTime>(LastTimeCreditButtonPressed+1500)) {
              LastTimeCreditButtonPressed = 0;
              LastTimeCreditButtonReleased = 0;
              CreditButtonSequenceGate = 0;  
            }
          }
          break;
      }
      RPU_SetDisplayBallInPlay(/*CreditButtonSequenceGate*/0);
    }
    
  }

  return returnState;
}


void IncreaseBonusX() {
  switch (BonusX[CurrentPlayer]) {
    case 1: BonusX[CurrentPlayer] = 2; break;
    case 2: BonusX[CurrentPlayer] = 3; break;
    case 3: BonusX[CurrentPlayer] = 5; break;
    case 5:
      // Extra ball collect
      if (NoveltyScoring) {
        CurrentScores[CurrentPlayer] += ExtraBallValue;
      } else {
        RPU_SetLampState(HEAD_SAME_PLAYER, 1);
        SamePlayerShootsAgain = true;
      }
      BonusX[CurrentPlayer] = 6;
    break;
    case 6:
      // Special collect
      if (NoveltyScoring) {
        CurrentScores[CurrentPlayer] += SpecialValue;
      } else {
        if (!SpecialCollectedThisBall) {
          RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
          AddCredit();
          if (OneSpecialPerBall) SpecialCollectedThisBall = true;
        } else {
          CurrentScores[CurrentPlayer] += SpecialValue;
        }
      }
      BonusX[CurrentPlayer] = 7;
    break;
  }
}


boolean PlayerWinsHasBeenShown = false;

boolean RunEndOfRound() {
  boolean endOfRoundDone = false;
  // Sweep the dealer's score
  // Hit dealer if less than 17
  // Flash player wins light if player wins
  // Add bet to bonus 1k at a time or
  // Subtract bet from bonus 1k at a time
  // return true after all that is done

  byte playersDisplay = 1, dealersDisplay = 3;
  if (CurrentPlayer%2) {
    playersDisplay = 0;
    dealersDisplay = 2;
  }

  if (PlayersTopCard!=0) {
    PlayersTally += PlayersTopCard;
    if (PlayersTopCard==1) PlayerHasAce = true;
    PlayersTopCard = 0;
  }

  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>125) {

    boolean playerWins = false;
    if (DealersTally>21) playerWins = true;
    if (PlayersTally<22 && PlayersTally>=DealersTally) playerWins = true;
    byte totalHand = DealersTopCard + DealersTally;
    boolean dealerHasToHit = false;
    unsigned long elapsedTime = CurrentTime-BettingModeStart;
    if (DealerHasAce) {
      if (totalHand<7 || (totalHand>11 && totalHand<17)) dealerHasToHit = true; 
    } else {
      if (totalHand<17) dealerHasToHit = true;      
    }

    if (elapsedTime<2000) {
      if (PlayersTally<12 && PlayerHasAce) {
        PlayersTally += 10;
        PlayerHasAce = false;
      }
      SetTallyLamps(PlayersTally, false, PLAYER_21);
      SlideCards(BettingModeStart, dealersDisplay, DealersTopCard, DealersTally, DealerHasAce);
      PlayerWinsHasBeenShown = false;
    } else if (dealerHasToHit) {
      // Dealer has to hit
      BettingModeStart = CurrentTime;
      DealersTally += DealersTopCard;
      DealersTopCard = GetNextCard();
      PlaySoundEffect(SOUND_EFFECT_DEAL_CARD_UP);
      if (DealersTopCard==1) DealerHasAce = true;
      SetTallyLamps(DealersTally, DealerHasAce, DEALER_21);
    } else if (elapsedTime<3000) {
      if (DealersTopCard) {
        DealersTally += DealersTopCard;
        DealersTopCard = 0;
        if (DealersTally<17 && DealerHasAce) {
          DealerHasAce = false;
          DealersTally += 10;
        }
        OverridePlayerDisplays(playersDisplay);
        OverridePlayerDisplays(dealersDisplay);
        RPU_SetDisplay(playersDisplay, PlayersTally);
        RPU_SetDisplayBlank(playersDisplay, 0x30);
        RPU_SetDisplay(dealersDisplay, DealersTally);
        RPU_SetDisplayBlank(dealersDisplay, 0x30);      
        SetTallyLamps(PlayersTally, false, PLAYER_21);
        SetTallyLamps(DealersTally, false, DEALER_21);
      } else {
        if (!PlayerWinsHasBeenShown && playerWins) {
          PlayerWinsHasBeenShown = true;
          RPU_SetLampState(PLAYER_WINS, playerWins, 0, 220);        
          if (playerWins) {
            PlaySoundEffect(SOUND_EFFECT_PLAYER_WINS);
            IncreaseBonusX();
            SetBonusXLights(BonusX[CurrentPlayer]);
          }
        }        
      }
    } else if (elapsedTime<4000) {      
      if (CurrentSweepingBet>0) {
        if (playerWins) {
          AddToBonus(1);
          PlaySoundEffect(SOUND_EFFECT_BONUS_ADD);
        } else {
          if (Bonus>1) Bonus -= 1;
          PlaySoundEffect(SOUND_EFFECT_BONUS_SUBTRACT);
        }
        if (CurrentSweepingBet>0) CurrentSweepingBet -= 1;
        BettingModeStart += 250;
      }
    } else {
      if (RPU_ReadSingleSwitchState(SW_SAUCER)) RPU_PushToTimedSolenoidStack(SOL_SAUCER, 5, 100); 
      SetTallyLamps(0, false, PLAYER_21);
      SetTallyLamps(0, false, DEALER_21);
      endOfRoundDone = true;     
      UsingTreeToShowBet = false;
      RPU_SetLampState(PLAYER_WINS, 0);      
    }

    LastTimeInfoUpdated = CurrentTime;
  }
  
  return endOfRoundDone;
}


boolean RunBonusCollect() {
  boolean doneWithCollect = false;
  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>125) {
    if (Bonus>0) {
      PlaySoundEffect(SOUND_EFFECT_BONUS_ADD);
      Bonus -= 1;
      CurrentScores[CurrentPlayer] += 5000;
      ShowBonusOnTree(Bonus, 0);
    } 
    if (Bonus==0) {
      doneWithCollect = true;
      if (RPU_ReadSingleSwitchState(SW_SAUCER)) RPU_PushToTimedSolenoidStack(SOL_SAUCER, 5, 100); 
      UsingTreeToShowBet = false;
      RPU_SetLampState(PLAYER_WINS, 0);
      BonusX[CurrentPlayer] = 6;
      SetBonusXLights(BonusX[CurrentPlayer]);
    }
    LastTimeInfoUpdated = CurrentTime;
  }
  return doneWithCollect;
}


boolean PayoutNatural() {
  boolean payoutDone = false;
  byte playersDisplay = 1;
  if (CurrentPlayer%2) {
    playersDisplay = 0;
  }

  if (LastTimeInfoUpdated==0 || (CurrentTime-LastTimeInfoUpdated)>125) {

    if (LastTimeInfoUpdated==0) {
      PlaySoundEffect(SOUND_EFFECT_PLAYER_WINS);
      IncreaseBonusX();
      SetBonusXLights(BonusX[CurrentPlayer]);
      SetTallyLamps(21, false, PLAYER_21);
      RPU_SetDisplay(playersDisplay, 21);
      RPU_SetLampState(PLAYER_WINS, 1);      
    }

    if ((CurrentTime-BettingModeStart)<3000) {
      if (((CurrentTime-BettingModeStart)/125)%2==0) {
        RPU_SetDisplayBlank(playersDisplay, 0x00);
      } else {
        RPU_SetDisplayBlank(playersDisplay, 0x30);        
      }
    } else if ((CurrentTime-BettingModeStart)<3500) {
      if (CurrentSweepingBet>0) {
        AddToBonus(1);
        PlaySoundEffect(SOUND_EFFECT_BONUS_ADD);
        CurrentSweepingBet -= 1;
        BettingModeStart += 250;
      }      
    } else {
      payoutDone = true;
      SetTallyLamps(0, false, PLAYER_21);
      SetTallyLamps(0, false, DEALER_21);
      RPU_SetLampState(PLAYER_WINS, 0);      
    }
    
    LastTimeInfoUpdated = CurrentTime;
  }

  return payoutDone;
}


//  Round doesn't start until Player bonus is greater than minimum bet
//  [BETTING_STAGE_BUILDING_STAKES]
//  Once minimum bet (MinimumBet) is reached, the bet sweeps from minimum to maximum bet (Bonus-1000) and saucer sweep lights are shown
//  [BETTING_STAGE_BET_SWEEP]
//  Once saucer is hit, cards are shown. Dealer's tally is not shown until the "Changer Dealer" standup is hit
//    [BETTING_STAGE_DEAL]
//    if player has A & 10, player's bonus += bet/2 and BettingStage goes to BETTING_STAGE_NATURAL_21 and then back to BETTING_STAGE_BET_SWEEP
//    Player lights show player tally (when it's in range)
//    Player's tally toggles (2Hz) if the player has an ace
//    [BETTING_STAGE_WAIT_FOR_COLLECT]
//    If (NoHitsOver16) is set, the hit button is disabled for scores>=17
//    "Change Player's Hand" hits (adds a card)
//      If player busts:
//        Bet is lost, and BettingStage goes back to BETTING_STAGE_BUILDING_STAKES 
//      if player hits spinner, SurrenderSpins increases
//        if SurrenderSpins>20, 1/2 of bet is surrendered
//    "Change Dealer's Hand" ShowDealerCount times will show the dealer's hidden card
//  Once saucer is hit during [BETTING_STAGE_WAIT_FOR_COLLECT]
//    [BETTING_STAGE_END_OF_ROUND]
//    if Player's hand >= Dealer's hand
//      +5000
//      Bonus += Bet
//      Advance BonusX
//    If Player's hand < Dealer's hand
//      +5000
//      Bonus -= Bet
//    Return to BETTING_STAGE_BUILDING_STAKES 




int NormalGamePlay() {
  int returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

  // If the playfield hasn't been validated yet, flash score and player up num
  if (BallFirstSwitchHitTime==0) {
    if (!PlayerUpLightBlinking) {
      SetPlayerLamps((CurrentPlayer+1), 500);
      PlayerUpLightBlinking = true;
    }

    // We're also going to show the skill shot here until a switch is hit
    // Cycle the top lights
    int topLightPulse = (CurrentTime/250)%16;
    PulseTopLights(topLightPulse, SuitsComplete[CurrentPlayer]);
    CurrentSkillShotSuit = 4 - (topLightPulse/4);

    BettingStage = BETTING_STAGE_BUILDING_STAKES;
  } else {
    if (PlayerUpLightBlinking) {
      SetPlayerLamps((CurrentPlayer+1));
      PlayerUpLightBlinking = false;
    }
    ShowSuitsComplete(SuitsComplete[CurrentPlayer]);
    CurrentSkillShotSuit = 0;
  }

  if (BallFirstSwitchHitTime!=0 && Bonus>=1 && BettingStage==BETTING_STAGE_BUILDING_STAKES) {
    // We're entering the betting/hand mode
    BettingStage = BETTING_STAGE_BET_SWEEP;     
    BettingModeStart = CurrentTime;
    LastTimeInfoUpdated = 0;
    ClearOverridePlayerDisplays();
    SetBonusXLights(BonusX[CurrentPlayer]);
  }

  if (BettingStage==BETTING_STAGE_BET_SWEEP) {
    // Can only bet up to Bonus-1k
    CalculateAndShowBetSweep(1, (Bonus>1)?(Bonus-1):1);
    if ( BonusX[CurrentPlayer]<7 && (((CurrentTime-BettingModeStart)/2000)%3)==0 ) SweepSaucerLights();
    else SetBonusXLights(BonusX[CurrentPlayer]);
  }    

  if (BettingStage==BETTING_STAGE_DEAL) {
    SweepSaucerLights(true);
    SetBonusXLights(BonusX[CurrentPlayer]);
    PlayersTopCard = GetNextCard();
    DealersTopCard = GetNextCard();    
    PlayersTally = GetNextCard();
    DealersTally = GetNextCard();
    PlayerHasAce = (PlayersTopCard==1 || PlayersTally==1);
    DealerHasAce = (DealersTopCard==1 || DealersTally==1);
    BettingStage = BETTING_STAGE_SHOW_DEAL;
    BettingModeStart = 0;
    LastTimeInfoUpdated = 0;
    SetBonusXLights(BonusX[CurrentPlayer]);
  }

  if (BettingStage==BETTING_STAGE_SHOW_DEAL) {
    if (BettingModeStart==0) {
      SetBonusXLights(BonusX[CurrentPlayer]);
      BettingModeStart = CurrentTime;
    }
    if (ShowCards()) {
      SetBonusXLights(BonusX[CurrentPlayer]);
      ShowDealersCardCountdown = NumberOfDealerHitsToShow;      
      BettingStage = BETTING_STAGE_WAIT_FOR_COLLECT;
      RPU_PushToTimedSolenoidStack(SOL_SAUCER, 5, 100);
      PlayerHits = false;
      PlayerCheats = false;
      BettingModeStart = 0;
      LastTimeInfoUpdated = 0;
      ShowingSurrenderSpins = 0;
      SurrenderSpins = 0;
    }
  }


  if (BettingStage==BETTING_STAGE_WAIT_FOR_COLLECT) {
    if (BettingModeStart==0) {
      if (!PlayerHits) {
               
        // Player hasn't hit yet, so we need to toggle aces (if any)
        if (PlayerHasAce) {
          ToggleAce();

          // Don't have to wait any longer if it's a natural 21
          if (Natural21) {
            if (CurrentSweepingBet>2) CurrentSweepingBet += (CurrentSweepingBet / 2);
            else CurrentSweepingBet += 1;
            BettingModeStart = 0;
            BettingStage=BETTING_STAGE_NATURAL_21;
          }
        }

        // Need to pulse the Player Wins light based on likelihood of win
        ShowPlayerWinsPulse();
        
        // Alert the player that they can finish the hand or surrender
        if (ShowingSurrenderSpins==0 || (CurrentTime-ShowingSurrenderSpins)>5000) {
          SweepSpinnerLights();
        } else {
          if (SurrenderSpins>10) {
            CurrentSweepingBet /= 2;
            if (CurrentSweepingBet==0) CurrentSweepingBet = 1;
            SurrenderSpins = 0;
          }
          ShowBetUntilThisTime = CurrentTime + 3000;
          ShowBetOnTree(CurrentSweepingBet);
          UsingTreeToShowBet = true;
          ShowSpinnerLights(SurrenderSpins, false);
        }
        
      } else {
        if (BettingModeStart==0) {
          if (PlayerCheats==false) {
            PlayersTopCard = GetNextCard();
          } else {
            PlayersTopCard = 1;
          }
          LastTimeInfoUpdated = 0;
          BettingModeStart = CurrentTime;
        }
      }
    }
    if (BettingModeStart!=0 && ShowPlayerHit()) {
      BettingModeStart = 0;
      PlayerHits = false;
    }
  }


  if (BettingStage==BETTING_STAGE_NATURAL_21) {
    if (BettingModeStart==0) {
      ShowPlayerWinsPulse(true);
      LastTimeInfoUpdated = 0;
      BettingModeStart = CurrentTime;
      SweepSpinnerLights(true);
    }
    if (PayoutNatural()) {
      BettingModeStart = 0;
      BettingStage = BETTING_STAGE_BUILDING_STAKES;
    }
  }


  if (BettingStage==BETTING_STAGE_END_OF_ROUND) {
    if (BettingModeStart==0) {
      ShowPlayerWinsPulse(true);
      LastTimeInfoUpdated = 0;
      BettingModeStart = CurrentTime;
      SweepSpinnerLights(true);
    }
    if (RunEndOfRound()) {
      BettingModeStart = 0;
      BettingStage = BETTING_STAGE_BUILDING_STAKES;
      if (BonusX[CurrentPlayer]==7) {
        BettingStage = BETTING_STAGE_WAIT_FOR_BONUS_COLLECT;
      }
    }
  }

  if (BettingStage==BETTING_STAGE_WAIT_FOR_BONUS_COLLECT) {
    if (BettingModeStart==0) {
      LastTimeInfoUpdated = 0;
      BettingModeStart = CurrentTime;     
      SweepSpinnerLights(true);
      ShowPlayerWinsPulse(true);
      ClearOverridePlayerDisplays();
      SetBonusXLights(BonusX[CurrentPlayer]);
    }

    unsigned long timeToWaitBetweenBeats = 1000;
    if (NumSecondsForBonusCollect!=0) {
      if (((CurrentTime-BettingModeStart)/1000)>((unsigned long)NumSecondsForBonusCollect-10)) timeToWaitBetweenBeats = 500;
      if (((CurrentTime-BettingModeStart)/1000)>((unsigned long)NumSecondsForBonusCollect)) {
        BettingModeStart = 0;
        BettingStage = BETTING_STAGE_BUILDING_STAKES;
      }
    }
    
    if ((CurrentTime-LastTimeInfoUpdated)>timeToWaitBetweenBeats) {
      if (SuitsComplete[CurrentPlayer]>0x2F) RPU_SetLampState(PLAYER_WINS, 1, 0, 100);
      else RPU_SetLampState(PLAYER_WINS, 0);
      PlaySoundEffect(SOUND_EFFECT_BONUS_COLLECT_HURRY_UP);
      LastTimeInfoUpdated = CurrentTime;
    }
  }

  if (BettingStage==BETTING_STAGE_BONUS_COLLECT) {
    if (BettingModeStart==0) {
      LastTimeInfoUpdated = 0;
      BettingModeStart = CurrentTime;
    }

    if (RunBonusCollect()) {
      BettingModeStart = 0;
      BettingStage = BETTING_STAGE_BUILDING_STAKES;
      // After they collect the bonus, restore top lanes to blank.
      SuitsComplete[CurrentPlayer] = 0x10;
//      ShowSuitsComplete(SuitsComplete[CurrentPlayer]);
    }    
  }
  
  // Check to see if ball is in the outhole
  if (RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
    if (BallTimeInTrough==0) {
      BallTimeInTrough = CurrentTime;
    } else {
      // Make sure the ball stays on the sensor for at least 
      // 0.5 seconds to be sure that it's not bouncing
      if ((CurrentTime-BallTimeInTrough)>500) {

        if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
        
        // if we haven't used the ball save, and we're under the time limit, then save the ball
        if (  !BallSaveUsed && 
              ((CurrentTime-BallFirstSwitchHitTime)/1000)<((unsigned long)BallSaveNumSeconds) ) {
        
          RPU_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
          if (BallFirstSwitchHitTime>0) {
            BallSaveUsed = true;
            RPU_SetLampState(SAME_PLAYER, 0);
//            RPU_SetLampState(HEAD_SAME_PLAYER, 0);
          }
          BallTimeInTrough = CurrentTime;

          returnState = MACHINE_STATE_NORMAL_GAMEPLAY;          
        } else {
          SetTallyLamps(0, false, PLAYER_21);
          SetTallyLamps(0, false, DEALER_21);
          SweepSpinnerLights(true);
          SweepSaucerLights(true);
          SetBonusXLights(BonusX[CurrentPlayer]);
          returnState = MACHINE_STATE_COUNTDOWN_BONUS;
        }
      }
    }
  } else {
    BallTimeInTrough = 0;
  }

  // Update Same player light
  if (BallSaveNumSeconds==0 || (BallFirstSwitchHitTime>0 && ((3000+CurrentTime-BallFirstSwitchHitTime)/1000)>BallSaveNumSeconds) || BallSaveUsed) {
    // Only set this lamp here, if there's no ballsave or
    // the playfield is validated & the ballsave is expired
    RPU_SetLampState(SAME_PLAYER, SamePlayerShootsAgain);
  } else if ( (BallSaveNumSeconds - (CurrentTime-BallFirstSwitchHitTime)/1000) < 4 ) {
    // if we're in the last 3 seconds of ball save, flash light very fast
    RPU_SetLampState(SAME_PLAYER, 1, 0, 150);
  }

  if (BallFirstSwitchHitTime!=0) {
    RPU_SetLampState(HEAD_SAME_PLAYER, SamePlayerShootsAgain);
  }

  return returnState;
}


unsigned long MatchSequenceStartTime = 0;
unsigned long MatchDelay = 150;
byte MatchDigit = 0;
byte NumMatchSpins = 0;
byte ScoreMatches = 0;

int ShowMatchSequence(boolean curStateChanged) {
  if (!MatchFeature) return MACHINE_STATE_ATTRACT;
  
  if (curStateChanged) {
    MatchSequenceStartTime = CurrentTime;
    MatchDelay = 1500;
    MatchDigit = random(0,9);
    NumMatchSpins = 0;
    RPU_SetLampState(MATCH, 1, 0);
    RPU_SetDisableFlippers();
    ScoreMatches = 0;
    RPU_SetLampState(BALL_IN_PLAY, 0);
    for (int count=0; count<CurrentNumPlayers; count++) OverridePlayerDisplays(count);
  }

  if (NumMatchSpins<40) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      MatchDigit += 1;
      if (MatchDigit>9) MatchDigit = 0;
      PlaySoundEffect(SOUND_EFFECT_MATCH_SPIN);
      RPU_SetDisplayBallInPlay((int)MatchDigit*10);
      MatchDelay += 50 + 4*NumMatchSpins;
      NumMatchSpins += 1;
      RPU_SetLampState(MATCH, NumMatchSpins%2, 0);

      if (NumMatchSpins==40) {
        RPU_SetLampState(MATCH, 0);
        MatchDelay = CurrentTime - MatchSequenceStartTime;
      }
    }
  }

  if (NumMatchSpins>=40 && NumMatchSpins<=43) {
    if (CurrentTime>(MatchSequenceStartTime + MatchDelay)) {
      if ( (CurrentNumPlayers>(NumMatchSpins-40)) && ((CurrentScores[NumMatchSpins-40]/10)%10)==MatchDigit) {
        ScoreMatches |= (1<<(NumMatchSpins-40));
        AddCredit();
        RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
        RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
        MatchDelay += 1000;
        NumMatchSpins += 1;
        RPU_SetLampState(MATCH, 1);
      } else {
        NumMatchSpins += 1;
      }
      if (NumMatchSpins==44) {
        MatchDelay += 5000;
      }
    }      
  }

  if (NumMatchSpins>43) {
    if (CurrentTime>(MatchSequenceStartTime + MatchDelay)) {
      ClearOverridePlayerDisplays();
      return MACHINE_STATE_ATTRACT;
    }    
  }

  for (int count=0; count<4; count++) {
    if ((ScoreMatches>>count)&0x01) {
      // If this score matches, we're going to flash the last two digits
      if ( (CurrentTime/200)%2 ) {
        RPU_SetDisplayBlank(count, RPU_GetDisplayBlank(count) & 0x0F);
      } else {
        RPU_SetDisplayBlank(count, RPU_GetDisplayBlank(count) | 0x30);
      }
    }
  }

  
  return MACHINE_STATE_MATCH_MODE;
}


void HandleBumperSwitch(byte switchNum) {

  unsigned long bumperMultiplier = (SuitsComplete[CurrentPlayer]/16);
  if (switchNum==SW_CLUB_BUMPER && (SuitsComplete[CurrentPlayer] & 0x01)) bumperMultiplier += 1;
  else if (switchNum==SW_SPADE_BUMPER && (SuitsComplete[CurrentPlayer] & 0x04)) bumperMultiplier += 1;
  else if (switchNum==SW_RED_BUMPER && (SuitsComplete[CurrentPlayer] & 0x0A)) bumperMultiplier += 1;

  if (bumperMultiplier) {
    if (switchNum!=SW_RED_BUMPER) {
      CurrentScores[CurrentPlayer] += 100;
      PlaySoundEffect(SOUND_EFFECT_BUMPER_100);
      CurrentScores[CurrentPlayer] += (100*bumperMultiplier);
    } else {
      CurrentScores[CurrentPlayer] += 1000;
      PlaySoundEffect(SOUND_EFFECT_BUMPER_1000);
      CurrentScores[CurrentPlayer] += (1000*bumperMultiplier);
    }
  } else {
    CurrentScores[CurrentPlayer] += 10;
    PlaySoundEffect(SOUND_EFFECT_BUMPER_10);
  }
  if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;        
}

void HandleTopLaneSwitch(byte switchNum) {

  byte suitBitMask = 0x08>>(switchNum-SW_HEARTS);
  if (CurrentSkillShotSuit==((switchNum-SW_HEARTS)+1)) {
    CurrentScores[CurrentPlayer] += 1000;    
    AddToBonus(2);
    PlaySoundEffect(SOUND_EFFECT_SKILL_SHOT);

    // Update SuitsComplete based on skill shot
    if ((switchNum-SW_HEARTS)%2) suitBitMask |= 0x05;
    else suitBitMask |= 0x0A;

  } else {
    CurrentScores[CurrentPlayer] += 100;
    if (SuitsComplete[CurrentPlayer]&suitBitMask) PlaySoundEffect(SOUND_EFFECT_TOPLANE_LIGHT);
    else PlaySoundEffect(SOUND_EFFECT_TOPLANE_LIGHT_PLUS);
  }
  
  SuitsComplete[CurrentPlayer] |= suitBitMask;
  if ((SuitsComplete[CurrentPlayer] & 0x0F)==0x0F) {
    if ((SuitsComplete[CurrentPlayer] & 0xF0)<0xF0) {
      SuitsComplete[CurrentPlayer] += 0x01;
    }
    CurrentScores[CurrentPlayer] += ((unsigned long)(SuitsComplete[CurrentPlayer]/16) * 1000);
    AddToBonus(SuitsComplete[CurrentPlayer]/8);
  }

//  ShowSuitsComplete(SuitsComplete[CurrentPlayer]);
  if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;


  // Allow player to cheat
  if (BettingStage==BETTING_STAGE_WAIT_FOR_COLLECT && PlayerHits==false) {
    if ( PlayersTally>=21 || (PlayerHasAce && PlayersTally==11)) {
      PlayerHits = false;
      PlayerCheats = false;
    } else {
      PlayerHits = true;
      PlayerCheats = true;
    }
  }
  

}


// This function handles state & switches for game play
int RunGamePlayMode(int curState, boolean curStateChanged) {
  int returnState = curState;
  unsigned long scoreAtTop = CurrentScores[CurrentPlayer];
  byte bonusAtTop = Bonus;
  
  // Very first time into gameplay loop
  if (curState==MACHINE_STATE_INIT_GAMEPLAY) {
    returnState = InitGamePlay(curStateChanged);    
  } else if (curState==MACHINE_STATE_INIT_NEW_BALL) {
    returnState = InitNewBall(curStateChanged, CurrentPlayer, CurrentBallInPlay);
  } else if (curState==MACHINE_STATE_NORMAL_GAMEPLAY) {
    returnState = NormalGamePlay();
  } else if (curState==MACHINE_STATE_COUNTDOWN_BONUS) {
    returnState = CountdownBonus(curStateChanged);
  } else if (curState==MACHINE_STATE_BALL_OVER) {
    // Clear out lights
    SetBonusXLights(0);
    ShowPlayerWinsPulse(true);
    ShowBonusOnTree(0, 0);
    if (SamePlayerShootsAgain) {
      returnState = MACHINE_STATE_INIT_NEW_BALL;
    } else {
      CurrentPlayer+=1;
      if (CurrentPlayer>=CurrentNumPlayers) {
        CurrentPlayer = 0;
        CurrentBallInPlay+=1;
      }
      scoreAtTop = CurrentScores[CurrentPlayer];
       
      if (CurrentBallInPlay>BallsPerGame) {
        CheckHighScores();
        PlaySoundEffect(SOUND_EFFECT_GAME_OVER);
        SetPlayerLamps(0);
        ClearOverridePlayerDisplays();
        ShowPlayerWinsPulse(true);
    
        returnState = MACHINE_STATE_MATCH_MODE;
      } else {
        PlaySoundEffect(SOUND_EFFECT_BALL_OVER);
        returnState = MACHINE_STATE_INIT_NEW_BALL;
      }
    }    
  } else if (curState==MACHINE_STATE_MATCH_MODE) {
    returnState = ShowMatchSequence(curStateChanged);    
  } else {
    returnState = MACHINE_STATE_ATTRACT;
  }

  byte switchHit;
  while ( (switchHit=RPU_PullFirstFromSwitchStack())!=SWITCH_STACK_EMPTY ) {

    if (switchHit==SW_SELF_TEST_SWITCH) {
      returnState = MACHINE_STATE_TEST_LAMPS;
      SetLastSelfTestChangedTime(CurrentTime);
    } else if (switchHit==SW_COIN_1 || switchHit==SW_COIN_2 || switchHit==SW_COIN_3) {
      byte chuteNum = 0;
      if (switchHit==SW_COIN_2) chuteNum = 1;
      if (switchHit==SW_COIN_3) chuteNum = 2;
      AddCoin(chuteNum);
      AddCoinToAudit(switchHit);
    } else if (NumTiltWarnings>MaxTiltWarnings && switchHit==SW_SAUCER) {
      RPU_PushToTimedSolenoidStack(SOL_SAUCER, 6, CurrentTime);
    } else if (NumTiltWarnings<=MaxTiltWarnings) {
      switch (switchHit) {
        case SW_CLUBS:
        case SW_DIAMONDS:
        case SW_SPADES:
        case SW_HEARTS:
          HandleTopLaneSwitch(switchHit);
        break;
        case SW_SPINNER:        
          if (SpinnerMadnessEndTime==0) {
            if (!Spinner1kLit) CurrentScores[CurrentPlayer] += 50;
            else CurrentScores[CurrentPlayer] += 1000;
          } else {
            if (!Spinner1kLit) CurrentScores[CurrentPlayer] += 1500;
            else CurrentScores[CurrentPlayer] += 3000;
          }
          if (BettingStage==BETTING_STAGE_WAIT_FOR_COLLECT) {
            ShowingSurrenderSpins = CurrentTime;
            SurrenderSpins += 1;
          } else {
            RegularSpins += 1;
            if (RegularSpins==5 || RegularSpins==9) {
              AddToBonus(1);
            }
            if (RegularSpins>9) RegularSpins = 0;
            ShowSpinnerLights(RegularSpins);
          }
          if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;                
        break;
        case SW_SAUCER:
          SweepSaucerLights(true);
          CurrentScores[CurrentPlayer] += 5000;
          // Saucer is hit as part of the skill shot!
          if (BallFirstSwitchHitTime==0) {
            SuitsComplete[CurrentPlayer] += 0x10;
            SuitsComplete[CurrentPlayer] &= 0xF0;
            if (SuitsComplete[CurrentPlayer]==0) SuitsComplete[CurrentPlayer] = 0xFF;
//            ShowSuitsComplete(SuitsComplete[CurrentPlayer]);
            PlaySoundEffect(SOUND_EFFECT_SKILL_SHOT);
            RPU_PushToTimedSolenoidStack(SOL_SAUCER, 6, CurrentTime + 1000);
          }
  
          if (BettingStage==BETTING_STAGE_BUILDING_STAKES) {
            PlaySoundEffect(SOUND_EFFECT_RANDOM_SAUCER);
            RPU_PushToTimedSolenoidStack(SOL_SAUCER, 6, CurrentTime + 500);
          } else if (BettingStage==BETTING_STAGE_BET_SWEEP) {
            BettingStage = BETTING_STAGE_DEAL;
            BettingModeStart = CurrentTime;
//            RPU_PushToTimedSolenoidStack(SOL_SAUCER, 6, CurrentTime + 500);
            // Ball will be ejected from the saucer after the deal.
          } else if (BettingStage==BETTING_STAGE_END_OF_ROUND) {
          } else if (BettingStage==BETTING_STAGE_DEAL) {
          } else if (BettingStage==BETTING_STAGE_SHOW_DEAL) {
          } else if (BettingStage==BETTING_STAGE_WAIT_FOR_COLLECT) {
            BettingModeStart = 0;
            BettingStage = BETTING_STAGE_END_OF_ROUND;
          } else if (BettingStage==BETTING_STAGE_WAIT_FOR_BONUS_COLLECT) {
            if (SuitsComplete[CurrentPlayer]>0x2F) {
              BettingStage = BETTING_STAGE_BONUS_COLLECT;
              BettingModeStart = 0;
            } else {
              RPU_PushToTimedSolenoidStack(SOL_SAUCER, 6, CurrentTime);
            }
          } else if (BettingStage==BETTING_STAGE_NATURAL_21) {
            RPU_PushToTimedSolenoidStack(SOL_SAUCER, 6, CurrentTime);
          }
          if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;        
        break;
        case SW_CLUB_BUMPER:
        case SW_SPADE_BUMPER:
        case SW_RED_BUMPER:
          HandleBumperSwitch(switchHit);
        break;
        case SW_CHANGE_PLAYER:
          if (BettingStage==BETTING_STAGE_WAIT_FOR_COLLECT) {
            PlayerHits = true;
            if (NoHitsOver16) {
              if (PlayersTally>16 || (PlayersTally<12 && PlayersTally>6 && PlayerHasAce)) {
                PlayerHits = false;
              }
            }
          }
          CurrentScores[CurrentPlayer] += 100;
          AddToBonus(2);
          PlaySoundEffect(SOUND_EFFECT_CHANGE_PLAYER);
          if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;
        break;
        case SW_CHANGE_DEALER:
          CurrentScores[CurrentPlayer] += 100;
          PlaySoundEffect(SOUND_EFFECT_CHANGE_DEALER);
          if (ShowDealersCardCountdown!=99 && ShowDealersCardCountdown>0) ShowDealersCardCountdown -= 1;
          if (ShowDealersCardCountdown==0) {
            if (BettingStage==BETTING_STAGE_WAIT_FOR_COLLECT) {
              RPU_SetDisplay(2 + (1-(CurrentPlayer%2)), ((unsigned long)DealersTopCard)*10000 + (unsigned long)DealersTally);
            }
          }
          if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;        
        break; 
        case SW_LEFT_SLING:
        case SW_RIGHT_SLING:
        case SW_10_PT_SWITCH:
          CurrentScores[CurrentPlayer] += 10;
          PlaySoundEffect(SOUND_EFFECT_SLING);
          if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;        
        break;
        case SW_LEFT_OUTLANE:
          if (LeftOutlaneLit) {
            CurrentScores[CurrentPlayer] += 50000;
            PlaySoundEffect(SOUND_EFFECT_OUTLANE_LIT);
          } else {
            CurrentScores[CurrentPlayer] += 100;
            PlaySoundEffect(SOUND_EFFECT_OUTLANE_UNLIT);
          }
          if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;        
        break;
        case SW_RIGHT_OUTLANE:
          if (RightOutlaneLit) {
            CurrentScores[CurrentPlayer] += 50000;
            PlaySoundEffect(SOUND_EFFECT_OUTLANE_LIT);
          } else {
            CurrentScores[CurrentPlayer] += 100;
            PlaySoundEffect(SOUND_EFFECT_OUTLANE_UNLIT);
          }
          if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;        
        break;
        case SW_INLANE:
          CurrentScores[CurrentPlayer] += 1000;
          AddToBonus(1);
          SpinnerMadnessEndTime = CurrentTime + 3000;
          PlaySoundEffect(SOUND_EFFECT_INLANE);
          if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;                
        break;
        case SW_BONUS_ROLLOVER:
          CurrentScores[CurrentPlayer] += 1000;
          AddToBonus(1);
          PlaySoundEffect(SOUND_EFFECT_BONUS_ROLLOVER);
          if (BallFirstSwitchHitTime==0) BallFirstSwitchHitTime = CurrentTime;                
        break;
        case SW_CREDIT_RESET:
          if (CurrentBallInPlay<2) {
            // If we haven't finished the first ball, we can add players
            AddPlayer();
          } else if (!NoResetDuringPlay) {
            // If the first ball is over, pressing start again resets the game
            if (Credits>=1 || FreePlayMode) {
              if (!FreePlayMode) {
                Credits -= 1;
                RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
                RPU_SetDisplayCredits(Credits, CreditDisplay && !FreePlayMode);
              }
              returnState = MACHINE_STATE_INIT_GAMEPLAY;
            }
          }
        break;        
        case SW_TILT:
          // This should be debounced
          if ((CurrentTime-LastTiltWarningTime > TILT_WARNING_DEBOUNCE_TIME) && (curState==MACHINE_STATE_NORMAL_GAMEPLAY)) {
            LastTiltWarningTime = CurrentTime;
            NumTiltWarnings += 1;
            if (NumTiltWarnings>MaxTiltWarnings) {
              RPU_DisableSolenoidStack();
              RPU_SetDisableFlippers(true);
              RPU_TurnOffAllLamps();
              RPU_SetLampState(CREDIT_LIGHT, Credits||FreePlayMode);
              RPU_SetLampState(TILT, 1);
            }
            PlaySoundEffect(SOUND_EFFECT_TILT_WARNING);
          }
        break;
      }
    }
  }

  if (scoreAtTop != CurrentScores[CurrentPlayer]) {
    for (int awardCount=0; awardCount<3; awardCount++) {
      if (AwardScores[awardCount]!=0 && scoreAtTop<AwardScores[awardCount] && CurrentScores[CurrentPlayer]>=AwardScores[awardCount]) {
        // Player has just passed an award score, so we need to award it
        if (((AwardScoresOverride>>awardCount)&0x01)==0x01) {
          AddCredit();
          RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
          RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
        } else {
          SamePlayerShootsAgain = true;
          RPU_SetLampState(SAME_PLAYER, SamePlayerShootsAgain);
          RPU_SetLampState(HEAD_SAME_PLAYER, SamePlayerShootsAgain);
          PlaySoundEffect(SOUND_EFFECT_EXTRA_BALL);
        }
      }
    }
  }

  
  ShowPlayerDisplays(scoreAtTop!=CurrentScores[CurrentPlayer]);

  boolean goBackToShowingBonus = false;
  if (UsingTreeToShowBet) {
    if (CurrentTime>ShowBetUntilThisTime) {
      UsingTreeToShowBet = false;
      goBackToShowingBonus = true;    
    }
  }

  if (!UsingTreeToShowBet && (bonusAtTop!=Bonus||goBackToShowingBonus)) {
    ShowBonusOnTree(Bonus, 0);
  }

  if (!RPU_ReadSingleSwitchState(SW_SAUCER)) {
    LastTimeSaucerEmpty = CurrentTime;    
  } else if ((CurrentTime-LastTimeSaucerEmpty)>30000) {
    RPU_PushToSolenoidStack(SOL_SAUCER, 5, true);
    LastTimeSaucerEmpty = CurrentTime;    
  }  

  return returnState;
}


void loop() {
  // This line has to be in the main loop
  RPU_DataRead(0);

  CurrentTime = millis();
  int newMachineState = MachineState;

  // Machine state is self-test/attract/game play
  if (MachineState<0) {
    newMachineState = RunSelfTest(MachineState, MachineStateChanged);
  } else if (MachineState==MACHINE_STATE_ATTRACT) {
    newMachineState = RunAttractMode(MachineState, MachineStateChanged);
  } else {
    newMachineState = RunGamePlayMode(MachineState, MachineStateChanged);
  }

  if (newMachineState!=MachineState) {
    MachineState = newMachineState;
    MachineStateChanged = true;
  } else {
    MachineStateChanged = false;
  }

  RPU_ApplyFlashToLamps(CurrentTime);
  RPU_UpdateTimedSolenoidStack(CurrentTime);
}
