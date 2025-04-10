#include <nds.h>
#include <stdio.h>
#include <time.h>
#include <maxmod9.h>
#include "soundbank.h"
#include "soundbank_bin.h"

PrintConsole topScreen;
PrintConsole bottomScreen;

const uint8_t HOR_OFFSET = 0;
const uint8_t VER_OFFSET = 0;
const uint8_t COLS = (31 + HOR_OFFSET);
const uint8_t ROWS = (23 + VER_OFFSET);

bool Resume = false;
bool GenBall = true;
bool Start = false;
bool PressedButton = false;
bool BallEaten = false;
bool doPause = false;

int BallX, BallY, ANSBallX, ANSBallY;
int SnakeX = COLS/2;
int SnakeY = ROWS/2;
int VSnakeX = 1;
int VSnakeY = 0;
int Lives = 3;
int Score = 0;
int SnakeLength = 2;
int counter = 0;
int SnakePOSbuffer[6000][2];
int Speed;

mm_sound_effect increase = {
	{ SFX_INCREASE } ,			// id
	(int)(1.0f * (1<<10)),	// rate
	0,		// handle
	255,	// volume
	0,		// panning
};

mm_sound_effect died = {
	{ SFX_DIED } ,			// id
	(int)(1.0f * (1<<10)),	// rate
	0,		// handle
	255,	// volume
	0,		// panning
};

mm_sound_effect lost = {
	{ SFX_LOST } ,			// id
	(int)(1.0f * (1<<10)),	// rate
	0,		// handle
	255,	// volume
	0,		// panning
};

mm_sound_effect start = {
	{ SFX_START } ,			// id
	(int)(1.0f * (1<<10)),	// rate
	0,		// handle
	255,	// volume
	0,		// panning
};

static void CheckInput() {
	scanKeys();
	int pressed = keysDown();
	
	if (pressed & KEY_START) {
		doPause = true;
	} else if ((pressed & KEY_UP) && (VSnakeX != 0 || !Start) && !PressedButton) {
		PressedButton = true;
		if (!Start) {
			Start = true;
		}
		VSnakeY = -1;
		VSnakeX = 0;
	} else if ((pressed & KEY_DOWN) && (VSnakeX != 0 || !Start) && !PressedButton) {
		PressedButton = true;
		if (!Start) {
			Start = true;
		}
		VSnakeY = 1;
		VSnakeX = 0;
	} else if ((pressed & KEY_LEFT) && (VSnakeY != 0 || !Start) && !PressedButton) {
		PressedButton = true;
		if (!Start) {
			Start = true;
		}
		VSnakeX = -1;
		VSnakeY = 0;
	} else if ((pressed & KEY_RIGHT) && (VSnakeY != 0 || !Start) && !PressedButton) {
		PressedButton = true;
		if (!Start) {
			Start = true;
		}
		VSnakeX = 1;
		VSnakeY = 0;
	}
}

void sleep(float delay) {
	float ticks = delay/20;
	float start = 0;
	while (start < ticks) {
		CheckInput();
		start++;
		swiWaitForVBlank();
	}
}

void POSCursor(uint8_t X, uint8_t Y) {
	iprintf("\x1b[%d;%dH", Y, X);
}

static void RenderBorders(bool DELAY, bool PLAYSOUND) {
	for (size_t Y = VER_OFFSET; Y <= ROWS; Y++) {
  
		for (size_t X = HOR_OFFSET; X <= COLS; X++) {
  
			if ( ( (X == HOR_OFFSET || X == COLS) && (Y >= VER_OFFSET && Y <= ROWS) )|| (Y == VER_OFFSET || Y == ROWS)) {
  				POSCursor(X, Y);
				if (DELAY) {
  					sleep(10);
				}
		  		iprintf("#");
  			}
		}
	}
	if (PLAYSOUND) {
		mmEffectEx(&start);
	}
}

static void RenderSnake() {
	POSCursor(SnakePOSbuffer[SnakeLength][0], SnakePOSbuffer[SnakeLength][1]);
	if (SnakePOSbuffer[SnakeLength][0] != 0 && SnakePOSbuffer[SnakeLength][1] != 0) {
		iprintf(" ");
	}	
	POSCursor(SnakeX, SnakeY);
	iprintf("#");	
	SnakePOSbuffer[0][0] = SnakeX;
	SnakePOSbuffer[0][1] = SnakeY;
	for (size_t i = SnakeLength; i > 0; i--) {
		SnakePOSbuffer[i][0] = SnakePOSbuffer[i - 1][0];
		SnakePOSbuffer[i][1] = SnakePOSbuffer[i - 1][1];
	}

}

static void DifficultySelect() {
	int Selection = 10;
	POSCursor(4, 8);
	iprintf("Choose the difficulty :");
	POSCursor(10, 10);
	iprintf("Easy");
	POSCursor(10, 12);
	iprintf("Medium");
	POSCursor(10, 14);
	iprintf("Hard");
	POSCursor(9, Selection);
	iprintf(">");
	while(1) {
		scanKeys();
		int pressed = keysDown();

		if ((pressed & KEY_DOWN) && Selection < 14) {
			POSCursor(9, Selection);
			iprintf(" ");
			Selection = Selection + 2;
			POSCursor(9, Selection);
			iprintf(">");
		} else if ((pressed & KEY_UP) && Selection > 10) {
			POSCursor(9, Selection);
			iprintf(" ");
			Selection = Selection - 2;
			POSCursor(9, Selection);
			iprintf(">");
		} else if (pressed & KEY_A) {
			iprintf("\x1b[2J");
			switch (Selection)
			{
				case 10:
					Speed = 500;
				break;
				
				case 12:
					Speed = 250;
				break;

				case 14:
					Speed = 100;
				break;

				default:
				break;
			}
			return;
		}
	}
}

static void GameOver() {
	POSCursor(10, 10);
	iprintf("Game Over!");
	POSCursor(8, 12);
	iprintf("Your Score : %d", Score);
	POSCursor(5, 14);
	iprintf("Press Start to exit or");
	POSCursor(4, 15);
	iprintf("A to restart the game...");

	while (1) {
		scanKeys();
		int pressed = keysDown();
	
		if (pressed & KEY_START) {
			exit(0);
		} else if (pressed & KEY_A) {
			iprintf("\x1b[2J");
			DifficultySelect();
			RenderBorders(true, true);
			return;
		}
	}

}

static void GenerateBall() {
	for (size_t i = 1; i <= SnakeLength; i++) {
		while (BallX < HOR_OFFSET + 1 || BallX > COLS || BallY < VER_OFFSET + 1 || BallY > ROWS || (BallX == SnakePOSbuffer[i][0] && BallY == SnakePOSbuffer[i][1]) || (BallX == ANSBallX && BallY == ANSBallY)) {
			BallX = HOR_OFFSET + 1 + rand() % (COLS - HOR_OFFSET - 1);
			BallY = VER_OFFSET + 1 + rand() % (ROWS - VER_OFFSET - 1);
		}
	}
	BallEaten = false;
	ANSBallX = BallX;
	ANSBallY = BallY;
	POSCursor(BallX, BallY);
	iprintf("O");
}

static void Loose() {
	iprintf("\x1b[2J");
	GenBall = true;
	Start = false;
	for (size_t i = 1; i < 254; i++) {
		SnakePOSbuffer[i][0] = 0;
		SnakePOSbuffer[i][1] = 0;
	}
	SnakeX = COLS/2;
	SnakeY = ROWS/2;
	VSnakeX = 0;
	VSnakeY = 0;
	SnakeLength = 2;
	if (Lives > 0) {
		mmEffectEx(&lost);
		sleep(1000);
		RenderBorders(false, true);
		Lives--;
	} else {
		mmEffectEx(&died);
		sleep(2000);
		GameOver();
		Lives = 3;
		Score = 0;
	}
}

static void ManageSnakePos() {
	CheckInput();
	if (SnakeX < HOR_OFFSET + 1 || SnakeX > COLS - 1 || SnakeY < VER_OFFSET + 1 || SnakeY > ROWS - 1) {
		Loose();
	}
	if (SnakeLength > 4) {
		for (size_t i = 4; i < SnakeLength + 1; i++) {
			if (SnakeX == SnakePOSbuffer[i][0] && SnakeY == SnakePOSbuffer[i][1]) {
				Loose();
			}
		}
	}
		
	if (SnakeX == BallX && SnakeY == BallY && !BallEaten) {
		mmEffectEx(&increase);
		Score++;
		SnakeLength++;
		GenBall = true;
	}
	SnakeX = SnakeX + VSnakeX;
	SnakeY = SnakeY + VSnakeY;
}

static void PrintGameStats() {
	consoleSelect(&bottomScreen);
	POSCursor(0, 0);
	iprintf(" Score : %d \n", Score);
	iprintf(" Lives : %d ", Lives);
	consoleSelect(&topScreen);
}

static void Pause() {
	iprintf("\x1b[2J");
	POSCursor(12, 10);
	iprintf("Paused!");
	POSCursor(7, 12);
	iprintf("Press Start to resume");
	POSCursor(3, 13);
	iprintf("Or Select to quit the game");
	while (1) {
		scanKeys();
		int pressed = keysDown();
		if (pressed & KEY_START) {
			iprintf("\x1b[2J");
			RenderBorders(false, false);
			for (size_t i = 1; i < SnakeLength; i++) {
				POSCursor(SnakePOSbuffer[i][0], SnakePOSbuffer[i][1]);
				printf("#");
			}
			if (!BallEaten && !GenBall) {
				POSCursor(BallX, BallY);
				printf("O");
			}			
			PrintGameStats();
			doPause = false;
			return;
		} else if (pressed & KEY_SELECT) {
			exit(0);
		}
	}
}

static void RunGame() {
	sleep(Speed);
	PrintGameStats();
	if (doPause) {
		Pause();
	}
	if (counter < 4*(1000/Speed)) {
		counter++;
	} else {
		counter = 0;
	}
	if (counter == 3*(1000/Speed) && GenBall) {
		GenerateBall();
		GenBall = false;
	}
	ManageSnakePos();
	RenderSnake();
	PressedButton = false;
	swiWaitForVBlank();
}

//---------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
//---------------------------------------------------------------------------------
	mmInitDefaultMem((mm_addr)soundbank_bin);
	mmLoadEffect( SFX_START );
	mmLoadEffect( SFX_DIED );
	mmLoadEffect( SFX_INCREASE );
	mmLoadEffect( SFX_LOST );

	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);

	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

	consoleInit(&topScreen, 3,BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
	consoleInit(&bottomScreen, 3,BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);

	consoleSelect(&topScreen);
	iprintf("\x1b[2J");
	iprintf("SnakeDS\nMade By Abdelali221\nGithub : \nhttps://github.com/abdelali221/\n");
	iprintf("\nPress A to start...");

	while(!Resume) {

		scanKeys();
		int pressed = keysDown();
		if ( pressed & KEY_A ) {
			Resume = true;
		}
		swiWaitForVBlank();
	}
	iprintf("\x1b[2J");
	sleep(200);
	DifficultySelect();	
	RenderBorders(true, true);

	while (1) {
		RunGame();
	}

	return 0;

}